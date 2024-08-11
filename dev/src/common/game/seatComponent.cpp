/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "seatComponent.h"
#include "advancedVehicle.h"
#include "pilotComponent.h"


IMPLEMENT_ENGINE_CLASS( CSeatComponent );


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
CSeatComponent::CSeatComponent()
	: m_vehicle	( nullptr )
	, m_rider	( nullptr )
{
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::GrabVehicleIfNeeded( )
{
	if( !m_vehicle )
	{
		CEntity* parentEntity = GetEntity();
		RED_ASSERT( parentEntity, TXT( "Could not find parent entity for CSeatComponent" ) );

		if( !parentEntity->IsInGame() )
		{
			return;
		}

		ComponentIterator< CAdvancedVehicleComponent > it( parentEntity );

		RED_ASSERT( it, TXT("Could not locate CAdvancedVehicleComponent in parentEntity" ) );

		CAdvancedVehicleComponent* vehicle = *it;

		if( vehicle )
		{
			m_vehicle	= vehicle;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CSeatComponent_OnAttached );

	GrabVehicleIfNeeded();

	// Find the slot if there is any and set the position on it
	if( m_slotName != CName::NONE )
	{
		const CEntityTemplate* templ = GetEntity()->GetEntityTemplate();
		if ( templ )
		{
			const EntitySlot* entitySlot = templ->FindSlotByName( m_slotName, true );
			if ( entitySlot )
			{
				Vector		position	=	entitySlot->GetTransform().GetPosition();
				EulerAngles	rotation	=	entitySlot->GetTransform().GetRotation();
				SetPosition( position );
				SetRotation( rotation );
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::OnMounted( CPilotComponent* rider )
{
	RED_ASSERT( m_vehicle, TXT( "A seat may not be instantiated without a vehicle" ) );

	if( !rider )
	{
		return;
	}

	// Set the new rider
	m_rider	= rider;

	// Reset rider position and attach it to the proper slot
	CEntity*	riderEntity		= m_rider->GetEntity();
	riderEntity->Teleport( Vector::ZEROS, EulerAngles::ZEROS );
	riderEntity->CreateAttachmentImpl( GetEntity(), m_slotName );

	// Tell the vehicle
	GrabVehicleIfNeeded();
	if( m_isPilot )
	{
		m_vehicle->OnPilotMounted( m_rider );
	}
	else
	{
		m_vehicle->OnPassengerMounted( m_rider );
	}

	// Confirm to the rider
	rider->SetRiding( this );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::OnDisMounted()
{
	RED_ASSERT( m_vehicle, TXT( "A seat may not be instantiated without a vehicle" ) );

	if( !m_rider )
	{
		return;
	}

	// Deattach
	m_rider->GetEntity()->BreakAttachment();

	// Tell the vehicle
	GrabVehicleIfNeeded();
	if( m_isPilot )
	{
		m_vehicle->OnPilotDisMounted();
	}
	else
	{
		m_vehicle->OnPassengerMounted( nullptr );
	}

	// Confirm to the rider
	m_rider->SetRiding( nullptr );
	
	m_rider	= nullptr;
}


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::funcIsPilot( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsPilot() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::funcOnMounted( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CPilotComponent>, pilot, nullptr );
	FINISH_PARAMETERS;

	OnMounted( pilot.Get() );
}

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CSeatComponent::funcOnDisMounted( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	OnDisMounted();
}