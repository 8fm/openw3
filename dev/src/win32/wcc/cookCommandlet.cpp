/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/depot.h"
#include "../../common/core/analyzer.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/bundledefinition.h"
#include "../../common/core/dependencyMapper.h"
#include "../../common/core/dependencySaver.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/dependencyLinkerFactory.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/objectMap.h"
#include "../../common/core/nullFile.h"
#include "../../common/core/cooker.h"
#include "../../common/core/speechCollector.h"
#include "cookCommandlet.h"
#include "textureCacheCooker.h"
#include "cookSeedFile.h"
#include "cookDataBase.h"
#include "cookFramework.h"
#include "cookFileWriter.h"
#include "textureCookerNonResource.h"

#include "../../common/engine/textureCache.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/collisionCache.h"

#include "../../common/engine/world.h"
#include "../../common/game/gameworld.h"
#include "../../common/engine/umbraTile.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/core/deferredDataBufferExtractor.h"

#include "../../common/engine/localizationManager.h"

#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"

IMPLEMENT_ENGINE_CLASS(CCookCommandlet)

//--

CCookCommandlet::Settings::Settings()
{
	// default settings
	m_verbose			= false;
	m_cookingPlatform	= PLATFORM_PC; // tempshit!
	m_useTextureCache	= true;
	m_ignoreCookingErrors = false;
	m_skipExistingFiles = false;
	m_createDummyImports = false;
	m_noDataBase		= false;
	m_dataBaseFileName  = TXT("cook.db");
	m_outputPath		= TXT(""); // empty now
	m_stringIDsPath		= TXT( "" );
	m_stringKeysPath	= TXT( "" );
	m_trimDirPath       = TXT( "" );
	m_excludeDirPath    = TXT( "" );
	m_cookDependencies  = true;

	m_dynamicMemoryLimitCPU = 8*1024; // 8192 MB
	m_dynamicMemoryLimitGPU = 256; // 256 MB
}


Bool CCookCommandlet::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	String platformName;
	if ( !options.GetSingleOptionValue( TXT("platform"), platformName ) )
	{
		ERR_WCC( TXT("Expecting platform name") );
		return false;
	}

	// extract the seed files
	if ( options.HasOption( TXT("seed") ) )
	{
		auto args = options.GetOptionValues( TXT("seed") );
		for ( auto it = args.Begin(); it != args.End(); ++it )
			m_seedFilePaths.PushBack( *it );
	}

	// single files
	if ( options.HasOption( TXT("file") ) )
	{
		auto fileList = options.GetOptionValues( TXT("file") );
		for ( auto it = fileList.Begin(); it != fileList.End(); ++it )
			m_manualFiles.PushBack( *it );
	}

	// custom base directory
	if ( options.GetSingleOptionValue( TXT("basedir"), m_customBaseDirectory ) )
	{
		// append path separator
		if ( !m_customBaseDirectory.EndsWith( TXT("\\") ) && !m_customBaseDirectory.EndsWith( TXT("/") ) )
			m_customBaseDirectory += TXT("\\");

		LOG_WCC( TXT("Custom base directory set to '%ls'"), m_customBaseDirectory.AsChar() );
	}

	// mod directory
	String modDir;
	if ( options.GetSingleOptionValue( TXT("mod"), modDir ) )
	{
		// append path separator
		if ( !modDir.EndsWith( TXT("\\") ) && !modDir.EndsWith( TXT("/") ) )
			modDir += TXT("\\");

		TDynArray< String > filesToCook;
		GFileManager->FindFilesRelative( modDir, TXT( "" ), TXT("*.*"), filesToCook, true );

		// no files
		if ( filesToCook.Empty() )
		{
			ERR_WCC( TXT("No modded files found in directory '%ls'"), modDir.AsChar() );
			return false;
		}

		LOG_WCC( TXT("Found %d files in mod directory '%ls'"), 
			filesToCook.Size(), modDir.AsChar() );

		// Add files to the cook.
		for ( const auto& path : filesToCook )
		{
			m_manualFiles.PushBack( path );

			// If the resource is part of a texture array, make sure all needed files are available.
			if( path.StringAfter( TXT( ".texarray.texture_" ) ).EndsWith( TXT( ".xbm" ) ) )
			{
				size_t fileNameIdx = 0;
				if( path.FindSubstring( TXT( "\\" ), fileNameIdx, true, 0 ) )
				{
					const String relativeDepotPath = path.LeftString( fileNameIdx ) + TXT( "\\" );

					size_t extensionIdx = 0;
					String texarrayFileName = path.RightString( path.Size( ) - fileNameIdx - 2 );
					texarrayFileName.FindCharacter( '.', extensionIdx );
					texarrayFileName = texarrayFileName.LeftString( extensionIdx );

					// Copy the uncooked texarray file.
					{
						const String texarrayRelativePath = relativeDepotPath + texarrayFileName + TXT( ".texarray" );
						const String texarrayDepotPath = m_customBaseDirectory + texarrayRelativePath;
						const String texarrayModdedPath = modDir + texarrayRelativePath;

						if( !GFileManager->FileExist( texarrayModdedPath ) )
						{
							GFileManager->CopyFileW( texarrayDepotPath, texarrayModdedPath, false );
							m_manualFiles.PushBack( texarrayRelativePath );
						}
					}

					TDynArray< String > extraFilesToCook;
					String searchPattern = texarrayFileName + TXT( ".texarray.texture_*.xbm" );
					GFileManager->FindFilesRelative( m_customBaseDirectory, relativeDepotPath, searchPattern, extraFilesToCook, false );

					// Copy all uncooked xbm's of the texture array that have not been modified.
					for( const auto& texarrayFile : extraFilesToCook )
					{
						const String depotPath = m_customBaseDirectory + texarrayFile;
						const String moddedPath = modDir + texarrayFile;

						if( !GFileManager->FileExist( moddedPath ) )
						{
							GFileManager->CopyFileW( depotPath, moddedPath, false );
							m_manualFiles.PushBack( texarrayFile );
						}
					}
				}
			}
		}

		// not all files can be there, allow us to create dummy imports
		m_createDummyImports = true;

		// disable dependency following
		m_cookDependencies = false;
	}	

	// we can't use the "seed" and "file" options at the same time
	if ( !m_manualFiles.Empty() && !m_seedFilePaths.Empty() )
	{
		ERR_WCC( TXT("Options 'seed' and 'file' cannot be used at the same time") );
		return false;
	}

	// ignore cooking errors (debug)
	if ( options.HasOption( TXT("noerrors") ) )
	{
		m_ignoreCookingErrors = true;
		WARN_WCC( TXT("Cooking errors WILL be IGNORED! Cooked data may crash the game.") );
	}

	// ignore cooking errors (debug)
	if ( options.HasOption( TXT("verbose") ) )
	{
		m_verbose = true;
		WARN_WCC( TXT("Cooking will use VERBOSE MODE - more stuff will be logged (more spam).") );
	}

	// do not use cooking data base
	if ( options.HasOption( TXT("nodb") ) )
	{
		m_noDataBase = true;
		WARN_WCC( TXT("Cooking DATA BASE will NOT be created." ) );
	}

	// parse custom db file name
	if ( options.GetSingleOptionValue( TXT("db"), m_dataBaseFileName ) )
	{
		LOG_WCC( TXT("Using custom cooking data base output to '%ls'."), m_dataBaseFileName.AsChar() );
	}	

	// skip existing files (debug, semi incremental cooking)
	if ( options.HasOption( TXT("skipexisting") ) )
	{
		m_skipExistingFiles = true;
		WARN_WCC( TXT("Cooking will SKIP EXISTING files. Cooked data may crash the game.") );
	}

	// ignored extensions
	if ( options.HasOption( TXT("ignore") ) )
	{
		TList< String > exts = options.GetOptionValues( TXT("ignore") );
		for ( auto it = exts.Begin(); it != exts.End(); ++it )
		{
			const String ext = (*it).ToLower();
			m_ignoreFileExtensions.Insert(ext);
			WARN_WCC( TXT("Cooking will IGNORE files with extension '%ls'. Cooked data may crash the game."), ext.AsChar() );
		}
	}

	// cooked extensions filter
	if ( options.HasOption( TXT("ext") ) )
	{
		TList< String > exts = options.GetOptionValues( TXT("ext") );
		for ( auto it = exts.Begin(); it != exts.End(); ++it )
		{
			const String ext = (*it).ToLower();
			m_cookedExtension.Insert(ext);
			WARN_WCC( TXT("Cooking will COOK files with extension '%ls'. Rest of the extensions will be ignored."), ext.AsChar() );
		}
	}

	// match platform name
	String platformDirName;
	if ( platformName == TXT("pc") )
	{
		m_cookingPlatform = PLATFORM_PC;
		m_physicalCookingPlatform = PLATFORM_PC;
		platformDirName = TXT("pc");
	}
	else if ( platformName == TXT("resave") )
	{
		m_cookingPlatform = PLATFORM_Resave;
		m_physicalCookingPlatform = PLATFORM_None;
		platformDirName = TXT("pc_resaved");
	}
#ifndef WCC_LITE
	else if ( platformName == TXT("xboxone") || platformName == TXT("durango") )
	{
		m_cookingPlatform = PLATFORM_XboxOne;
		m_physicalCookingPlatform = PLATFORM_XboxOne;
		platformDirName = TXT("xb1");
	}
	else if ( platformName == TXT("ps4") || platformName == TXT("orbis") )
	{
		m_cookingPlatform = PLATFORM_PS4;
		m_physicalCookingPlatform = PLATFORM_PS4;
		platformDirName = TXT("ps4");
	}
#endif
	else if ( platformName == TXT("null") )
	{
		m_cookingPlatform = PLATFORM_Null;
		m_physicalCookingPlatform = PLATFORM_None;
		platformDirName = TXT("null"); // HACK - do not cook, just gather dependencies
	}	
	else
	{
		ERR_WCC( TXT("Invalid platform name: %ls"), platformName.AsChar() );
		return false;
	}

	// default output path
	m_outputPath = GFileManager->GetTempDirectory() + TXT("cooked\\") + platformDirName + TXT("\\");

	// override default output path
	if ( options.GetSingleOptionValue( TXT("outdir"), m_outputPath ) )
	{
		if ( !m_outputPath.EndsWith( TXT("\\") ) && !m_outputPath.EndsWith( TXT("/") ) )
			m_outputPath += TXT("\\");

		LOG_WCC( TXT("Cooker OUTPUT PATH override to '%ls'"), m_outputPath.AsChar() );
	}
	else
	{
		LOG_WCC( TXT("Using default cooker output path '%ls'"), m_outputPath.AsChar() );
	}

	// in the resave/null cook do not create cache files
	if ( m_cookingPlatform == PLATFORM_Null || m_cookingPlatform == PLATFORM_Resave )
	{
		LOG_WCC( TXT("Caches disabled due to fake cooking mode") );
		m_useTextureCache = false;
	}

	// parse the world remapping stuff
	if ( options.HasOption( TXT("remap") ) )
	{
	}

	if( options.HasOption( TXT( "stringids" ) ) )
	{
		options.GetSingleOptionValue( TXT( "stringids" ), m_stringIDsPath );
		LOG_WCC( TXT( "Cooker will capture and save all cooked string ID's into '%s'" ), m_stringIDsPath.AsChar() );
	}

	if( options.HasOption( TXT( "stringkeys" ) ) )
	{
		options.GetSingleOptionValue( TXT( "stringkeys" ), m_stringKeysPath );
		LOG_WCC( TXT( "Cooker will capture and save all cooked string key's into '%s'" ), m_stringKeysPath.AsChar() );		
	}

	if( options.HasOption( TXT( "trimdir" ) ) )
	{
		options.GetSingleOptionValue( TXT( "trimdir" ), m_trimDirPath );
		LOG_WCC( TXT( "Cooker will trim resources from outside of '%s'" ), m_trimDirPath.AsChar() );
	}

	if( options.HasOption( TXT( "excludedir" ) ) )
	{
		options.GetSingleOptionValue( TXT( "excludedir" ), m_excludeDirPath );
		LOG_WCC( TXT( "Cooker will exclude resources from outside of '%s'" ), m_excludeDirPath.AsChar() );
	}

	if( options.HasOption( TXT( "additionalDB" ) ) )
	{
		options.GetSingleOptionValue( TXT("additionalDB"), m_additionalDataBasePath );
		LOG_WCC( TXT( "Cooker will check whether resources can be found in additional database '%s'" ), m_additionalDataBasePath.AsChar() );
	}

	return true;
}

Bool CCookCommandlet::Settings::ShouldIgnoreExtension( const String& ext ) const
{
	if ( m_ignoreFileExtensions.Exist( ext ) )
		return true;

	if ( !m_cookedExtension.Empty() )
	{
		if ( !m_cookedExtension.Exist( ext ) )
			return true;
	}

	return false;
}

//--

class CCookerDependencyImportLoader : public IDependencyImportLoader
{
public:
	CCookerDependencyImportLoader( const Bool fakeDeps )
		: m_createFakeDependencies( fakeDeps )
	{}

	virtual ~CCookerDependencyImportLoader()
	{
		// discard the temporary fake resources
		for ( CResource* res : m_fakeResources )
		{
			res->RemoveFromRootSet();
			res->Discard();
		}

		m_fakeFiles.Clear(); // the files are part of the depot now, do not delete them manually
		m_fakeFilesMap.Clear();
		m_fakeResources.Clear();
	}

	/// Load file as dependency
	virtual BaseSafeHandle LoadDependency( const CClass* resourceClass, const AnsiChar* path, const CDependencyFileData::Import& importData, const Bool silent ) override
	{
		// load normal dependency
		// if we will be able to create fake resource dependency than there's no point to report errors here
		BaseSafeHandle res = IDependencyImportLoader::GetDefault()->LoadDependency( resourceClass, path, importData, /*silent*/ m_createFakeDependencies );

		// there's no resource, create fake one if allowed
		if ( !res.IsValid() && m_createFakeDependencies )
		{
			res = GetFakeResource( resourceClass, ANSI_TO_UNICODE(path) );
			if ( res.IsValid() )
			{
				LOG_WCC( TXT("Using fake import dependency '%hs'"), path );
			}
		}

		return res;
	}

private:
	Bool							m_createFakeDependencies;

	BaseSafeHandle GetFakeResource( const CClass* resourceClass, const String& path )
	{
		RED_FATAL_ASSERT( resourceClass != nullptr, "Invalid resource class" );		
		RED_FATAL_ASSERT( resourceClass->IsA< CResource >(), "Resource class is not actually a resource" );		
		RED_FATAL_ASSERT( !resourceClass->IsAbstract(), "Resource class is abstract" );		

		// get fake file holder
		CDiskFile* file = GetFakeFile( path );

		// no fake file there, we really cannot load it
		if ( !file )
		{
			ERR_WCC( TXT("Unable to create fake file '%ls'. Import will be gone."), path.AsChar() );
			return nullptr;
		}

		// no resource, create it
		if ( !file->GetResource() )
		{
			CResource* res = static_cast< CResource* >( CObject::CreateObjectStatic( (CClass*)resourceClass, nullptr, 0, true ) );
			if ( !res )
			{
				ERR_WCC( TXT("Unable to create fake resource of class '%ls' for file '%ls'"), resourceClass->GetName().AsChar(), path.AsChar() );
				return nullptr;
			}

			// keep the fake resource around
			m_fakeResources.PushBack( res );
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

	CDiskFile* GetFakeFile( const String& path )
	{
		// look up existing file
		CDiskFile* file = nullptr;
		if ( m_fakeFilesMap.Find( path, file ) )
		{
			RED_FATAL_ASSERT( file->GetResource() != nullptr, "Fake file has no resource" );
			return file;
		}

		// disassemble path
		const CFilePath fullPath( path );

		// create fake file
		CDiskFile* fakeFile = GDepot->CreateNewFile( path.AsChar() );
		RED_FATAL_ASSERT( fakeFile != nullptr, "Failed to create fake dependency file" );

		// map
		m_fakeFiles.PushBack( fakeFile );
		m_fakeFilesMap.Set( path, fakeFile );
		return fakeFile;
	}

	TDynArray< CDiskFile* >				m_fakeFiles;
	TDynArray< CResource* >				m_fakeResources;
	THashMap< String, CDiskFile* >		m_fakeFilesMap;
};

// hack
IDependencyImportLoader* CreateDepencencyLoaderWithFakeDependencies( const Bool fakeImports )
{
	return new CCookerDependencyImportLoader( fakeImports );
}

//---

void CCookCommandlet::CookingStats::Clear()
{
	m_totalLoadingTime = 0.0;
	m_totalSavingTime = 0.0;
	m_totalCookingTime = 0.0;
	m_totalCopyTime = 0.0;
	m_totalCookedObjects = 0;
	m_stats.Clear();
}

void CCookCommandlet::CookingStats::AddLoading( Double time )
{
	m_totalLoadingTime += time;
}

void CCookCommandlet::CookingStats::AddSaving( Double time )
{
	m_totalSavingTime += time;
}

void CCookCommandlet::CookingStats::AddCopying( Double time )
{
	m_totalCopyTime += time;
}

void CCookCommandlet::CookingStats::ReportCookingTime( const CClass* objectClass, const Double time )
{
	PerClassStat* stats = m_stats.FindPtr( objectClass );
	if ( stats != nullptr )
	{
		stats->m_count += 1;
		stats->m_time += time;
	}
	else
	{
		PerClassStat stats;
		stats.m_class = objectClass;
		stats.m_count = 1;
		stats.m_time = time;
		m_stats.Insert( objectClass, stats );
	}

	m_totalCookingTime += time;
	m_totalCookedObjects += 1;
}

void CCookCommandlet::CookingStats::Dump()
{
	LOG_WCC( TXT("Total loading time: %1.2fs"), m_totalLoadingTime );
	LOG_WCC( TXT("Total saving time: %1.2fs"), m_totalSavingTime );
	LOG_WCC( TXT("Total cooking time: %1.2fs (%d objects)"), m_totalCookingTime, m_totalCookedObjects );
	
	TDynArray< PerClassStat > stats;
	stats.Reserve( m_stats.Size() );
	for ( auto it=m_stats.Begin(); it != m_stats.End(); ++it )
	{
		stats.PushBack((*it).m_second);
	}
	::Sort(stats.Begin(), stats.End(), [](const PerClassStat& a, const PerClassStat& b) { return a.m_time > b.m_time; } );

	LOG_WCC( TXT("Per class cooking stats (%d classes):"), m_stats.Size() );
	for ( auto s : stats )
	{
		LOG_WCC( TXT("  %1.3fs: %ls (%d objects)"), 
			s.m_time, s.m_class->GetName().AsChar(), s.m_count );
	}
}

//--

CCookCommandlet::CookedResourceInfo::CookedResourceInfo(const String& depotFilePath)
	: m_isSeedFile( false )
	, m_isInHotList( false )
	, m_isCooked( false )
	, m_isPending( false )
	, m_path(depotFilePath)
	, m_pathAnsi(UNICODE_TO_ANSI(depotFilePath.AsChar()))
	, m_sourceHash(0) // not yet initialized
	, m_sourceDateTime() // not yet initialized
	, m_numHardImports(0)
	, m_numSoftImports(0)
	, m_depotFile(NULL)
{
}

CCookCommandlet::CookedResourceInfo::~CookedResourceInfo()
{
}

//--

CCookCommandlet::CCookCommandlet()
	: m_dataBase( NULL )
	, m_additionalDataBase( NULL )
	, m_dependencyLoaderWithFakeFiles( NULL )
{
	m_commandletName	= CName( TXT("cook") );
}

CCookCommandlet::~CCookCommandlet()
{
	Reset();
}

void CCookCommandlet::Reset()
{
	delete m_dataBase;
	m_dataBase = NULL;

	delete m_additionalDataBase;
	m_additionalDataBase = NULL;

	delete m_errors;
	m_errors = NULL;

	delete m_dependencyLoaderWithFakeFiles;
	m_dependencyLoaderWithFakeFiles = NULL;

	m_files.ClearPtrFast();
	m_pendingFilesToCook.Clear();

	m_fileCounter = 0;
	m_lastFlushCounter = 0;
}

namespace
{
	template < typename T >
	static Float Prc( T val, T max ) 
	{
		return (Float)( ((Double)val / (Double)max) * 100.0 );
	}
}

void CCookCommandlet::PreserveState()
{
	const Uint32 dataBaseFlushFrequency = 5000;

	// flush current state of cooker DB after every N cooked files
	if ( m_fileCounter >= (m_lastFlushCounter + dataBaseFlushFrequency) )
	{
		m_lastFlushCounter = m_fileCounter;

		if ( m_dataBase )
		{
			m_dataBase->SaveToFile( m_dataBasePath );
		}
	}
}

void CCookCommandlet::CleanupMemory()
{
	const Uint32 totalAllocateGPU_MB = (Uint32)(GpuApi::GpuApiMemory::GetInstance()->GetMetricsCollector().GetTotalBytesAllocated() >> 20);
	const Uint32 totalAllocateCPU_MB = (Uint32)(Memory::GetTotalBytesAllocated() >> 20);
	const Uint32 totalObjects = GObjectsMap->GetNumLiveObjects();

	const Uint32 maxObjects = (GObjectsDiscardList->GetCapacity() * 5) / 6;

	if ( (totalAllocateCPU_MB > m_settings.m_dynamicMemoryLimitCPU) ||
		(totalAllocateGPU_MB > m_settings.m_dynamicMemoryLimitGPU) ||
		(totalObjects > maxObjects) )
	{
		LOG_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!") );
		LOG_WCC( TXT("!! MEMORY LIMIT REACHED !!") );
		LOG_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!") );
		LOG_WCC( TXT("") );

		LOG_WCC( TXT("CPU budget: (%3.1f%%) %d/%d MB"), 
			Prc(totalAllocateCPU_MB, m_settings.m_dynamicMemoryLimitCPU), 
			totalAllocateCPU_MB, m_settings.m_dynamicMemoryLimitCPU );
		LOG_WCC( TXT("GPU budget: (%3.1f%%) %d/%d MB"), 
			Prc(totalAllocateGPU_MB, m_settings.m_dynamicMemoryLimitGPU), 
			totalAllocateGPU_MB, m_settings.m_dynamicMemoryLimitGPU );
		LOG_WCC( TXT("OBJ budget: (%3.1f%%) %d/%d"), 
			Prc(totalObjects, maxObjects),
			totalObjects, maxObjects );

		LOG_WCC( TXT("") );
		RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( "CookCommandlet" );

		// cleanup the hot list
		m_hotList.Clear();

		// do memory cleanup cycle
		{
			SGarbageCollector::GetInstance().CollectNow();
			GObjectsDiscardList->ProcessList( true );
		}

		const Uint32 newTotalAllocateGPU_MB = (Uint32)(GpuApi::GpuApiMemory::GetInstance()->GetMetricsCollector().GetTotalBytesAllocated() >> 20);
		const Uint32 newTotalAllocateCPU_MB = (Uint32)(Memory::GetTotalBytesAllocated() >> 20);
		const Uint32 newTotalObjects = GObjectsMap->GetNumLiveObjects();

		LOG_WCC( TXT("CPU budget after cleanup: (%3.1f%%) %d/%d MB"), 
			Prc(newTotalAllocateCPU_MB, m_settings.m_dynamicMemoryLimitCPU), 
			newTotalAllocateCPU_MB, m_settings.m_dynamicMemoryLimitCPU );
		LOG_WCC( TXT("GPU budget after cleanup: (%3.1f%%) %d/%d MB"), 
			Prc(newTotalAllocateGPU_MB, m_settings.m_dynamicMemoryLimitGPU), 
			newTotalAllocateGPU_MB, m_settings.m_dynamicMemoryLimitGPU );
		LOG_WCC( TXT("OBJ budget after cleanup: (%3.1f%%) %d/%d"), 
			Prc(newTotalObjects, maxObjects),
			newTotalObjects, maxObjects );

		// we failed to clean up - a memory leak in the cooker
		if ( newTotalAllocateCPU_MB > m_settings.m_dynamicMemoryLimitCPU )
		{
			ERR_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") );
			ERR_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") );
			ERR_WCC( TXT("!!!         FATAL OUT OF MEMORY IN THE COOKER           !!!") );
			ERR_WCC( TXT("!!!     There's more than 8GB of memory allocated       !!!") );
			ERR_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") );
			ERR_WCC( TXT("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!") );

			RED_FATAL( "Cooker run out of memory - look at the memory metrics and check recent changes" );
			return;
		}
	}
}

void CCookCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  cook <options>") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Platform:") );
	LOG_WCC( TXT("   null	- Dummy cooker output (only cook.db)") );
	LOG_WCC( TXT("   resave - Resave assets (no cooking, PC only)") );
	LOG_WCC( TXT("   pc		- 64-bit PC") );
	LOG_WCC( TXT("   ps4	- PlayStation 4") );
	LOG_WCC( TXT("   xb1	- XBoxOne") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Options:") );
	LOG_WCC( TXT("  -platform=<name>    - Select target platform (required)") );
	LOG_WCC( TXT("  -outdir=<path>      - Sepcify output directory (required)") );
	LOG_WCC( TXT("  -seed=<path>        - Input seed file (required)") );
	LOG_WCC( TXT("  -file=<path>        - Manual depot file for cooking") );
	LOG_WCC( TXT("  -noerrors           - Allow cooking errors") );
	LOG_WCC( TXT("  -noasserts          - Allow asserts") );
	LOG_WCC( TXT("  -silent             - No output") );
	LOG_WCC( TXT("  -ignore=<file>      - Ignore files with given extension") );
	LOG_WCC( TXT("  -stringids=<file>   - Capture and save all cooked string ID's") );
	LOG_WCC( TXT("  -stringkeys=<file>  - Capture and save all cooked string key's") );
	LOG_WCC( TXT("  -excludedir=<path>  - Don't cook resources from one directory") );
	LOG_WCC( TXT("  -trimdir=<path>     - Cook resources only from one directory") );
	LOG_WCC( TXT("  -trimedfiles=<file> - List of trimmed files from cook") );
	LOG_WCC( TXT("  -additionalDB=<file>- Cook.db file to check if it containes trimmed files") );
}

Bool CCookCommandlet::Execute( const CommandletOptions& options )
{
	// get name of the platform
	if ( !m_settings.Parse( options ) )
		return false;

	// repopulate the depot from another directory
	if ( !m_settings.m_customBaseDirectory.Empty() )
	{
		String modDir;
		if ( options.GetSingleOptionValue( TXT( "mod" ), modDir ) )
		{
			if( !modDir.EndsWith( TXT("\\") ) && !modDir.EndsWith( TXT("/") ) )
				modDir += TXT("\\");
			GDepot->Remap( modDir );
		}
		else
		{
			GDepot->Remap( m_settings.m_customBaseDirectory );
		}
	}

	// reset files
	Reset();

	// create handler for missing dependencies
	m_dependencyLoaderWithFakeFiles = new CCookerDependencyImportLoader( m_settings.m_createDummyImports );

	// Layer cooking disabled again. We consume more memory with this on. 
	ICooker::EnableCookerClasses( ClassID<CLayer>(), false );

	// create error reporter
	m_errors = new CCookerErrorReporter();

	// load cooking data base
	if ( !m_settings.m_noDataBase )
	{
		m_dataBase = new CCookerDataBase();
		m_dataBasePath = m_settings.m_outputPath + m_settings.m_dataBaseFileName;

		// create or load existing DB
		if ( !m_dataBase->LoadFromFile( m_dataBasePath.AsChar() ) )
		{
			WARN_WCC( TXT("Failed to load existing cooking data base from '%ls'. New one will be generated."),
				m_dataBasePath.AsChar() );
		}
	}

	if ( m_settings.m_additionalDataBasePath.GetLength() > 0 )
	{
		m_additionalDataBase = new CCookerDataBase();

		if ( !m_additionalDataBase->LoadFromFile( m_settings.m_additionalDataBasePath.AsChar() ) )
		{
			ERR_WCC( TXT("Failed to load additional cooking data base from '%ls'."),
				m_settings.m_additionalDataBasePath );
			return false;
		}
	}

	// initialize caches
	if ( m_settings.m_cookingPlatform != PLATFORM_Null )
	{
		// create output path
		if ( !GFileManager->CreatePath( m_settings.m_outputPath ) )
		{
			ERR_WCC( TXT("Failed to create output directory '%ls'"),
				m_settings.m_outputPath.AsChar() );

			return false;
		}

		// Create texture cache
		if ( m_settings.m_useTextureCache )
		{
			// We don't attach the default texcache cooker to any file. It should only be written to during the texcache builder, which
			// creates its own temporary cooker. We just need to create this here so that we can get the proper cooking function.
			RED_LOG( Cooker, TXT("Initializing texture cache...") );
			GTextureCacheCooker = new CWccTextureCacheCooker();
		}
	}
		
	// process the seed list
	if ( !m_settings.m_seedFilePaths.Empty() )
	{
		for ( const String& filePath : m_settings.m_seedFilePaths )
			if ( !AddSeedFile( filePath ) )
			{
				ERR_WCC( TXT("Failed to add seed file '%ls' to cooking list"), filePath.AsChar() );
			}

	}
	else if ( !m_settings.m_manualFiles.Empty() )
	{
		for ( const String& filePath : m_settings.m_manualFiles )
			if ( !AddFile( filePath ) )
			{
				ERR_WCC( TXT("Failed to add '%ls' to cooking list"), filePath.AsChar() );
			}
	}

	// no files to process
	LOG_WCC( TXT("Found %d files in the initial cooking list"), 
		m_pendingFilesToCook.Size() );
	if ( m_pendingFilesToCook.Empty() )
	{
		ERR_WCC( TXT("No files added from in the initial seed lists") );
		return false;
	}

	// dump the list of initial files
#if 0
	{
		FILE* f = fopen("Z:\\initial_cook_files.txt","w");
		if ( f )
		{
			for (Uint32 i=0; i<m_pendingFilesToCook.Size(); ++i )
			{
				fwprintf( f, TXT("Cook[%d]: %ls\n"), i, m_pendingFilesToCook[i]->m_path.AsChar() );
			}
			fclose(f);
		}
	}
#endif

	// reset cooking stats
	m_stats.Clear();

	// cook files
	if ( !CookPendingFiles() )
		return false;

	// cooked !
	LOG_WCC( TXT("Cooked %d files in total"), m_files.Size() );

	// dump stats
	m_stats.Dump();

	// dump layer cooking stats
	extern void DumpLayerCookingStats();
	DumpLayerCookingStats();

	// output the cook list
	if ( m_dataBase )
	{
		if ( !m_dataBase->SaveToFile( m_dataBasePath ) )
		{
			ERR_WCC( TXT("Failed to save cooker data base file '%ls'"),
				m_dataBasePath.AsChar() );
		}
	}

	// Create texture cache
	if ( m_settings.m_useTextureCache )
	{
		RED_LOG( Cooker, TXT("Flushing texture cache...") );
		GTextureCacheCooker->Flush();
		delete GTextureCacheCooker;
		GTextureCacheCooker = NULL;
	}

	// Output the collected string ids.
	if ( !m_speechMap.Empty() || !m_stringKeysMap.Empty() )
	{
		// Make sure that stringIDs for the stringKeys are included
		for( TStringKeysMap::const_iterator it = m_stringKeysMap.Begin( ); it != m_stringKeysMap.End( ); ++it )
		{
			const auto& path = it->m_first;

			for( const String& key : it->m_second )
			{
				const auto id = SLocalizationManager::GetInstance().GetStringIdByStringKey( key );
				if ( id != 0 )
				{
					TSpeechIDs& ids = m_speechMap.GetRef( path );
					ids.PushBackUnique( id );

					LOG_WCC( TXT("Matched string key '%ls' to ID %d. Reporting it as used by resource '%ls'"), key.AsChar(), id, path.AsChar() );
				}
			}
		}

		// save the maps, BOTH of them
		SaveStringKeysMap();
		SaveSpeechMap();
	}

	// flush errors to log
	if ( m_errors )
		m_errors->DumpErrorsToLog();

	// done
	return true;		
}

Bool CCookCommandlet::AddSeedFile( const String& seedFilePath )
{
	// load the seed file
	CCookerSeedFile seedFile;
	if ( !seedFile.LoadFromFile( seedFilePath ) )
	{
		ERR_WCC( TXT("Failed to read seed file from '%ls'"), seedFilePath.AsChar() );
		return false;
	}

	// process the files
	const Uint32 numStartFiles = m_files.Size();
	const Uint32 numFiles = seedFile.GetNumEntries();
	for (Uint32 j=0; j<numFiles; ++j)
	{
		const CCookerSeedFileEntry* fileDesc = seedFile.GetEntry(j);

		// make sure the depot file exist
		const String fileDepotPath = ANSI_TO_UNICODE( fileDesc->GetFilePath().AsChar() );
		const String absolutePath = GFileManager->GetDataDirectory() + fileDepotPath;
		if ( !GFileManager->FileExist(absolutePath) && !GDepot->FindFileUseLinks( fileDepotPath, 0 ) )
		{
			// HACK: buffers :(
			if ( fileDepotPath.EndsWith( TXT("buffer") ) )
				continue;

			ERR_WCC( TXT("File '%ls' in bundle '%ls' in file '%ls' does not exist in the depot"), 
				fileDepotPath.AsChar(), ANSI_TO_UNICODE( fileDesc->GetBundleName().AsChar() ), seedFilePath.AsChar() );

			if ( !m_settings.m_ignoreCookingErrors )
				return false;

			continue;
		}

		// create bundle definition (if does not exist yet)
		CookedResourceInfo* file = AddFile( fileDepotPath );
		file->m_isSeedFile = true;
	}

	// stats
	LOG_WCC( TXT("Parsed seed list '%ls', found %d seed files to cook"),
		seedFilePath.AsChar(), 
		(m_files.Size() - numStartFiles) ); 

	// done
	return true;
}

CCookCommandlet::CookedResourceInfo* CCookCommandlet::AddFile( const String& unsafeDepotPath )
{
	// TODO: the incoming depot paths should be conformed to a single standard, right now force the format
	String tempPath;
	const String& depotPath = CFilePath::ConformPath( unsafeDepotPath, tempPath );

	// try to use existing
	CCookCommandlet::CookedResourceInfo* ret = NULL;
	if ( !m_files.Find( depotPath, ret ) )
	{
		ret = new CCookCommandlet::CookedResourceInfo( depotPath );
		m_files.Insert( depotPath, ret );

		// file has pending cook
		ret->m_isPending = true;
		ret->m_isSeedFile = true;
		m_pendingFilesToCook.PushBack(ret);
	}

	// make sure the censored version of file is also cooked (if we have it)
	const Char* censoredFileMarker = TXT("_censored");
	if ( nullptr == Red::StringSearch( depotPath.AsChar(), censoredFileMarker ) )
	{
		CFilePath fullFilePath( depotPath );
		fullFilePath.SetFileName( fullFilePath.GetFileName() + censoredFileMarker );

		// do we have the file on disk ?
		const String censoredFilePath = fullFilePath.ToString();
		if ( GDepot->FindFile( censoredFilePath.AsChar() ) != nullptr )
		{
			// add to to cooking
			CCookCommandlet::CookedResourceInfo* censoredCrap = AddFile( censoredFilePath );

			// make sure files are dragged along
			ret->m_softDependencies.PushBack( censoredCrap ); 
			censoredCrap->m_numSoftImports += 1;
		}
	}

	return ret;
}

CCookCommandlet::CookedResourceInfo* CCookCommandlet::GetFileToProcess( Uint32& localPendingIndex, TPendingCookFiles& localPendingList )
{
	// try to get file from hot list (it's already loaded)
	{
		Red::Threads::CScopedLock< Mutex > lock( m_hotListLock );
		if ( !m_hotList.Empty() )
		{
			CookedResourceInfo* ret = m_hotList.Front(); // FIFO list
			ret->m_isInHotList = false;
			m_hotList.PopFront();

			// check that the file is still loaded - it may be not
			if ( !ret->m_depotFile || !ret->m_depotFile->IsLoaded() )
			{
				ERR_WCC( TXT("!! HotList trashed by some rouge GC/Discard !!") );
				ERR_WCC( TXT("!! Resource trashed: '%ls' !!"), ret->m_path.AsChar() );

				m_hotList.Clear();
			}
			else
			{
				// use the hot resource
				ret->m_isPending = false;
				return ret;
			}
		}
	}

	// get resource from normal list
	if ( localPendingIndex < localPendingList.Size() )
	{
		CookedResourceInfo* ret = localPendingList[ localPendingIndex++ ]; // preserve order
		ret->m_isPending = false;
		return ret;
	}

	// no more resources
	return nullptr;
}

Bool CCookCommandlet::CookPendingFiles()
{
	CTimeCounter cookTimer;

	// cook while there are pending files
	Uint32 passIndex = 0;
	while ( !m_pendingFilesToCook.Empty() )
	{
		TPendingCookFiles filesThisPass;
		Swap( filesThisPass, m_pendingFilesToCook );

		// timer and stats
		CTimeCounter timer;
		Uint32 startFileCounter = m_fileCounter;

		// process files in a loop
		Uint32 fileIndex = 0;
		while ( 1 )
		{
			// pop a file to process
			CookedResourceInfo* info = GetFileToProcess( fileIndex, filesThisPass );
			if ( !info )
				break;

			// show progress - only in verbose mode (waste of time)
			if ( m_settings.m_verbose )
			{
				LOG_WCC( TXT("Status: Pass %d, [%d/%d] Processing '%ls'..."), 
					passIndex, fileIndex, filesThisPass.Size(), info->m_path.AsChar() );
			}

			// resource is already cooked (was in the hot list at some earlier stage)
			if ( info->m_isCooked )
				continue;

			// cook single file
			if ( !CookSingleFile( info ) )
			{
				ERR_WCC( TXT("Failed to cook '%ls' !"), 
					info->m_path.AsChar() );
			}

			// mark resource as cooked
			info->m_isCooked = true;

			// do optional memory cleanup
			CleanupMemory();

			// flush the DB (conditional)
			PreserveState();
		}

		// flush writing queue
		SCookerOutputFileManager::GetInstance().Flush();

		// print pass info
		{
			const Uint32 numFilesProcessed = m_fileCounter - startFileCounter;
			fprintf( stdout, "Pass %d, cooked %d files in %1.2fs\n", 
				passIndex, numFilesProcessed, timer.GetTimePeriod() ); // DIRECT OUTPUT ! (forced)
			passIndex += 1;
		}

		// flush DB state
		if ( m_dataBase )
		{
			m_dataBase->SaveToFile( m_dataBasePath );
		}
	}

	// finished, no cooking errors
	LOG_WCC( TXT("Cooking finished in %1.2fs"), cookTimer.GetTimePeriod() );
	return true;
}

Bool CCookCommandlet::IsResourceFile( const CookedResourceInfo* resourceInfo, const String& fileExtension ) const
{
	// TODO: this list should be either passed as command line parameter or be read from some file

	// list of known extensions that are not resources
	if ( fileExtension == TXT("xml") ) return false;
	if ( fileExtension == TXT("swf") ) return false;
	if ( fileExtension == TXT("usm") ) return false;
	if ( fileExtension == TXT("subs") ) return false;

	// hack for David and ModTools
#ifdef WCC_LITE
	if ( fileExtension == TXT("redswf") ) return false;
#endif

	// loose texture files
	if ( fileExtension == TXT("dds") ) return false;
	if ( fileExtension == TXT("png") ) return false;
	if ( fileExtension == TXT("jpg") ) return false;
	if ( fileExtension == TXT("jpeg") ) return false;	

	// navigation files (they are outside resource system)
	if ( fileExtension == TXT("navconfig") ) return false;
	if ( fileExtension == TXT("navgraph") ) return false;
	if ( fileExtension == TXT("navmesh") ) return false;
	if ( fileExtension == TXT("naviobstacles") ) return false;
	if ( fileExtension == TXT("navtile") ) return false;

	// TODO: csv files can be cooked (so we don't have to parse the text file, right now this is not working so just copy the file)
	if ( fileExtension == TXT("csv") ) return false;
	if( fileExtension == TXT("buffer") ) return false;

	// Game save files can be attached to game data, but are no resources
	if ( fileExtension == TXT("sav") ) return false;

	// rest of the files should be cooked
	return true;
}

Bool CCookCommandlet::CookLooseTextureFile( CookedResourceInfo* resourceInfo )
{
	// raw textures are not cooked in the NULL mode
	if ( m_settings.m_cookingPlatform == PLATFORM_Null )
		return true;

	// raw textures are just copied in the Resave mode
	if ( m_settings.m_cookingPlatform == PLATFORM_Resave )
		return CopyLooseFile( resourceInfo );

	// We don't need to do anything for an actual cook, because these will be dealt with in the texture cache builder.

	// create the DB entry for the loose texture.
	if ( m_dataBase )
	{
		const CCookerDataBase::TCookerDataBaseID dbID = m_dataBase->AddFileEntry( resourceInfo->m_path );
		m_dataBase->SetFileConsumedFlag( dbID, true ); // file was consumed by the cooker
		m_dataBase->SetFileDiskSize( dbID, 0 ); // not stored in the cooked data
	}

	return true;
}

Bool CCookCommandlet::CopyLooseFile( CookedResourceInfo* resourceInfo )
{
	const String depotPath = GFileManager->GetDataDirectory();
	const String inputFilePath = depotPath + resourceInfo->m_path;

	// stats
	if ( m_settings.m_verbose )
	{
		LOG_WCC( TXT("Copying '%ls'..."), resourceInfo->m_path.AsChar() );
	}

	// create input reader
	IFile* inputFile = GFileManager->CreateFileReader( inputFilePath, FOF_AbsolutePath );
	if ( !inputFile )
	{
		ERR_WCC( TXT("Copy of '%ls' failed: input file does not exit!"), resourceInfo->m_path.AsChar() );
		return false;
	}

	// create output file
	const Uint32 asyncCopySizeLimit = 50 * 1024 * 1024;
	const String outputFilePath = m_settings.m_outputPath + resourceInfo->m_path;
	const Bool asyncWrite = (inputFile->GetSize() < asyncCopySizeLimit);
	IFile* outputFile = CreateOutputFile( outputFilePath, asyncWrite );
	if ( !outputFile )
	{
		delete inputFile;
		ERR_WCC( TXT("Copy of '%ls' failed: unable to create output file!"), resourceInfo->m_path.AsChar() );
		return false;
	}

	CTimeCounter copyTimer;

	// copy
	// TODO: move to CFileSystem
	Uint64 pos = 0;
	const Uint64 size = inputFile->GetSize();
	while ( pos < size )
	{
		Uint8 buffer[4096];

		const Uint64 blockSize = Red::Math::NumericalUtils::Min< Uint64 >( size-pos, sizeof(buffer) );
		inputFile->Serialize( buffer, (size_t)blockSize );
		outputFile->Serialize( buffer, (size_t)blockSize );

		pos += blockSize;
	}

	// update stats
	m_stats.AddCopying(copyTimer.GetTimePeriod());

	// close files
	delete inputFile;
	delete outputFile;

	// update DB entry with file size
	if ( m_dataBase )
	{
		const CCookerDataBase::TCookerDataBaseID dbID = m_dataBase->AddFileEntry( resourceInfo->m_path );
		m_dataBase->SetFileDiskSize( dbID, (const Uint32)size );
	}

	return true;
}

Bool CCookCommandlet::CookNonResourceFile( CookedResourceInfo* resourceInfo )
{
	const String fileExt = StringHelpers::GetFileExtension( resourceInfo->m_path );

	// HACK: a special way to handle loose .png and .dds files
	if ( fileExt == TXT("png") || fileExt == TXT("dds") || fileExt == TXT("jpg") )
	{
		return CookLooseTextureFile( resourceInfo );
	}

	// just copy the file from data directory to cooked output
	return CopyLooseFile( resourceInfo );
}

Bool CCookCommandlet::ProcessExistingFile( CookedResourceInfo* resourceInfo )
{
	// naive skipping
	if ( m_settings.m_verbose )
	{
		WARN_WCC( TXT("Skipping '%ls'"), resourceInfo->m_path.AsChar() );
	}

	// no cooker data base - try to extract dependencies manually (unsafe)
	if ( !m_dataBase )
	{
		WARN_WCC( TXT("No cooking data base. No dependencies will be visited for '%ls'. Cook may be incomplete." ),
			resourceInfo->m_path.AsChar() );
		return false;
	}

	// look up file ID in the cooking DB
	const CCookerDataBase::TCookerDataBaseID dbID = m_dataBase->GetFileEntry( resourceInfo->m_path );
	if ( dbID == CCookerDataBase::NO_ENTRY )
	{
		WARN_WCC( TXT("Unable to find entry for '%ls' in existing cooking data base. Cook is incomplete or damaged." ),
			resourceInfo->m_path.AsChar() );
		return false;
	}

	// update file size
	{
		const String fileExtension = StringHelpers::GetFileExtension( resourceInfo->m_path );
		const Uint64 fileSize = GFileManager->GetFileSize( m_settings.m_outputPath + resourceInfo->m_path );
		if ( fileSize > 0 )
		{
			m_dataBase->SetFileDiskSize( dbID, (const Uint32)fileSize );
		}
	}

	// get dependencies from the cooking data base
	typedef TDynArray< CCookerResourceEntry > TCookerFileDependencies;
	TCookerFileDependencies hardDeps, softDeps, inplaceDeps;
	m_dataBase->GetFileDependencies( dbID, hardDeps, softDeps, inplaceDeps );

	// feed hard dependencies back into the system
	for ( Uint32 i=0; i<hardDeps.Size(); ++i )
	{
		const StringAnsi& dependencyDepotPath = hardDeps[i].GetFilePath();

		CookedResourceInfo* dependency = ResolveHardDependency( resourceInfo, ANSI_TO_UNICODE( dependencyDepotPath.AsChar() ) );
		if (dependency)
		{
			resourceInfo->m_hardDependencies.PushBack(dependency);
		}
	}

	// feed soft dependencies back into the system
	for ( Uint32 i=0; i<softDeps.Size(); ++i )
	{
		const StringAnsi& dependencyDepotPath = softDeps[i].GetFilePath();

		CookedResourceInfo* dependency = ResolveSoftDependency( resourceInfo, ANSI_TO_UNICODE( dependencyDepotPath.AsChar() ) );
		if (dependency)
		{
			resourceInfo->m_softDependencies.PushBack(dependency);
		}
	}

	// done
	return true;
}

IFile* CCookCommandlet::CreateOutputFile( const String& outputFilePath, const Bool asyncWrite )
{
	// null cooker
	if ( m_settings.m_cookingPlatform == PLATFORM_Null )
	{
		return new CNullFileWriter();	
	}

	// HACK: make sure the output path exists (the copy file may have trouble creating it)
	if ( !GFileManager->CreatePath( outputFilePath ) )
		return false;

	// async write
	if ( asyncWrite )
	{
		IFile* file = SCookerOutputFileManager::GetInstance().CreateWriter( outputFilePath );
		if ( file )
			return file;
	}

	// open normal file writer
	return GFileManager->CreateFileWriter( outputFilePath, FOF_AbsolutePath | FOF_Buffered );
}

extern const Red::System::AnsiChar* GCurrentCookedResourcePath;
extern Red::System::Bool GDeterministicGUIDGeneration;
extern Uint32 GCurrentCookedResourceGUIDIndex;

class CCookFileHelper
{
public:
	CCookFileHelper( const AnsiChar* resourcePath )
	{
		m_prev = GDeterministicGUIDGeneration;
		GDeterministicGUIDGeneration = true;

		GCurrentCookedResourcePath = resourcePath;
		GCurrentCookedResourceGUIDIndex = 0;
	}

	~CCookFileHelper( )
	{
		GDeterministicGUIDGeneration = m_prev;
		GCurrentCookedResourcePath = nullptr;
		GCurrentCookedResourceGUIDIndex = 0;
	}

private:
	Bool m_prev;
};

THandle< CResource > CCookCommandlet::LoadResourceForCooking( const String& localCookPath )
{

	ResourceLoadingContext loadingContext;
	loadingContext.m_importLoader = m_dependencyLoaderWithFakeFiles;

	// directly load from depot
	return GDepot->LoadResource( localCookPath, loadingContext, false ); // never use links
}

Bool CCookCommandlet::CookSingleFile( CookedResourceInfo* resourceInfo )
{
	// Disable CRC verification on loading
	extern Bool GSkipCRCVerification;
	GSkipCRCVerification = true;

	// GUID hack
	CCookFileHelper guidHack( resourceInfo->m_pathAnsi.AsChar( ) );

	// trimming check - we should not have manual files that are outside the trim directory
	if ( !m_settings.m_trimDirPath.Empty() && !resourceInfo->m_path.BeginsWith( m_settings.m_trimDirPath ) )
	{
		if ( m_additionalDataBase && m_additionalDataBase->GetFileEntry( resourceInfo->m_path ) == CCookerDataBase::NO_ENTRY )
		{
			//TODO HACK Put game data in dlc
			ERR_WCC( TXT( "Resource that is not cooked could not be found in additional database '%ls' Let's cook it! " ), resourceInfo->m_path.AsChar() );
		} else 
		{
			ERR_WCC( TXT("Trying to cook file '%ls' that is outside trim directory '%ls'. File will not be cooked."), 
				resourceInfo->m_path.AsChar(), m_settings.m_trimDirPath.AsChar() );
			return false;
		}
	}

	// exclusion check - we should not have manual files that are inside the exclude directory
	if ( !m_settings.m_excludeDirPath.Empty() && resourceInfo->m_path.BeginsWith( m_settings.m_excludeDirPath ) )
	{
		ERR_WCC( TXT("Trying to cook file '%ls' that is inside excluded directory '%ls'. File will not be cooked."), 
			resourceInfo->m_path.AsChar(), m_settings.m_excludeDirPath.AsChar() );
		return false;
	}

	// right now just check by extension
	// TODO: we may need to open the file here to do popper checking
	const String fileExtension = StringHelpers::GetFileExtension( resourceInfo->m_path );

	// we DO NOT cook buffers
	if ( fileExtension == TXT("buffer") )
	{
		// update size of the buffer in the DB
		if ( m_dataBase )
		{
			const CCookerDataBase::TCookerDataBaseID dbID = m_dataBase->AddFileEntry( resourceInfo->m_path );
			const Uint64 bufferFileSize = GFileManager->GetFileSize( m_settings.m_outputPath + resourceInfo->m_path );
			m_dataBase->SetFileDiskSize( dbID, (const Uint32)bufferFileSize  );
		}

		// assume cooked
		return true;
	}

	// some file types can be ignored (debug, tests, etc)
	if ( m_settings.ShouldIgnoreExtension( fileExtension ) )
	{
		WARN_WCC( TXT("Ignored '%ls'"), resourceInfo->m_path.AsChar() );
		return true;
	}

	// there are some files that are not cooked when they are only dependencies (w2w, w2ter)
	if ( !resourceInfo->m_isSeedFile && FileTypeRequiresDirectDependency( fileExtension ) )
	{
		WARN_WCC( TXT("Ignoring '%ls' because it's not referenced directly in the seed files."), resourceInfo->m_path.AsChar() );
		return true;
	}

	// check if the file exists already
	// we can skip cooking of this file IFF the cooked file exists and we have an entry for it in the DB
	// BIG TODO: the files like .dds and .png are LOST during the cooking - they are NEVER outputed so we will ALWAYS recook them :(
	const String outputFilePath = m_settings.m_outputPath + resourceInfo->m_path;
	if ( m_settings.m_skipExistingFiles )
		if ( GFileManager->FileExist( outputFilePath ) )
			if ( ProcessExistingFile( resourceInfo ) )
				return true; // don't recook only if existing file processing did not fail

	// count files that we actually processed
	m_fileCounter += 1;

	// create the entry in the cooker data base for the file we are going to cook
	CCookerDataBase::TCookerDataBaseID dbID = CCookerDataBase::NO_ENTRY;
	if ( m_dataBase )
	{
		dbID = m_dataBase->AddFileEntry( resourceInfo->m_path );
		m_dataBase->SetFileSeedFlag( dbID, resourceInfo->m_isSeedFile );
	}

	// some file types are not resources and are processed differently (usually just copied)
	// for example: XML, SWF, SRT, etc
	if ( !IsResourceFile( resourceInfo, fileExtension ) )
		return CookNonResourceFile( resourceInfo );

	//-----------
	// Cooking 
	//-----------

	// stats
	if ( m_settings.m_verbose )
	{
		LOG_WCC( TXT("Cooking '%ls'..."), resourceInfo->m_path.AsChar() );
	}

	// load the file to cook, if we fail - exit
	ResourceLoadingContext loadingContext;
	CTimeCounter loadingTimer;
	THandle< CResource > resourceToCook = LoadResourceForCooking( resourceInfo->m_path );
	m_stats.AddLoading( loadingTimer.GetTimePeriod() );
	if ( !resourceToCook )
	{
		// check if the source file is there
		ERR_WCC( TXT("Failed to load '%ls' - is this a resource"), 
			resourceInfo->m_path.AsChar() );
		return false;
	}

	// build cooking framework wrapper
	CCookerDependencyTracker dependencyTracker;
	CCookerFramework frameworkWrapper( resourceToCook.Get(), m_settings.m_cookingPlatform, m_errors, &dependencyTracker, &m_stats );
	Red::TUniquePtr< DeferredDataBufferExtractor > deferedDataBufferExtractor = DeferredDataBufferExtractor::Create( m_settings.m_outputPath, resourceInfo->m_path );

	// prepare saving context (with cooking)
	DependencySavingContext savingContext( resourceToCook.Get() );
	savingContext.m_cooker = &frameworkWrapper; // enable cooker functionality
	savingContext.m_bufferExtractor = deferedDataBufferExtractor.Get();
	//savingContext.m_hashPaths = true; // hash the paths to save time and obfuscate the files more

	// Prepare to collect string ID's.
	CSpeechCollector speechCollector;
	if( !m_settings.m_stringIDsPath.Empty() || !m_settings.m_stringKeysPath.Empty() )
		savingContext.m_speechCollector = &speechCollector;	

	// create output file
	IFile* outputFile = CreateOutputFile( outputFilePath, true ); // cooked files are not big, always use the async writer for them
	if ( !outputFile )
	{
		ERR_WCC( TXT("Failed to create output file '%ls'"), 
			resourceInfo->m_path.AsChar() );
		return false;
	}

	// hack for resave only - do pull the layer infos into the world
	if ( resourceToCook->IsA< CWorld >() && m_settings.m_cookingPlatform == PLATFORM_Resave )
	{
		Cast< CWorld >( resourceToCook )->PullLayerGroupsAndLayerInfos();
	}

	// save
	IDependencySaver* saver = SDependencyLinkerFactory::GetInstance().GetSaver( fileExtension, *outputFile );
	if( saver == nullptr )
	{
		RED_LOG_ERROR( Cooker, TXT( "Unable to find saver for resource '%ls'" ),  resourceInfo->m_path.AsChar() );
		return false;
	}

	CTimeCounter savingTimer;
	const Bool fileCooked = saver->SaveObjects( savingContext );
	m_stats.AddSaving(savingTimer.GetTimePeriod());
	const Uint64 fileSize = outputFile->GetSize();
	PostProcessFile( resourceInfo->m_path, outputFilePath, outputFile );
	
	// cleanup
	delete outputFile;
	delete saver;

	// file not cooked properly
	if ( !fileCooked )
	{
		GFileManager->DeleteFile(outputFilePath);

		ERR_WCC( TXT("Failed to save cooked file '%ls'"), 
			resourceInfo->m_path.AsChar() );
		return false;
	}

	// update speech map collection
	if( !speechCollector.speechIds.Empty( ) )
	{
		TSpeechIDs& ids = m_speechMap.GetRef( resourceInfo->m_path );
		ids.PushBackUnique( speechCollector.speechIds );
	}

	// update string keys map collection
	if( !speechCollector.stringKeys.Empty( ) )
	{
		TStringKeys& keys = m_stringKeysMap.GetRef( resourceInfo->m_path );
		keys.PushBackUnique( speechCollector.stringKeys );
	}
	

	// rules about adding the dependencies to bundles:
	//  1. if a dependency file is a seed file in other bundle it's not multiplied
	//  2. if a dependency is a soft dependency and it exists in any other bundle then it's not added (to save space)
	//  in all other cases the dependency will be put in the same bundle

	// NOTE: all dependencies are tracked REGARDLESS if the files are put in the same bundle or not
	// please also note, that the case 1) will break the burst loading (it will make it slower)

	// follow dependencies
	if ( m_settings.m_cookDependencies )
	{
		// resolve and gather normal (hard) dependencies
		TDynArray< String > hardDependencies;
		dependencyTracker.GetHardDependencies( hardDependencies );
		for ( const String& dependencyDepotPath : hardDependencies )
		{
			// trimming
			Bool trimmed = false;
			if ( !m_settings.m_trimDirPath.Empty() && !dependencyDepotPath.BeginsWith( m_settings.m_trimDirPath ) )
			{
				if ( m_additionalDataBase && m_additionalDataBase->GetFileEntry( dependencyDepotPath.AsChar() ) == CCookerDataBase::NO_ENTRY )
				{
					ERR_WCC( TXT( "Resource that is not cooked could not be found in additional database - '%ls' referenced from '%ls' Let's cook it!" ), 
						dependencyDepotPath.AsChar(), 
						resourceInfo->m_path.AsChar() );
				}
				else 
				{
					LOG_WCC( TXT("Not procesing '%ls' because it's outside the trim directory"),
						dependencyDepotPath.AsChar() );
					trimmed = true;
				}
			}	
			// excluding	
			if ( !m_settings.m_excludeDirPath.Empty() && dependencyDepotPath.BeginsWith( m_settings.m_excludeDirPath ) )
			{	
				LOG_WCC( TXT("Not procesing '%ls' because it's inside excluded directory"),
					dependencyDepotPath.AsChar() );
				trimmed = true;
			}

			// dependency was trimmed, report is as additional dependency in the cook.db but DO NOT COOK IT
			if ( trimmed )
			{
				const CCookerDataBase::TCookerDataBaseID dbDepID = m_dataBase->AddFileEntry( dependencyDepotPath.AsChar() );
				m_dataBase->AddFileHardDependency( dbID, dbDepID );
				m_dataBase->SetFileAdditionalFlag( dbDepID, true );

				continue;
			}

			// process
			CookedResourceInfo* dependency = ResolveHardDependency( resourceInfo, dependencyDepotPath );
			if (dependency)
			{
				resourceInfo->m_hardDependencies.PushBack(dependency);

				// update data base with dependency info
				if ( m_dataBase )
				{
					const CCookerDataBase::TCookerDataBaseID dbDepID = m_dataBase->AddFileEntry( dependency->m_path );
					m_dataBase->AddFileHardDependency( dbID, dbDepID );
				}
			}
		}

		// resolve and gather soft (async) dependencies
		TDynArray< String > softDependencies;
		dependencyTracker.GetSoftDependencies( softDependencies );
		for ( const String& dependencyDepotPath : softDependencies )
		{
			// trimming
			Bool trimmed = false;
			if ( !m_settings.m_trimDirPath.Empty() && !dependencyDepotPath.BeginsWith( m_settings.m_trimDirPath ) )
			{
				if ( m_additionalDataBase && m_additionalDataBase->GetFileEntry( dependencyDepotPath.AsChar() ) == CCookerDataBase::NO_ENTRY )
				{
					ERR_WCC( TXT( "Resource that is not cooked could not be found in additional database - '%ls' referenced from '%ls' Let's cook it!" ), 
						dependencyDepotPath.AsChar(), 
						resourceInfo->m_path.AsChar() );
				}
				else
				{
					LOG_WCC( TXT("Not procesing '%ls' because it's outside the trim directory"),
						dependencyDepotPath.AsChar() );
					trimmed = true;
				}
			}	
			// excluding	
			if ( !m_settings.m_excludeDirPath.Empty() && dependencyDepotPath.BeginsWith( m_settings.m_excludeDirPath ) )
			{	
				LOG_WCC( TXT("Not procesing '%ls' because it's inside excluded directory"),
					dependencyDepotPath.AsChar() );
				trimmed = true;
			}

			// dependency was trimmed, report is as additional dependency in the cook.db but DO NOT COOK IT
			if ( trimmed )
			{
				const CCookerDataBase::TCookerDataBaseID dbDepID = m_dataBase->AddFileEntry( dependencyDepotPath.AsChar() );
				m_dataBase->AddFileSoftDependency( dbID, dbDepID );
				m_dataBase->SetFileAdditionalFlag( dbDepID, true );

				continue;
			}

			// process
			CookedResourceInfo* dependency = ResolveSoftDependency( resourceInfo, dependencyDepotPath );
			if (dependency)
			{
				resourceInfo->m_softDependencies.PushBack(dependency);

				// update data base with dependency info
				if ( m_dataBase )
				{
					const CCookerDataBase::TCookerDataBaseID dbDepID = m_dataBase->AddFileEntry( dependency->m_path );
					m_dataBase->AddFileSoftDependency( dbID, dbDepID );
				}
			}
		}
	}

	// store the cooked size in the cooked DB
	if ( m_dataBase )
		m_dataBase->SetFileDiskSize( dbID, (const Uint32)fileSize );

	// store class name of valid CResource
	if ( m_dataBase && resourceToCook )
		m_dataBase->SetFileResourceClass( dbID, resourceToCook->GetClass()->GetName() );

	// TODO: track embedded files (important only for incremental cooker)

	//-----------
	// End cooking -.-
	//-----------

	// already cooked, we should not get here
	return true;
}

namespace Hacks
{

Bool ShouldAppendLayerInfoForPath( const String& path )
{
	// forgive me this piece of... code
	// CLayerInfo should be added only to layers from "dlc" directories which are not "levels\bob" subdirectories
	const String dlc = TXT( "dlc\\" );
	const String bob = TXT( "levels\\bob" );
	return path.ContainsSubstring( dlc, true, 0, true ) && !path.ContainsSubstring( bob, true, 0, true );
}

}

Bool CCookCommandlet::PostProcessFile( const String& sourcePath, const String& outputPath, IFile* outputFile /* = nullptr */ )
{
	Bool ret = true;
	// HACK!!!
	// adding CLayerInfo at the end of cooked dlc layer file
	if ( sourcePath.EndsWith( ResourceExtension< CLayer >() ) && Hacks::ShouldAppendLayerInfoForPath( sourcePath ) )
	{
		// Get layer contents and copy them into new instance.
		CLayerInfo* layerInfo = CLayerGroup::GrabRawDynamicLayerInfo( sourcePath );
		if ( !layerInfo )
		{
			ERR_WCC( TXT( "Layer %ls does not contain layer info and is not valid." ), sourcePath.AsChar( ) );
			return false;
		}
		else
		{
			if ( outputFile )
			{
				if ( !layerInfo->AppendLayerInfoObject( *outputFile ) )
				{
					ERR_WCC( TXT( "Could not update layer info for resaved layer %ls." ), outputPath.AsChar() );
					ret = false;
				}
			}
			else
			{
				if ( !layerInfo->AppendLayerInfoObject( outputPath ) )
				{
					ERR_WCC( TXT( "Could not update layer info for resaved layer %ls." ), outputPath.AsChar() );
					ret = false;
				}
			}
		}			
		delete layerInfo;
	}
	return ret;
}

void CCookCommandlet::ProcessHotListCandidate(CookedResourceInfo* file )
{
	Red::Threads::CScopedLock<Mutex> lock( m_hotListLock );

	if ( !file->m_isInHotList )
	{
		CDiskFile* depotFile = GDepot->FindFile( file->m_path );
		if ( depotFile && depotFile->IsLoaded() )
		{
			file->m_depotFile = depotFile; // cache the depot file
			file->m_isInHotList = true;
			m_hotList.PushBack( file );
		}
	}
}

CCookCommandlet::CookedResourceInfo* CCookCommandlet::ResolveHardDependency( const CookedResourceInfo* fileBeingCooked, const String& depdenencyFile )
{
	// get/create the file (we are not checking if the file actually exists yet)
	CCookCommandlet::CookedResourceInfo* ret = AddFile( depdenencyFile );
	RED_ASSERT( ret != NULL );

	// count the hard dependencies on the file
	ret->m_numHardImports += 1;

	// when we resolve a hard dependency there's a big chance is that the file is "hot" (it was already loaded), check that and add the file to hot list (files in the hot list have priority in cooking)
	// the resource will stay loaded until we call GC
	ProcessHotListCandidate( ret );

	// TODO: we may use this opportunity now to register some other relations between files

	return ret;
}

CCookCommandlet::CookedResourceInfo* CCookCommandlet::ResolveSoftDependency( const CookedResourceInfo* fileBeingCooked, const String& depdenencyFile )
{
	// get/create the file (we are not checking if the file actually exists yet)
	CCookCommandlet::CookedResourceInfo* ret = AddFile( depdenencyFile );
	RED_ASSERT( ret != NULL );

	// count the soft dependencies on the file
	ret->m_numSoftImports += 1;

	// when we resolve a soft dependency there's a slim chance is that the file is "hot" (it was already loaded), check that and add the file to hot list (files in the hot list have priority in cooking)
	// the resource will stay loaded until we call GC
	ProcessHotListCandidate( ret );

	// TODO: we may use this opportunity now to register some other relations between files

	return ret;
}

Bool CCookCommandlet::FileTypeRequiresDirectDependency( const String& fileExtension ) const
{
	// following files should not propagate cooking
	if ( fileExtension == ResourceExtension< CWorld >() ) return true;
	if ( fileExtension == ResourceExtension< CGameWorld >() ) return true;
	if ( fileExtension == ResourceExtension< CTerrainTile >() ) return true;
#ifdef USE_UMBRA
	if ( fileExtension == ResourceExtension< CUmbraTile >() ) return true;
#endif // USE_UMBRA

	// hack for the old terrain extension
	if ( fileExtension == TXT("w2ter") ) return true;

	return false;
}

Bool CCookCommandlet::SaveSpeechMap( )
{
	IFile* outputFile = GFileManager->CreateFileWriter( m_settings.m_stringIDsPath, FOF_AbsolutePath );
	if( outputFile == nullptr )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), m_settings.m_stringIDsPath.AsChar( ) );
		return true;
	}

	// Scoped JSONStreamWriter (needs to be destroyed before IFile is deleted).
	{
		typedef JSONFileHelper::JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer( fileStream );

		writer.StartObject( );
		{
			writer.String( "files" );
			writer.StartArray( );

			for( TSpeechMap::const_iterator it = m_speechMap.Begin( ); it != m_speechMap.End( ); ++it )
			{
				const String& depotPath = it->m_first;
				const TSpeechIDs& ids = it->m_second;

				if( !ids.Empty( ) )
				{
					writer.StartObject( );
					writer.String( "path" );
					writer.String( UNICODE_TO_ANSI( depotPath.AsChar( ) ) );
					writer.String( "string_ids" );
					writer.StartArray( );

					for( Uint32 id : ids )
					{
						writer.Uint( id );
					}

					writer.EndArray( );
					writer.EndObject( );
				}
			}

			writer.EndArray( );
		}
		writer.EndObject( );
	}

	delete outputFile;

	return true;
}

Bool CCookCommandlet::SaveStringKeysMap( )
{
	IFile* outputFile = GFileManager->CreateFileWriter( m_settings.m_stringKeysPath, FOF_AbsolutePath );
	if( outputFile == nullptr )
	{
		ERR_WCC( TXT( "Error creating file writer '%s'!" ), m_settings.m_stringKeysPath.AsChar( ) );
		return true;
	}

	// Scoped JSONStreamWriter (needs to be destroyed before IFile is deleted).
	{
		typedef JSONFileHelper::JSONStreamWriter< 4096 > TFileStream;

		TFileStream fileStream( outputFile );
		rapidjson::PrettyWriter< TFileStream > writer( fileStream );

		writer.StartObject( );
		{
			writer.String( "files" );
			writer.StartArray( );

			for( TStringKeysMap::const_iterator it = m_stringKeysMap.Begin( ); it != m_stringKeysMap.End( ); ++it )
			{
				const String& depotPath = it->m_first;
				const TStringKeys& keys = it->m_second;

				if( !keys.Empty( ) )
				{
					writer.StartObject( );
					writer.String( "path" );
					writer.String( UNICODE_TO_ANSI( depotPath.AsChar( ) ) );
					writer.String( "string_keys" );
					writer.StartArray( );

					for( const String& key : keys )
					{
						writer.String( UNICODE_TO_ANSI( key.AsChar( ) ) );
					}

					writer.EndArray( );
					writer.EndObject( );
				}
			}

			writer.EndArray( );
		}
		writer.EndObject( );
	}

	delete outputFile;

	return true;
}
