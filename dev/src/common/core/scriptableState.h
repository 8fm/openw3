/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptable.h"
#include "scriptableStateMachine.h"

/// Base state of an object in a state machine
/// Not this class does not derive from CObject any more
class CScriptableState : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CScriptableState );

private:
	IScriptable*		m_stateMachine;		//!< The state machine that spawned us

public:
	//! Get the state machine object
	RED_INLINE IScriptable* GetStateMachine() const { return m_stateMachine; }

public:
	CScriptableState();

	//! Is this state the current state in state machine ?
	Bool IsActive() const;

	//! Get the name of the state
	CName GetStateName() const;

	//! Can we leave this state right now ?
	virtual Bool OnCanEnterState( const CName& previousState );

	//! Can we enter this state right now ?
	virtual Bool OnCanLeaveState( const CName& newState );

	//! State has been activated
	virtual void OnEnterState( const CName& previousState );

	//! State has been exited
	virtual void OnLeaveState( const CName& newState );

	//! Get friendly name
	virtual String GetFriendlyName() const;

private:
	void CallEnterTransitionEvent( CScriptableStateMachine::EStateTransitionType transitionType, const CName& previousState );
	void CallLeaveTransitionEvent( CScriptableStateMachine::EStateTransitionType transitionType, const CName& newState );
	void BindStateMachine( IScriptable* stateMachineScriptableObject );

	void funcIsActive( CScriptStackFrame& stack, void* result );	
	void funcGetStateName( CScriptStackFrame& stack, void* result );	
	void funcCanEnterState( CScriptStackFrame& stack, void* result );
	void funcCanLeaveState( CScriptStackFrame& stack, void* result );
	void funcBeginState( CScriptStackFrame& stack, void* result );
	void funcEndState( CScriptStackFrame& stack, void* result );
	void funcContinuedState( CScriptStackFrame& stack, void* result );
	void funcPausedState( CScriptStackFrame& stack, void* result );

	friend class CScriptableStateMachine;
};

BEGIN_CLASS_RTTI( CScriptableState );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "GetStateName", funcGetStateName );
	NATIVE_FUNCTION( "IsActive", funcIsActive );
	NATIVE_FUNCTION( "CanEnterState", funcCanEnterState );
	NATIVE_FUNCTION( "CanLeaveState", funcCanLeaveState );
	NATIVE_FUNCTION( "BeginState", funcBeginState );
	NATIVE_FUNCTION( "EndState", funcEndState );
	NATIVE_FUNCTION( "ContinuedState", funcContinuedState );
	NATIVE_FUNCTION( "PausedState", funcPausedState );
END_CLASS_RTTI();