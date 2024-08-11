/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "worldFileAnalyzer.h"

#include "../../common/core/depot.h"
#include "../../common/core/resource.h"
#include "../../common/engine/foliageResource.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/umbraTile.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/core/garbageCollector.h"

const AnsiChar* CWorldFileAnalyzer::BUNLDE_NAME_COMMON = "world_";
const Char* CWorldFileAnalyzer::BUNLDE_NAME_STREAMING = TXT("world_streaming_");
const Char* CWorldFileAnalyzer::WORLD_FILE_EXTENSION = TXT("w2w"); 

IMPLEMENT_ENGINE_CLASS(CWorldFileAnalyzer)
//////////////////////////////////////////////////////////////////////////
// Settings
//////////////////////////////////////////////////////////////////////////
CWorldFileAnalyzer::Settings::Settings()
	: m_worldDepotPath( String::EMPTY )
{

}

//////////////////////////////////////////////////////////////////////////
Bool CWorldFileAnalyzer::Settings::Parse( const ICommandlet::CommandletOptions& options )
{
	// Extract the world depot path. (w2w)
	for (Uint32 i=0; i < options.GetFreeArguments().Size(); ++i)
	{
		const String& arg = options.GetFreeArguments()[i];
		const CFilePath filePath( arg );
		if ( filePath.GetExtension() == WORLD_FILE_EXTENSION )
		{
			m_worldDepotPath = arg;
			break;
		}
	}
	
	// Return if no path found.
	if( m_worldDepotPath.Empty() )
	{
		ERR_WCC( TXT("World file not specified") );
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// CWorldFileAnalyzer
//////////////////////////////////////////////////////////////////////////
CWorldFileAnalyzer::CWorldFileAnalyzer()
{
	GDepot->GetAbsolutePath( m_depotBasePath );
}

//////////////////////////////////////////////////////////////////////////
CWorldFileAnalyzer::~CWorldFileAnalyzer()
{
}

//////////////////////////////////////////////////////////////////////////
bool CWorldFileAnalyzer::DoAnalyze( const ICommandlet::CommandletOptions& options, CAnalyzerOutputList& outputList )
{
	if( !m_settings.Parse( options ) )
	{
		ERR_WCC( TXT( "Unable to parse commandline options" ) );
		return false;
	}

	// load the world file and analyze the common world resources
	WorldLoadingContext loadingContext;
	THandle< CWorld > world = CWorld::LoadWorld( m_settings.m_worldDepotPath, loadingContext );
	if ( !world )
	{
		ERR_WCC( TXT("Could not load world '%ls'"),  m_settings.m_worldDepotPath.AsChar() );
		return false;
	}

	// get the world directory
	CDirectory* worldDir = world->GetFile()->GetDirectory();
	if ( !worldDir )
	{
		ERR_WCC( TXT("Failed to get master directory for world '%ls'. Is it in the depot ?"), m_settings.m_worldDepotPath.AsChar() );
		return false;
	}

	// show some paths
	LOG_WCC( TXT("World directory: '%ls'"), worldDir->GetDepotPath().AsChar() );
	LOG_WCC( TXT("World name: '%ls'"), worldDir->GetName().AsChar() );

	// Disable all world streaming before the analysis
	world->EnableStreaming( false, false );

	// all files go to the PlayGO chunk related with the world
	const auto& worldChunks = world->GetPlayGoChunks();
	outputList.SetContentChunks( &worldChunks );

	// analyze the world data
	ExtractToBundles( world, worldDir, outputList );

	// Finished, unload and clean up
	CWorld::UnloadWorld( world.Get() );
	SGarbageCollector::GetInstance().CollectNow();	

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CWorldFileAnalyzer::ExtractToBundles( CWorld* world, CDirectory* worldDir, CAnalyzerOutputList& outputList )
{
	const StringAnsi worldNameAnsi( UNICODE_TO_ANSI( worldDir->GetName().AsChar() ) );
	const StringAnsi worldBundlePrefix = StringAnsi( BUNLDE_NAME_COMMON ) + worldNameAnsi + "_";

	// there are two packages (bundles) for each world
	//  world_name_startup - startup data for world (always loaded) - NOT SPLIT
	//  world_name_runtime - runtime data for world (can go away) - NOT SPLIT

	// we put following things into the startup data for the world:
	//  - w2w
	//  - terrain texture arrays
	//  - all static layers that are not in quest groups
	//  - all content from static layers
	//  - some of the SRTs (depending of the coverage)
	//  - occlusion tiles
	//  - terrain tiles
	
	// runtime data:
	//  - quest layers and their content
	//  - foliage tiles
	//  - navigation tiles

	// Get all world layers
	TDynArray< CLayerInfo* > allLayers;
	world->GetWorldLayers()->GetLayers( allLayers, false, true );

	THashSet< CLayerInfo* > startupLayers;
	startupLayers.Reserve( allLayers.Size() );
	
	// Create world startup bundle
	{	
		outputList.SetBundleName( worldBundlePrefix + "startup" );

		// The world file  
		outputList.AddFile( UNICODE_TO_ANSI( world->GetDepotPath().AsChar() ) );

		// Layers
		for ( CLayerInfo* info : allLayers )
		{
			outputList.AddFile( UNICODE_TO_ANSI( info->GetDepotPath().AsChar() ) );
			startupLayers.Insert( info );
		}

		// Terrain files
		{
			ExtractFiles( worldDir->FindLocalDirectory( TXT("terrain_tiles") ), ResourceExtension<CTerrainTile>(), NULL, false, outputList );
			ExtractFiles( worldDir->FindLocalDirectory( TXT("terrain_tiles") ), TXT("w2ter"), NULL, false, outputList );
		}		

		// Umbra files
#ifdef USE_UMBRA
		{
			ExtractFiles( worldDir->FindLocalDirectory( TXT("occlusion_tiles") ), ResourceExtension<CUmbraTile>(),NULL,  false, outputList );
		}
#endif
	}

	// Create runtime bundle
	{
		outputList.SetBundleName( worldBundlePrefix + "runtime" );

		// Foliage files
		{
			ExtractFiles( worldDir->FindLocalDirectory( TXT("source_foliage") ), ResourceExtension<CFoliageResource>(), NULL, false, outputList );
		}

		// Navigation files
		{
			ExtractFiles( worldDir->FindLocalDirectory( TXT("navi_cooked") ), NULL, NULL, false, outputList );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CWorldFileAnalyzer::ExtractFiles( CDirectory* mainDir, const Char* fileExtensionFilter, const TDynArray< String >* excludedFileExtensions, const Bool recursive,  CAnalyzerOutputList& outputList )
{
	if ( !mainDir )
		return;

	// filter files
	for ( CDiskFile* file : mainDir->GetFiles() )
	{
		const CFilePath filePath( file->GetDepotPath() );
		const String conformedPath = filePath.ToString();
		const String ext = filePath.GetExtension();

		// filter out files with invalid extension
		if (fileExtensionFilter)
		{
			if ( ext != fileExtensionFilter )
				continue;
		}

		// excluded
		if (excludedFileExtensions)
		{
			if ( excludedFileExtensions->Exist(ext) )
				continue;
		}

		// add to list
		outputList.AddFile( UNICODE_TO_ANSI( conformedPath.AsChar() ) );
	}

	// recurse to sub directories
	if ( recursive )
	{
		for ( CDirectory* subDir : mainDir->GetDirectories() )
		{
			ExtractFiles( subDir, fileExtensionFilter, excludedFileExtensions, recursive, outputList );
		}
	}
}
