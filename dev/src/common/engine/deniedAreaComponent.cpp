#include "build.h"
#include "deniedAreaComponent.h"
#include "layer.h"
#include "entity.h"
#include "world.h"

IMPLEMENT_ENGINE_CLASS( CDeniedAreaComponent );

void CDeniedAreaComponent::SetPathLibCollisionGroupInternal( EPathLibCollision collisionGroup )
{
	m_collisionType = collisionGroup;
}
EPathLibCollision CDeniedAreaComponent::GetPathLibCollisionGroup() const
{
	return m_collisionType;
}

CComponent* CDeniedAreaComponent::AsEngineComponent()
{
	return this;
}
PathLib::IComponent* CDeniedAreaComponent::AsPathLibComponent()
{
	return this;
}

Bool CDeniedAreaComponent::IsLayerBasedGrouping() const
{
	return !m_canBeDisabled && m_collisionType == PLC_Dynamic;
}

Bool CDeniedAreaComponent::IsNoticableInGame( PathLib::CProcessingEvent::EType eventType ) const
{
	switch ( m_collisionType )
	{
	default:
	case PLC_Disabled:
	case PLC_Walkable:
	case PLC_Static:
	case PLC_StaticWalkable:
	case PLC_StaticMetaobstacle:
		return false;
	case PLC_Dynamic:
		// check if it uses layer based grouping
		if ( !m_canBeDisabled )
		{
			return false;
		}
		// no break!
	case PLC_Immediate:
		return eventType == PathLib::CProcessingEvent::TYPE_ATTACHED || eventType == PathLib::CProcessingEvent::TYPE_DETACHED;
	}
}

void CDeniedAreaComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	if ( const SDeniedAreaSpawnData* spawnData = ( const SDeniedAreaSpawnData* ) spawnInfo.m_customData )
	{
		m_collisionType = spawnData->m_collisionType;
	}

	TBaseClass::OnSpawned( spawnInfo );
}

void CDeniedAreaComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CDeniedAreaComponent_OnAttached );

	if ( m_isEnabled )
	{
		IObstacleComponent::Attach( world );
	}
}
void CDeniedAreaComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	if ( m_isEnabled )
	{
		IObstacleComponent::Detach( world );
	}
}
void CDeniedAreaComponent::OnEditorEndVertexEdit()
{
	TBaseClass::OnEditorEndVertexEdit();

	UpdateObstacle();
}

#ifndef NO_EDITOR

void CDeniedAreaComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();

	UpdateObstacle();
}

void CDeniedAreaComponent::EditorPreDeletion()
{
	CLayer* layer = GetEntity()->GetLayer();
	if ( layer )
	{
		CWorld* world = layer->GetWorld();
		if ( world )
		{
			IObstacleComponent::Remove( world );
		}
	}
}

Bool CDeniedAreaComponent::RemoveOnCookedBuild()
{
	switch ( m_collisionType )
	{
	case PLC_Dynamic:
		if ( !m_canBeDisabled )
		{
			return true;
		}
		// notice no break
	case PLC_Immediate:
		return false;
	default:
		break;
	}
	return true;
}

#endif

void CDeniedAreaComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	static const CName collisionType( TXT("collisionType") );

	if ( property->GetName() == collisionType )
	{
		CEntity* entity = GetEntity();
		if ( entity && !entity->IsInGame() )
		{
			CLayer* layer = entity->GetLayer();
			if ( layer )
			{
				CWorld* world = layer->GetWorld();
				if ( world )
				{
					CWorld* world = layer->GetWorld();
					if ( world )
					{
						IObstacleComponent::OnCollisionGroupUpdated( world, m_collisionType );
					}
				}
			}
		}
	}
	if ( property->GetName() == CNAME( isEnabled ) )
	{
		CWorld* world = GetWorld();
		CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
		if ( pathlib )
		{
			if ( m_isEnabled )
			{
				IObstacleComponent::Enable( world );
			}
			else
			{
				IObstacleComponent::Disable( world );
			}
		}
	}
}

//! Visualize denied area state
Color CDeniedAreaComponent::CalcLineColor() const
{
	Color c;
	Uint8 a = m_isEnabled ? 220 : 60;
	switch( m_collisionType )
	{
	default:
	case PLC_Disabled:
		c = Color( 180, 180, 180, a );
		break;
	case PLC_Static:
		c = Color( 255, 0, 0, a );
		break;
	case PLC_StaticWalkable:
		c = Color( 0, 255, 0, a );
		break;
	case PLC_StaticMetaobstacle:
		c = Color( 0, 128, 255, a );
		break;
	case PLC_Immediate:
		c = Color( 255, 0, 255, a );
		break;
	case PLC_Dynamic:
		c = Color( 128, 0, 255, a );
		break;
	case PLC_Walkable:
		c = Color( 0, 70, 175, a );
		break;
	}
	return c;
}

void CDeniedAreaComponent::SetEnabled( Bool enabled )
{
	if ( enabled != m_isEnabled )
	{
		m_isEnabled = enabled;
		CWorld* world = GetLayer()->GetWorld();
		if ( world )
		{
			if ( m_isEnabled )
			{
				IObstacleComponent::Enable( world );
			}
			else
			{
				IObstacleComponent::Disable( world );
			}
		}
	}
}

void CDeniedAreaComponent::UpdateObstacle()
{
	CEntity* entity = GetEntity();
	if ( entity->IsInGame() )
	{
		return;
	}
	CLayer* layer = entity->GetLayer();
	if ( layer )
	{
		CWorld* world = layer->GetWorld();
		if ( world )
		{
			IObstacleComponent::Update( world );
		}
	}
}