/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "pathlibCooker.h"

#include "../../common/core/directory.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/objectMap.h"

#include "../../common/engine/mesh.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/pathlibCookerData.h"
#include "../../common/engine/pathlibNavgraph.h"
#include "../../common/engine/pathlibNavmesh.h"
#include "../../common/engine/pathlibNavmeshArea.h"
#include "../../common/engine/pathlibNavmeshAreaProcessing.h"
#include "../../common/engine/pathlibObstacleSimplificationProcessing.h"
#include "../../common/engine/pathlibSpecialZoneMap.h"
#include "../../common/engine/pathlibTerrain.h"
#include "../../common/engine/pathlibTerrainSurfaceProcessing.h"

#include "../../common/game/expManager.h"
#include "../../common/game/expCooking.h"
#include "../../common/game/wayPointCookingContext.h"


#include "pathlibTaskPool.h"
#include "pathlibObstacleCooker.h"

///////////////////////////////////////////////////////////////////////////////
// CWCCNavigationCookingContext
///////////////////////////////////////////////////////////////////////////////
void CWCCNavigationCookingContext::InitializeSystems( CWorld* world, Bool pathlibCook )
{
	CExplorationCookingContext* explorationCookingContext = new CExplorationCookingContext( *world, m_cookDirectory );
	RegisterSystem( explorationCookingContext );

	CWayPointCookingContext* waypointCookingContext = new CWayPointCookingContext();
	RegisterSystem( waypointCookingContext );

	CNavigationCookingContext::InitializeSystems( world, pathlibCook );
}

///////////////////////////////////////////////////////////////////////////////
// CPathLibCooker
///////////////////////////////////////////////////////////////////////////////
CPathLibCooker::~CPathLibCooker()
{
	ASSERT( !m_taskPool );
}

void CPathLibCooker::GarbageCollect()
{
	SGarbageCollector::GetInstance().CollectNow();
}
void CPathLibCooker::TryGarbageCollect()
{
	const Uint32 totalObjects = GObjectsMap->GetNumLiveObjects();
	const Uint32 maxObjects = (GObjectsDiscardList->GetCapacity() * 3) / 4;
	
	// Collect garbage if too much is allocated
	if(	Memory::GetTotalBytesAllocated() > m_memoryBudget || totalObjects > maxObjects )
	{
		m_taskPool->ProcessAllPendingTasks();

		SGarbageCollector::GetInstance().CollectNow();

		// Release empty memory pools to recover some memory
		Memory::ReleaseFreeMemoryToSystem();
	}
}

void CPathLibCooker::InitTime()
{
	m_timeStarted = m_timeCheckpoint = Float( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );
	LOG_WCC( TXT("Engine startup time: %0.2f") );
}
void CPathLibCooker::LogTimeCheckpoint()
{
	Float time = Float( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );
	Float stepTime = time - m_timeCheckpoint;
	Float overallTime = time - m_timeStarted;
	LOG_WCC( TXT("Step time: %0.2f overal: %0.2f"), stepTime, overallTime );
	m_timeCheckpoint = time;
}

Bool CPathLibCooker::PrepeareOutputDirectory()
{

	m_inputDirectory = m_pathlib->GetSourceDataDirectory();
	if ( !m_inputDirectory )
	{
		return false;
	}

	m_outputDirectory = m_pathlib->GetCookedDataDirectory();
	if ( !m_outputDirectory )
	{
		return false;
	}

	// copy whole files list
	TFiles files = m_outputDirectory->GetFiles();
	// clear out output folder
	for ( auto it = files.Begin(), end = files.End(); it != end; ++it )
	{
		(*it)->Delete( false, false );
	}

	return true;
}
Bool CPathLibCooker::PopulateInitialAreaList()
{
	// reinitialize system
	if ( !m_pathlib->ReinitializeSystem() )
	{
		return false;
	}

	// push every navmesh instance back into the system
	const auto& fileList = m_inputDirectory->GetFiles();
	for ( auto it = fileList.Begin(), end = fileList.End(); it != end; ++it )
	{
		CDiskFile* file = *it;
		if ( file->GetFileName().EndsWith( PathLib::CNavmeshRes::GetFileExtension() ) )
		{
			PathLib::AreaId areaId = m_pathlib->GetInstanceAreaIdFromFileName( file->GetFileName() );
			if ( areaId == PathLib::INVALID_AREA_ID )
			{
				continue;
			}

			PathLib::CNavmeshRes* navmesh = new PathLib::CNavmeshRes();

			if ( navmesh->VLoad( file->GetDepotPath(), nullptr ) )
			{
				PathLib::CNavmeshAreaDescription* naviArea = new PathLib::CNavmeshAreaDescription( *m_pathlib, areaId );
				naviArea->SetNavmesh( navmesh );
				m_pathlib->AddInstanceArea( naviArea );
				naviArea->PreCooking();
			}

			delete navmesh;
		}
	}
	
	// mark all areas as loaded
	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		terrainArea->PreCooking();
		++it;
	}

	return true;
}

Bool CPathLibCooker::InitialWorldProcessing()
{
	TDynArray< CLayerInfo* > layersToLoad;

	CLayerGroup* rootLayers = m_world->GetWorldLayers();
	rootLayers->GetLayers( layersToLoad, false, true, false );

	Uint32 layersProcessed = 0;
	Uint32 entitiesProcessed = 0;
	// iterate over all layers
	for ( auto itLayers = layersToLoad.Begin(), endLayers = layersToLoad.End(); itLayers != endLayers; ++itLayers )
	{
		// process layer
		{
			CLayerInfo* layerInfo = *itLayers;
			LayerLoadingContext context;
			context.m_loadHidden = true;
			++layersProcessed;
			if ( !layerInfo->SyncLoad( context ) )
			{
				continue;
			}
			// iterate over all entities at the layer
			CLayer* layer = layerInfo->GetLayer();
			if ( !layer )
			{
				continue;
			}

			// stream all
			const LayerEntitiesArray& entities = layer->GetEntities();
			for ( auto itEntities = entities.Begin(), endEntities = entities.End(); itEntities != endEntities; ++itEntities )
			{
				(*itEntities)->OnNavigationCookerInitialization( m_world, m_context );
			}

			entitiesProcessed += entities.Size();

			layerInfo->SyncUnload();
		}

		if ( entitiesProcessed > 1000 )
		{
			TryGarbageCollect();
			entitiesProcessed = 0;
		}

		LOG_WCC( TXT("Layers read: %d/%d"), layersProcessed, layersToLoad.Size() );
	}

	TryGarbageCollect();

	return true;
}

Bool CPathLibCooker::ProcessTerrainMapSurfaces()
{
	m_taskPool->StartProcessing();

	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		if ( terrainArea )
		{
			CPathLibTaskPool::CAsyncTask* task = new PathLib::CTerrainSurfaceProcessingThread( terrainArea, true );
			m_taskPool->AddTask( task );
		}
		++it;
	}

	m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::SmoothTerrainMapSurfaces()
{
	PathLib::CDetailedSurfaceData::GlobalSmoothOutProcess( *m_pathlib, m_context->GetPathlibCookerData()->GetSurfaceCollection() );

	return ApplyDetailedSurfaceData();
}

Bool CPathLibCooker::ComputeTerrainHeightStructures()
{
	m_taskPool->StartProcessing();

	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		if ( terrainArea )
		{
			CPathLibTaskPool::CAsyncTask* task = new PathLib::CTerrainHeightComputationThread( terrainArea );
			m_taskPool->AddTask( task );
		}
		++it;
	}

	m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::ProcessPathlibComponents()
{
	// Collect all madafakin layers
	CObstacleCooker obstacleCooker( m_world, m_pathlib, m_taskPool, this, m_context );
	obstacleCooker.DoStuff();

	return true;
}

Bool CPathLibCooker::SimplifyObstaclesMap()
{
	m_taskPool->StartProcessing();

	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				CPathLibTaskPool::CAsyncTask* task = new PathLib::CObstacleSimplificationProcessingThread( terrainArea );
				m_taskPool->AddTask( task );
			}

			++it;
		}
	}
	
	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			CPathLibTaskPool::CAsyncTask* task = new PathLib::CObstacleSimplificationProcessingThread( naviArea );
			m_taskPool->AddTask( task );

			++it;
		}
	}
	

	m_taskPool->CompleteProcessing();

	return true;
}

Bool CPathLibCooker::ComputeNavmeshNeigbours()
{
	m_taskPool->StartProcessing();

	CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CNavmeshAreaDescription* naviArea = *it;

		CPathLibTaskPool::CAsyncTask* task = new PathLib::CNavmeshDetermineNavmeshNeighbours( naviArea );
		m_taskPool->AddTask( task );

		++it;
	}

	m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::MarkNavmeshes( Bool onSurface, Bool onObstacles )
{
	m_taskPool->StartProcessing();

	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		if ( terrainArea )
		{
			CPathLibTaskPool::CAsyncTask* task = new PathLib::CTerrainMarkNavmeshesProcessingThread( terrainArea, onSurface, onObstacles );
			m_taskPool->AddTask( task );
		}
		++it;
	}

	m_taskPool->CompleteProcessing();
	return true;
}
Bool CPathLibCooker::ApplyDetailedSurfaceData()
{
	m_taskPool->StartProcessing();

	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		if ( terrainArea )
		{
			CPathLibTaskPool::CAsyncTask* task = new PathLib::CTerrainApplySurfaceDataProcessingThread( terrainArea );
			m_taskPool->AddTask( task );
		}
		++it;
	}

	m_taskPool->CompleteProcessing();
	return true;
}
Bool CPathLibCooker::ComputeNavgraphs()
{
	m_taskPool->StartProcessing();

	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			CPathLibTaskPool::CAsyncTask* task = new PathLib::CAreaGenerationJob( naviArea, PathLib::CAreaDescription::DIRTY_GENERATE, false, false );
			m_taskPool->AddTask( task );

			++it;
		}
	}

	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				CPathLibTaskPool::CAsyncTask* task = new PathLib::CAreaGenerationJob( terrainArea, PathLib::CAreaDescription::DIRTY_GENERATE, false, false );
				m_taskPool->AddTask( task );
			}
			++it;
		}
	}

	m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::WaterPrecomputation()
{
	m_taskPool->StartProcessing();

	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			CPathLibTaskPool::CAsyncTask* task = new PathLib::CSpecialZonesProcessing( naviArea, m_context->GetPathlibCookerData()->GetSpecialZones() );
			m_taskPool->AddTask( task );

			++it;
		}
	}

	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				CPathLibTaskPool::CAsyncTask* task = new PathLib::CSpecialZonesProcessing( terrainArea, m_context->GetPathlibCookerData()->GetSpecialZones() );
				m_taskPool->AddTask( task );
			}
			++it;
		}
	}

	m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::ConnectAreaDescriptions()
{
	// go through all areas and connect them together
	TSortedArray< PathLib::AreaId > neighbourAreas;
	TDynArray< Vector3 > connectorLocations;

	// connect navmesh areas to other navmesh areas and terrain
	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			LOG_WCC( TXT("NavmeshInstance %4x: compute connections"), naviArea->GetId() );

			naviArea->GatherNeighbourAreas( neighbourAreas );

			for( Uint32 i = 0, n = neighbourAreas.Size(); i != n; ++i )
			{
				PathLib::AreaId neighbourId = neighbourAreas[ i ];
				PathLib::CAreaDescription* neighbourArea;
				if ( ( neighbourId & PathLib::CAreaDescription::ID_MASK_TERRAIN ) == 0 )
				{
					// navmesh area
					if ( neighbourId > naviArea->GetId() )
					{
						continue;
					}
					PathLib::CNavmeshAreaDescription* neighbourNavi = m_pathlib->GetInstanceAreaDescription( neighbourId );
					if ( !neighbourNavi )
					{
						continue;
					}
					neighbourNavi->GatherPossibleConnectors( naviArea->GetId(), connectorLocations );
					neighbourArea = neighbourNavi;
				}
				else
				{
					neighbourArea = m_pathlib->GetTerrainAreaDescription( neighbourId );
					if ( !neighbourArea )
					{
						continue;
					}
				}

				naviArea->GatherPossibleConnectors( neighbourId, connectorLocations );

				PathLib::CNavGraph::TryToCreateConnectors( naviArea, neighbourArea, connectorLocations );

				connectorLocations.ClearFast();
			}

			neighbourAreas.ClearFast();

			++it;
		}
	}

	// compact navmesh areas as we won't modify them again
	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			PathLib::CAreaNavgraphsRes* navgraphs = naviArea->GetNavgraphs();
			if ( navgraphs )
			{
				navgraphs->CompactData();
			}
			++it;
		}
	}

	// connect terrain areas to other terrain areas
	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				Int32 x, y;
				m_pathlib->GetTerrainInfo().GetTileCoordsFromId( terrainArea->GetId(), x, y );
				LOG_WCC( TXT("Terrain area %dx%d: compute connections"), x, y );

				// East area
				{
					PathLib::AreaId neighbourId = terrainArea->GetNeighbourId( 1, 0 );
					if ( neighbourId != PathLib::INVALID_AREA_ID )
					{
						PathLib::CTerrainAreaDescription* neighbourArea = m_pathlib->GetTerrainAreaDescription( neighbourId );
						if ( neighbourArea )
						{
							terrainArea->GatherPossibleConnectors( neighbourId, connectorLocations );

							PathLib::CNavGraph::TryToCreateConnectors( terrainArea, neighbourArea, connectorLocations );

							connectorLocations.ClearFast();
						}
					}
				}

				// South area
				{
					PathLib::AreaId neighbourId = terrainArea->GetNeighbourId( 0, 1 );
					if ( neighbourId != PathLib::INVALID_AREA_ID )
					{
						PathLib::CTerrainAreaDescription* neighbourArea = m_pathlib->GetTerrainAreaDescription( neighbourId );
						if ( neighbourArea )
						{
							terrainArea->GatherPossibleConnectors( neighbourId, connectorLocations );

							PathLib::CNavGraph::TryToCreateConnectors( terrainArea, neighbourArea, connectorLocations );

							connectorLocations.ClearFast();
						}
					}
				}
			}
			++it;
		}
	}

	// compact terrain areas
	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;

			PathLib::CAreaNavgraphsRes* navgraphs = terrainArea->GetNavgraphs();
			if ( navgraphs )
			{
				navgraphs->CompactData();
			}
			++it;
		}
	}
	
	return true;
}

Bool CPathLibCooker::ComputeHighLevelGraph()
{
	struct Functor
	{
		void operator()( PathLib::CNavGraph* navgraph )
		{
			navgraph->ComputeCoherentRegionsMarking();
		}
	} fun;

	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;
			naviArea->GetNavgraphs()->IterateGraphs( fun );

			++it;
		}
	}

	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				terrainArea->GetNavgraphs()->IterateGraphs( fun );
			}

			++it;
		}
	}

	//m_taskPool->StartProcessing();

	//{
	//	CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
	//	while ( it )
	//	{
	//		PathLib::CNavmeshAreaDescription* naviArea = *it;

	//		CPathLibTaskPool::CAsyncTask* task = new PathLib::CAreaCoherentRegionsComputationJob( naviArea );
	//		m_taskPool->AddTask( task );

	//		++it;
	//	}
	//}

	//{
	//	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	//	while ( it )
	//	{
	//		PathLib::CTerrainAreaDescription* terrainArea = *it;
	//		if ( terrainArea )
	//		{
	//			CPathLibTaskPool::CAsyncTask* task = new PathLib::CAreaCoherentRegionsComputationJob( terrainArea );
	//			m_taskPool->AddTask( task );
	//		}
	//		++it;
	//	}
	//}

	//m_taskPool->CompleteProcessing();
	return true;
}

Bool CPathLibCooker::CommitOutput()
{
	m_context->CommitOutput();

	if ( m_cookerFlags & FLAG_NO_PATHLIB )
	{
		return true;
	}

	// start to save everything in output folder
	{
		CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CTerrainAreaDescription* terrainArea = *it;
			if ( terrainArea )
			{
				terrainArea->PostCooking();
				if ( !terrainArea->Save( m_outputDirectory ) )
				{
					ERR_WCC( TXT("Problem while saving terrain area %d"), terrainArea->GetId() & PathLib::CAreaDescription::ID_MASK_INDEX );
					return false;
				}
			}
			++it;
		}
	}
	{
		CPathLibWorld::InstanceAreasIterator it( *m_pathlib );
		while ( it )
		{
			PathLib::CNavmeshAreaDescription* naviArea = *it;

			naviArea->PostCooking();
			if ( !naviArea->Save( m_outputDirectory ) )
			{
				ERR_WCC( TXT("Problem while saving navi area %04x"), naviArea->GetId() & PathLib::CAreaDescription::ID_MASK_INDEX );
				return false;
			}

			++it;
		}
	}
	if ( !m_pathlib->SaveSystemConfiguration() )
	{
		ERR_WCC( TXT("Problem saving system configuration") );
	}

	return true;
}
void CPathLibCooker::Shutdown()
{
	m_taskPool->Shutdown();
	m_taskPool->InterruptProcessing();
	delete m_taskPool;
	m_taskPool = nullptr;
	delete m_context;
	m_context = nullptr;
}

Bool CPathLibCooker::Cook()
{
	InitTime();

	LOG_WCC( TXT("Cooker.. Initializing.") );

	CMesh::ActivatePipelineMesh();
	m_world->EnableStreaming( false, false );
	m_pathlib = m_world->GetPathLibWorld();
	m_taskPool = new CPathLibTaskPool();

	m_context = new CWCCNavigationCookingContext();
	m_context->Initialize( m_world, ( m_cookerFlags & FLAG_NO_PATHLIB ) == 0 );
	
	m_pathlib->SetCookerMode( m_context );

	struct Guard
	{
		CPathLibCooker*		m_this;
		Guard( CPathLibCooker* me )
			: m_this( me ) {}
		~Guard()
		{
			m_this->Shutdown();
		}
	} guard( this );

	ASSERT( m_pathlib );

	if ( ( m_cookerFlags & FLAG_NO_PATHLIB ) == 0 )
	{
		LOG_WCC( TXT("Cooker.. Prepearing output directory.") );
		if ( !PrepeareOutputDirectory() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem prepearing output directory!") );
			return false;
		}
		LogTimeCheckpoint();

		LOG_WCC( TXT("Cooker.. Populating initial area list.") );
		if ( !PopulateInitialAreaList() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while populating initial area list!") );
			return false;
		}
	}

	LogTimeCheckpoint();
	LOG_WCC( TXT("Cooker.. Initial world processing.") );
	if ( !InitialWorldProcessing() )
	{
		ERR_WCC( TXT("COOKER FAILED! Problem while initially processing world!") );
		return false;
	}

	if ( ( m_cookerFlags & FLAG_NO_PATHLIB ) == 0 )
	{
		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Processing terrain map surfaces.") );
		if ( !ProcessTerrainMapSurfaces() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while processing terrain map surfaces!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Compute navmesh neighbours.") );
		if ( !ComputeNavmeshNeigbours() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while computing navmesh neighbours!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Marking navmeshes on terrain surface.") );
		if ( !MarkNavmeshes( true, false ) )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while marking navmeshes on terrain surface!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Smooth out terrain map surfaces.") );
		if ( !SmoothTerrainMapSurfaces() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while smoothing terrain map surfaces!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Compute terrain height data.") );
		if ( !ComputeTerrainHeightStructures() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while computing terrain height data!") );
			return false;
		}

		PathLib::CCookerData* cookerData = m_context->GetPathlibCookerData();
		if ( cookerData )
		{
			cookerData->DumpSurfaceData();
		}

	}

	if ( ( m_cookerFlags & FLAG_IGNORE_OBSTACLES ) == 0 )
	{
		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Processing pathlib components.") );
		if ( !ProcessPathlibComponents() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while processing pathlib components (obstacles, metalinks and stuffz)!") );
			return false;
		}
	}
	else
	{
		LOG_WCC( TXT("Cooker.. Ignoring pathlib component processing step.") );
	}

	if ( ( m_cookerFlags & FLAG_NO_PATHLIB ) == 0 )
	{
		m_context->GetPathlibCookerData()->GetSpecialZones()->FinalizeCollection();
	
		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Processing foliage.") );
		if ( !ProcessFoliage() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while processing foliage!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Simplifying obstacles maps.") );
		if ( !SimplifyObstaclesMap() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while simplifying obstacles map!") );
			return false;
		}
		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Marking navmeshes on terrain obstacles.") );
		if ( !MarkNavmeshes( false, true ) )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while marking navmeshes on terrain obstacles!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Processing navgraphs.") );
		if ( !ComputeNavgraphs() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while computing navgraphs!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Water navgraph precomputation.") );
		if ( !WaterPrecomputation() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while precomputing water into navgraphs!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Cooker.. Connect area descriptions.") );
		if ( !ConnectAreaDescriptions() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while doing inter-area connections!") );
			return false;
		}

		LogTimeCheckpoint();
		LOG_WCC( TXT("Compute high level pathfinding graph.") );
		if ( !ComputeHighLevelGraph() )
		{
			ERR_WCC( TXT("COOKER FAILED! Problem while computing high level pathfinding graph!") );
			return false;
		}
	
	}

	LogTimeCheckpoint();
	LOG_WCC( TXT("Cooker.. Commiting output.") );
	if ( !CommitOutput() )
	{
		ERR_WCC( TXT("COOKER FAILED! Problem when commiting the output (fucken last stage)!") );
		return false;
	}

	LogTimeCheckpoint();

	return true;
}
