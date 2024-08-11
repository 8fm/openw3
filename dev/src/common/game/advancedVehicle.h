
#pragma once

#include "vehicle.h"
#include "pilotComponent.h"
#include "seatComponent.h"


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
class CAdvancedVehicleComponent : public CVehicleComponent
{
	DECLARE_ENGINE_CLASS( CAdvancedVehicleComponent, CVehicleComponent, 0 );

protected:
	// State
	Bool							m_isIdle;

	// Riders
	CPilotComponent*				m_pilot;
	TDynArray< CSeatComponent* >	m_passengerSeats;

	// Player related
	Bool							m_isPlayerControlled;
	CName							m_inputContext;

public:
	RED_INLINE void SetIdle	( Bool idle )					{ m_isIdle = idle; };

public:
	virtual void OnPilotMounted			( CPilotComponent* pilot );
	virtual void OnPilotDisMounted		( );
	virtual void OnPassengerMounted		( CPilotComponent* passenger );

protected:
	virtual void OnAttached		( CWorld* world ) override;
	void OnAttachFinishedEditor	( CWorld* world );

	void UpdateInput				( Float timeDelta );
	virtual void UpdatePlayerInput	( Float timeDelta );
	virtual void UpdateAIInput		( Float timeDelta );

	virtual void OnTick			( Float timeDelta ) override;
	virtual void OnTickIdle		( Float timeDelta );
	void OnTickActive			( Float timeDelta );
	
	virtual void UpdateLogic		( Float timeDelta );
	virtual void UpdateAfterLogic	( Float timeDelta );

private:
	void funcIsPlayerControlled	( CScriptStackFrame& stack, void* result );
	void funcOnPilotMounted		( CScriptStackFrame& stack, void* result );
	void funcOnPilotDisMounted	( CScriptStackFrame& stack, void* result );
	void funcSetIdle			( CScriptStackFrame& stack, void* result );

};
BEGIN_CLASS_RTTI( CAdvancedVehicleComponent );
	PARENT_CLASS( CVehicleComponent );
	PROPERTY_INLINED( m_passengerSeats		, TXT("Passenger seats") );
	PROPERTY_EDIT( m_inputContext			, TXT("player input context name") );
	NATIVE_FUNCTION( "I_IsPlayerControlled"	, funcIsPlayerControlled );	
	NATIVE_FUNCTION( "I_SetIdle"			, funcSetIdle );	
END_CLASS_RTTI();
