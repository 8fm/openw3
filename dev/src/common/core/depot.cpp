/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "resourceDefManager.h"
#include "packageManager.h"
#include "filePath.h"
#include "dataError.h"
#include "gatheredResource.h"
#include "jobGenericJobs.h"
#include "loadingJobManager.h"
#include "tokenizer.h"
#include "depotBundles.h"
#include "xmlFileReader.h"
#include "ioTagResolver.h"

// Initialize depot
CDepot *GDepot;
extern Bool GDataAsserts;

CDepot::CDepot( const String & rootDataPath )
	: CDirectory( TXT("depot"), (Uint32)-1, nullptr )
	, m_rootDataPath( rootDataPath )
	, m_strictErrorReporting( false )
	, m_bundles( nullptr )
	, m_depotHashed( false )
{	
	// Sanity check
	if ( !m_rootDataPath.EndsWith( TXT("\\") ) && !m_rootDataPath.EndsWith( TXT("/") ) )
	{
		HALT( "Root depot path should end with \\ or /" );
	}
}

CDepot::~CDepot()
{
}

void CDepot::ToggleMissingFileMessages( const Bool flag )
{
	m_strictErrorReporting = flag;
}

void CDepot::EnableFallbackResources( const Bool flag )
{
	m_enableFallbackResources = flag;
}

Bool CDepot::Initialize( const Core::CommandLineArguments& coreArguments )
{
	// Clear current file mapping
	m_fileMap.Clear();

	// Try to load dependency cache
	if ( m_dependencyCache.Load( m_rootDataPath + TXT("dep.cache") ) )
	{
		LOG_CORE( TXT("Depot will use dependency cache") );

		// create the index -> CDiskFile* mapping
		const Uint32 numResources = m_dependencyCache.GetNumResources();
		m_dependencyDiskFileMapping.Resize( numResources );
		Red::MemoryZero( m_dependencyDiskFileMapping.Data(), m_dependencyDiskFileMapping.DataSize() );
	}
#ifdef RED_PLATFORM_CONSOLE
	//!========================================================================================================================
	//! Search for patched dependency caches for DLC`s
	//! if dep.cache is found under patch<num>\dlc folder relative path is added to m_patchedDepCashes list
	//! ex. "D:\APP_HOME\app\content\patch1\dlc\bob\dep.cache" to list is added "bob\dep.cache"
	//!========================================================================================================================
	const Uint32 maxPatchGroups = 16;
	for( Int32 i = maxPatchGroups-1; i >= 0; --i )
	{
		static const String pattern(TXT("*dep.cache") );

		const String& patchDLCDirectoryAbsolute = GFileManager->GetBundleDirectory() + String::Printf( TXT("patch%d\\dlc\\"), i );
		
		TDynArray< String > filePaths;
		GFileManager->FindFilesRelative(patchDLCDirectoryAbsolute, TXT(""), pattern, filePaths, true);
		
		for( const String& filePath : filePaths )
		{
			String depChachePath = filePath;
			//! m_patchedDepCashes is checked under OnContentAvailable where all path are normalized to use backslashes
			depChachePath.ReplaceAll('/', '\\');

			if( m_patchedDepCashes.Exist(depChachePath) == false )
			{
				const String& patchedDepChachePath = patchDLCDirectoryAbsolute + filePath;	
				CDependencyCache additionalCache;
				if( additionalCache.Load( patchedDepChachePath ) )
				{
					LOG_CORE( TXT("Attaching dependency cache from '%ls'"), patchedDepChachePath.AsChar() );

					m_dependencyCache.AppendDynamicCache( additionalCache );
					const Uint32 numOldResources = m_dependencyDiskFileMapping.Size();
					const Uint32 numNewResources = m_dependencyCache.GetNumResources();
					LOG_CORE( TXT("Dependency table mapping: %d -> %d resources"), numOldResources, numNewResources );

					// update the file table
					m_dependencyDiskFileMapping.Resize( numNewResources );
					for ( Uint32 i=numOldResources; i<numNewResources; ++i )
						m_dependencyDiskFileMapping[i] = 0;
						

					m_patchedDepCashes.Insert(depChachePath);
				}
			}
		}
	}
	//!========================================================================================================================
#endif //! RED_PLATFORM_CONSOLE

	// Bind files from bundles
	if ( coreArguments.m_useBundles )
	{
		CDepotBundles* bundles = new CDepotBundles( GFileManager->GetBundleDirectory(), coreArguments.m_splitCook );
		if ( !bundles->Initialize( m_fileMap ) )
			return false;

		LOG_CORE( TXT("Depot will use bundles") );
		m_bundles = bundles;
	}

	// Populate with rest of the files
	if ( !IsUsingBundles() )
	{
		// Fill only with free floating files
		CDirectory::Repopulate( false );
	}

	// Repopulate loose files
	if ( coreArguments.m_useBundles && GFileManager->IsReadOnly() )
	{
#ifndef RED_FINAL_BUILD
		const Bool fileOverride = Red::System::StringSearch( SGetCommandLine(), TXT( "-fileoverride" ) ) != nullptr;
		if ( !m_populated && fileOverride )
		{
			LOG_CORE( TXT("!!! ENABLING FILE OVERRIDE OPTION !!!") );

			// rescan
			{
				CTimeCounter timer;
				CDirectory::Repopulate( true );
				LOG_CORE( TXT("Depot loose population took %1.2fs"), timer.GetTimePeriod() );
			}

			// we used file overrides, we need to rehash the internal file lookup map because it's no longer valid
			{
				CTimeCounter timer;
				TDynArray< CDiskFile* > allFiles;
				CollectFiles( allFiles, String::EMPTY );

				// rebuild the hashmap
				m_fileMap.Clear();
				for ( CDiskFile* file : allFiles )
				{
					const TDepotFileHash hash = Red::System::CalculatePathHash64( file->GetDepotPath().AsChar() );
					m_fileMap.Insert( hash, file );
				}

				LOG_CORE( TXT("Rehashing depot took %1.2fs"), timer.GetTimePeriod() );
			}
		}
#endif

		// we have a valid depot hash
		m_depotHashed = true;
	}

	// In read only file system prevent OS file enumeration
	if ( GFileManager->IsReadOnly() )
	{
		MarkAsPopulated( true );
	}

	// Initialize loading priority table (we need mapped depot)
	if ( !GFileSysPriorityResovler.InitializePriorityTables() )
		return false;

	return true;
}

void CDepot::Remap( const String& newAbsolutePath, const Bool forceRescan )
{
	m_rootDataPath = newAbsolutePath;
	CDirectory::Remap( newAbsolutePath, forceRescan );
}

void CDepot::GetAbsolutePath( String& str ) const
{
	if ( m_overridePath )
		str = *m_overridePath;

	str = m_rootDataPath;
}

void CDepot::GetDepotPath( String& str ) const
{
	str = String::EMPTY;
}

CDiskFile* CDepot::CreateNewFile( const Char* fileDepotPath )
{
	// create the directories, extract the file name
	const Char* fileName = nullptr;
	CDirectory* dir = CDirectory::CreatePath( fileDepotPath, &fileName );
	if ( !dir )
		return nullptr;

	// create file in the directory, can fail if already exists
	return dir->CreateNewFile( fileName );
}

CDiskFile* CDepot::FindFile( const TDepotFileHash hash ) const
{
	RED_FATAL_ASSERT( GFileManager->IsReadOnly(), "This function can only be used in the read only file system" );

	// Depot is not hashed and we are doing a hash based lookup - hash it to avoid loosing depot consistency
	if ( !m_depotHashed )
	{
		CTimeCounter timer;
		TDynArray< CDiskFile* > allFiles;
		CollectFiles( allFiles, String::EMPTY ); // existing files

		// rebuild the hashmap
		m_fileMap.Clear();
		for ( CDiskFile* file : allFiles )
		{
			const TDepotFileHash hash = Red::System::CalculatePathHash64( file->GetDepotPath().AsChar() );
			if( m_fileMap.Insert( hash, file ) == false )
			{
				ERR_CORE( TXT("Hash collision for files (already inserted, new file): %ls %ls"), m_fileMap[hash]->GetDepotPath().AsChar(), file->GetDepotPath().AsChar() );
			}
		}

		LOG_CORE( TXT("Hashing depot took %1.2fs"), timer.GetTimePeriod() );
		m_depotHashed = true;
	}

	CDiskFile* file = nullptr;
	m_fileMap.Find( hash, file );
	return file;
}

CDiskFile* CDepot::FindFile( const String& path ) const
{
	String temp;
	const String& safePath = CFilePath::ConformPath( path, temp );
	return FindFile( safePath.AsChar() );
}

const Char* CDepot::ResolvePath( const Char* path )
{
	// magic support for aliased names
	if ( 0 == Red::StringCompare( path, CResourceDefManager::RESDEF_PROTOCOL.AsChar(), CResourceDefManager::RESDEF_PROTOCOL.GetLength() ) )
	{
		const Char* aliasName = path +  CResourceDefManager::RESDEF_PROTOCOL.GetLength();
		return SResourceDefManager::GetInstance().GetPath( aliasName ).AsChar();
	}
	else
	{
		return path;
	}
}

CDiskFile* CDepot::FindFile( const Char* path ) const
{
	// resolve path
	const Char* resolvedPath = ResolvePath( path );

	// find the directory
	const Char* fileNamePart = nullptr;
	const CDirectory* dir = FindPath( resolvedPath, &fileNamePart );
	if ( !dir )
		return nullptr;

	// find the file in directory
	return dir->FindLocalFile( fileNamePart );
}

THandle< CResource > CDepot::FindResource( const String& depotFileName )
{
	// Find depot file
	CDiskFile* file = FindFile( depotFileName );
	if ( file && file->IsLoaded() ) // we can't be in transient state
	{
		return file->GetResource();
	}

	// Not found
	return NULL;
}

Bool GAllowFindFileFallback = true;

CDiskFile* CDepot::FindFileUseLinks( const String& path, Uint32 recurencyLevel )
{
	// sanity check
	if ( recurencyLevel > 10 )
	{
		WARN_CORE( TXT("FindFileUseLinks(): recurencyLevel 10 exceeded for file '%ls'. Links may be poining to each other."), path.AsChar() );
		return NULL;
	}

	// Find depot file
	CDiskFile* file = FindFile( path );
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	if ( file && !file->IsDeleted() )
#else
	if ( file )
#endif
		return file;

	// Do not use links in final build
#if !defined( RED_FINAL_BUILD )

	// File not found in depot, if .link file exists for this file find it
	const Char* processedPath = ResolvePath( path.AsChar() );
	if ( !processedPath || !processedPath[0] )
		return NULL;

	// Get file name
	String linkFileName( processedPath );
	linkFileName += TXT(".link");

	// Find link file
	CDiskFile* linkDiskFile = FindFile( linkFileName );
	if ( linkDiskFile )
	{
		// New path is written in .link file
		String newPath;
		{
			Red::TScopedPtr<IFile> linkFileReader( linkDiskFile->CreateReader() );
			if ( linkFileReader )
			{
				GFileManager->LoadFileToString( linkFileReader.Get(), newPath );

				// Fix hand made links ( spaces at the end )
				newPath.Trim();
			}
		}

		// New path is valid, look in the target location
		if ( !newPath.Empty() )
		{
			return FindFileUseLinks( newPath, recurencyLevel + 1 );
		}
	}

	// Use fallbacks
	if ( m_enableFallbackResources )
	{
		CResource* fallbackResource = GetFallbackResource( path );
		if ( fallbackResource && fallbackResource->GetFile() )
			return fallbackResource->GetFile();
	}

	// Try to use default
	if ( GAllowFindFileFallback )
	{
		CClass* resClass = ResourceClassByExtension( CFilePath( path ).GetExtension() );
		if ( resClass )
		{
			CResource * defObj = resClass->GetDefaultObject< CResource >();
			CGatheredResource * defRes = defObj->GetDefaultResource();
			if ( defRes )
			{
				return FindFile( defRes->GetPath().ToString() );
			}
		}
	}

#endif

	// No resource for that extension, we are doomed
	return NULL;
}

CXMLReader* CDepot::LoadXML( const String& depotFileName )
{
	PC_SCOPE( LoadXML );

	CDiskFile* file = FindFileUseLinks( depotFileName, 0 );
	if ( !file )
		return nullptr;

	IFile* reader = file->CreateReader();
	if ( !reader )
		return nullptr;

	CXMLFileReader* xml = new CXMLFileReader( *reader );
	delete reader;

	// return loaded xml
	return xml;
}

THandle< CResource > CDepot::LoadResource( const String& depotFileName, Bool useLinks /*= true*/ )
{
	ResourceLoadingContext context;
	return LoadResource( depotFileName, context, useLinks );
}

THandle< CResource > CDepot::LoadResource( const String& depotFileName, ResourceLoadingContext& context, Bool useLinks /*= true*/ )
{
	// Get file
	CDiskFile* file = NULL;
	if ( useLinks )
	{
		file = FindFileUseLinks( depotFileName, 0 );
	}
	else
	{
		file = FindFile( depotFileName );
	}

	// Use fallback solution
	if ( m_enableFallbackResources && !file )
	{
		CResource* fallbackResource = GetFallbackResource( depotFileName );
		if ( fallbackResource )
			return fallbackResource;
	}

	// File is missing
	if ( !file )
	{
		return NULL;
	}

	// Load a file
	return file->Load( context );
}

THandle< CResource > CDepot::LoadResourceAsync( const String& depotFileName )
{   
    THandle< CResource > res = FindResource( depotFileName );
    if ( res )
    {
        return res;
    }

	// this is very important to avoid memory leaks, please don't remove it
	THashMap< String, CJobLoadResource* >::iterator it = m_loadResourceJobs.Find( depotFileName );
	if ( it != m_loadResourceJobs.End() )
	{
		return NULL; //there is already a job for this
	}
    
    CJobLoadResource* job = new CJobLoadResource( depotFileName );
    m_loadResourceJobs.Insert( depotFileName, job );
    SJobManager::GetInstance().Issue( job );

	// not yet loaded
    return NULL;
}

CDepot::AsyncResourceState CDepot::GetAsyncResource( const String& depotFileName, THandle< CResource >& resource )
{
    THashMap< String, CJobLoadResource* >::iterator it = m_loadResourceJobs.Find( depotFileName );
    if ( it == m_loadResourceJobs.End() )
    {
        CResource* res = FindResource( depotFileName );
        if ( res )
        {
            resource = res;
            return CDepot::ARS_LOADED;
        }

        return CDepot::ARS_FAILED;
    }
    else
    {
        CDepot::AsyncResourceState result;

        CJobLoadResource* job = it->m_second;
        if ( ! job->HasEnded() )
        {
            result = CDepot::ARS_LOADING;
            return result;
        }
        else
        {
            if ( job->HasFinishedWithoutErrors() )
            {
                result = CDepot::ARS_LOADED;
                resource = job->GetResource();
            }
            else
            {
                result = CDepot::ARS_FAILED;
            }
        }
        job->Release();
        m_loadResourceJobs.Erase( it );
        return result;
    }
}

CDiskFile* CDepot::GetMappedDiskFile( const Uint32 dependencyCacheIndex ) const
{
	if ( dependencyCacheIndex < m_dependencyDiskFileMapping.Size() )
		return m_dependencyDiskFileMapping[ dependencyCacheIndex ];

	return nullptr;
}

const Uint32 CDepot::MapFileToDependencyCache( CDiskFile* file )
{
	if ( m_dependencyCache.GetNumResources() > 0 )
	{
		const CDependencyCache::ResourceID id( file->GetDepotPath() );
		const Uint32 index = m_dependencyCache.MapResourceToIndex( id );
		if ( index )
		{
			RED_ASSERT( index < m_dependencyDiskFileMapping.Size(), TXT("Disk file '%ls' dependency cache index is out of range (%d/%d)"), 
				file->GetDepotPath().AsChar(), index, m_dependencyDiskFileMapping.Size() );

			if ( index < m_dependencyDiskFileMapping.Size() )
			{
				const CDependencyCache::ResourceID reverseId = m_dependencyCache.MapIndexToResource( index );
				RED_ASSERT( reverseId == id, TXT("Corrupted dependency cache mapping for disk file '%ls'"), file->GetDepotPath().AsChar() );

				RED_ASSERT( m_dependencyDiskFileMapping[ index ] == nullptr, TXT("Disk file '%ls' is already mapped in the dependecy cache"), 
					file->GetDepotPath().AsChar() );

				m_dependencyDiskFileMapping[ index ] = file;
				return index;
			}
		}
		else
		{
			// report missing entries (only first 1000)
			static Uint32 numMissingEntriesReported = 0;
			if ( numMissingEntriesReported++ < 1000 )
			{
				LOG_CORE( TXT("Missing dependency file entry for '%ls'"), file->GetDepotPath().AsChar() );
			}
		}
	}

	// not found
	return 0;	
}

void CDepot::UnmapFileFromDependencyCache( CDiskFile* file )
{
	const Uint32 index = file ? file->GetDepCacheIndex() : 0 ;
	if ( index && index < m_dependencyDiskFileMapping.Size() )
	{
		RED_ASSERT( m_dependencyDiskFileMapping[ index ] == file, TXT("Dependency cache file map is inconsistent") );
		m_dependencyDiskFileMapping[ index ] = nullptr;
	}
}

bool CDepot::FileExist( const String& path )
{
	return FindFile( path ) != nullptr;
}

const Char* CDepot::GetName() const
{
	return TXT("Depot");
}

String GW3StreamingCacheAbsoluteFilePath;
String GEP2StreamingCacheAbsoluteFilePath;

void CDepot::OnContentAvailable( const SContentInfo& contentInfo )
{
	StringAnsi metaDataStorePathNormal, metaDataStorePathCensored;
	Bool rehashRequired = false;

	// Dependency cache - only for dynamic content (DLC)
	// IT MUST BE ATTACHED FIRST
	if ( contentInfo.m_packageID != BASE_RUNTIME_PACKAGE_ID && !contentInfo.m_isMod )
	{
		for ( const SContentFile* file : contentInfo.m_contentFiles )
		{			
			if ( file->m_path.EndsWith( "dep.cache" ) )
			{
				const String fullPath = String::Printf(TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), file->m_path.AsChar() );

#ifdef RED_PLATFORM_CONSOLE
				//!========================================================================================================================
				//! dep.cache can be already loaded for DLC form patch
				//! then we can`t load it second time
				//!========================================================================================================================
				bool depCachePatched = false;
				const String pathAsUTF16( ANSI_TO_UNICODE(file->m_path.AsChar()) );
				for( const String patchedDepCache : m_patchedDepCashes )
				{
					if( pathAsUTF16.EndsWith( patchedDepCache.AsChar() ) )
					{
						depCachePatched = true;
						break;
					}
				}

				if( depCachePatched )
				{
					LOG_CORE( TXT("Depedency cache already patched '%ls'"), fullPath.AsChar() );
					continue;
				}
				//!========================================================================================================================
#endif //! RED_PLATFORM_CONSOLE

				LOG_CORE( TXT("Attaching dynamic dependency cache from '%ls'"), fullPath.AsChar() );

				CDependencyCache additionalCache;
				if ( additionalCache.Load( fullPath ) )
				{
					m_dependencyCache.AppendDynamicCache( additionalCache );

					const Uint32 numOldResources = m_dependencyDiskFileMapping.Size();
					const Uint32 numNewResources = m_dependencyCache.GetNumResources();
					LOG_CORE( TXT("Dependency table mapping: %d -> %d resources"), numOldResources, numNewResources );

					// update the file table
					m_dependencyDiskFileMapping.Resize( numNewResources );
					for ( Uint32 i=numOldResources; i<numNewResources; ++i )
						m_dependencyDiskFileMapping[i] = 0;

					rehashRequired = true;
				}
				else
				{
					ERR_CORE( TXT("Failed to load dynamic dependency cache from '%ls'"), fullPath.AsChar() );
				}
			}
		}
	}

	// Other content files
	for ( const SContentFile* file : contentInfo.m_contentFiles )
	{
		if ( file->m_path.EndsWith( "streaming.cache" ) )
		{
			const String cachePath = String::Printf(TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), file->m_path.AsChar() );
			if ( contentInfo.m_packageID == BASE_RUNTIME_PACKAGE_ID )
			{
				if ( !GW3StreamingCacheAbsoluteFilePath.Empty() )
				{
					WARN_CORE(TXT("GW3StreamingCacheAbsoluteFilePath already has a path: %ls"), GW3StreamingCacheAbsoluteFilePath.AsChar() );
				}
				GW3StreamingCacheAbsoluteFilePath = std::move(cachePath);
				LOG_CORE(TXT("GW3StreamingCacheAbsoluteFilePath new path: %ls"), GW3StreamingCacheAbsoluteFilePath.AsChar() );
			}
			else
			{
				// Assuming only one DLC with a streaming cache! If broken, then need another hack, like getting the product ID
				if ( !GEP2StreamingCacheAbsoluteFilePath.Empty() )
				{
					WARN_CORE(TXT("GEP2StreamingCacheAbsoluteFilePath already has a path: %ls"), GEP2StreamingCacheAbsoluteFilePath.AsChar() );
				}
				GEP2StreamingCacheAbsoluteFilePath = std::move(cachePath);
				LOG_CORE(TXT("GEP2StreamingCacheAbsoluteFilePath new path: %ls"), GEP2StreamingCacheAbsoluteFilePath.AsChar() );
			}
		}
		else
		// Bundle
		if ( file->m_path.EndsWith( ".bundle") )
		{
			if ( contentInfo.m_packageID == BASE_RUNTIME_PACKAGE_ID )
			{
				// THIS IS A BUNDLE FROM THE BASE PACKAGE
				// This is usually called from stream install stuff
				m_bundles->AttachStaticBundle( file->m_path );
			}
			else if ( contentInfo.m_isMod )
			{
				// THIS IS A BUNDLE FROM DYNAMIC USER PACKAGE (MOD)
				// Do nothing here, all of the work is done when the store from the dynamic content is attached
				LOG_CORE( TXT("Encountered bundle from user content: '%hs'"), file->m_path.AsChar() );
			}
			else
			{
				// THIS IS A BUNDLE FROM DYNAMIC PACKAGE (DLC)
				// Do nothing here, all of the work is done when the store from the dynamic content is attached
				LOG_CORE( TXT("Encountered bundle from dynamic content: '%hs'"), file->m_path.AsChar() );
			}
		}

		// Metadata store, only attach it if it's from DLC
		if ( contentInfo.m_packageID != BASE_RUNTIME_PACKAGE_ID )
		{
			if ( contentInfo.m_isMod )
			{
				// THIS IS A METADATA STORE FROM MOD
				// We have a fast way to attach it to our current bundle system
				// We assume that the bundles are stored in the bundles subdirectory of the directory in which the metadata.store resides, hence the shitty code to extract the path
				const String metadataStore = String::Printf(TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), file->m_path.AsChar() );
				const String bundlesRoot( StringHelpers::GetBaseFilePath( metadataStore ) + TXT("\\") );
				m_bundles->AttachDynamicOverrideBundles( metadataStore, bundlesRoot, m_fileMap );

				// resort file map
				rehashRequired = true;
			}
			else
			{
				if ( file->m_path.EndsWith( ".censored.store" ) )
				{
					metaDataStorePathCensored = file->m_path;
				}
				else if( file->m_path.EndsWith( ".store" ) )
				{
					metaDataStorePathNormal = file->m_path;
				}
			}
		}
	}

	// load metadata store if there was one in the package
	// NOTE: we try to load the metadata store matching the censorship state of the game
	if ( !metaDataStorePathCensored.Empty() || !metaDataStorePathNormal.Empty() )
	{
		const AnsiChar* matadataStorePaths[2] =
		{
			// try the preferred one first
			m_bundles->IsCensored() ? metaDataStorePathCensored.AsChar() : metaDataStorePathNormal.AsChar(), 

			// fallback to the other one
			m_bundles->IsCensored() ? metaDataStorePathNormal.AsChar() : metaDataStorePathCensored.AsChar(), 
		};

		// try to load in the preferred order
		for ( Uint32 i=0; i<ARRAY_COUNT(matadataStorePaths); ++i )
		{
			const auto* path = matadataStorePaths[i];
			if ( Red::StringLength( path ) > 0 )
			{
				const String metadataStore = String::Printf(TXT("%ls%hs"), contentInfo.m_mountPath.AsChar(), path );
				const String bundlesRoot( StringHelpers::GetBaseFilePath( metadataStore ) + TXT("\\") );
			
				if ( m_bundles->AttachDynamicBundles( metadataStore, bundlesRoot, m_fileMap ) )
				{
					LOG_CORE( TXT("Attached dynamic metadata store from path '%hs'"), path );
					rehashRequired = true;

					break; // do not try the other one
				}
			}
		}
	}

	// a file system table rehash is required
	// TODO: remove or optimize this shit
	if ( rehashRequired )
	{
		CTimeCounter timer;
		m_fileMap.Resort();
		LOG_CORE( TXT("Resorting depot hashmap took %1.2fms"), timer.GetTimePeriodMS() );
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CResource* CDepot::GetFallbackResource( const String& path )
{
	RED_FATAL_ASSERT( FindFile( path ) == nullptr, "File already exists" );

	// get fake file holder
	CDiskFile* file = GetFallbackFile( path );

	// no fake file there, we really cannot load it
	if ( !file )
	{
		ERR_CORE( TXT("Unable to create fallback file '%ls'."), path.AsChar() );
		return nullptr;
	}

	// resolve resource class
	const CClass* resourceClass = file->GetResourceClass();
	if ( !resourceClass )
	{
		ERR_CORE( TXT("Unable to resolve class for fallback file '%ls'."), path.AsChar() );
		return nullptr;
	}

	// validate class
	RED_FATAL_ASSERT( resourceClass->IsA< CResource >(), "Resource class is not actually a resource" );		
	RED_FATAL_ASSERT( !resourceClass->IsAbstract(), "Resource class is abstract" );		

	// no resource, create it
	if ( !file->GetResource() )
	{
		CResource* res = static_cast< CResource* >( CObject::CreateObjectStatic( (CClass*)resourceClass, nullptr, 0, true ) );
		if ( !res )
		{
			ERR_CORE( TXT("Unable to create fake resource of class '%ls' for file '%ls'"), resourceClass->GetName().AsChar(), path.AsChar() );
			return nullptr;
		}

		// keep the fake resource around
		m_fallbackResources.PushBack( res );
		res->AddToRootSet();

		// set the resource
		file->Rebind( res );
		RED_FATAL_ASSERT( res->GetFile() == file, "Binding was not sucessful" );
		RED_FATAL_ASSERT( file->GetResource() == res, "Reverse binding was not sucessful" );
	}

	// return resource
	RED_FATAL_ASSERT( !file->GetResource() || file->GetResource()->IsA( (CClass*)resourceClass ), "Resource has invalid class" );
	return file->GetResource();
}

CDiskFile* CDepot::GetFallbackFile( const String& path )
{
	// create fake file
	CDiskFile* fakeFile = GDepot->CreateNewFile( path.AsChar() );
	RED_FATAL_ASSERT( fakeFile != nullptr, "Failed to create fake dependency file" );
	RED_LOG( WCC, TXT("Created fallback file '%ls'"), path.AsChar() );
	return fakeFile;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CDeferredInit* GDeferredInit = nullptr;

CDeferredInit::~CDeferredInit()
{
	for (Int32 i=0, n=mContentManagerInitDelegates.Size(); i<n; ++i)
	{
		delete mContentManagerInitDelegates[i];
	}
	mContentManagerInitDelegates.Clear();
}

void CDeferredInit::AddDelegate(IDeferredInitDelegate* d)
{
	mContentManagerInitDelegates.PushBack(d);
}


void CDeferredInit::OnPostContentManagerInit()
{
	for (Int32 i=0, n=mContentManagerInitDelegates.Size(); i<n; ++i)
	{
		IDeferredInitDelegate* d = mContentManagerInitDelegates[i];
		d->OnPostContentManagerInit();
	}
}

void CDeferredInit::OnBaseEngineInit()
{
	for (Int32 i=0, n=mContentManagerInitDelegates.Size(); i<n; ++i)
	{
		IDeferredInitDelegate* d = mContentManagerInitDelegates[i];
		d->OnBaseEngineInit();
	}
}

