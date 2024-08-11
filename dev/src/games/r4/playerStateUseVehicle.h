
#pragma once

#include "../../common/game/playerStateBase.h"

class CPlayerStateUseVehicle : public CPlayerStateBase
{	
	DECLARE_RTTI_SIMPLE_CLASS( CPlayerStateUseVehicle );

public:
	virtual void OnEnterState( const CName& previousState );
	virtual void OnLeaveState( const CName& newState );
};

BEGIN_CLASS_RTTI( CPlayerStateUseVehicle );
	PARENT_CLASS( CPlayerStateBase );
END_CLASS_RTTI();


class CPlayerStatePostUseVehicle : public CPlayerStateBase
{	
    DECLARE_RTTI_SIMPLE_CLASS( CPlayerStatePostUseVehicle );

public:
    virtual void OnEnterState( const CName& previousState );
    virtual void OnLeaveState( const CName& newState );

private:
	void funcHACK_DeactivatePhysicsRepresentation( CScriptStackFrame& stack, void* result );
	void funcHACK_ActivatePhysicsRepresentation( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CPlayerStatePostUseVehicle );
	PARENT_CLASS( CPlayerStateBase );
	NATIVE_FUNCTION( "HACK_DeactivatePhysicsRepresentation", funcHACK_DeactivatePhysicsRepresentation );
	NATIVE_FUNCTION( "HACK_ActivatePhysicsRepresentation", funcHACK_ActivatePhysicsRepresentation );
END_CLASS_RTTI();