
#pragma once

#include "seatComponent.h"
#include "advancedVehicle.h"
#include "seatComponent.h"

class CPilotComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CPilotComponent, CComponent, 0 );

private:
	CSeatComponent*	m_ridingSeat;

public:
	RED_INLINE Bool						IsPlayer	( )								{ return GetEntity()	== GCommonGame->GetPlayerEntity(); };
	RED_INLINE Bool						IsDriving	( )								{ return m_ridingSeat && m_ridingSeat->IsPilot(); };
	RED_INLINE void						SetRiding	( CSeatComponent* _ridingSeat )	{ m_ridingSeat	= _ridingSeat; };
	RED_INLINE CSeatComponent*			GetSeat		( )								{ return m_ridingSeat; };
	RED_INLINE CAdvancedVehicleComponent*	GetVehicle	( )								{ return m_ridingSeat->GetVehicle(); };

	void OnAttached	( CWorld* world );

protected:
	void funcIsPlayer	( CScriptStackFrame& stack, void* result );
	void funcGetSeat	( CScriptStackFrame& stack, void* result );
	void funcGetVehicle	( CScriptStackFrame& stack, void* result );
	void funcIsDriving	( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CPilotComponent );
	PARENT_CLASS( CComponent );
	NATIVE_FUNCTION( "I_IsPlayer"	, funcIsPlayer );
	NATIVE_FUNCTION( "I_IsDriving"	, funcIsDriving );
END_CLASS_RTTI();
