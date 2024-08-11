#include "build.h"

#include "cookerUtils.h"
#include "materialCooker.h"
#include "wccVersionControl.h"
#include "wccStats.h"
#include "../../common/core/wildcard.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/changelist.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/depot.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/particleSystem.h"
#include "../../common/engine/particleEmitter.h"
#include "../../common/engine/particleDrawer.h"
#include "../../common/engine/shaderCache.h"
#include "../../common/engine/staticShaderCache.h"
#include "../../common/engine/furMeshResource.h"


class CMaterialCookerCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CMaterialCookerCommandlet, ICommandlet, 0 );

	ECookingPlatform			m_cookingPlatform;
	IShaderCache*				m_shaderCacheCooker;
	IStaticShaderCache*			m_staticShaderCacheCooker;

public:
	CMaterialCookerCommandlet();

	// Executes commandlet command
	virtual Bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const { return TXT( "Cooks materials" ); };

	// Prints commandlet help
	virtual void PrintHelp() const 
	{
		LOG_WCC( TXT("Use: ") );
		LOG_WCC( TXT("wcc cookmaterials PLATFORM (-static) (-fastfx) (-allmaterials) (-material=[path]) (-resaveCRC) (-fur[=paths])") );
		LOG_WCC( TXT("here should be the description of all the parameters..") );
		LOG_WCC( TXT("..meanwhile you can go and ask the engine team") );
	};

	//! Cook static shaders
	Bool CookStaticShaders( const MaterialCookingOptions& options );

	//! Cook materials (w2mg files)
	Bool CookMaterials( const MaterialCookingOptions& options, const TList< CWildcard >& toExclude, const TDynArray< String >& materialsCookedWithFastFXPath );

	//! Cook materials in /fx folder,
	Bool CookFXMaterialsFast( MaterialCookingOptions& options, const TDynArray< String >& listOfMaterialsToCook, TDynArray< String >& materialsCookedWithFastFXPath );

	Bool CookFurShaders( const CommandletOptions& options, const MaterialCookingOptions& mtlOptions, const TDynArray< String >& furPaths );
	Bool CookAllFurShaders( const CommandletOptions& options, const MaterialCookingOptions& mtlOptions );

protected:
	Bool CookMaterialsArray( const MaterialCookingOptions& options, const TDynArray< String >& materials );

	void BuildShaderCacheOutputDirectory( const CommandletOptions& options );

	Bool OnPreCookEntries( const MaterialCookingOptions& options, TDynArray< Processors::Entries::CMaterialProcessorEntry* >& entries );
	Bool OnPostCookEntries( const MaterialCookingOptions& options, TDynArray< Processors::Entries::CMaterialProcessorEntry* >& entries );

	void SplitMaterialsFromList( const TList< String >& materialsToCook, TDynArray< String >& fastFXMaterials, TDynArray< String >& regularMaterials ) const;
};

BEGIN_CLASS_RTTI( CMaterialCookerCommandlet )
	PARENT_CLASS( ICommandlet )
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CMaterialCookerCommandlet );

//////////////////////////////////////////////////////////////////////////

CMaterialCookerCommandlet::CMaterialCookerCommandlet()
{
	m_commandletName = CNAME( cookmaterials );
}

Bool FindMaterialByNameOnly( const String& path, String& relativePath )
{
	String depotAbsolutePath;
	GDepot->GetAbsolutePath( depotAbsolutePath );
	const Uint32 depotAbsolutePathSize = depotAbsolutePath.Size();

	String subDirs[ 5 ] = { TXT("engine\\materials\\"), TXT("environment\\"), TXT("fx\\"), TXT("levels\\"), TXT("qa\\") };
	for ( Uint32 i = 0; i < 5; ++i )
	{
		TDynArray< String > absolutePaths;
		GFileManager->FindFiles( GDepot->GetRootDataPath() + subDirs[i], path, absolutePaths, true );
		if ( absolutePaths.Size() > 0 )
		{
			String potentialRelativePath = absolutePaths[0].RightString( absolutePaths[0].Size() - depotAbsolutePathSize );
			if ( GDepot->FileExist( potentialRelativePath ) )
			{
				relativePath = potentialRelativePath;
				return true;
			}
		}
	}

	return false;
}

void ParseSingleListFile( const String& filePath, TDynArray< String >& materials )
{
	FILE* fp = _wfopen( filePath.AsChar(), TXT("r") );
	if ( !fp )
	{
		WARN_WCC( TXT("File not found: '%s', trying depot path"), filePath.AsChar() );

		String newFilePath;
		GDepot->GetAbsolutePath( newFilePath );
		newFilePath += filePath;

		fp = _wfopen( newFilePath.AsChar(), TXT("r") );
		if ( !fp )
		{
			WARN_WCC( TXT("File (depot path) not found: '%s'"), newFilePath.AsChar() );
			return;
		}
		else
		{
			LOG_WCC( TXT("List file found: '%s'"), newFilePath.AsChar() );
		}
	}
	else
	{
		LOG_WCC( TXT("List file found: '%s'"), filePath.AsChar() );
	}

	ASSERT( fp );

	Char buf[ 4096 ];
	while ( fgetws( buf, 4096, fp ) )
	{
		String path = String::Printf( TXT("%s"), buf );
		path.Trim();

		if ( path.EndsWith( CMaterialGraph::GetFileExtension() ) )
		{
			// potentially material
			if ( GDepot->FileExist( path ) )
			{
				if ( !materials.PushBackUnique( path ) )
				{
					LOG_WCC( TXT("Found duplicate: '%s'"), path.AsChar() );
				}
			}
			else
			{
				// potentially a single material name
				String relativePath;
				if ( FindMaterialByNameOnly( path, relativePath ) )
				{
					if ( !materials.PushBackUnique( relativePath ) )
					{
						LOG_WCC( TXT("Found duplicate: '%s'"), relativePath.AsChar() );
					}
				}
				else
				{
					WARN_WCC( TXT("Material file not found: '%s'"), path.AsChar() );
				}
			}
		}
		else
		{
			// directory?
			CDirectory* dir = GDepot->FindPath( path.AsChar() );
			if ( dir )
			{
				TDynArray< String > materialPathsFromDirectory;
				dir->FindResourcesByExtension( CMaterialGraph::GetFileExtension(), materialPathsFromDirectory );
				for ( auto it = materialPathsFromDirectory.Begin(); it != materialPathsFromDirectory.End(); ++it )
				{
					if ( !materials.PushBackUnique( *it ) )
					{
						LOG_WCC( TXT("Found duplicate: '%s'"), it->AsChar() );
					}
				}
			}
			else
			{
				WARN_WCC( TXT("Invalid entry: '%s'"), path.AsChar() );
			}
		}
	}

	fclose( fp );
}

void ParseMaterialListFiles( const TList< String >& list, TList< String >& materials )
{
	TDynArray< String > materialsToCook;
	for( auto file : list )
	{
		ParseSingleListFile( file, materialsToCook );
	}

	materials.PushBack( materialsToCook );
}

//////////////////////////////////////////////////////////////////////////

Bool ShouldCook( const String& materialFilePath, const TList< CWildcard >& materialsToExclude, const TDynArray< String >& materialsCookedInFastFXPath )
{
	for ( auto materialToExclude : materialsToExclude )
	{
		if ( materialToExclude.Matches( materialFilePath ) )
		{
			LOG_WCC( TXT("Material '%ls' will not be cooked because of exclude list."), materialFilePath.AsChar() );
			return false;
		}
	}

	if ( materialsCookedInFastFXPath.FindPtr( materialFilePath ) && materialFilePath.BeginsWith( TXT("fx") ) )
	{
		LOG_WCC( TXT("Material '%ls' will not be cooked because it was already cooked in the FastFX mode."), materialFilePath.AsChar() );
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

CMaterialShadersStats GMaterialShadersStats;

Bool CMaterialCookerCommandlet::Execute( const CommandletOptions& options )
{
	m_cookingPlatform = CookerUtils::GetCookingPlatform( options );

	Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, options.HasOption( TXT("noassert") ) );

	Bool cookFur					= options.HasOption( TXT("fur") );
	Bool cookStaticShaders			= options.HasOption( TXT("static") );
	Bool cookMaterialGraphs			= options.HasOption( TXT("allmaterials") ) || options.HasOption( TXT("materials") );
	Bool cookFXMaterialsFast		= options.HasOption( TXT("fastfx") );
	Bool dumpAssembly				= options.HasOption( TXT("dumpAssembly") );
	Bool collectStats				= options.HasOption( TXT("stats") );
	Bool multithreadedTechniques	= options.HasOption( TXT("multithreadedTechniques" ) );

	TList< String > materialsToCook;
	if ( options.HasOption( TXT( "material") ) )
	{
		materialsToCook = options.GetOptionValues( TXT( "material" ) );
	}

	if ( options.HasOption( TXT( "list" ) ) )
	{
		ParseMaterialListFiles( options.GetOptionValues( TXT( "list" ) ), materialsToCook );
	}

	TList< CWildcard > materialsToExclude;
	if( options.HasOption( TXT( "ignoreMaterial" ) ) )
	{
		auto filters = options.GetOptionValues( TXT( "ignoreMaterial" ) );
		for( auto filter : filters )
		{
			materialsToExclude.PushBack( CWildcard( filter ) );
		}
	}

	Bool doResaveCRC = false;
	SChangelist materialsChangelist = SChangelist::DEFAULT;
	if ( options.HasOption( TXT("resaveCRC") ) )
	{
		if ( InitializeWCCVersionControl( options ) )
		{
			doResaveCRC = true;
			
			// Get changelist, if specified
			Uint32 clNumber = 0;
			String clString;
			if ( !options.GetSingleOptionValue( TXT( "cl" ), clString ) || !::FromString( clString, clNumber ) )
			{
				clNumber = 0;
			}
			if ( !static_cast< CSourceControlP4* >( GVersionControl )->CreateChangelistWithNumber( clNumber, materialsChangelist ) )
			{
				materialsChangelist = SChangelist::DEFAULT;
			}
		}
		else
		{
			WARN_WCC( TXT("Failed to initialized WCC version control!") );
		}
	}
	
	BuildShaderCacheOutputDirectory( options );

	SYSTEMTIME localTime;
	GetLocalTime( &localTime );
	
	String platform = CMaterialCooker::CookingPlatformToString( m_cookingPlatform );

	MaterialCookingOptions cookingOptions;
	cookingOptions.m_platform = m_cookingPlatform;
	cookingOptions.m_doResaveCRC = doResaveCRC;
	cookingOptions.m_collectStats = collectStats;
	cookingOptions.m_changelist = &materialsChangelist;
	cookingOptions.m_shaderCacheCooker = m_shaderCacheCooker;
	cookingOptions.m_staticShaderCacheCooker = m_staticShaderCacheCooker;

	// setup dump directories
	const String separator = TXT("\\");
	const String& binDirectory = GFileManager->GetBaseDirectory();
	const String tmpDirectory = binDirectory + TXT("shadersDump") + separator;
	RED_VERIFY( GSystemIO.CreateDirectory( tmpDirectory.AsChar() ) );

	//Setup dump folder and file names regardless of whether dumpassebly switch was used - we need it to launch compiler processes for consoles
	{
		String dumpFileName = String::Printf( TXT( "%s_%d-%s%d-%s%d_%s%d.%s%d.txt" ), platform.AsChar(),
			localTime.wYear,
			localTime.wMonth < 10 ? TXT("0") : TXT(""), localTime.wMonth,
			localTime.wDay < 10 ? TXT("0") : TXT(""), localTime.wDay,
			localTime.wHour < 10 ? TXT("0") : TXT(""), localTime.wHour,
			localTime.wMinute < 10 ? TXT("0") : TXT(""), localTime.wMinute );

		TDynArray< String > temporaryShadersDumpDirectories;
		temporaryShadersDumpDirectories.PushBack( TXT("vs") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("ps") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("cs") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("gs") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("hs") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("ds") + separator );
		temporaryShadersDumpDirectories.PushBack( TXT("compilerErrors") + separator );

		for ( auto dir : temporaryShadersDumpDirectories )
		{
			String dumpPath = tmpDirectory + dir;
			RED_VERIFY( GSystemIO.CreateDirectory( dumpPath.AsChar() ) );
			
			dumpPath = tmpDirectory + dir + separator + TXT("sdbs");
			RED_VERIFY( GSystemIO.CreateDirectory( dumpPath.AsChar() ) );
		}

		cookingOptions.m_dumpFileName = dumpFileName;
		cookingOptions.m_dumpDirPath = tmpDirectory;
		cookingOptions.m_dumpAssembly = dumpAssembly;
	}

	CTimeCounter timer;

	if ( cookFur )
	{
		const String& cookedDataDirectory = GFileManager->GetDataDirectory();
		cookingOptions.m_furShaderCachePath = cookedDataDirectory + TXT("furshader.cache");

		TList< String > materialsToCook = options.GetOptionValues( TXT("fur") );
		if ( materialsToCook.Empty() )
		{
			RED_VERIFY( CookAllFurShaders( options, cookingOptions ) );
		}
		else
		{
			TDynArray< String > furs;
			furs.Reserve( materialsToCook.Size() );
			for ( const String& path : materialsToCook )
			{
				furs.PushBack( path );
			}
			RED_VERIFY( CookFurShaders( options, cookingOptions, furs ) );
		}
	}

	// compile shaders
	if ( cookStaticShaders )
	{
		RED_VERIFY( CookStaticShaders( cookingOptions ) );
	}

	TDynArray< String > fastFXMaterials;
	TDynArray< String > regularMaterials;
	if ( !materialsToCook.Empty() )
	{
		SplitMaterialsFromList( materialsToCook, fastFXMaterials, regularMaterials );
	}	

	TDynArray< String > materialsCookedWithFastFXPath;
	if ( cookFXMaterialsFast || !fastFXMaterials.Empty() )
	{
		RED_VERIFY( CookFXMaterialsFast( cookingOptions, fastFXMaterials, materialsCookedWithFastFXPath ) );
		cookingOptions.m_materialVertexFactories.Clear();
	}
	
	if ( !regularMaterials.Empty() )
	{
		TDynArray< String > materialsToCookArray;
		for ( auto material : regularMaterials )
		{
			if ( ShouldCook( material, materialsToExclude, materialsCookedWithFastFXPath ) )
			{
				materialsToCookArray.PushBack( material );
			}
		}

		RED_VERIFY( CookMaterialsArray( cookingOptions, materialsToCookArray ) );
		cookMaterialGraphs = false;
	}

	if ( cookMaterialGraphs )
	{
		RED_VERIFY( CookMaterials( cookingOptions, materialsToExclude, materialsCookedWithFastFXPath ) );
	}

	m_shaderCacheCooker->Flush();
	m_staticShaderCacheCooker->Flush();

	// get the elapsed time of cook and convert it to human readable form
	Int32 timeOfCook = static_cast< Int32 >( timer.GetTimePeriodMS() );
	Int32 ms = timeOfCook % 1000;
	timeOfCook /= 1000;
	Int32 s = timeOfCook % 60;
	timeOfCook /= 60;
	Int32 min = timeOfCook % 60;
	timeOfCook /= 60;
	Int32 h = timeOfCook % 24;
	LOG_WCC( TXT("Material cooking took: %dh %dmin %ds %dms"), h, min, s, ms );
	
	// Done
	return true;
}

void CMaterialCookerCommandlet::BuildShaderCacheOutputDirectory( const CommandletOptions& options )
{
	String staticShaderCacheFilename;
	String shaderCacheFilename;
	switch( m_cookingPlatform )
	{
	case PLATFORM_PC:
		{
			staticShaderCacheFilename = TXT("staticshader.cache");
			shaderCacheFilename = TXT("shader.cache");
			break;
		}
#ifndef WCC_LITE
	case PLATFORM_PS4:
		{
			staticShaderCacheFilename = TXT("staticshaderps4.cache");
			shaderCacheFilename = TXT("shaderps4.cache");
			break;
		}
	case PLATFORM_XboxOne:
		{
			staticShaderCacheFilename = TXT("staticshaderxboxone.cache");
			shaderCacheFilename = TXT("shaderxboxone.cache");
			break;
		}
#endif
	default:
		HALT( "Unsupported platform" );
	}

	String overridenCacheFileName;
	if ( options.GetSingleOptionValue( TXT( "shaderCacheFileName" ), overridenCacheFileName ) )
	{
		if ( !overridenCacheFileName.EndsWith( TXT(".cache") ) )
		{
			overridenCacheFileName += TXT(".cache");
		}
		shaderCacheFilename = overridenCacheFileName;
	}

	const String& cookedDataDirectory = GFileManager->GetDataDirectory();
	LOG_WCC( TXT( "Cooked data directory %s" ), cookedDataDirectory.AsChar() );

	String shaderInputPath = String( TXT( "" ) );
	String shaderOutputPath = cookedDataDirectory + shaderCacheFilename;
	String staticShaderInputPath = String( TXT( "" ) );
	String staticShaderOutputPath = cookedDataDirectory + staticShaderCacheFilename;

	// Attempt to make an incremental build using the cache from previous one.
	if( options.HasOption( TXT( "useoldcache" ) ) )
	{
		shaderInputPath = ( GFileManager->GetDataDirectory( ) + shaderCacheFilename ).ToLower( );
		staticShaderInputPath = ( GFileManager->GetDataDirectory( ) + staticShaderCacheFilename ).ToLower( );
	}

	// Initialize cooker with previous cache, if any.
	m_shaderCacheCooker = IShaderCache::CreateReadWrite( shaderInputPath );
	m_staticShaderCacheCooker = IStaticShaderCache::CreateReadWrite( staticShaderInputPath );

	// Set output path in the cookers.
	m_shaderCacheCooker->SetAbsolutePath( shaderOutputPath );
	m_staticShaderCacheCooker->SetAbsolutePath( staticShaderOutputPath );

	// When cooking, always save the results to output folder.
	m_shaderCacheCooker->ForceDirty( );
	m_staticShaderCacheCooker->ForceDirty( );
}

Bool CMaterialCookerCommandlet::CookStaticShaders( const MaterialCookingOptions& options )
{
	CMaterialCooker materialCooker;
	return materialCooker.CompileStaticShaders( options );
}

Bool CMaterialCookerCommandlet::CookMaterials( const MaterialCookingOptions& options, const TList< CWildcard >& toExclude, const TDynArray< String >& materialsCookedWithFastFXPath )
{
	TDynArray< String > materialFilePaths;
	GDepot->FindResourcesByExtension( CMaterialGraph::GetFileExtension(), materialFilePaths );

	TDynArray< String > materialFilePathsToCook;
	materialFilePathsToCook.Reserve( materialFilePaths.Size() );
	for ( auto path : materialFilePaths )
	{
		if ( ShouldCook( path, toExclude, materialsCookedWithFastFXPath ) )
		{
			materialFilePathsToCook.PushBack( path );
		}
	}

	// add the shader file path to compile only this material
	/*
	materialFilePathsToCook.ClearFast();
	materialFilePathsToCook.PushBack( TXT("levels/kaer_morhen/terrain_material.w2mg") );
	//*/
	
	return CookMaterialsArray( options, materialFilePathsToCook );
}

Bool CMaterialCookerCommandlet::CookMaterialsArray( const MaterialCookingOptions& options, const TDynArray< String >& materials )
{
	TDynArray< Processors::Entries::CMaterialProcessorEntry* > materialProcessorEntries;
	for ( auto materialPath : materials )
	{
		if ( materialPath.Empty() || !GDepot->FindFile( materialPath ) )
		{
			continue;
		}

		// load material
		ResourceLoadingContext context;
		CResource* resource = GDepot->LoadResource( materialPath, context );
		if ( !resource )
		{
			continue;
		}

		Processors::Entries::CMaterialProcessorEntry* entry = new Processors::Entries::CMaterialProcessorEntry();
		entry->m_graph = Cast< CMaterialGraph >( resource );
		entry->m_options = options;
		materialProcessorEntries.PushBack( entry );
	}

	// Stats
	if ( options.m_collectStats )
	{
		GMaterialShadersStats.Clear();
		GMaterialShadersStats.Reserve( materialProcessorEntries.Size() );
	}

	OnPreCookEntries( options, materialProcessorEntries );

	CMaterialCooker cooker;
	Processors::CMaterialProcessor processor( &cooker );
	auto params = CParallelForTaskSingleArray< Processors::Entries::CMaterialProcessorEntry*, Processors::CMaterialProcessor >::SParams::Create();
	{
		params->m_array				= materialProcessorEntries.TypedData();
		params->m_numElements		= materialProcessorEntries.Size();
		params->m_processFunc		= &Processors::CMaterialProcessor::Compute;
		params->m_processFuncOwner	= &processor;
		params->m_priority			= TSP_Critical;
		params->SetDebugName		( TXT("MaterialCook") );
	}
	params->ProcessNow();
	params->Release();

	OnPostCookEntries( options, materialProcessorEntries );

	if( options.m_collectStats )
	{
		GMaterialShadersStats.DumpStatsToFile( String::Printf( TXT("MaterialsCacheStats_%s.csv"), CMaterialCooker::CookingPlatformToString( m_cookingPlatform ).AsChar() ) );
	}

	// cleanup
	for ( auto entry : materialProcessorEntries )
	{
		entry->m_graph->Discard();
		delete entry;
	}

	materialProcessorEntries.Clear();

	// Done
	return true;
}

Bool CMaterialCookerCommandlet::CookFXMaterialsFast( MaterialCookingOptions& options, const TDynArray< String >& listOfMaterialsToCook, TDynArray< String >& materialsCookedWithFastFXPath )
{
	RED_ASSERT( m_shaderCacheCooker );
	RED_ASSERT( m_staticShaderCacheCooker );
	
	CMaterialCooker::TFastFXMap	fastFXMap;
	CMaterialCooker::BuildFastFXMap( fastFXMap );

	// Gather materials to process
	TDynArray< Processors::Entries::CMaterialProcessorEntry* > materialProcessorEntries;
	for ( auto fastFXEntry : fastFXMap )
	{
		THandle< CResource > resource = GDepot->LoadResource( fastFXEntry.m_first );
		if ( CMaterialGraph* graph = SafeCast< CMaterialGraph >( resource.Get() ) )
		{
			TDynArray< EMaterialVertexFactory >& factoriesToCookFor = fastFXEntry.m_second;
			if ( !factoriesToCookFor.Empty() )
			{
				Processors::Entries::CMaterialProcessorEntry* processorEntry = new Processors::Entries::CMaterialProcessorEntry();
				processorEntry->m_graph = graph;
				options.m_materialVertexFactories = factoriesToCookFor;
				processorEntry->m_options = options;
				materialProcessorEntries.PushBack( processorEntry );
			}
		}
	}

	// Init stats
	if( options.m_collectStats )
	{
		GMaterialShadersStats.Reserve( materialProcessorEntries.Size() );
	}
	
	OnPreCookEntries( options, materialProcessorEntries );

	// Process materials in parallel
	CMaterialCooker materialCooker;
	Processors::CMaterialProcessor processor( &materialCooker, &materialsCookedWithFastFXPath );
	auto params = CParallelForTaskSingleArray< Processors::Entries::CMaterialProcessorEntry*, Processors::CMaterialProcessor >::SParams::Create();
	{
		params->m_array				= materialProcessorEntries.TypedData();
		params->m_numElements		= materialProcessorEntries.Size();
		params->m_processFunc		= &Processors::CMaterialProcessor::Compute;
		params->m_processFuncOwner	= &processor;
		params->m_priority			= TSP_Critical;
		params->SetDebugName		( TXT("MaterialCook") );
	}
	params->ProcessNow();
	params->Release();

	OnPostCookEntries( options, materialProcessorEntries );

	if( options.m_collectStats )
	{
		GMaterialShadersStats.DumpStatsToFile( String::Printf( TXT("FXMaterialsCacheStats_%s.csv"), CMaterialCooker::CookingPlatformToString( m_cookingPlatform ).AsChar() ) );
	}
	
	// cleanup
	for ( auto entry : materialProcessorEntries )
	{
		entry->m_graph->Discard();
		delete entry;
	}
	materialProcessorEntries.Clear();

	return true;
}

Bool CMaterialCookerCommandlet::OnPreCookEntries( const MaterialCookingOptions& options, TDynArray< Processors::Entries::CMaterialProcessorEntry* >& entries )
{
	if ( !options.m_doResaveCRC )
	{
		return true;
	}

	for ( auto entry : entries )
	{
		CMaterialGraph* graph = entry->m_graph;
		CDiskFile* diskFile = graph->GetFile();
		
		const String absolutePath = diskFile->GetAbsolutePath();
		diskFile->GetStatus();
		if ( options.m_changelist && *options.m_changelist != SChangelist::DEFAULT )
		{
			diskFile->SetChangelist( *options.m_changelist );
		}
		if ( GVersionControl->IsSourceControlDisabled() )
		{
			// remove the readonly flag to save the file
			if ( GFileManager->IsFileReadOnly( absolutePath ) )
			{
				RED_VERIFY( GFileManager->SetFileReadOnly( absolutePath, false ) );
			}
		}
	}

	return true;
}

Bool CMaterialCookerCommandlet::OnPostCookEntries( const MaterialCookingOptions& options, TDynArray< Processors::Entries::CMaterialProcessorEntry* >& entries )
{
	if ( !options.m_doResaveCRC )
	{
		return true;
	}

	// save
	for ( auto entry : entries )
	{
		if ( entry && entry->m_graph && entry->m_doSave )
		{
			const String& depotPath = entry->m_graph->GetDepotPath();
			if ( entry->m_graph->MarkModified() )
			{
				if ( !entry->m_graph->Save() )
				{
					RED_LOG_ERROR( Shaders, TXT("Error saving material file: %ls"), depotPath.AsChar() );
				}	
			}
			else
			{
				ERR_WCC( TXT( "Material '%s' could not be checked out. Expect errors." ), depotPath.AsChar() );
			}
		}
	}

	return true;
}

void CMaterialCookerCommandlet::SplitMaterialsFromList( const TList< String >& materialsToCook, TDynArray< String >& fastFXMaterials, TDynArray< String >& regularMaterials ) const
{
	if ( materialsToCook.Empty() )
	{
		// early exit
		return;
	}

	CMaterialCooker::TFastFXMap fastFXMap;
	CMaterialCooker::BuildFastFXMap( fastFXMap );

	for ( auto materialPath : materialsToCook )
	{
		if ( materialPath.BeginsWith( TXT("fx\\") ) )
		{
			if ( fastFXMap.KeyExist( materialPath ) )
			{
				fastFXMaterials.PushBackUnique( materialPath );
			}
			else
			{
				regularMaterials.PushBackUnique( materialPath );
			}
		}
		else
		{
			regularMaterials.PushBackUnique( materialPath );
		}
	}


	RED_LOG_SPAM( Shaders, TXT("Split %u materials to lists (%u fastFX, %u regular)"), materialsToCook.Size(), fastFXMaterials.Size(), regularMaterials.Size() );
}


Bool CMaterialCookerCommandlet::CookFurShaders( const CommandletOptions& options, const MaterialCookingOptions& mtlOptions, const TDynArray< String >& furPaths )
{
	if ( m_cookingPlatform != PLATFORM_PC )
	{
		ERR_WCC( TXT("Fur only available for PC") );
		return false;
	}

	CMaterialCooker materialCooker;
	if ( !materialCooker.InitFurCooking() )
	{
		ERR_WCC( TXT("Failed to initialize fur cooking.") );
		return false;
	}

	if ( options.HasOption( TXT("useoldcache") ) )
	{
		String shaderInputPath = GFileManager->GetDataDirectory( ) + TXT("furshader.cache");
		materialCooker.LoadExistingFurShaders( shaderInputPath );
	}

	for ( const String& path : furPaths )
	{
		CFurMeshResource* furResource = LoadResource< CFurMeshResource >( path );
		if ( furResource == nullptr )
		{
			ERR_WCC( TXT("Failed to open %ls. Skipping."), path.AsChar() );
			continue;
		}

		if ( !materialCooker.CompileFurShader( furResource ) )
		{
			ERR_WCC( TXT("Failed to cook fur shaders for %ls."), path.AsChar() );
		}
	}

	if ( !materialCooker.FinishFurCooking( mtlOptions ) )
	{
		ERR_WCC( TXT("Failed to finish fur cooking.") );
		return false;
	}

	return true;
}

Bool CMaterialCookerCommandlet::CookAllFurShaders( const CommandletOptions& options, const MaterialCookingOptions& mtlOptions )
{
	if ( m_cookingPlatform != PLATFORM_PC )
	{
		ERR_WCC( TXT("Fur only available for PC") );
		return false;
	}

	TDynArray< String > furPaths;
	GDepot->FindResourcesByExtension( CFurMeshResource::GetFileExtension(), furPaths );

	return CookFurShaders( options, mtlOptions, furPaths );
}

