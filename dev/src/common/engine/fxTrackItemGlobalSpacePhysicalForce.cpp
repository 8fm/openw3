/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemGlobalSpacePhysicalForce.h"
#include "fxTrackGroup.h"
#include "phantomComponent.h"
#include "layer.h"
#include "fxDefinition.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemGlobalSpacePhysicalForce );

class CFXTrackItemGlobalSpacePhysicalForcePlayData : public IFXTrackItemPlayData
{
public:
	const CFXTrackItemGlobalSpacePhysicalForce*	m_track;
	THandle< CForceFieldEntity >				m_entity;
	THandle< CPhantomComponent >				m_createdPhantomComponent;

public:
	CFXTrackItemGlobalSpacePhysicalForcePlayData( const CFXTrackItemGlobalSpacePhysicalForce* trackItem, CForceFieldEntity* ent, CComponent* component )
		: IFXTrackItemPlayData( component, trackItem )
		, m_track( trackItem )
		, m_entity( ent )
	{
	};

	virtual ~CFXTrackItemGlobalSpacePhysicalForcePlayData()
	{
		CEntity *entity = m_entity;
		if ( entity )
		{
			CForceFieldEntity::m_elements.RemoveFast( m_entity );

			entity->Destroy();
			m_entity = NULL;
		}
	}

	virtual void OnStop() {}

	virtual void OnTick( const CFXState& fxState, Float timeDelta )
	{
		if( fxState.IsStopNotified() ) return;

		CComponent* phantomComponent = m_createdPhantomComponent.Get();
		if( !phantomComponent )
		{
			const TDynArray< CComponent* >& components = m_entity->GetComponents();
			for( Uint32 i = 0; i != components.Size(); ++i )
			{
				CPhantomComponent* phantomComponent = Cast< CPhantomComponent >( components[ i ] );
				if( phantomComponent )
				{
					m_createdPhantomComponent = phantomComponent;
					break;
				}
			}
		}
		
		if ( phantomComponent )
		{
			const TDynArray< STriggeringInfo >& triggered = m_createdPhantomComponent.Get()->GetTriggered();

			m_track->GetForceObject()->OnTick( m_entity, triggered, m_track, fxState, timeDelta );
		}
	}

};

CFXTrackItemGlobalSpacePhysicalForce::CFXTrackItemGlobalSpacePhysicalForce()
: CFXTrackItemCurveBase( 4 )
{
}


IFXTrackItemPlayData* CFXTrackItemGlobalSpacePhysicalForce::OnStart( CFXState& fxState ) const
{
	CComponent* component = GetTrack()->GetTrackGroup()->GetAffectedComponent( fxState );
	if( !component )
	{
		WARN_ENGINE( TXT("No component set for world force effect track item") );
		return NULL;
	}

	if ( !m_forceObject )
	{
		WARN_ENGINE( TXT("No force phantom set for world force effect track item") );
		return NULL;
	}

	EntitySpawnInfo einfo;
	einfo.m_spawnPosition = component->GetWorldPosition();
	einfo.m_entityClass = CForceFieldEntity::GetStaticClass();

	CForceFieldEntity* entity = Cast< CForceFieldEntity >( fxState.CreateDynamicEntity( einfo ) );
	CForceFieldEntity::m_elements.PushBack( entity );

	CPhantomComponent* createdComponent = m_forceObject->OnSpawn( entity, component, this, fxState );
	entity->m_data = m_forceObject;
	entity->m_phantomComponent = createdComponent;

	CFXTrackItemGlobalSpacePhysicalForcePlayData* playData = new CFXTrackItemGlobalSpacePhysicalForcePlayData( this, entity, createdComponent );

	createdComponent->Activate( createdComponent );

	CPhysicsWrapperInterface* wrapper = createdComponent->GetPhysicsRigidBodyWrapper();

	if ( !createdComponent )
	{
		ERR_ENGINE( TXT("Unable to create component for physics force track") );
		entity->Destroy();
		return NULL;
	}

	return playData;
}

