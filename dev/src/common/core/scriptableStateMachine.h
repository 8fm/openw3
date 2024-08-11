/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "scriptThread.h"

class CScriptableState;

/// Part of IScriptable responsible for state machine handling
/// This class is never exposed directly to outside world
/// This class is never instanced directly
class CScriptableStateMachine : public CScriptThread::IListener
{
protected:
	// Types of transitions between states
	enum EStateTransitionType
	{
		EStateTransitionType_None,		//! No transition
		EStateTransitionType_Push,		//! State change by pushing new state onto the stack via PushState()
		EStateTransitionType_Pop,		//! State change by popping state from the stack via PopState()
		EStateTransitionType_Direct		//! Direct state change via GoToState() (optionally clearing the stack)
	};

	// State machine flags
	enum EStateMachineFlags
	{
		SMF_ChangingState			= FLAG( 0 ),		//!< We are changing state right now
		SMF_MachineBlocked			= FLAG( 1 ),		//!< State machine locked (changed only from code)
		SMF_EntryFunctionLocked		= FLAG( 2 ),		//!< Current entry function is locked	
		SMF_DumpEntryFunctionCalls	= FLAG( 3 ),		//!< Entry function logging enabled
	};

	// Internal data
	struct SActiveData
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_ScriptData );

		typedef THashMap< CName, CScriptableState* >		TStateMap;			// TODO: memory class support
		typedef THashMap< CName, const CFunction* >			TFunctionMap;		// TODO: memory class support
		typedef TDynArray< CName, MC_ScriptData >		TStateStack;

		TStateMap					m_spawnedStates;		//!< All spawned states
		TFunctionMap				m_entryFunctions;		//!< Cached entry functions
		CScriptableState*			m_currentState;			//!< Current active state
		THandle< CScriptThread >	m_thread;				//!< Entry function thread
		CName						m_cleanupFunctionName;	//!< Event called on function break
		EStateTransitionType		m_currentTransitionType;//!< Transition type for current transition
		TStateStack					m_stateStack;			//!< State stack used for push/pop functionality
		Uint8						m_stateMachineFlags;	//!< State machine flags

		SActiveData();
		~SActiveData();
	};

public:
	//! Get the current state
	CScriptableState* GetCurrentState() const;

	//! Get name of current state
	const CName GetCurrentStateName() const;

	//! Get the top level thread function
	const CFunction* GetTopLevelFunction() const;

	//! Get state transition type
	const EStateTransitionType GetCurrentTransitionType() const;

	//! Get all states
	void GetStates( TDynArray< CScriptableState* >& states ) const;
	
	//! Restore state (used only by script reloaded)
	void RestoreState( CScriptableState* state, Bool active );

	//! Change state ( forced way, stack can be NULL )
	Bool ChangeState( CName stateName, CScriptStackFrame* stack );

	//! Removes current state, called before destroying object
	void KillCurrentState();

	//! Enter state via given state function, returns true if state was changed
	Bool EnterState( CName stateName, const CFunction* stateEntryFunction, CScriptStackFrame& stack );

	//! Gets current transition type
	const EStateTransitionType GetTransitionType() const;

	//! Kill internal thread
	void KillThread();

	//! Find entry function for a state in this state machine
	const CFunction* FindEntryFunction( const CName name );

	//! Block state machine
	void BlockStateMachine( Bool block );

	//! Serialization (GC)
	void DumpStatesForGC( IFile& file );

protected:
	//! CScriptThread interface - a script thread was killed
	virtual void OnScriptThreadKilled( CScriptThread* thread, Bool finished );
	virtual String GetDebugName() const { return TXT("CScriptedStateMachine"); }

	//! Get the state machine owner (IScriptable)
	virtual IScriptable* GetStateMachineOwner() const = 0;

	//! The interface function - creates the script data
	virtual SActiveData* GetStateMachineData( const Bool createIfNotFound = false ) const = 0;

private:
	// Find virtual function with given name in this object, this uses the active state for filtering
	const CFunction* FindStateFunction( IScriptable*& context, const CName functionName ) const;

	// Test for cleanup function on stack
	Bool ChangeStateCleanupTest( const CScriptStackFrame* stack );

	// Internal function - Push state onto the state machine
	void PushState( const CName stateName, CScriptStackFrame* stack );

	// Internal function - Pop state from the state machine
	void PopState( Bool popAll, Bool eventsOnly, CScriptStackFrame* stack );

	// Internal function - pop all of the states stored in the stack frame state list
	const CName PopStateStack( CScriptStackFrame* stack );

	// Internal function - Goto new state
	void GotoState( CName newState, Bool keepStack, Bool forceEvents, CScriptStackFrame* stack );

	// Internal function - Check if we are inside a given state
	Bool IsInState( const CName stateName );
	
	// Internal function - Change state to new one
	Bool ChangeState( const CName newState, CScriptStackFrame* stack, EStateTransitionType transition );

	// Internal function - Change state to the automatic one
	void GotoStateAuto( CScriptStackFrame* stack );

	// Internal function - Log states to log output
	void LogStates( const String& context = String() );

	// Internal function - Get state for name (from the internal data stack)
	CScriptableState* GetState( const CName stateName );

private:
	void funcIsInState( CScriptStackFrame& stack, void* result );
	void funcPushState( CScriptStackFrame& stack, void* result );
	void funcPopState( CScriptStackFrame& stack, void* result );
	void funcGotoState( CScriptStackFrame& stack, void* result );
	void funcGotoStateAuto( CScriptStackFrame& stack, void* result );
	void funcLogStates( CScriptStackFrame& stack, void* result );
	void funcChangeState( CScriptStackFrame& stack, void* result );
	void funcGetState( CScriptStackFrame& stack, void* result );
	void funcGetCurrentState( CScriptStackFrame& stack, void* result );
	void funcGetCurrentStateName( CScriptStackFrame& stack, void* result );
	void funcStop( CScriptStackFrame& stack, void* result );
	void funcLockEntryFunction( CScriptStackFrame& stack, void* result );
	void funcSetCleanupFunction( CScriptStackFrame& stack, void* result );
	void funcClearCleanupFunction( CScriptStackFrame& stack, void* result );
	void funcDebugDumpEntryFunctionCalls( CScriptStackFrame& stack, void* result );

	friend class IScriptable;
	friend class IScriptableClassBuilder; // HACK!!
	friend class CScriptableState;
};
