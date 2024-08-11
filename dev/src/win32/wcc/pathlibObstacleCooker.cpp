/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstacleCooker.h"

#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/pathlibObstacleMapper.h"
#include "../../common/engine/pathlibWorld.h"

#include "pathlibCooker.h"
#include "pathlibTaskPool.h"

///////////////////////////////////////////////////////////////////////////////
// CObstacleCookerLayerTask
///////////////////////////////////////////////////////////////////////////////
Bool CObstacleCookerLayerTask::PreProcessingSync()
{
	if ( !Super::PreProcessingSync() )
	{
		m_cooker->TrashLayer( m_layerInfo );
		return false;
	}
	return true;
}
PathLib::IGenerationManagerBase::CAsyncTask* CObstacleCookerLayerTask::PostProcessingSync()
{
	m_cooker->TrashLayer( m_layerInfo );

	return Super::PostProcessingSync();
}

///////////////////////////////////////////////////////////////////////////////
// CObstacleCooker
///////////////////////////////////////////////////////////////////////////////
void CObstacleCooker::TrashLayer( CLayerInfo* layer )
{
	Lock lock( m_mutex );
	m_layersToTrash.PushBack( layer );
}
void CObstacleCooker::TryUnloadStuff()
{
	CLayerInfo* layerToUnload;
	do
	{
		layerToUnload = nullptr;
		{
			Lock lock( m_mutex );
			if ( !m_layersToTrash.Empty() )
			{
				layerToUnload = m_layersToTrash.PopBackFast();
			}
		}
		if ( layerToUnload )
		{
			CLayer* layer = layerToUnload->GetLayer();
			if ( layer )
			{
				const auto& entityList = layer->GetEntities();
				for ( auto itEntites = entityList.Begin(), endEntities = entityList.End(); itEntites != endEntities; ++itEntites )
				{
					CEntity* entity = *itEntites;
					entity->PostNavigationCook( m_world );
				}
			}
			
			layerToUnload->SyncUnload();
			--m_layersProcessing;
		}
	}
	while( layerToUnload != nullptr );
}

void CObstacleCooker::DoStuff()
{
	// Collect all madafakin layers
	TDynArray< CLayerInfo* > layersToLoad;

	if ( false == m_context->ShouldIgnorePathlib() )
	{
		m_taskPool->StartProcessing( 1 );
	}

	CLayerGroup* rootLayers = m_world->GetWorldLayers();
	rootLayers->GetLayers( layersToLoad, false, true, false );
	PathLib::CObstaclesMapper* obstaclesMapper = m_pathlib->GetObstaclesMapper();
	
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
				(*itEntities)->OnNavigationCook( m_world, m_context );
			}

			++layersProcessed;
			entitiesProcessed += entities.Size();

			// from now one we think of this layer as 'loaded'
			++m_layersProcessing;

			if ( false == m_context->ShouldIgnorePathlib() )
			{
				// and we run special processing job that will process all obstacles inside
				m_taskPool->AddTask( new CObstacleCookerLayerTask( obstaclesMapper, layerInfo, this ) );
			}

		}

		if ( false == m_context->ShouldIgnorePathlib() )
		{
			TryUnloadStuff();
		}
		else
		{
			( *itLayers )->SyncUnload();
			--m_layersProcessing;
		}

		if ( entitiesProcessed > 1000 )
		{
			m_cooker->TryGarbageCollect();
			entitiesProcessed = 0;
		}

		{
			CLayerInfo* layerInfo = *itLayers;
			if( CLayer* layer = layerInfo->GetLayer() )
			{
				if( CWorld* world = layer->GetWorld() )
				{
					CPhysicsWorld* physicsWorld;
					if( world->GetPhysicsWorld( physicsWorld ) )
					{
						physicsWorld->FetchCurrentFrameSimulation( false );
						physicsWorld->CompleteCurrentFrameSimulation();
						physicsWorld->TickRemovalWrappers( true );
					}
				}
			}
		}

		LOG_WCC( TXT("Layers processed: %d/%d in-memory: %d"), layersProcessed, layersToLoad.Size(), m_layersProcessing );

	}

	if ( false == m_context->ShouldIgnorePathlib() )
	{
		m_taskPool->CompleteProcessing();
	
		while ( m_layersProcessing > 0 )
		{
			Red::Threads::SleepOnCurrentThread( 10 );
			TryUnloadStuff();
		}
	}

	m_cooker->GarbageCollect();
}
