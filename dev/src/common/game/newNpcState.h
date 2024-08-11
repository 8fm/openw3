/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/scriptableState.h"

class CNewNPC;

/// Basic state of NPC
class CNewNPCStateBase : public CScriptableState
{
	DECLARE_RTTI_SIMPLE_CLASS( CNewNPCStateBase );

public:
	CNewNPCStateBase() {}

	//! Get the NPC we are in
	RED_INLINE CNewNPC* GetNPC() const;

public:
	//! Activate state
	virtual void OnEnterState( const CName& previousState );

	//! Leave state
	virtual void OnLeaveState( const CName& newState );

	//! Update state
	virtual void OnUpdate( Float timeDelta, Bool prepareForDeactivation );

	//! Can react from current state
	virtual Bool CanReact() const { return false; }

private:
	void funcMarkGoalFinished( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CNewNPCStateBase );
	PARENT_CLASS( CScriptableState );
	NATIVE_FUNCTION( "MarkGoalFinished", funcMarkGoalFinished );
END_CLASS_RTTI();

/// Base reacting state
class CNewNPCStateReactingBase : public CNewNPCStateBase
{
	DECLARE_RTTI_SIMPLE_CLASS( CNewNPCStateReactingBase );

public:

	//! Can react from current state
	virtual Bool CanReact() const { return true; }
};

BEGIN_CLASS_RTTI( CNewNPCStateReactingBase );
	PARENT_CLASS( CNewNPCStateBase );
END_CLASS_RTTI();
