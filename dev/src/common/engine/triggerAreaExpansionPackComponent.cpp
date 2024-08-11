/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "game.h"
#include "tagManager.h"
#include "tickManager.h"
#include "gameTimeManager.h"
#include "world.h"
#include "entity.h"
#include "triggerAreaExpansionPackComponent.h"

IMPLEMENT_ENGINE_CLASS( CTriggerAreaExpansionPackComponent );


CTriggerAreaExpansionPackComponent::CTriggerAreaExpansionPackComponent() 
	: m_deactivateTime( 5 )
{
}

void CTriggerAreaExpansionPackComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	if ( !IsEnabled() )
		return;

	if ( !GGame->IsActive() )
	{
		return;
	}

	m_attachTime = GGame->GetTimeManager()->GetTime();
	world->GetTickManager()->AddToGroup( this, TICK_Main );
}

void CTriggerAreaExpansionPackComponent::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	if ( GetSecondsSinceAttach() > m_deactivateTime )
	{
		Deactivate();
	}
}

Int32 CTriggerAreaExpansionPackComponent::GetSecondsSinceAttach() const
{
	return ( GGame->GetTimeManager()->GetTime() - m_attachTime ).GetSeconds();
}

void CTriggerAreaExpansionPackComponent::OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator )
{
	auto* playerToTeleport = GetPlayerToTeleport( activator );
	if ( playerToTeleport == nullptr )
	{
		return;
	}

	Deactivate();
	TeleportPlayerToTargetNode( playerToTeleport );
	TBaseClass::OnActivatorEntered( object, activator );
}

CEntity* CTriggerAreaExpansionPackComponent::GetPlayerToTeleport( const class ITriggerActivator* activator )
{
	const auto* component = activator ? activator->GetComponent() : nullptr;
	if ( component == nullptr )
	{
		return nullptr;
	}

	auto* entity = component->GetEntity();
	if ( entity == nullptr )
	{
		return nullptr;
	}

	if ( !entity->IsPlayer() )
	{	
		return nullptr;
	}

	return entity;
}

void CTriggerAreaExpansionPackComponent::TeleportPlayerToTargetNode( CEntity* playerEntity ) const
{
	const auto* targetEntity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_targetTag );
	if ( targetEntity )
	{
		const auto worldPosition = targetEntity->GetWorldPosition();
		playerEntity->Teleport( worldPosition, playerEntity->GetRotation() );
	}
}

void CTriggerAreaExpansionPackComponent::Deactivate()
{
	GetWorld()->GetTickManager()->RemoveFromGroup( this, TICK_Main );
	SetEnabled( false );
}
