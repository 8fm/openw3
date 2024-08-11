/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "depot.h"
#include "depotBundles.h"
#include "diskBundle.h"
#include "bundleMetadataStore.h"
#include "bundleMetadataStoreBuilder.h"
#include "bundleDepotPopulator.h"
#include "asyncIO.h"
#include "diskBundleIOCache.h"
#include "diskBundleCache.h"
#include "diskBundleCacheEntry.h"
#include "contentManifest.h"
#include "bundlePreamble.h"
#include "contentManager.h"

#if !defined( RED_FINAL_BUILD ) || defined( RED_PROFILE_BUILD )
# ifdef RED_PLATFORM_ORBIS
 	#undef LOG_CORE
 	#define LOG_CORE( x, ... ) { fwprintf( stderr, (x), __VA_ARGS__ ); fwprintf( stderr, TXT("\n")); }
# endif
#endif

const Uint32 c_bundleBufferCount = 2;
const Uint32 c_bundleBufferSize = 4 * 1024 * 1024;
const Uint32 c_diskBundleIOCacheSize = 10 * 1024 * 1024; // 99.9% of files are less than 5mb. 10mb should be good enough for ringbuffer.

namespace Config
{
	// Max buffers to cache
	TConfigVar< Int32 >			cvBundleNumFileEntries( "MetadataStore", "NumFileEntries", 64 );

	// Number of data buffers to store decompressed data
	TConfigVar< Int32 >			cvBundleBufferCount( "MetadataStore", "DataBufferCount", 1 );

	// Size of data buffers to store decompressed data - only for sync CreateReader() calls
	TConfigVar< Int32 >			cvBundleBufferSize( "MetadataStore", "DataBufferSize", 4 * 1024 * 1024 );

	// IO Buffer for reading data from disk - only for sync CreateReader() calls
	TConfigVar< Int32 >			cvBundleIOCacheSize( "MetadataStore", "IOBufferSize", 10 * 1024 * 1024 );

	// Cutoff size for compressed files read through CreateReader() - note, this may cause content problems
	TConfigVar< Int32 >			cvBundleFileSizeCutoff( "MetadataStore", "FileSizeCutoff", 4 * 1024 * 1024 );
}

CDepotBundles::CDepotBundles( const String& rootPath, Bool splitCook )
	: m_rootPath( rootPath )
	, m_splitCook( splitCook )
	, m_dataCache( Config::cvBundleNumFileEntries.Get() )
{
	m_dataCache.Initialize( c_bundleBufferSize, Config::cvBundleBufferCount.Get() ); // ctremblay refactor in progress
	m_ramCache = new CDiskBundleCache();
}

CDepotBundles::~CDepotBundles()
{
	// delete ram cache
	if ( m_ramCache != nullptr )
	{
		delete m_ramCache;
		m_ramCache = nullptr;
	}

	// delete the meta store
	if ( m_metadataStore != nullptr )
	{
		delete m_metadataStore;
		m_metadataStore = nullptr;
	}

	m_patchedDLCFiles.ClearPtr();
}

Bool CDepotBundles::IsCensored() const
{
	return m_metadataStore ? m_metadataStore->IsCensored() : false;
}

Bool CDepotBundles::Initialize( TDepotFileMap& outFileMap )
{
	CTimeCounter initTimer;

	// Create the meta data store, there's always one although it can be empty
	m_metadataStore = new CBundleMetadataStore( m_rootPath );


	m_patchedDLCFiles.ClearPtr();

	// Load meta data store or rebuild it if required
	if ( !LoadMetadataStore() )
		return false;

	m_ioCache.Initialize( c_diskBundleIOCacheSize );

	// Create the bundle wrappers (CDiskBundle)
	// They are used to as an access point for each individual bundle
	if ( !CreateWrappers() )
		return false;

	// Populate the depot using the known bundle files
	if ( !PopulateDepot( outFileMap ) )
		return false;

	m_installedFilter.Initialize( *m_metadataStore );
	if ( m_splitCook )
	{
		// In split cook all content is disabled by default until it's mounted
		m_installedFilter.DisableAll();
	}
	else
	{
		// In normal build all content is avaiable
		m_installedFilter.EnableAll();
	}

	// World filter is disabled by default
	m_worldFilter.Initialize( *m_metadataStore );
	m_worldFilter.DisableAll();

	// Initialize merged filter - nothing available
	m_mergedFilter.Initialize( *m_metadataStore );
	m_mergedFilter.DisableAll();

	// Enable only the bundles not related to worlds
	SwitchWorldBundles( StringAnsi::EMPTY );

	// Cache initial content
	// Dex's Note: this is not ideal fix for streaming but we cannot work without it
	// Dex's Note 2: i've changed this and instead of precaching the content for the whole game I've added more stuff to the world startup bundle
	//CacheContent();

	// final stats
	LOG_CORE( TXT("Bundles initialized in %1.2f sec"), initTimer.GetTimePeriod() );
	return true;
}

void CDepotBundles::GetAllBundles( TDynArray< CDiskBundle* >& outBundles ) const
{
	outBundles.Reserve( m_bundles.Size() );

	for ( CDiskBundle* bundle : m_bundles )
	{
		outBundles.PushBack( bundle );
	}
}

const Bool CDepotBundles::IsBundleMounted( const Red::Core::Bundle::BundleID bundleID ) const
{
	// index out of range
	if ( bundleID > m_bundles.Size() )
		return nullptr;

	// check in the merged mask
	return m_mergedFilter.IsBundleEnabled( bundleID );
}

CBundleDiskFile* CDepotBundles::GetBundleDiskFile( Red::Core::Bundle::FileID fileID ) const
{
	if ( fileID >= m_files.Size() )
		return nullptr;

	return m_files[ fileID ];
}

void CDepotBundles::GetFileEntries( Red::Core::Bundle::FileID fileID, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement ) const
{
	if ( fileID >= m_files.Size() )
		return;

	m_metadataStore->GetFileAccessInfos( fileID, outFilesPlacement );
}

CDiskBundle* CDepotBundles::GetBundle( const Red::Core::Bundle::BundleID bundleID ) const
{
	// index out of range
	if ( bundleID > m_bundles.Size() )
		return nullptr;

	// get the bundle
	return m_bundles[ bundleID ];
}

void CDepotBundles::CacheContent()
{
	CTimeCounter timer;

	struct CacheEntry
	{
		Red::Core::Bundle::FileID		m_file;
		Uint32							m_maxSize;
	};

	// find all files to cache
	TDynArray< CacheEntry > filesToCache;
	for ( CBundleDiskFile* file : m_files )
	{
		// selective caching
		if ( file ) 
		{
			if ( file->GetFileName().EndsWith( TXT("w2mesh") ) )
			{
				CacheEntry entry;
				entry.m_file = file->GetFileID();
				entry.m_maxSize = 1024*1024;
				filesToCache.PushBack(entry);
			}

			/*if ( file->GetFileName().EndsWith( TXT("xbm") ) )
			{
				CacheEntry entry;
				entry.m_file = file->GetFileID();
				entry.m_maxSize = 8*1024;
				filesToCache.PushBack(entry);
			}*/
		}
	}

	// when caching data we use NO FILTER
	CBundleMetadataStoreFilter allBundlesFilter;
	allBundlesFilter.Initialize( *m_metadataStore );
	allBundlesFilter.EnableAll();

	// put files in the cache
	Uint32 totalDataCached = 0;
	Uint32 totalFilesCached = 0;
	for ( const auto& entry : filesToCache )
	{
		const auto fileID = entry.m_file;

		// find file placement
		Red::Core::Bundle::SMetadataFileInBundlePlacement placement;
		if ( !m_metadataStore->GetFileAccessInfo( fileID, allBundlesFilter, /*out*/ placement ) )
			continue;

		// file is to big to be cached
		if ( placement.m_sizeInBundle > entry.m_maxSize )
			continue;

		// get the disk bundle
		CDiskBundle* bundle = GetBundle( placement.m_bundleID );
		if ( !bundle )
			continue;

		// prepare data buffers
		CDiskBundleCacheEntryMemoryBlock* readBuffer = CDiskBundleCacheEntryMemoryBlock::Create( false, placement.m_sizeInBundle );
		if ( !readBuffer )
			continue;

		// read raw data
		Uint32 sizeRead = 0;
		if ( !bundle->ReadRawData( readBuffer->GetData(), placement.m_offsetInBundle, placement.m_sizeInBundle, sizeRead ) )
		{
			readBuffer->Release();
			continue;
		}

		// not enough read
		if ( sizeRead != placement.m_sizeInBundle )
		{
			readBuffer->Release();
			continue;
		}

		// yuppi, we have valid data
		m_ramCache->AddCachedData( fileID, placement.m_compression, placement.m_sizeInMemory, readBuffer );
		readBuffer->Release(); // reference held in cache

		// stats
		totalFilesCached += 1;
		totalDataCached += sizeRead;
	}

	// stats
	LOG_CORE( TXT("Cached content of %d files (%1.2f KB) in %1.2fms"), 
		totalFilesCached, totalDataCached / 1024.0f, timer.GetTimePeriodMS() );
	fprintf( stderr, "Cached content of %d files (%1.2f KB) in %1.2fms\n", 
		totalFilesCached, totalDataCached / 1024.0f, timer.GetTimePeriodMS() );
	fflush( stderr );
}

void CDepotBundles::MergeBundleFilters()
{
	const auto prevMerged = m_mergedFilter;

	// we can access only bundles that are installed and are related for current world
	m_mergedFilter = m_installedFilter;
	m_mergedFilter.MergeAnd( m_worldFilter );

	// print report
	Uint32 numMounted=0, numUnmounted=0;
	for ( Uint32 i=1; i<m_bundles.Size(); ++i )
	{
		const Bool wasEnabled = prevMerged.IsBundleEnabled(i);
		const Bool isEnabled = m_mergedFilter.IsBundleEnabled(i);

		const StringAnsi name = m_bundles[i]->GetFullPath();

		if ( wasEnabled && !isEnabled )
		{
			LOG_CORE( TXT("Bundle '%hs': UNMOUNTED"), name.AsChar() );
			numUnmounted += 1;
		}
		else if ( !wasEnabled && isEnabled )
		{
			LOG_CORE( TXT("Bundle '%hs': MOUNTED"), name.AsChar() );
			numMounted += 1;
		}
	}

	LOG_CORE( TXT("Bundle filter changed: %d mounted, %d unmounted"), numMounted, numUnmounted );
}

void CDepotBundles::SwitchWorldBundles( const StringAnsi& worldName )
{
	// clear current mask for worlds
	m_worldFilter.DisableAll();

	// EP2 HACK - there is stuff in Novigrad that needs EP2 bundles
	StringAnsi secondaryWorldName;
	if ( worldName == "novigrad" )
	{
		secondaryWorldName = "bob";
	}

	// filter out bundles only for given world
	for ( CDiskBundle* bundle : m_bundles )
	{
		if ( !bundle )
			continue;

		const StringAnsi& bundleName = bundle->GetShortName();
		const auto bundleId = bundle->GetBundleID();

		// is this a world bundle ?
		if ( bundleName.BeginsWith( "world_" ) )
		{
			// get world name this bundle is for
			const StringAnsi bundleWorldName = bundleName.StringAfter( "world_" ).StringBefore( "_", true );
			if ( bundleWorldName == worldName || (!secondaryWorldName.Empty() && bundleWorldName == secondaryWorldName) )
			{
				// it's a bundle for this world, use it
				m_worldFilter.EnableBundle( bundleId );
			}
		}
		else
		{
			// not a world bundle (generic bundle) - enable it
			m_worldFilter.EnableBundle( bundleId );
		}
	}


	// merge final bundles
	MergeBundleFilters();

	// reset failed flag on all files (EP2 BOB hack)
	// this will allow us to reload the files that may have failed loading because of the cross DLC references
	for ( auto* ptr : m_files )
		if ( ptr != nullptr )
			ptr->EverybodyDeservesASecondChance();
}

CDiskBundle* CDepotBundles::GetStartupBundle( const StringAnsi& worldName ) const
{
	// format the bundle name
	StringAnsi bundleName;
	if ( worldName.Empty() )
	{
		// general engine startup
		bundleName = "startup";
	}
	else
	{
		// world startup
		bundleName = "world_";
		bundleName += worldName;
		bundleName += "_startup";
	}

	// find the bundle
	for ( CDiskBundle* bundle : m_bundles )
	{
#ifdef RED_PLATFORM_CONSOLE
		if(bundle && bundle->GetShortName().ContainsSubstring("bob"))
		{
			if (!bundle->GetFullPath().ContainsSubstring("patch"))
			{
				continue;
			}
		}
#endif
		if ( bundle && (bundle->GetShortName() == bundleName) )
			return bundle;
	}

	// not found
	return nullptr;
}

CDiskBundle* CDepotBundles::GetBundle( const StringAnsi& bundlePath ) const
{
	const Red::Core::Bundle::BundleID id = m_metadataStore->FindBundleID( bundlePath );
	return GetBundle( id );
}

Red::Core::Bundle::FileID CDepotBundles::GetBufferFileID( Red::Core::Bundle::FileID fileID, const Uint32 bufferIndex ) const
{
	return m_metadataStore->GetBufferFileID( fileID, bufferIndex );
}

IFile* CDepotBundles::CreateFileReader( const Red::Core::Bundle::FileID fileID )
{
	// try in the ram cache
	{
		IFile* cachedFile = m_ramCache->CreateReader( fileID );
		if ( cachedFile != nullptr )
			return cachedFile;
	}

	// lookup the best file to read, use current bundle filtering
	// returns access information for file: bundleID, placement, compression, etc
	Red::Core::Bundle::SMetadataFileInBundlePlacement placement;
	if ( !m_metadataStore->GetFileAccessInfo( fileID, m_mergedFilter, /*out*/ placement ) )
	{
		CBundleMetadataStoreFilter filter;
		filter.Initialize( *m_metadataStore );
		filter.EnableAll();
		const AnsiChar* bundlePath = "<NO BUNDLES>";
		if ( m_metadataStore->GetFileAccessInfo( fileID, filter, placement ) )
		{
			const auto bundleID = placement.m_bundleID;
			bundlePath = m_metadataStore->GetBundlePath( bundleID );
		}
		ERR_CORE( TXT("No file placement in currently attached bundles for '%hs' (FileID %u - placement found in '%hs')"), m_metadataStore->GetFilePath( fileID ), fileID, bundlePath );

		return nullptr;
	}

	// get the disk bundle
	CDiskBundle* bundle = GetBundle( placement.m_bundleID );
	if ( !bundle )
	{
		ERR_CORE( TXT("Invalid bundle ID %d: bundle does not exist in depot"), placement.m_bundleID );
		return nullptr;
	}

	// ask the selected bundle to create the file reader
	return bundle->CreateReader( m_dataCache, m_ioCache, placement );
}

EAsyncReaderResult CDepotBundles::CreateAsyncFileReader( const Red::Core::Bundle::FileID fileID, const Uint8 ioTag, IAsyncFile*& outAsyncReader )
{
	// try in the ram cache
	{
		IAsyncFile* cachedFile = m_ramCache->CreateAsyncReader( fileID );
		if ( cachedFile != nullptr )
		{
			outAsyncReader = cachedFile;
			return eAsyncReaderResult_OK;
		}
	}

	// lookup the best file to read, use current bundle filtering
	// returns access information for file: bundleID, placement, compression, etc
	Red::Core::Bundle::SMetadataFileInBundlePlacement placement;
	if ( !m_metadataStore->GetFileAccessInfo( fileID, m_mergedFilter, /*out*/ placement ) )
	{

		CBundleMetadataStoreFilter filter;
		filter.Initialize( *m_metadataStore );
		filter.EnableAll();
		const AnsiChar* bundlePath = "<NO BUNDLES>";
		if ( m_metadataStore->GetFileAccessInfo( fileID, filter, placement ) )
		{
			const auto bundleID = placement.m_bundleID;
			bundlePath = m_metadataStore->GetBundlePath( bundleID );
		}
		ERR_CORE( TXT("No file placement in currently attached bundles for '%hs' (FileID %u - placement found in '%hs')"), m_metadataStore->GetFilePath( fileID ), fileID, bundlePath );

		return eAsyncReaderResult_Failed;
	}

	// get the disk bundle
	CDiskBundle* bundle = GetBundle( placement.m_bundleID );
	if ( !bundle )
	{
		ERR_CORE( TXT("Invalid bundle ID %d: bundle does not exist in depot"), placement.m_bundleID );
		return eAsyncReaderResult_Failed;
	}

	// ask the bundle to create the reader
	return bundle->CreateAsyncReader( placement, ioTag, outAsyncReader );
}

//-------

Bool CDepotBundles::CollectSourceBundles( const String& rootPath, TDynArray< String >& baseBundles, TDynArray< String >& patchBundles ) const
{
	// get all bundles
	TDynArray< String > bundles;
	GFileManager->FindFilesRelative( rootPath, TXT(""), TXT("*.bundle"), bundles, true );

	// sort the in groups: 
	//  base (directory path begins with "content")
	//  patch (directory path begins with "patch")
	//  ignore all other bundles
	for ( const String& localPath : bundles )
	{
		// base bundle
		if ( localPath.BeginsWith( TXT("content") ) )
		{
			LOG_CORE( TXT("Found local bundle '%ls'"), localPath.AsChar() );
			baseBundles.PushBack( localPath );
		}
		else if ( localPath.BeginsWith( TXT("patch") ) )
		{
			LOG_CORE( TXT("Found patch bundle '%ls'"), localPath.AsChar() );
			patchBundles.PushBack( localPath );
		}
		else
		{
			ERR_CORE( TXT("Found unclasified bundle '%ls'!!! Bundle will NOT be included in the game"), localPath.AsChar() );
		}
	}

	// stats
	LOG_CORE( TXT("Found %d base bundles, %d patch bundles"), baseBundles.Size(), patchBundles.Size() );
	return true;
}

Bool CDepotBundles::BuildMetadataStore( const String& rootPath, const String& outPath, const TDynArray< String >& baseBundles, const TDynArray< String >& patchBundles ) const
{
	CTimeCounter rebuildTimer;

	// Construct meta store ad hoc
	Red::Core::Bundle::CBundleMetadataStoreBuilder::Settings settings;
	settings.m_rootPath = m_metadataStore->GetRootPath();
	settings.m_bundlesPaths = baseBundles;

	// The metadata default censorship state is set based on the config
	settings.m_isCensored = false;

	// Extract patch groups
	// TODO: this is LAME
	if ( !patchBundles.Empty() )
	{
		Uint32 numGroupsExtracted = 0;

		const Uint32 maxPatchGroups = 16;
		for ( Uint32 i=0; i<maxPatchGroups; ++i )
		{
			// start building new patch group
			Red::Core::Bundle::CBundleMetadataStoreBuilder::PatchGroup patchGroup;
			patchGroup.m_patchRootPath = rootPath;

			// assign bundles
			const String patchGroupName = String::Printf( TXT("patch%d\\"), i );
			for ( const String& bundleRelativePath : patchBundles )
			{
				if ( bundleRelativePath.ContainsSubstring( patchGroupName ) )
				{
					patchGroup.m_patchBundlesPaths.PushBack( bundleRelativePath );
				}
			}

			// if group is not empty add it to metadata store build list
			if ( !patchGroup.m_patchBundlesPaths.Empty() )
			{
				settings.m_patchGroups.PushBack( patchGroup );
				numGroupsExtracted += 1;
			}
		}

		LOG_CORE( TXT("Extracted %d patch groups"), numGroupsExtracted );
	}

	Red::Core::Bundle::CBundleMetadataStoreBuilder storeBuilder( *GDeprecatedIO );
	storeBuilder.ProcessBundles( settings );
	m_metadataStore->InitializeFromBuilder( storeBuilder );
	LOG_CORE( TXT( "Building the metadata store took %1.1f sec ( %u bundle entries, %u file entries )" ), 
		rebuildTimer.GetTimePeriod(), m_metadataStore->BundleCount(), m_metadataStore->ItemCount() );

	// Validate the store
	if ( !m_metadataStore->Validate() )
	{
		ERR_CORE( TXT( "Generated meta data store failed internal validation. It will not be saved.") );
		return false;
	}

	// Store the meta data
	// NOTE: it's not a fatal error if this fails - we will just rebuild the metadata store next time
	const String storePath = rootPath + TXT( "metadata.store" );
	Red::TScopedPtr<IFile> writer( GFileManager->CreateFileWriter( storePath, FOF_AbsolutePath ) );
	if( writer )
	{
		m_metadataStore->Save( *writer );
	}
	else
	{
		ERR_CORE( TXT("Saving of the metadata store filed. Store will be rebuild again on next run."), storePath.AsChar() );
	}

	// Return true if valid metadata store was build
	return true;
}

namespace Helper
{
	Uint64 ComputeBundleCRC( const String& rootPath, const String& relativePath, Uint64 crc )
	{
		const String absolutePath = rootPath + relativePath;
		if ( GFileManager->FileExist( absolutePath ) )
		{
			Uint64 bundleSize = GFileManager->GetFileSize( absolutePath );
			Uint32 bundleTime = GFileManager->GetFileTime( absolutePath ).GetTimeRaw();

			crc = Red::CalculateHash64( UNICODE_TO_ANSI( absolutePath.AsChar() ), crc );
			crc = Red::CalculateHash64( &bundleTime, sizeof(bundleTime), crc );
			crc = Red::CalculateHash64( &bundleSize, sizeof(bundleSize), crc );
		}

		return crc;
	}

	Uint64 ComputeBundlesCRC( const String& rootPath, const TDynArray<String>& relativePaths, Uint64 crc )
	{
		for ( const String& path : relativePaths )
		{
			crc = ComputeBundleCRC( rootPath, path, crc );
		}

		return crc;
	}

	Bool ReadMagicValue( const String& absolutePath, Uint64& outValue )
	{
		const String truePath = absolutePath + TXT(".stamp");
		Red::TScopedPtr< IFile > file( GFileManager->CreateFileReader( truePath, FOF_AbsolutePath ) );
		if ( file )
		{
			file->Serialize( &outValue, sizeof(outValue) );
			return !file->HasErrors();
		}

		return false;
	}

	Bool StoreMagicValue( const String& absolutePath, Uint64 value )
	{
		const String truePath = absolutePath + TXT(".stamp");
		Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( truePath, FOF_AbsolutePath ) );
		if ( file )
		{
			file->Serialize( &value, sizeof(value) );
			return !file->HasErrors();
		}

		return false;
	}
	
}

Bool CDepotBundles::LoadMetadataStore()
{
	// Determine bundle path
	const String& rootBundlePath = GFileManager->GetBundleDirectory();
	const String storePath = rootBundlePath + TXT( "metadata.store" );
	LOG_CORE( TXT("Bundle metadata store: '%ls'"), storePath.AsChar() );

	// If we are not rebuilding it - try to load it
	Bool rebuildStore = Red::System::StringSearch( SGetCommandLine(), TXT( "-rebuild-store" ) ) != nullptr;
	if ( !rebuildStore )
	{
		CTimeCounter loadTimer;

		Red::TScopedPtr<IFile> reader( GFileManager->CreateFileReader( storePath, FOF_AbsolutePath ) ); // no buffered access needed for the new format of data
		if ( reader )
		{
			if ( m_metadataStore->Load( *reader ) )
			{
				LOG_CORE( TXT( "Loading of the metadata store took %1.2f sec" ), loadTimer.GetTimePeriod() );
			}
			else
			{
				rebuildStore = true;
				ERR_CORE( TXT( "Loading of the metadata store failed, we will try to rebuild it" ) );
			}
		}
		else
		{
			rebuildStore = true;
			ERR_CORE( TXT( "There's no meta store, we will try to rebuild it" ) );
		}
	}

	// PC only - rebuild store ALWAYS if there are bundles that have newer data then the metadata store itself
	// NOTE - if we HAD a bundle in the metadata store that is now gone we need to rebuild to
#ifdef RED_PLATFORM_WINPC
	{
		// get existing bundles
		TDynArray< String > baseBundles, patchBundles;
		CollectSourceBundles( rootBundlePath, baseBundles, patchBundles );

		// compute "magic" value
		Uint64 currentMagicCRC = RED_FNV_PRIME64;
		currentMagicCRC = Helper::ComputeBundlesCRC( rootBundlePath, baseBundles, currentMagicCRC );
		currentMagicCRC = Helper::ComputeBundlesCRC( rootBundlePath, patchBundles, currentMagicCRC );

		// get current "magic"
		Uint64 storedMagicCRC = 0;
		if ( !Helper::ReadMagicValue( storePath, storedMagicCRC ) )
		{
			LOG_CORE( TXT("No metadata store magic value stored. Rebuilding metadata store.") );
			rebuildStore = true;
		}
		else if ( storedMagicCRC != currentMagicCRC )
		{
			LOG_CORE( TXT("Metadata store magic value is not valid. Bundles has changed. Rebuilding metadata store.") );
			rebuildStore = true;
		}

		// Rebuild meta data store
		if ( rebuildStore )
		{
			// Ensure that the folder exists
			GFileManager->CreatePath( rootBundlePath );

			// Delete existing file
			GFileManager->DeleteFile( storePath );

			// Rebuild metadata store and safe it
			if ( !BuildMetadataStore( rootBundlePath, storePath, baseBundles, patchBundles ) )
				return false;
			
			// Store magic value
			Helper::StoreMagicValue( storePath, currentMagicCRC );
		}
	}
#else
	{
		if ( rebuildStore )
		{
			ERR_CORE( TXT("Metadata store CANNOT be rebuild on consoled") );
			return false;
		}
	}
#endif

	// meta store is valid
	return true;
}

Bool CDepotBundles::CreateWrappers()
{
	CTimeCounter timer;

	const Uint32 maxBundles = m_metadataStore->GetMaxBundleID();

	// Create the one single array for all the bundles
	m_bundles.Resize( maxBundles );
	m_bundles[0] = nullptr;

	// Create the bundle wrappers
	for ( Red::Core::Bundle::BundleID id = 1; id < maxBundles; ++id )
	{
		// get bundle path relative to the bundle root
		const AnsiChar* bundleLocalPath = m_metadataStore->GetBundlePath( id );
		if ( !bundleLocalPath )
		{
			ERR_CORE( TXT("Bundle #%d not mapped: no path specified"), id );
			return false;
		}

		// create absolute path to the bundle file
		const String bundleFilePath = m_rootPath + ANSI_TO_UNICODE( bundleLocalPath );
#ifdef RED_PLATFORM_WINPC
		/*if ( !GFileManager->FileExist( bundleFilePath ) )
		{
			ERR_CORE( TXT("Bundle file '%ls' does not exist even though it's referenced in the meta store. Unable to initialize the bundles."), 
				bundleFilePath.AsChar() );
			return false;
		}*/
#endif

		// determine short bundle name
		StringAnsi shortBundleName( bundleLocalPath );
		shortBundleName = shortBundleName.StringAfter( "\\", true );
		shortBundleName = shortBundleName.StringBefore( ".", true );

		// create the wrapper
		m_bundles[id] = new CDiskBundle( shortBundleName, bundleLocalPath, bundleFilePath, id );
	}

	LOG_CORE( TXT( "Creating bundle wrappers took %1.2f sec" ), timer.GetTimePeriod() );
	return true;
}

Bool CDepotBundles::AttachStaticBundle( const StringAnsi& bundlePath )
{
	const Red::Core::Bundle::BundleID id = m_metadataStore->FindBundleID( bundlePath.AsChar() );
	if ( id == 0 )
	{
		ERR_CORE( TXT("Bundle '%hs' not mapped to an ID"), bundlePath.AsChar() );
		return false;
	}

	LOG_CORE( TXT("CDepotBundles: attaching '%hs'"), bundlePath.AsChar() );

	// open the file
	CDiskBundle* diskBundle = GetBundle( id );
	RED_FATAL_ASSERT( diskBundle != nullptr, "Missing bundle for given ID" );
	diskBundle->OnAttached();

	// mark bundle as installed
	m_installedFilter.EnableBundle( id );
	MergeBundleFilters();

	// content attached
	return true;
}

Bool CDepotBundles::AttachDynamicBundles( const String& storeAbsolutePath, const String& bundleRootDirectory, TDepotFileMap& outFileMap )
{
	CTimeCounter timer;

	// open metadata store
	Red::TScopedPtr< IFile > storeFile( GFileManager->CreateFileReader( storeAbsolutePath, FOF_AbsolutePath ) );
	if ( !storeFile.Get() )
	{
		ERR_CORE( TXT("Unable to open dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// load meta data store
	CBundleMetadataStore dynamicStore( bundleRootDirectory );
	if ( !dynamicStore.Load( *storeFile ) )
	{
		ERR_CORE( TXT("Unable to load content from dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// stats
	LOG_CORE( TXT("Loaded store for dynamic content form '%ls, found %d files in %d bundles"), 
		storeAbsolutePath.AsChar(), dynamicStore.EntryCount(), dynamicStore.BundleCount() );

	// attach dynamic content to our main metadata store
	// NOTE: this may fail due to conflicts, should be transactional though
	FileID baseFileID;
	BundleID baseBundleID;
	TDynArray< FileID > addedFiles;
	TDynArray< BundleID > addedBundles;
	if ( !m_metadataStore->AddDynamicBundles( dynamicStore, addedFiles, addedBundles, baseFileID, baseBundleID ) )
	{
		ERR_CORE( TXT("Unable to attach content from dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// create bundle wrappers
	for ( Uint32 i=0; i<addedBundles.Size(); ++i )
	{
		const Red::Core::Bundle::BundleID globalBundleID = addedBundles[i];

		// get bundle path relative to the bundle root
		const Red::Core::Bundle::BundleID localBundleID = globalBundleID - baseBundleID;
		const AnsiChar* bundleLocalPath = dynamicStore.GetBundlePath( localBundleID );
		if ( !bundleLocalPath )
		{
			ERR_CORE( TXT("Bundle #%d from dynamic content not mapped: no path specified"), localBundleID );
			continue;
		}

		// create absolute path to the bundle file
		const String bundleFilePath = bundleRootDirectory + ANSI_TO_UNICODE( bundleLocalPath );

		// determine short bundle name
		StringAnsi shortBundleName( bundleLocalPath );
		shortBundleName = shortBundleName.StringAfter( "\\", true );
		shortBundleName = shortBundleName.StringBefore( ".", true );

		// create entry	
		CDiskBundle* diskBundle = new CDiskBundle( shortBundleName, bundleLocalPath, bundleFilePath, globalBundleID );
		m_bundles.PushBack( diskBundle );

		// make sure the file handle is created
		diskBundle->OnAttached();
	}

	// populate the depot with the new files
	if ( !dynamicStore.PopulateDepot( GDepot, m_files, outFileMap, baseFileID, true, &m_patchedDLCFiles ) )
	{
		ERR_CORE( TXT("Failed to populate depot with content of the loaded dynamic store" ) );
		return false;
	}

	// install all of the dynamic bundles, a little bit hacky
	m_installedFilter.Resize( *m_metadataStore, /* is installed */ true );
	m_worldFilter.Resize( *m_metadataStore, /* is installed */ true );
	m_mergedFilter.Resize( *m_metadataStore, /* is installed */ true );

	// done
	LOG_CORE( TXT("Dynamic store attached in %1.2f ms (%d actual files)"), 
		timer.GetTimePeriodMS(), addedFiles.Size() );
	return true;
}

CBundleDiskFile* CDepotBundles::GetOriginalDLCFile( const CBundleDiskFile& file ) const
{	
	CBundleDiskFile* originalDLCFile = nullptr;
	m_patchedDLCFiles.Find(file.GetFileID(), originalDLCFile);
	return originalDLCFile;
}

Bool CDepotBundles::AttachDynamicOverrideBundles( const String& storeAbsolutePath, const String& bundleRootDirectory, TDepotFileMap& outFileMap )
{
	CTimeCounter timer;

	// open metadata store
	Red::TScopedPtr< IFile > storeFile( GFileManager->CreateFileReader( storeAbsolutePath, FOF_AbsolutePath ) );
	if ( !storeFile.Get() )
	{
		ERR_CORE( TXT("Unable to open dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// load meta data store
	CBundleMetadataStore dynamicStore( bundleRootDirectory );
	if ( !dynamicStore.Load( *storeFile ) )
	{
		ERR_CORE( TXT("Unable to load content from dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// stats
	LOG_CORE( TXT("Loaded store for dynamic content form '%ls, found %d files in %d bundles"), 
		storeAbsolutePath.AsChar(), dynamicStore.EntryCount(), dynamicStore.BundleCount() );

	// attach dynamic content to our main metadata store
	// NOTE: this may fail due to conflicts, should be transactional though
	FileID baseFileID;
	BundleID baseBundleID;
	TDynArray< FileID > addedFiles, overridenFiles, addedFilesOriginalIDs;
	TDynArray< BundleID > addedBundles;
	if ( !m_metadataStore->AddOverrideBundles( dynamicStore, addedFiles, addedFilesOriginalIDs, addedBundles, overridenFiles, baseFileID, baseBundleID ) )
	{
		ERR_CORE( TXT("Unable to attach content from dynamic store '%ls'"), storeAbsolutePath.AsChar() );
		return false;
	}

	// create bundle wrappers
	for ( Uint32 i=0; i<addedBundles.Size(); ++i )
	{
		const Red::Core::Bundle::BundleID globalBundleID = addedBundles[i];

		// get bundle path relative to the bundle root
		const Red::Core::Bundle::BundleID localBundleID = globalBundleID - baseBundleID;
		const AnsiChar* bundleLocalPath = dynamicStore.GetBundlePath( localBundleID );
		if ( !bundleLocalPath )
		{
			ERR_CORE( TXT("Bundle #%d from dynamic content not mapped: no path specified"), localBundleID );
			continue;
		}

		// create absolute path to the bundle file
		const String bundleFilePath = bundleRootDirectory + ANSI_TO_UNICODE( bundleLocalPath );

		// determine short bundle name
		StringAnsi shortBundleName( bundleLocalPath );
		shortBundleName = shortBundleName.StringAfter( "\\", true );
		shortBundleName = shortBundleName.StringBefore( ".", true );

		// create entry	
		CDiskBundle* diskBundle = new CDiskBundle( shortBundleName, bundleLocalPath, bundleFilePath, globalBundleID );
		m_bundles.PushBack( diskBundle );

		// make sure the file handle is created
		diskBundle->OnAttached();
	}

	// manual population of the metadata store (not using the initialization data)
	// populate the depot with the new files
	if ( !dynamicStore.MergeDepot( GDepot, addedFiles, addedFilesOriginalIDs, overridenFiles, m_files, outFileMap, baseFileID ) )
	{
		ERR_CORE( TXT("Failed to populate depot with content of the loaded dynamic store" ) );
		return false;
	}
		
	// install all of the dynamic bundles, a little bit hacky
	m_installedFilter.Resize( *m_metadataStore, /* is installed */ true );
	m_worldFilter.Resize( *m_metadataStore, /* is installed */ true );
	m_mergedFilter.Resize( *m_metadataStore, /* is installed */ true );

	// done
	LOG_CORE( TXT("Dynamic override store attached in %1.2f ms (%d actual files)"), 
		timer.GetTimePeriodMS(), addedFiles.Size() );
	return true;
}

Bool CDepotBundles::PopulateDepot( TDepotFileMap& outFileMap )
{
	CTimeCounter timer;

	m_metadataStore->PopulateDepot( GDepot, m_files, outFileMap, 0 );
	m_metadataStore->DiscardNonPeristentData();

	LOG_CORE( TXT( "Populating the depot took %1.2f sec" ), timer.GetTimePeriod() );
	return true;
}

void CDepotBundles::GetBundleContent( const Red::Core::Bundle::BundleID bundleID, const Uint32 startOffset, const Uint32 endOffset, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement, TDynArray< CBundleDiskFile* >& outFiles ) const
{
	// get entries from meta store
	m_metadataStore->GetFilesInBundle( bundleID, startOffset, endOffset, outFilesPlacement );

	// make sure the file ordering is OK
	Uint32 prevOffset = 0;
	for ( Uint32 i=0; i<outFilesPlacement.Size(); ++i )
	{
		RED_FATAL_ASSERT( outFilesPlacement[i].m_offsetInBundle >= prevOffset, "Invalid ordering of files in the bundle" );
		prevOffset = outFilesPlacement[i].m_offsetInBundle + outFilesPlacement[i].m_sizeInBundle;
	}

	// lookup the disk files
	outFiles.Resize( outFilesPlacement.Size() );
	for ( Uint32 i=0; i<outFilesPlacement.Size(); ++i )
	{
		const Red::Core::Bundle::FileID fileId = outFilesPlacement[i].m_fileID;
		outFiles[i] = m_files[ fileId ];
	}
}

void CDepotBundles::GetBundleContentAll( const Red::Core::Bundle::BundleID bundleID, TDynArray< Red::Core::Bundle::SMetadataFileInBundlePlacement >& outFilesPlacement ) const
{
	m_metadataStore->GetFilesInBundle( bundleID, 0, 0xFFFFFFFF, outFilesPlacement );
}

void CDepotBundles::GetBundleBurstReadRange( const Red::Core::Bundle::BundleID bundleID, Uint32& outStartOffset, Uint32& outEndOffset ) const
{
	Red::Core::Bundle::SMetadataBundleEntry data;
	m_metadataStore->GetBundleInfo( bundleID, data );

	outStartOffset = data.m_dataBlockOffset;
	outEndOffset = data.m_dataBlockOffset + data.m_dataBlockSize;
}
