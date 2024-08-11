/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "playerStateUseVehicle.h"

IMPLEMENT_ENGINE_CLASS( CPlayerStateUseVehicle );
IMPLEMENT_ENGINE_CLASS( CPlayerStatePostUseVehicle );

void CPlayerStateUseVehicle::OnEnterState( const CName& previousState )
{
	CPlayerStateBase::OnEnterState( previousState );

	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

	mac->SetUseExtractedMotion( false );
	mac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_Default );
}

void CPlayerStateUseVehicle::OnLeaveState( const CName& newState )
{
	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

	mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_Default );
	mac->SetUseExtractedMotion( true );

	CPlayerStateBase::OnLeaveState( newState );
}

void CPlayerStatePostUseVehicle::OnEnterState( const CName& previousState )
{
	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

    // This one disables character controller collisions and pushes character under terrain (eg when falling on horse)
    // So do not disable collisions when exiting vehicle
	//mac->EnableCollisions( false, CMovingAgentComponent::LS_Default );

	CPlayerStateBase::OnEnterState( previousState );
}

void CPlayerStatePostUseVehicle::OnLeaveState( const CName& newState )
{
    CPlayerStateBase::OnLeaveState( newState );

	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

	mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_Default );
}

void CPlayerStatePostUseVehicle::funcHACK_DeactivatePhysicsRepresentation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

	mac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_Default );
}

void CPlayerStatePostUseVehicle::funcHACK_ActivatePhysicsRepresentation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CMovingAgentComponent* mac = GetPlayer()->GetMovingAgentComponent();
	ASSERT( mac );

	mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_Default );
}

