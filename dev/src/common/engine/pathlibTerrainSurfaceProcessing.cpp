/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTerrainSurfaceProcessing.h"

#include "clipMap.h"
#include "pathlibCookerData.h"
#include "pathlibDetailedSurfaceData.h"
#include "pathlibTerrain.h"
#include "pathlibObstaclesMap.h"
#include "terrainTile.h"
#include "world.h"

namespace PathLib
{

#ifndef NO_EDITOR_PATHLIB_SUPPORT

////////////////////////////////////////////////////////////////////////////
// CTerrainProcessingThread
////////////////////////////////////////////////////////////////////////////
CTerrainProcessingThread::CTerrainProcessingThread( CTerrainAreaDescription* area )
	: Super( area )
	, m_detailedSurfaceData( nullptr )
{
	CNavigationCookingContext* cookingContext = area->GetPathLib().GetCookingContext();
	if ( cookingContext )
	{
		CDetailedSurfaceCollection* surfaceCollection = cookingContext->GetPathlibCookerData()->GetSurfaceCollection();
		if ( surfaceCollection )
		{
			m_detailedSurfaceData = surfaceCollection->GetSurface( area->GetId() );
		}
	}
}

RED_INLINE CTerrainAreaDescription* CTerrainProcessingThread::GetArea() const
{
	return static_cast< CTerrainAreaDescription* >( m_area );
}

////////////////////////////////////////////////////////////////////////////
// CTerrainSurfaceProcessingThread
////////////////////////////////////////////////////////////////////////////
CTerrainSurfaceProcessingThread::CTerrainSurfaceProcessingThread( CTerrainAreaDescription* area, Bool evictTerrainData )
	: Super( area )
	, m_dataDirty( false )
	, m_evictTerrainData( evictTerrainData )
{
}

Bool CTerrainSurfaceProcessingThread::PreProcessingSync()
{
	CTerrainAreaDescription* terrainArea = GetArea();
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	Int32 x,y;
	CPathLibWorld& pathlib = terrainArea->GetPathLib();
	pathlib.GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	terrainArea->GetHeightData().InitializeSync( pathlib.GetWorld(), x, y );
#endif
	terrainArea->StartProcessing();
	return true;
}
Bool CTerrainSurfaceProcessingThread::ProcessPathLibTask() 
{
	CTerrainAreaDescription* terrainArea = GetArea();

	if ( m_detailedSurfaceData )
	{
		terrainArea->ComputeTerrainData( m_detailedSurfaceData, this );
		terrainArea->ApplySurfaceData( m_detailedSurfaceData );
	}
	else
	{
		terrainArea->ComputeTerrainData( this );
	}
	
	m_dataDirty = !terrainArea->GetTerrainMap()->IsInitialVersion();
	return true;
}
IGenerationManagerBase::CAsyncTask* CTerrainSurfaceProcessingThread::PostProcessingSync()
{
	Super::PostProcessingSync();

	// Warning its quite heavy stuff that hacks in terrain system, and unloads all tile data!
	if ( m_evictTerrainData )
	{
		Int32 x,y;
		CTerrainAreaDescription* terrainArea = GetArea();
		CPathLibWorld& pathlib = terrainArea->GetPathLib();
		pathlib.GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
		CClipMap* clipMap = pathlib.GetWorld()->GetTerrain();
		CTerrainTile* tile = clipMap ? clipMap->GetTile( x, y ) : nullptr;
		if ( tile )
		{
			tile->ForceEvictMipmaps();
		}
	}

	m_area->EndProcessing();
	if ( m_dataDirty )
	{
		m_area->MarkDirty( CAreaDescription::DIRTY_GENERATE | CAreaDescription::DIRTY_SAVE, 1.f );
#ifndef NO_EDITOR_PATHLIB_SUPPORT
		CPathLibWorld& pathlib = m_area->GetPathLib();
		pathlib.GetGenerationManager()->RecalculateWaypoints( m_area->GetBBox() );
#endif
	}
	//m_area->SyncProcessing();

	return NULL;
}
void CTerrainSurfaceProcessingThread::DescribeTask( String& task )
{
	Int32 x,y;
	m_area->GetPathLib().GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	task = String::Printf( TXT( "Processing surface of %d x %d" ), x, y );
}

////////////////////////////////////////////////////////////////////////////
// CTerrainMarkNavmeshesProcessingThread
////////////////////////////////////////////////////////////////////////////
CTerrainMarkNavmeshesProcessingThread::CTerrainMarkNavmeshesProcessingThread( CTerrainAreaDescription* area, Bool markOnSurface, Bool markOnObstacles )
	: Super( area )
	, m_markOnSurface( markOnSurface )
	, m_markOnObstacles( markOnObstacles )
{
}
Bool CTerrainMarkNavmeshesProcessingThread::PreProcessingSync()
{
	CPathLibWorld& pathlib = m_area->GetPathLib();

	if ( !m_markOnSurface && m_area->GetObstaclesMap() == nullptr )
	{
		return false;
	}

	Int32 x,y;
	pathlib.GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );

	// collect navmesh areas at given location
	const CInstanceMapCel& mapCel = pathlib.GetInstanceMap()->GetMapCelAt( x, y );
	const TDynArray< AreaId >& areaList = mapCel.GetInstancesList();
	m_naviAreas.Reserve( areaList.Size() );
	for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
	{
		CNavmeshAreaDescription* naviArea = pathlib.GetInstanceAreaDescription( *it );
		if ( naviArea )
		{
			m_naviAreas.PushBack( naviArea );
		}
	}

	// bail out if there are no navmeshes there
	if ( m_naviAreas.Empty() )
	{
		return false;
	}

	// mark terrain area as being processed
	m_area->StartProcessing();

	return true;
}
Bool CTerrainMarkNavmeshesProcessingThread::ProcessPathLibTask()
{
	CTerrainAreaDescription* area = GetArea();

	CDetailedSurfaceData fallbackSurface;

	CDetailedSurfaceData* surface = m_detailedSurfaceData;
	if ( !surface && m_markOnSurface )
	{
		surface = &fallbackSurface;
		fallbackSurface.Initialize( area->GetPathLib().GetTerrainInfo().GetTilesResolution() );
		area->ExtractSurfaceData( surface );
	}

	CObstaclesMap* obstaclesMap = m_markOnObstacles ? area->GetObstaclesMap() : nullptr;
	for ( auto it = m_naviAreas.Begin(), end = m_naviAreas.End(); it != end; ++it )
	{
		CNavmeshAreaDescription* naviArea = *it;
		if ( m_markOnSurface )
		{
			area->Mark( surface, naviArea, true );
		}
		if ( obstaclesMap )
		{
			obstaclesMap->MarkInstance( naviArea );
		}
	}

	if ( m_markOnSurface )
	{
		area->ApplySurfaceData( surface );
	}

	return true;
}
IGenerationManagerBase::CAsyncTask* CTerrainMarkNavmeshesProcessingThread::PostProcessingSync()
{
	Super::PostProcessingSync();

	m_area->EndProcessing();

	return nullptr;
}
void CTerrainMarkNavmeshesProcessingThread::DescribeTask( String& task )
{
	Int32 x,y;
	m_area->GetPathLib().GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	task = String::Printf( TXT( "Mark navmeshes of %d x %d" ), x, y );
}

////////////////////////////////////////////////////////////////////////////
// CTerrainApplySurfaceDataProcessingThread
////////////////////////////////////////////////////////////////////////////
Bool CTerrainApplySurfaceDataProcessingThread::ProcessPathLibTask()
{
	if ( !m_detailedSurfaceData )
	{
		return false;
	}

	CTerrainAreaDescription* area = GetArea();
	area->ApplySurfaceData( m_detailedSurfaceData );

	return true;
}

void CTerrainApplySurfaceDataProcessingThread::DescribeTask( String& task )
{
	Int32 x,y;
	m_area->GetPathLib().GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	task = String::Printf( TXT( "Apply surface data tof %d x %d" ), x, y );
}


////////////////////////////////////////////////////////////////////////////
// CTerrainHeightComputationThread
////////////////////////////////////////////////////////////////////////////
Bool CTerrainHeightComputationThread::ProcessPathLibTask()
{
	CTerrainAreaDescription* area = GetArea();
	return area->GenerateHeightData();
}
void CTerrainHeightComputationThread::DescribeTask( String& task )
{
	Int32 x,y;
	m_area->GetPathLib().GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	task = String::Printf( TXT( "Compute terrain height data for %d x %d" ), x, y );
}


#endif // NO_EDITOR_PATHLIB_SUPPORT

};		// namespace PathLib
