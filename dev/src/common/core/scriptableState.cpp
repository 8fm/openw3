/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptableStateMachine.h"
#include "scriptableState.h"
#include "scriptStackFrame.h"
#include "functionCalling.h"

IMPLEMENT_RTTI_CLASS( CScriptableState );

CScriptableState::CScriptableState()
{
}

Bool CScriptableState::IsActive() const
{
	if ( nullptr != m_stateMachine )
	{
		return m_stateMachine->GetCurrentState() == this;
	}

	return false;
}

CName CScriptableState::GetStateName() const
{
	// Make sure class has the state flag :)
	CClass* thisClass = GetClass();
	ASSERT( thisClass->IsState() );

	// Get the state name
	return thisClass->GetStateName();
}

String CScriptableState::GetFriendlyName() const
{
	// Get name of the state
	String stateName = GetStateName().AsString();

	// Get the state machine
	if ( nullptr != m_stateMachine )
	{
		stateName += TXT(" in ");
		stateName += m_stateMachine->GetFriendlyName();
	}

	// Return composed name
	return stateName;
}

Bool CScriptableState::OnCanEnterState( const CName& previousState )
{
	Bool result = true;

	// Call script function	
	if( !CallFunctionRet< Bool >( this, CNAME( OnCanEnterState ), previousState, result ) )
	{
		// backward compatibility
		CallFunctionRet< Bool >( this, CNAME( OnCanEnterState ), result );
	}

	return result;
}

Bool CScriptableState::OnCanLeaveState( const CName& newState )
{
	Bool result = true;

	// Call script function
	if( !CallFunctionRet< Bool >( this, CNAME( OnCanLeaveState ), newState, result ) )
	{
		// backward compatibility
		CallFunctionRet< Bool >( this, CNAME( OnCanLeaveState ), result );
	}

	return result;
}

void CScriptableState::CallEnterTransitionEvent( CScriptableStateMachine::EStateTransitionType transitionType, const CName& previousState )
{
	switch ( transitionType )
	{
	case CScriptableStateMachine::EStateTransitionType_Pop:
		CallFunction( this, CNAME( ContinuedState ) );
		break;
	case CScriptableStateMachine::EStateTransitionType_Direct:
	case CScriptableStateMachine::EStateTransitionType_Push:
		CallFunction( this, CNAME( BeginState ), previousState );
		break;
	}
}

void CScriptableState::OnEnterState( const CName& previousState )
{
	// Invoke transition callback
	CallEnterTransitionEvent( m_stateMachine->GetCurrentTransitionType(), previousState );

	// Call event
	EEventCallResult res = CallEvent( CNAME( OnEnterState ), previousState );

	// backward compatibility
	if( res == CR_EventNotFound )
	{
		CallEvent( CNAME( OnEnterState ) );
	}
}

void CScriptableState::CallLeaveTransitionEvent( CScriptableStateMachine::EStateTransitionType transitionType, const CName& newState )
{
	switch ( transitionType )
	{
	case CScriptableStateMachine::EStateTransitionType_Push:
		CallFunction( this, CNAME( PausedState ) );
		break;
	case CScriptableStateMachine::EStateTransitionType_Pop:
	case CScriptableStateMachine::EStateTransitionType_Direct:
		CallFunction( this, CNAME( EndState ), newState );
		break;
	default:
		break;
	}	
}

void CScriptableState::BindStateMachine( IScriptable* stateMachineScriptableObject )
{
	ASSERT( stateMachineScriptableObject != nullptr );
	ASSERT( m_stateMachine == nullptr || m_stateMachine == stateMachineScriptableObject );
	m_stateMachine = stateMachineScriptableObject;
}

void CScriptableState::OnLeaveState( const CName& newState )
{
	// Invoke transition callback
	CallLeaveTransitionEvent( m_stateMachine->GetCurrentTransitionType(), newState );

	// Call event
	EEventCallResult res = CallEvent( CNAME( OnLeaveState ), newState );

	// backward compatibility
	if( res == CR_EventNotFound )
	{
		CallEvent( CNAME( OnLeaveState ) );
	}
}

void CScriptableState::funcIsActive( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsActive() );
}

void CScriptableState::funcGetStateName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetStateName() );
}

void CScriptableState::funcCanEnterState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( true );
}

void CScriptableState::funcCanLeaveState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( true );
}

void CScriptableState::funcBeginState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, prevStateName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_VOID();
}

void CScriptableState::funcEndState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, nextStateName, CName::NONE );
	FINISH_PARAMETERS;
	RETURN_VOID();
}

void CScriptableState::funcContinuedState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_VOID();
}

void CScriptableState::funcPausedState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_VOID();
}