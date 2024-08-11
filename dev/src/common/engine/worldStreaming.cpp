/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "world.h"
#include "worldStreaming.h"
#include "soundStartData.h"
#include "renderCommands.h"
#include "renderFence.h"
#include "worldIterators.h"
#include "layerInfo.h"
#include "layerGroup.h"
#include "tagManager.h"
#include "umbraScene.h"
#include "rawInputManager.h"
#include "entity.h"
#include "clipMap.h"
#include "../core/loadingProfiler.h"

#ifdef RED_PLATFORM_ORBIS
	#include <perf.h>
#endif

//---

Red::Threads::CMutex					CWorldLayerStreamingFence::st_lock;
TDynArray< CWorldLayerStreamingFence* > CWorldLayerStreamingFence::st_list;

CWorldLayerStreamingFence::CWorldLayerStreamingFence( const String& name )
	: m_refCount( 1 )
	, m_isInitialized( false )
	, m_isCompleted( false )
	, m_name( name )
	, m_createTime( EngineTime::GetNow() )
	, m_issueTime( m_createTime )
	, m_finishTime( m_createTime )
{
	// keep a global list around for debugging
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );
	st_list.PushBack( this );
}

CWorldLayerStreamingFence::~CWorldLayerStreamingFence()
{
	// keep a global list around for debugging
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );
	st_list.Remove( this );
}

void CWorldLayerStreamingFence::AddRef()
{
	Red::Threads::AtomicOps::Increment32( &m_refCount );
}

void CWorldLayerStreamingFence::Release()
{
	if ( 0 == Red::Threads::AtomicOps::Decrement32( &m_refCount ) )
	{
		delete this;
	}
}

const Bool CWorldLayerStreamingFence::CheckIfCompleted()
{
	// not initialized fences are not complete
	if ( !m_isInitialized)
		return false;

	// already completed
	if ( m_isCompleted )
		return true;

	// loop through the layers and check their loading state
	Bool somethingStillLoading = false;
	for ( Uint32 i=0; i<m_layersToLoad.Size(); ++i )
	{
		CLayerInfo* info = m_layersToLoad[i];
		if ( !info->IsLoaded() || info->IsLoading() )
		{
			somethingStillLoading = true;
			break;
		}
	}

	// we finished loading
	if ( !somethingStillLoading )
	{
		m_finishTime = EngineTime::GetNow();
		m_isCompleted = true;

		LOG_ENGINE( TXT("World loading fence for '%ls' completed in %1.2fs (%d layers)"),
			m_name.AsChar(), (Double)( m_finishTime - m_issueTime ), m_layersToLoad.Size() );
	}

	return m_isCompleted;
}

const Double CWorldLayerStreamingFence::GetTimeFromStart() const
{
	return ( EngineTime::GetNow() - m_createTime );
}

void CWorldLayerStreamingFence::GatherDebugInformation( TDynArray< DebugInfo >& outDebugInfo )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	outDebugInfo.Reserve( st_list.Size() );

	// create the output data
	for ( const CWorldLayerStreamingFence* fence : st_list )
	{
		DebugInfo* info = new ( outDebugInfo ) DebugInfo();
		info->m_name = fence->m_name;
		info->m_isCompleted = fence->m_isCompleted;
		info->m_isStarted = fence->m_isInitialized;
		info->m_timeSinceStart = (Float)( EngineTime::GetNow() - fence->m_createTime );
		info->m_timeSinceIssue = fence->m_isInitialized ? (Float)( EngineTime::GetNow() - fence->m_issueTime ) : 0.0f;
		info->m_timeCompleted = fence->m_isCompleted ? (Float)( fence->m_finishTime - fence->m_issueTime ) : 0.0f;

		// list layers that are still loading
		for ( CLayerInfo* layerInfo : fence->m_layersToLoad )
		{
			if ( !layerInfo->IsLoaded() || layerInfo->IsLoading() )
			{
				info->m_layersNotLoaded.PushBack( layerInfo->GetDepotPath() );
			}
		}
	}
}

void CWorldLayerStreamingFence::MarkIssued( const TDynArray< CLayerInfo* >& layersToLoad )
{
	RED_FATAL_ASSERT( !m_isInitialized, "Layer streaming fence '%ls' already submitted", m_name.AsChar() );

	m_isInitialized = true;
	m_issueTime = EngineTime::GetNow();

	m_layersToLoad.Reserve( layersToLoad.Size() );
	for ( CLayerInfo* info : layersToLoad )
	{
		if ( info )
		{
			m_layersToLoad.PushBack( info );
		}
	}
}

//----

void CWorld::UpdateLayersLoadingState( Bool sync /*=false*/ )
{
	PC_SCOPE(UpdateLayersLoadingState);

	Uint32 currentIndex = 0;
	Uint32 iterationCount = 0;
	Uint32 staringLayerCount = m_needsUpdating.Size();
	while ( iterationCount < staringLayerCount )
	{
		CLayerInfo* currentLayerInfo = m_needsUpdating[ currentIndex ];
		if (!currentLayerInfo->UpdateLoadingState( sync ) )
		{
			++currentIndex;
		}
		++iterationCount;
	}
}

void CWorld::UpdateLoadingState( Bool sync /*= false*/ )
{
	PC_SCOPE(CWorld::UpdateLoadingState);

	// Process world layers loading status
	UpdateLayersLoadingState( sync );
}

extern Bool GCrapDoNotAttach;

void CWorld::LoadStaticData()
{
	PC_SCOPE(LoadStaticData);

	// Get initial list of layers to load
	if ( !m_worldLayers )
		return;

	// Get list of initial layers to load
	TDynArray< CLayerInfo* > visibleLayers;
	visibleLayers.Reserve( 10000 );
	m_worldLayers->ListAllVisibleLayers( visibleLayers, true );
	LOG_ENGINE( TXT("Found %d visible layers"), visibleLayers.Size() );

	// Load the layers
	{
		CTimeCounter timer;

		for ( Uint32 i=0; i<visibleLayers.Size(); ++i )
		{
			CLayerInfo* info = visibleLayers[i];

			// load the layer
			LayerLoadingContext context;
			GCrapDoNotAttach = true;
			if ( !info->SyncLoad( context ) )
			{
				// failed to load this layer, do not attach it
				visibleLayers[i] = nullptr;
			}
			GCrapDoNotAttach = false;
		}

		GLoadingProfiler.FinishStage( TXT("LoadStaticLayers") );
	}

	// Attach loaded layers
	{
		CTimeCounter timer;

		for ( Uint32 i=0; i<visibleLayers.Size(); ++i )
		{
			CLayerInfo* info = visibleLayers[i];
			if ( info )
			{
				info->ConditionalAttachToWorld();
			}
		}

		GLoadingProfiler.FinishStage( TXT("AttachStaticLayers") );
	}
}

void CWorld::ChangeLayersVisibility( const TDynArray< String >& groupsToHide, const TDynArray< String >& groupsToShow, Bool purgeStorages, class CWorldLayerStreamingFence* fence )
{
	PC_SCOPE( ChangeLayersVisibility )

	// nothing to do
	if ( groupsToHide.Empty() && groupsToShow.Empty() )
	{
		return;
	}

	// change the flags on the groups
	CLayerGroup* worldLayers = GetWorldLayers();
	if ( worldLayers == NULL )
	{
		WARN_ENGINE( TXT("The world doesn't have any layer groups defined") );
		return;
	}

	// dump
	for ( const String& name : groupsToShow )
	{
		LOG_ENGINE( TXT("Layer group to show: '%ls'"), name.AsChar() );
	}
	for ( const String& name : groupsToHide )
	{
		LOG_ENGINE( TXT("Layer group to hide: '%ls'"), name.AsChar() );
	}

	// get the layer lists
	TDynArray< CLayerInfo* > layersToUnload, layersToLoad;
	{
		PC_SCOPE( GetLayerLists )

		// crappy
		layersToLoad.Reserve( 1000 );
		layersToUnload.Reserve( 1000 );

		// Cache groups
		for ( Uint32 i = 0; i < groupsToHide.Size(); ++i )
		{
			// Find referenced layer group
			CLayerGroup* foundGroup = worldLayers->FindGroupByPath( groupsToHide[i] );
			if ( foundGroup )
			{
				foundGroup->SetVisiblityFlag( false, true, layersToUnload );
			}
			else
			{
				WARN_ENGINE( TXT("Quest is referencing a layer group '%ls' that does not exist"), groupsToHide[i].AsChar() );
			}
		}

		// get layers to unload
		for ( Uint32 i = 0; i < groupsToShow.Size(); ++i )
		{
			// Find referenced layer group
			CLayerGroup* foundGroup = worldLayers->FindGroupByPath( groupsToShow[i] );
			if ( foundGroup )
			{
				foundGroup->SetVisiblityFlag( true, true, layersToLoad );
			}
			else
			{
				WARN_ENGINE( TXT("Quest is referencing a layer group '%ls' that does not exist"), groupsToShow[i].AsChar() );
			}
		}

		// filter out hidden layers (we can't load then any way)
		for ( Uint32 i = 0; i < layersToLoad.Size(); ++i )
		{
			if ( !layersToLoad[i]->GetMergedVisiblityFlag() )
				layersToLoad[i] = nullptr;
		}

		// filter out already visible layers (we can't hide them)
		for ( Uint32 i = 0; i < layersToUnload.Size(); ++i )
		{
			if ( layersToUnload[i]->GetMergedVisiblityFlag() )
				layersToUnload[i] = nullptr;
		}
	}

	// unload the layers
	Uint32 numLayersToUnload = 0;
	{
		PC_SCOPE( SyncUnload );

		for ( Uint32 i=0; i<layersToUnload.Size(); ++i )
		{
			CLayerInfo* info = layersToUnload[i];
			if ( info )
			{
				info->RequestUnload( purgeStorages );
				numLayersToUnload += 1;
			}
		}
	}

	// start loading the new layers
	Uint32 numLayersToLoad = 0;
	{
		PC_SCOPE( SyncLoad );

		for ( Uint32 i=0; i<layersToLoad.Size(); ++i )
		{
			CLayerInfo* info = layersToLoad[i];
			if ( info )
			{
				info->RequestLoad();
				numLayersToLoad += 1;
			}
		}
	}

	// mark the fence as issued, keep the list of layers to load inside the fence
	if ( fence )
	{
		fence->MarkIssued( layersToLoad );
	}

	// stats
	if ( numLayersToLoad || numLayersToUnload )
	{
		LOG_ENGINE( TXT("ChangeLayerVisiblity: %d layers to load, %d layers to unload"), numLayersToLoad, numLayersToUnload );
	}
}

void CWorld::RegisterLayerGroup( CLayerGroup* layerGroup )
{
	ASSERT( m_layerGroupsMap.Find( layerGroup->GetLayerGroupId() ) == m_layerGroupsMap.End(), TXT("Layer group hash collision!") );

	m_layerGroupsMap.Insert( layerGroup->GetLayerGroupId(), layerGroup );
}

void CWorld::UnregisterLayerGroup( CLayerGroup* layerGroup )
{
	auto itFind = m_layerGroupsMap.Find( layerGroup->GetLayerGroupId() );

	ASSERT( itFind != m_layerGroupsMap.End(), TXT("Layer groups map desynchronized") );

	m_layerGroupsMap.Erase( itFind );
}

CLayerGroup* CWorld::GetLayerGroupById( Uint64 layerGroupId )
{
	auto itFind = m_layerGroupsMap.Find( layerGroupId );

	if ( itFind == m_layerGroupsMap.End() )
	{
		return nullptr;
	}

	return itFind->m_second;
}

void CWorld::FlushTags()
{
	CTimeCounter timeCounter;

	// Clear tag manager
	m_tagManager->Clear();

	// Components do not register to tag manager!
	// Register components in tag list
	//Uint32 totalComponentTags = 0;
	//for ( WorldAttachedComponentsIterator it( this ); it; ++it )
	//{
	//	CComponent *comp = *it;
	//	const TagList& tags = comp->GetTags();
	//	if ( !tags.Empty() )
	//	{
	//		totalComponentTags += tags.GetTags().Size();
	//		m_tagManager->AddNode( comp, tags );
	//	}
	//}

	// Register entities in the tag list
	Uint32 totalEntitiesTags = 0;
	for ( WorldAttachedEntitiesIterator it( this ); it; ++it )
	{
		CEntity *entity = *it;
		const TagList& tags = entity->GetTags();
		if ( !tags.Empty() )
		{
			totalEntitiesTags += tags.GetTags().Size();
			m_tagManager->AddNode( entity, tags );
		}
	}

	// Show info
	LOG_ENGINE( TXT("%i entities tags registered in %1.2fms"), totalEntitiesTags, timeCounter.GetTimePeriod() * 1000.0f );
}

const Float CWorld::GetWorldDimensionsWithTerrain() const
{
	Float worldSize = 128.0f; // world without the terrain
	if ( m_terrainClipMap )
	{
		worldSize = m_terrainClipMap->GetTerrainSize();
	}

	return worldSize;
}

