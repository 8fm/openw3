#include "build.h"
#include "pathlibObstacleMapper.h"

#include "pathlibComponent.h"
#include "pathlibObstacleShape.h"
#include "pathlibObstaclesMap.h"
#include "pathlibNavmeshArea.h"
#include "pathlibTaskObstacleProcessing.h"
#include "pathlibTerrain.h"
#include "pathlibObstacleAsyncProcessing.h"
#include "pathlibInstanceMap.h"
#include "pathlibWorld.h"
#include "world.h"
#include "entity.h"
#include "layer.h"
#include "layerGroup.h"
#include "baseEngine.h"
#include "component.h"

namespace PathLib
{


////////////////////////////////////////////////////////////////////////////
// CAIStorage
////////////////////////////////////////////////////////////////////////////
CObstaclesMapper::CObstaclesMapper( CPathLibWorld& pathlib )
	: m_pathlib( pathlib )
	, m_hasEvents( false )
	, m_componentProcessingContext( this )
{
	
}

CObstaclesMapper::~CObstaclesMapper()
{

}

void CObstaclesMapper::StealAllEvents( EventList& outEvents )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_eventListMutex );
	outEvents = m_eventList;
	m_eventList.ClearFast();
	m_hasEvents = false;
}

void CObstaclesMapper::RemoveImmediateObstacles()
{
	for ( auto it = m_immediateObstacles.Begin(), end = m_immediateObstacles.End(); it != end; ++it )
	{
		const SComponentMapping& componentMapping = it->m_first;

		auto itFind = m_obstacles.Find( componentMapping );
		if ( itFind == m_obstacles.End() )
		{
			continue;
		}

		const ObstacleInfo& infoList = itFind->m_second;
		for ( auto it = infoList.m_areaInfo.Begin(), end = infoList.m_areaInfo.End(); it != end; ++it )
		{
			const ObstacleAreaInfo& info = *it;
			CAreaDescription* area = m_pathlib.GetAreaDescription( info.m_areaId );
			if ( area && area->IsReady() )
			{
				CObstaclesMap* obstaclesMap = area->GetObstaclesMap();
				if ( obstaclesMap )
				{
					obstaclesMap->RemoveObstacle( info.m_obstacleId );
				}
			}
		}
		m_obstacles.Erase( itFind );
	}

	m_immediateObstacles.ClearFast();
}

void CObstaclesMapper::ObstaclesMappingUpdated( const SComponentMapping& mapping, ObstacleInfo&& newInfo, ObstacleInfo* prevInfo, Bool markDirty )
{
	if ( prevInfo )
	{
		for ( auto it = prevInfo->m_areaInfo.Begin(), end = prevInfo->m_areaInfo.End(); it != end; ++it )
		{
			AreaId prevAreaId = it->m_areaId;
			Bool foundArea = false;
			for ( auto itFind = newInfo.m_areaInfo.Begin(), endFind = newInfo.m_areaInfo.End(); itFind != endFind; ++itFind )
			{
				if ( itFind->m_areaId == prevAreaId )
				{
					foundArea = true;
					break;
				}
			}
			if ( !foundArea )
			{
				CAreaDescription* prevArea = m_pathlib.GetAreaDescription( prevAreaId );
				if ( prevArea )
				{
					ASSERT( prevArea->IsReady() );
					CObstaclesMap* obstaclesMap = prevArea->GetObstaclesMap();
					if ( obstaclesMap )
					{
						if ( it->m_obstacleId & EXTERNAL_MASK_METACONNECTION )
						{
							obstaclesMap->RemoveMetalink( it->m_obstacleId );
							if ( markDirty )
							{
								prevArea->MarkDirty( CAreaDescription::DIRTY_GENERATE );
							}
						}
						else
						{
							if ( obstaclesMap->RemoveObstacle( it->m_obstacleId ) && markDirty )
							{
								prevArea->MarkDirty( CAreaDescription::DIRTY_GENERATE );
							}
						}
					}
				}
			}
		}

		*prevInfo = Move( newInfo );
	}
	else
	{
		if ( !newInfo.m_areaInfo.Empty() )
		{
			m_obstacles.Insert( mapping, Move( newInfo ) );
		}
	}
}

Bool CObstaclesMapper::ProcessObstacleInternal( const CObstacleSpawnData& data, Bool obstacleIsUpdated, Bool checkNavmeshAreas, Bool markDirty, ObstacleInfo* prevInfo )
{
	struct Functor : public CInstanceMap::CInstanceFunctor
	{
		Functor( const CObstacleSpawnData& data, Bool update, Bool markDirty, ObstacleInfo* prevInfo = NULL )
			: m_data( data )
			, m_prevInfo( prevInfo )
			, m_isUpdate( update )
			, m_markDirty( markDirty )	{}

		RED_INLINE Bool operator()( CAreaDescription* area )
		{
			ASSERT ( area->IsReady() );

			CObstaclesMap* obstaclesMap = area->LazyInitializeObstaclesMap();
			return m_isUpdate ? UpdateObstacle( area, obstaclesMap ) : AttachObstacle( area, obstaclesMap );
		}
		RED_INLINE Bool UpdateObstacle( CAreaDescription* area, CObstaclesMap* obstaclesMap )
		{
			CObstacle::Id obstacleId = CObstacle::INVALID_ID;
			const auto& prevAreaInfo = m_prevInfo->m_areaInfo;
			for ( Uint32 i = 0; i < prevAreaInfo.Size(); ++i )
			{
				if ( prevAreaInfo[ i ].m_areaId == area->GetId() )
				{
					obstacleId = prevAreaInfo[ i ].m_obstacleId;
					break;
				}
			}
			{
				CObstacle* obstacle = obstacleId == CObstacle::INVALID_ID ? NULL : obstaclesMap->GetObstacle( obstacleId );
				if( obstacle )
				{
					if( obstaclesMap->ObstacleTypeChanged( m_data.m_collisionType, obstacle ) )
					{
						if ( obstaclesMap->RemoveObstacle( obstacleId ) )
						{
							if ( m_markDirty )
							{
								area->MarkDirty( CAreaDescription::DIRTY_GENERATE | CAreaDescription::DIRTY_SAVE );
							}
						}
						delete obstacle;
					}
					else
					{
						if ( obstaclesMap->UpdateObstacleShape( m_data, obstacle ) )
						{
							if ( m_markDirty )
							{
								area->MarkDirty( CAreaDescription::DIRTY_GENERATE | CAreaDescription::DIRTY_SAVE );
							}
						}
						ObstacleAreaInfo areaInfo;
						areaInfo.m_areaId = area->GetId();
						areaInfo.m_obstacleId = obstacleId;
						m_info.m_areaInfo.PushBack( areaInfo );
						return true;
					}
				}
			}

			AttachObstacle( area, obstaclesMap );
			return true;
		}
		RED_INLINE Bool AttachObstacle( CAreaDescription* area, CObstaclesMap* obstaclesMap )
		{
			CObstacle* obstacle = obstaclesMap->CreateObstacle( m_data );
			if ( obstacle )
			{
				ObstacleAreaInfo areaInfo;
				areaInfo.m_areaId = area->GetId();
				areaInfo.m_obstacleId = obstaclesMap->AddObstacle( obstacle, m_data );
				m_info.m_areaInfo.PushBack( areaInfo );
				if ( m_markDirty )
				{
					area->MarkDirty( CAreaDescription::DIRTY_GENERATE | CAreaDescription::DIRTY_SAVE );
				}
			}
			return true;
		}

		Bool Handle( CNavmeshAreaDescription* naviArea ) override
		{
			return operator()( naviArea );
		}

		const CObstacleSpawnData&		m_data;
		ObstacleInfo					m_info;
		ObstacleInfo*					m_prevInfo;
		Bool							m_isUpdate;
		Bool							m_markDirty;
	} functor( data, obstacleIsUpdated, markDirty, prevInfo );

	if ( checkNavmeshAreas )
	{
		// Support instances
		m_pathlib.GetInstanceMap()->IterateAreasAt( data.m_bbox, &functor );
	}

	const CTerrainInfo& info = m_pathlib.GetTerrainInfo();
	Int32 minTileX, minTileY, maxTileX, maxTileY;
	info.GetTileCoordsAtPosition( data.m_bbox.Min.AsVector2(), minTileX, minTileY );
	info.GetTileCoordsAtPosition( data.m_bbox.Max.AsVector2(), maxTileX, maxTileY );
	for ( Int32 x = minTileX; x <= maxTileX; ++x )
	{
		for ( Int32 y = minTileY; y <= maxTileY; ++y )
		{
			CTerrainAreaDescription* terrainArea = m_pathlib.GetTerrainAreaDescription( info.GetTileIdFromCoords( x, y ) );
			if ( terrainArea )
			{
				if ( !functor( terrainArea ) )
				{
					break;
				}
			}
		}
	}

	ObstaclesMappingUpdated( data.m_mapping, Move( functor.m_info ), prevInfo, markDirty );

	return prevInfo || !functor.m_info.m_areaInfo.Empty();
}

Bool CObstaclesMapper::ProcessObstacleOffline( const CObstacleSpawnData& data, Bool obstacleIsUpdated )
{
	auto itFind = m_obstacles.Find( data.m_mapping );
	if ( obstacleIsUpdated )
	{
		// if obstacle object has no mapping threat this call as normal 'Attach'
		if ( itFind == m_obstacles.End() )
		{
			obstacleIsUpdated = false;
		}
	}
	else
	{
		if ( itFind != m_obstacles.End() )
		{
			return true;
		}
	}

	Bool checkNavmeshAreas = false;

	switch ( data.m_collisionType )
	{
	default:
	case PLC_Disabled:
	case PLC_Walkable:
	case PLC_Immediate:
		if ( obstacleIsUpdated )
		{
			ProcessObstacleRemovalOffline( data.m_mapping );
		}
		return false;
	case PLC_Dynamic:
	case PLC_StaticMetaobstacle:
		checkNavmeshAreas = true;
	case PLC_Static:
	case PLC_StaticWalkable:
		break;
	}

	return ProcessObstacleInternal( data, obstacleIsUpdated, checkNavmeshAreas, true, obstacleIsUpdated ? &itFind->m_second : NULL );
}

Bool CObstaclesMapper::ProcessObstacleRemovalOffline( const SComponentMapping& componentMapping )
{
	auto itFind = m_obstacles.Find( componentMapping );
	if ( itFind == m_obstacles.End() )
	{
		return false;
	}

	const auto& infoList = itFind->m_second.m_areaInfo;
	for ( auto it = infoList.Begin(), end = infoList.End(); it != end; ++it )
	{
		const ObstacleAreaInfo& info = *it;
		CAreaDescription* area = m_pathlib.GetAreaDescription( info.m_areaId );
		if ( area )
		{
			if ( area->IsReady() )
			{
				CObstaclesMap* obstaclesMap = area->GetObstaclesMap();
				if ( obstaclesMap )
				{
					CObstacle* obstacle = obstaclesMap->GetObstacle( info.m_obstacleId);
					CObstacleShape* shape = obstacle ? obstacle->GetShape() : NULL;
#ifndef NO_EDITOR_PATHLIB_SUPPORT
					if ( shape )
					{
						Vector3 bboxMin = shape->GetBBoxMin();
						Vector3 bboxMax = shape->GetBBoxMax();

						area->VLocalToWorld( bboxMin );
						area->VLocalToWorld( bboxMax );
						m_pathlib.GetGenerationManager()->UpdateObstacles( Box( bboxMin, bboxMax ) );
					}
#endif // NO_EDITOR_PATHLIB_SUPPORT
				
					obstaclesMap->RemoveObstacle( info.m_obstacleId );
					area->MarkDirty( CAreaDescription::DIRTY_GENERATE | CAreaDescription::DIRTY_SAVE );
				}
			}
			else
			{
				// TODO: Support non-ready areas
				ASSERT( false );
			}
		}
	}
	m_obstacles.Erase( itFind );
	return true;
}

Bool CObstaclesMapper::IsDynamicObstacleEnabled( const SComponentMapping& mapping ) const
{
	auto itFind = m_obstacles.Find( mapping );
	if ( itFind == m_obstacles.End() )
	{
		return false;
	}
	return itFind->m_second.m_isRuntimeObstacleEnabled;
}

void CObstaclesMapper::AddEvent( CProcessingEvent&& e )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_eventListMutex );

	if ( !m_hasEvents )
	{
		m_hasEvents = true;

		if ( m_pathlib.IsGameRunning() )
		{
			// if game is running, for update we will need to run RuntimeProcessing Task
			CTaskManager* taskManager = m_pathlib.GetTaskManager();
			CTaskObstacleProcessing* obstacleTask = new CTaskObstacleProcessing( *taskManager );
			taskManager->AddTask( obstacleTask );
			obstacleTask->Release();
		}
	}

	for ( auto it = m_eventList.Begin(), end = m_eventList.End(); it != end; ++it )
	{
		if ( e.CompareEvents(*it) )
		{
			// NOTICE: Thats risky mechanic, but it should be ok.
			*it = e;
			return;
		}
	}

	m_eventList.PushBack( Move( e ) );
}


const CObstaclesMapper::ObstacleInfo* CObstaclesMapper::GetMapping( const SComponentMapping& mapping ) const
{
	auto it = m_obstacles.Find( mapping ); if ( it != m_obstacles.End() ) { return &it->m_second; } return NULL;
}

CObstaclesMapper::ObstacleInfo* CObstaclesMapper::GetMapping( const SComponentMapping& mapping )
{ 
	auto it = m_obstacles.Find( mapping ); if ( it != m_obstacles.End() ) { return &it->m_second; } return NULL; 
}

CObstaclesMapper::ObstacleInfo& CObstaclesMapper::RequestMapping( const SComponentMapping& mapping )
{
	return m_obstacles[ mapping ];
}

Bool CObstaclesMapper::ForgetMapping( const SComponentMapping& mapping )
{
	return m_obstacles.Erase( mapping );
}

void CObstaclesMapper::NotifyOfComponentAttached( IComponent* component )
{
	CProcessingEvent e;
	component->ProcessingEventAttach( e );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfComponentUpdated( IComponent* component )
{
	CProcessingEvent e;
	component->ProcessingEventUpdate( e );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfComponentDetached( IComponent* component )
{
	CProcessingEvent e;
	component->ProcessingEventDetach( e );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfComponentRemoved( IComponent* component )
{
	CProcessingEvent e;
	component->ProcessingEventRemove( e );
	AddEvent( Move( e ) );
}

void CObstaclesMapper::NotifyOfObstacleAttached( CComponent* component, EPathLibCollision collisionType )
{
	CProcessingEvent e( CProcessingEvent::TYPE_ATTACHED, component );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfObstacleUpdated( CComponent* component, EPathLibCollision collisionType )
{
	CProcessingEvent e( CProcessingEvent::TYPE_UPDATED, component );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfObstacleDetached( CComponent* component, EPathLibCollision collisionType )
{
	CProcessingEvent e( CProcessingEvent::TYPE_DETACHED, component );
	AddEvent( Move( e ) );
}
void CObstaclesMapper::NotifyOfObstacleRemoved( CComponent* component )
{
	SComponentMapping mapping( component );
	NotifyOfObstacleRemoved( mapping );
}
void CObstaclesMapper::NotifyOfObstacleRemoved( const SComponentMapping& mapping )
{
	CProcessingEvent e( CProcessingEvent::TYPE_REMOVED, mapping );
	AddEvent( Move( e ) );
}


Bool CObstaclesMapper::CheckIfObstacleMappingIsObsolate( CWorld* world, const SComponentMapping& mapping )
{
	return false;
}

void CObstaclesMapper::CheckForObsolateObstacles()
{
}

Bool CObstaclesMapper::HasMapping( const SComponentMapping& mapping ) const
{ 
	return m_obstacles.Find( mapping ) != m_obstacles.End();
}

void CObstaclesMapper::ObstacleLoaded( const SComponentMapping& obstacleMapping, AreaId areaId, ExternalDepenentId obstacleId )
{
	//ASSERT( !obstacle->m_mapping.m_entityGuid.IsZero() && !obstacle->m_mapping.m_componentHash != 0 );

	
	
	ObstacleAreaInfo areaInfo;
	areaInfo.m_areaId = areaId;
	areaInfo.m_obstacleId = obstacleId;

	auto itFind = m_obstacles.Find( obstacleMapping );
	if ( itFind == m_obstacles.End() )
	{
		
		ObstacleInfo data;
		data.m_areaInfo.PushBack( areaInfo );
		m_obstacles.Insert( obstacleMapping, Move( data ) );
	}
	else
	{
		auto& obstacleInfo = itFind->m_second.m_areaInfo;
		for ( auto it = obstacleInfo.Begin(), end = obstacleInfo.End(); it != end; ++it )
		{
			if ( it->m_areaId == areaId )
			{
				it->m_obstacleId = obstacleId;
				return;
			}
		}
		obstacleInfo.PushBack( areaInfo );
	}
}
//void CObstaclesMapper::ObstacleDestroyed( CObstacle* obstacle, AreaId areaId )
//{
//	auto itFind = m_obstacles.Find( obstacle->GetMapping() );
//	if ( itFind != m_obstacles.End() )
//	{
//		for ( auto it = itFind->m_second.m_areaInfo.Begin(), end = itFind->m_second.m_areaInfo.End(); it != end; ++it )
//		{
//			if ( it->m_areaId == areaId )
//			{
//				itFind->m_second.m_areaInfo.Erase( it );
//				if ( itFind->m_second.m_areaInfo.Empty() && !itFind->m_second.m_isRuntimeObstacleEnabled )
//				{
//					m_obstacles.Erase( itFind );
//				}
//				break;
//			}
//		}
//	}
//}

};				// namespace PathLib
