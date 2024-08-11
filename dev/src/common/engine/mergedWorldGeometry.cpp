/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mergedMeshComponent.h"
#include "mergedWorldGeometry.h"
#include "mergedWorldGeometryBuilder.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "world.h"

#include "../core/depot.h"
#include "../core/directory.h"

//-----------------

IMPLEMENT_ENGINE_CLASS( CMergedWorldGeometry );
IMPLEMENT_ENGINE_CLASS( IMergedWorldGeometryData );

//-----------------

IMergedWorldGeometryData::IMergedWorldGeometryData()
{
}

IMergedWorldGeometryData::~IMergedWorldGeometryData()
{
}

//-----------------

const Char* CMergedWorldGeometry::LAYER_NAME = TXT("merged_geometry");
const Char* CMergedWorldGeometry::CONTENT_DIR_NAME = TXT("merged_content");
// W3 BOB Hack
const Char* CMergedWorldGeometry::BOB_DIR_NAME = TXT("dlc\\bob\\data\\");
// End of hack

CMergedWorldGeometry::CMergedWorldGeometry()
	: m_gridSize( 64 )
{
}

CMergedWorldGeometry::~CMergedWorldGeometry()
{
}

THandle< CLayer > CMergedWorldGeometry::GetContentLayer( CWorld* world )
{
	// no valid world
	if ( !world )
		return nullptr;

	// get existing layer
	CLayerInfo* layerInfo = world->GetWorldLayers()->FindLayer( LAYER_NAME );
	if ( layerInfo )
	{
		LayerLoadingContext loadingContext;
		if ( !layerInfo->SyncLoad( loadingContext ) )
			return nullptr;

		return layerInfo->GetLayer();
	}

#ifndef NO_RESOURCE_COOKING
	// no layer found, create new one
	CLayerInfo* info = world->GetWorldLayers()->CreateLayer( LAYER_NAME, true );
	if ( !info )
		return nullptr;

	// save the layre
	if ( !info->Save() )
		return nullptr;

	// get the layer object
	return info->GetLayer();
#else
	return nullptr;
#endif
}

CDirectory* CMergedWorldGeometry::GetContentDirectory( CWorld* world )
{
	// no valid world
	if ( !world )
		return nullptr;

	// get world name
	String worldName = world->GetWorldDirectoryName();
	if ( worldName.Empty() )
		return nullptr;

	CDirectory* mergedRootDir = nullptr;
	// get the root directory
	// W3 BOB HACK
	if ( worldName == TXT("bob") )
	{
		CDirectory* bobRootDir = GDepot->FindPath( BOB_DIR_NAME );
		mergedRootDir = bobRootDir->CreateNewDirectory( CONTENT_DIR_NAME, false );
	}
	else
	// End of hack
	{
		mergedRootDir = GDepot->CreateNewDirectory( CONTENT_DIR_NAME, false );
	}

	if ( !mergedRootDir )
		return nullptr;

	// create the directory for the additional merged content
	CDirectory* mergedWorldDir = mergedRootDir->CreateNewDirectory( worldName, false );
	if ( !mergedWorldDir )
		return nullptr;

	// return created directory
	LOG_ENGINE( TXT("Created content directory '%ls'"), mergedWorldDir->GetDepotPath().AsChar() );
	return mergedWorldDir;
}

#ifndef NO_RESOURCE_IMPORT

const Bool CMergedWorldGeometry::Build( CDirectory* additionalContentDirectory, CLayer* contentLayer, const IMergedWorldGeometrySupplier* worldDataSupplier, const Vector& worldCenter, const Float worldRadius )
{
	// invalid layer
	if ( !contentLayer )
		return false;

	// no content directory
	if ( !additionalContentDirectory )
		return false;
	
	// build the shit
	if ( !MergedWorldGeometryBuilder::Build( contentLayer, additionalContentDirectory, m_gridSize, m_mergers, worldDataSupplier, worldCenter, worldRadius ) )
		return false;

	// done
	return true;
}

#endif

//-----------------


