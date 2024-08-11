/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "advancedVehicle.h"
#include "../../common/engine/inputManager.h"

IMPLEMENT_ENGINE_CLASS( CAdvancedVehicleComponent );

RED_DEFINE_STATIC_NAME( OnVehicleInit );
RED_DEFINE_STATIC_NAME( OnVehicleTickIdle );
RED_DEFINE_STATIC_NAME( OnVehicleTickLogic );
RED_DEFINE_STATIC_NAME( OnVehicleTickAfterLogic );

RED_DEFINE_STATIC_NAME( OnVehiclePilotMounted );
RED_DEFINE_STATIC_NAME( OnVehiclePilotDisMounted );
RED_DEFINE_STATIC_NAME( OnVehiclePassengerMounted );

RED_DEFINE_STATIC_NAME( OnUpdatePlayerInput );
RED_DEFINE_STATIC_NAME( OnUpdateAIInput );


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CAdvancedVehicleComponent_OnAttached );

	// Get the seats for passengers
	ComponentIterator< CSeatComponent > it( GetEntity() );
	for ( ; it; ++it )
	{
		CSeatComponent* seat = *it;

		m_passengerSeats.PushBack( seat );
	}

	m_isIdle	= true;

	// Init scripts
	CallEvent( CNAME( OnVehicleInit ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnAttachFinishedEditor( CWorld* world )
{
	/*TBaseClass::OnAttachFinishedEditor( world );

	// Create seats for passengers
	ComponentIterator< CSeat > it( GetEntity() );
	for ( ; it; ++it )
	{
		CSeat* seat = *it;

		m_passengerSeats.PushBack( seat );
	}
	*/
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnPilotMounted( CPilotComponent* pilot )
{
	if( !pilot )
	{
		return;
	}

	m_pilot	= pilot;

	m_isPlayerControlled = m_pilot->IsPlayer();

	// Input
	if( m_isPlayerControlled )
	{
		GGame->GetInputManager()->SetCurrentContext( m_inputContext );
	}

	
	CallEvent( CNAME( OnVehiclePilotMounted ), THandle< CPilotComponent >( pilot ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnPilotDisMounted( )
{
	m_pilot	= nullptr;

	// Input
	if( m_isPlayerControlled )
	{
		GGame->GetInputManager()->SetCurrentContext( CNAME( Exploration ) );
	}

	m_isPlayerControlled	= false;

	CallEvent( CNAME( OnVehiclePilotDisMounted ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnPassengerMounted( CPilotComponent* passenger )
{
	RED_ASSERT( passenger , TXT( "Trying to mount a NULL passenger" ) );
	CallEvent( CNAME( OnVehiclePassengerMounted ), THandle< CPilotComponent >( passenger ) );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnTick( Float timeDelta )
{
	if( m_isIdle )
	{
		OnTickIdle( timeDelta );
	}
	else
	{
		OnTickActive( timeDelta );
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnTickIdle( Float timeDelta )
{
	CallEvent( CNAME( OnVehicleTickIdle ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::OnTickActive( Float timeDelta )
{
	UpdateInput( timeDelta );

	UpdateLogic( timeDelta );

	UpdateAfterLogic( timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::UpdateInput( Float timeDelta )
{
	if( !m_isIdle && m_pilot )
	{
		if( m_pilot->IsPlayer() )
		{
			UpdatePlayerInput( timeDelta );
		}
		else
		{
			UpdateAIInput( timeDelta );
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::UpdatePlayerInput( Float timeDelta )
{
	CallEvent( CNAME( OnUpdatePlayerInput ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::UpdateAIInput( Float timeDelta )
{
	CallEvent( CNAME( OnUpdateAIInput ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::UpdateLogic( Float timeDelta )
{
	CallEvent( CNAME( OnVehicleTickLogic ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::UpdateAfterLogic( Float timeDelta )
{
	CallEvent( CNAME( OnVehicleTickAfterLogic ), timeDelta );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::funcSetIdle( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool , idle, false );
	FINISH_PARAMETERS;

	SetIdle( idle );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::funcOnPilotMounted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CPilotComponent>, pilot, nullptr );
	FINISH_PARAMETERS;

	OnPilotMounted( pilot.Get() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::funcOnPilotDisMounted( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	OnPilotDisMounted();
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAdvancedVehicleComponent::funcIsPlayerControlled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_isPlayerControlled );
}