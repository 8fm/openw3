/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibComponent.h"

#include "pathlibAreaDescription.h"
#include "pathlibTaskImmediateObstacle.h"
#include "pathlibMetalink.h"
#include "pathlibNavmeshArea.h"
#include "pathlibObstacle.h"
#include "pathlibObstaclesMap.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstacleAsyncProcessing.h"
#include "pathlibTaskManager.h"
#include "pathlibTerrain.h"
#include "world.h"
#include "layer.h"
#include "entity.h"
#include "component.h"

namespace PathLib
{

namespace 
{

	struct ObstacleDynamicDetachmentMechanism : public CProcessingEvent::GeneralProcessingMechanism
	{
		Bool RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e ) override
		{
			IObstacleComponent::RemoveDynamicObstacle( context, e );
			return true;
		}
		static RED_INLINE ObstacleDynamicDetachmentMechanism* GetInstance();
	};

	struct ObstacleImmediateDetachmentMechanism : public CProcessingEvent::GeneralProcessingMechanism
	{
		Bool RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e ) override
		{
			IObstacleComponent::RemoveImmediateObstacle( context, e );
			return true;
		}
		static RED_INLINE ObstacleImmediateDetachmentMechanism* GetInstance();
	};

	struct ObstacleRemovalMechanism : public CProcessingEvent::GeneralProcessingMechanism
	{
		Bool ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, CGenerationManager::CAsyncTask::CSynchronousSection& section ) override
		{
			section.End();

			processingJob->GetObstacleMapper()->ProcessObstacleRemovalOffline( e.m_componentMapping );
			return true;
		}
		static RED_INLINE ObstacleRemovalMechanism* GetInstance();
	};

	ObstacleDynamicDetachmentMechanism																g_ObstacleDynamicDetachmentMechanism;
	ObstacleImmediateDetachmentMechanism															g_ObstacleImmediateDetachmentMechanism;
	ObstacleRemovalMechanism																		g_ObstacleRemovalMechanism;
	ObstacleDynamicDetachmentMechanism*		ObstacleDynamicDetachmentMechanism::GetInstance()		{ return &g_ObstacleDynamicDetachmentMechanism; }
	ObstacleImmediateDetachmentMechanism*	ObstacleImmediateDetachmentMechanism::GetInstance()		{ return &g_ObstacleImmediateDetachmentMechanism; }
	ObstacleRemovalMechanism*				ObstacleRemovalMechanism::GetInstance()					{ return &g_ObstacleRemovalMechanism; }

};

////////////////////////////////////////////////////////////////////////////
// IComponent
////////////////////////////////////////////////////////////////////////////


void IComponent::SafePtr::operator=( IComponent* c )
{
	m_component = c;
	m_engineObj = c ? c->AsEngineComponent() : nullptr;
}

IComponent::SafePtr::SafePtr( IComponent* c )
	: m_component( c )
	, m_engineObj( c ? c->AsEngineComponent() : nullptr )			
{}

void IComponent::Attach( CWorld* world )
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfComponentAttached( this );
	}
}
void IComponent::Detach( CWorld* world )
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfComponentDetached( this );
	}
}
void IComponent::Remove( CWorld* world	)
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfComponentRemoved( this );
	}
}
void IComponent::Update( CWorld* world )
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfComponentUpdated( this );
	}
}
void IComponent::Enable( CWorld* world )
{
	Attach( world );
}
void IComponent::Disable( CWorld* world )
{
	Detach( world );
}

IComponent* IComponent::Get( CWorld* world, const SComponentMapping& mapping )
{
	CEntity* entity = world->FindEntity( mapping.m_entityGuid );
	if ( !entity )
	{
		// entity was deleted
		return NULL;
	}
	IComponent* component = NULL;
	const auto& components = entity->GetComponents();
	for ( auto it = components.Begin(), end = components.End(); it != end; ++it )
	{
		CComponent* c = *it;
		// firstly check if there is PathLib component inside - because it looks much cheaper than to start from hash checking
		IComponent* o = c->AsPathLibComponent();
		if ( o )
		{
			Uint32 h = 0;
			c->GetName().SimpleHash( h );
			if ( h == mapping.m_componentHash )
			{
				component = o;
			}
		}
	}

	return component;
}

void IComponent::ProcessingEventAttach( CProcessingEvent& e )
{
	e = CProcessingEvent( CProcessingEvent::TYPE_ATTACHED, AsEngineComponent() );
}
void IComponent::ProcessingEventUpdate( CProcessingEvent& e )
{
	e = CProcessingEvent( CProcessingEvent::TYPE_UPDATED, AsEngineComponent() );
}
void IComponent::ProcessingEventDetach( CProcessingEvent& e )
{
	e = CProcessingEvent( CProcessingEvent::TYPE_DETACHED, AsEngineComponent() );
}
void IComponent::ProcessingEventRemove( CProcessingEvent& e )
{
	e = CProcessingEvent( CProcessingEvent::TYPE_REMOVED, AsEngineComponent() );
}

Bool IComponent::IsLayerBasedGrouping() const
{
	return false;
}

Bool IComponent::IsNoticableInGame( CProcessingEvent::EType eventType ) const
{
	return false;
}
Bool IComponent::IsNoticableInEditor( CProcessingEvent::EType eventType ) const
{
	return false;
}
void IComponent::OnPathLibReload( CWorld* world )
{
	Attach( world );
}
IObstacleComponent*	IComponent::AsObstacleComponent()
{
	return NULL;
}
IMetalinkComponent* IComponent::AsMetalinkComponent()
{
	return NULL;
}
////////////////////////////////////////////////////////////////////////////
// IObstacleComponent
////////////////////////////////////////////////////////////////////////////
void IObstacleComponent::OnCollisionGroupUpdated( CWorld* world, EPathLibCollision newCollisionGroup )
{
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( pathlib )
	{
		pathlib->NotifyOfComponentUpdated( this, true );
	}
}

void IObstacleComponent::SetPathLibCollisionGroup( EPathLibCollision collisionGroup )
{
	SetPathLibCollisionGroupInternal( collisionGroup );

	CComponent* engineComponent = AsEngineComponent();
	CWorld* world = engineComponent->GetLayer()->GetWorld();
	OnCollisionGroupUpdated( world, collisionGroup );
}

void IObstacleComponent::RemoveImmediateObstacle( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )
{
	// firstly hide obstacle, as we will get all hard computation done in same processing branch
	RemoveDynamicObstacle( context, e );
	
	CObstaclesMapper* mapper = context.GetMapper();
	CTaskManager* taskManager = mapper->GetPathLib().GetTaskManager();

	CTaskRemoveImmediateObstacle* task = new CTaskRemoveImmediateObstacle( *taskManager, *mapper, e.m_componentMapping );
	taskManager->AddTask( task );
	task->Release();
}

void IObstacleComponent::RemoveDynamicObstacle( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )
{
	CObstaclesMapper* mapper = context.GetMapper();
	auto* info = mapper->GetMapping( e.m_componentMapping );
	if ( !info )
	{
		// obstacle not registered anyhow yet
		return;
	}

	if ( !info->m_isRuntimeObstacleEnabled )
	{
		// already disabled
		return;
	}

	info->m_isRuntimeObstacleEnabled = false;

	CPathLibWorld& pathlib = mapper->GetPathLib();
	auto& areaList = info->m_areaInfo;

	for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
	{
		CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
		CObstaclesMap* obstacles = area->GetObstaclesMap();
		if ( obstacles )
		{
			obstacles->HideObstacle( it->m_obstacleId, context );
		}
	}

	if ( areaList.Empty() )
	{
		mapper->ForgetMapping( e.m_componentMapping );
	}
}


Bool IObstacleComponent::RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )
{
	ASSERT( e.m_type != CProcessingEvent::TYPE_DETACHED && e.m_type != CProcessingEvent::TYPE_REMOVED );

	EPathLibCollision collisionGroup = GetPathLibCollisionGroup();
	CObstaclesMapper* mapper = context.GetMapper();

	if ( collisionGroup == PLC_Immediate )
	{
		// ATTACHMENT OR UPDATE - whatever, we handle them the same
		CTaskManager* taskManager = mapper->GetPathLib().GetTaskManager();
		CTaskProcessImmediateObstacle* task = new CTaskProcessImmediateObstacle( *taskManager, *mapper, e.m_component );
		taskManager->AddTask( task );
		task->Release();
		return true;
	}

	if ( collisionGroup != PLC_Dynamic )
	{
		return false;
	}

	CObstaclesMapper::ObstacleInfo& mapping = mapper->RequestMapping( e.m_componentMapping );
	Bool enabledState = (e.m_type != CProcessingEvent::TYPE_DETACHED);
	if ( mapping.m_isRuntimeObstacleEnabled != enabledState )
	{
		mapping.m_isRuntimeObstacleEnabled = enabledState;

		CPathLibWorld& pathlib = mapper->GetPathLib();
		const auto& areaList = mapping.m_areaInfo;

		for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
		{
			CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
			CObstaclesMap* obstacles = area->GetObstaclesMap();
			if ( obstacles )
			{
				obstacles->ShowObstacle( it->m_obstacleId, context );
			}
			

			/*if ( obstacles )
			{
				switch ( e.m_type )
				{
				case CProcessingEvent::TYPE_ATTACHED:
					obstacles->ShowObstacle( it->m_obstacleId );
					break;
				case CProcessingEvent::TYPE_DETACHED:
					if ( isImmediate )
					{
						// run heavy processing adding immediate obstacle to the game
						obstacles->RemoveObstacle( it->m_obstacleId );
					}
					else // isDynamic 
					{
						obstacles->HideObstacle( it->m_obstacleId );
					}
					break;
				case CProcessingEvent::TYPE_UPDATED:
					if ( isImmediate )
					{
						// run heavy processing adding immediate obstacle to the game
						obstacles->RemoveObstacle( it->m_obstacleId );
					}
					else // isDynamic 
					{
						obstacles->ShowObstacle( it->m_obstacleId );
					}
					break;
				case CProcessingEvent::TYPE_REMOVED:
				default: 
					ASSERT( false );
					ASSUME( false );
				}
			}*/
		}
	}

	return true;

	
}

Bool IObstacleComponent::IsEffectingInstanceAreas( EPathLibCollision colGroup ) const
{
	return colGroup == PLC_Dynamic || colGroup == PLC_Immediate;
}

Bool IObstacleComponent::ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, CGenerationManager::CAsyncTask::CSynchronousSection& section )
{
	if ( e.m_type != CProcessingEvent::TYPE_ATTACHED && e.m_type != CProcessingEvent::TYPE_UPDATED )
	{
		return false;
	}
	CComponent* engineComponent = AsEngineComponent();
	CObstacleSpawnData obstacleData;
	// we need to compute static shape input
	section.Begin();
	CLayerGroup* layerGroup = processingJob->GetProcessedLayerGroup();
	if ( !layerGroup )
	{
		return false;
	}
	if ( !obstacleData.Initialize( engineComponent, processingJob->GetObstacleMapper()->GetPathLib(), layerGroup ) )
	{
		return false;
	}
	section.End();

	return processingJob->GetObstacleMapper()->ProcessObstacleOffline( obstacleData, e.m_type == CProcessingEvent::TYPE_UPDATED );
}

void IObstacleComponent::ProcessingEventDetach( CProcessingEvent& e )
{
	Super::ProcessingEventDetach( e );

	switch( GetPathLibCollisionGroup() )
	{
	case PLC_Immediate:
		e.m_generalProcessingImplementation = ObstacleImmediateDetachmentMechanism::GetInstance();
		break;
	case PLC_Dynamic:
		e.m_generalProcessingImplementation = ObstacleDynamicDetachmentMechanism::GetInstance();
		break;
	default:
		ASSERT( false, TXT("Hope it won't hit. If so, plz uncomment assume below.") );
		//ASSUME( false );
	}
}

void IObstacleComponent::ProcessingEventRemove( CProcessingEvent& e )
{
	Super::ProcessingEventRemove( e );

	e.m_generalProcessingImplementation = ObstacleRemovalMechanism::GetInstance();
}

Bool IObstacleComponent::IsNoticableInGame( CProcessingEvent::EType eventType ) const
{
	EPathLibCollision collisionType = GetPathLibCollisionGroup();

	switch ( collisionType )
	{
	case PLC_Disabled:
	case PLC_Walkable:
	case PLC_Static:
	case PLC_StaticWalkable:
	case PLC_StaticMetaobstacle:
		return false;
	case PLC_Immediate:
	case PLC_Dynamic:
		return eventType == CProcessingEvent::TYPE_ATTACHED || eventType == CProcessingEvent::TYPE_DETACHED;
	default:
		ASSUME( false );
	}
}
Bool IObstacleComponent::IsNoticableInEditor( CProcessingEvent::EType eventType ) const
{
	EPathLibCollision collisionType = GetPathLibCollisionGroup();

	switch ( collisionType )
	{
	case PLC_Disabled:
	case PLC_Walkable:
	case PLC_Immediate:
		return false;
	case PLC_Static:
	case PLC_StaticWalkable:
	case PLC_StaticMetaobstacle:
	case PLC_Dynamic:
		return eventType != CProcessingEvent::TYPE_DETACHED;
	default:
		ASSUME( false );
	}
}

IObstacleComponent* IObstacleComponent::AsObstacleComponent()
{
	return this;
}

};				// namespace PathLib


