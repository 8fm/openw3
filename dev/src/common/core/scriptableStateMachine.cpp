/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptingSystem.h"
#include "scriptableStateMachine.h"
#include "scriptableState.h"
#include "scriptStackFrame.h"

//---------------------------------------------------------------------------

CScriptableStateMachine::SActiveData::SActiveData()
	: m_currentState( NULL )
	, m_thread( NULL )
	, m_currentTransitionType( CScriptableStateMachine::EStateTransitionType_None )
	, m_stateMachineFlags( 0 )
{
}

CScriptableStateMachine::SActiveData::~SActiveData()
{
	RED_ASSERT( !m_thread.IsLost() );

	for ( TStateMap::const_iterator it = m_spawnedStates.Begin();
		it != m_spawnedStates.End(); ++it )
	{
		delete it->m_second;
	}

	if ( m_thread.IsValid() )
	{
		m_thread->ForceKill();
		m_thread = NULL;
	}
}

//---------------------------------------------------------------------------

void CScriptableStateMachine::DumpStatesForGC( IFile& file )
{
	// this code is temporary till we remove the GC

	SActiveData* data = GetStateMachineData(); // do not created data if not there
	if ( nullptr != data )
	{
		for ( SActiveData::TStateMap::const_iterator it = data->m_spawnedStates.Begin();
			it != data->m_spawnedStates.End(); ++it )
		{
			THandle< CScriptableState > stateHandle( it->m_second );
			file << stateHandle;
		}

		THandle< CScriptableState > activeState( data->m_currentState );
		file << activeState;
	}
}

void CScriptableStateMachine::KillThread()
{
	SActiveData* data = GetStateMachineData(); // do not created data if not there

	if ( nullptr != data && data->m_thread.IsValid() )
	{
		data->m_thread->ForceKill();
		data->m_thread = NULL;
	}
}

void CScriptableStateMachine::RestoreState( CScriptableState* state, Bool active )
{
	ASSERT( state->GetStateMachine() == nullptr || state->GetStateMachine() == GetStateMachineOwner() );
	state->BindStateMachine( GetStateMachineOwner() );

	SActiveData* data = GetStateMachineData( true ); // create data if missing

	// insert state into the state map (name->state)
	if ( data->m_spawnedStates.FindPtr( state->GetStateName() ) == nullptr )
	{
		data->m_spawnedStates.Insert( state->GetStateName(), state );
	}

	// activate the inserted state
	if ( active )
	{
		ChangeState( state->GetStateName(), NULL );
	}
}

void CScriptableStateMachine::GetStates( TDynArray< CScriptableState* >& states ) const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there

	if ( nullptr != data )
	{
		for ( SActiveData::TStateMap::const_iterator i = data->m_spawnedStates.Begin(); 
			i != data->m_spawnedStates.End(); ++i )
		{
			CScriptableState* state = i->m_second;
			states.PushBack( state );
		}
	}
}

CScriptableState* CScriptableStateMachine::GetCurrentState() const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there
	return (data != nullptr) ? data->m_currentState : NULL;
}

const CName CScriptableStateMachine::GetCurrentStateName() const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there
	return ( (data != nullptr) && (data->m_currentState != nullptr) ) 
		? data->m_currentState->GetStateName() 
		: CName::NONE;
}

const CScriptableStateMachine::EStateTransitionType CScriptableStateMachine::GetCurrentTransitionType() const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there
	if ( nullptr != data )
	{
		return data->m_currentTransitionType;
	}

	return EStateTransitionType_None;
}

const CFunction* CScriptableStateMachine::GetTopLevelFunction() const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there

	// Get top function
	if ( data && data->m_thread.IsValid() && !data->m_thread->IsKilled() )
	{
		const CScriptThread::TFrameStack& frames = data->m_thread->GetFrames();
		if ( !frames.Empty() )
		{
			CScriptStackFrame* topFrame = frames[ frames.Size()-1 ];
			if ( topFrame )
			{
				return topFrame->m_function;
			}
		}
	}

	// No top function
	return NULL;
}

Bool CScriptableStateMachine::ChangeStateCleanupTest( const CScriptStackFrame* stack )
{
	if ( nullptr == stack )
	{
		return true;
	}

	const CScriptStackFrame* frame = stack;
	do
	{
		if ( frame->m_function->IsCleanup() )
		{
			// If is in cleanup function of the same object fail the test
			const CScriptableState* parentState = Cast< CScriptableState >( frame->GetContext() );
			if ( parentState && parentState->GetStateMachine() == GetStateMachineOwner() )
			{
				return false;
			}
			else
			{
				return true;
			}
		};

		frame = frame->m_parent;
	}
	while ( frame );

	// Not in cleanup function
	return true;
}

Bool CScriptableStateMachine::ChangeState( const CName stateName, CScriptStackFrame* stack )
{
	SCRIPT_LOG_CONDITION( stack, *stack, TXT( "Changing state to %" ) RED_PRIWs TXT( " in %" ) RED_PRIWs, 
		stateName.AsChar(), 
		GetStateMachineOwner()->GetFriendlyName().AsChar() );

	SActiveData* data = GetStateMachineData( true ); // create if required

	// If machine blocked state change not allowed (this can be set only from C++)
	if ( data->m_stateMachineFlags & SMF_MachineBlocked )
	{
		return false;
	}

	// Entry function is locked, state change and entry function change not allowed
	if ( data->m_stateMachineFlags & SMF_EntryFunctionLocked )
	{
		return false;
	}

	// Changing state (or entry function) from cleanup function not allowed
	ASSERT( ChangeStateCleanupTest( stack ) );

	// Perform cleanup
	if ( data->m_cleanupFunctionName != CName::NONE )
	{
		if ( data->m_thread.IsValid() && data->m_currentState )
		{
			CScriptStackFrame* topFrame = data->m_thread->GetTopFrame();
			if ( topFrame )
			{
				// Call only if change entry function request didn't came from current entry function
				const CFunction* currentEntryFunction = topFrame->GetEntryFunction();
				const CFunction* callingEntryFunction = stack ? stack->GetEntryFunction() : NULL;

				if ( currentEntryFunction && currentEntryFunction != callingEntryFunction )
				{
					CallFunction( data->m_currentState, data->m_cleanupFunctionName );
				}
			}
		}

		// Reset cleanup function
		data->m_cleanupFunctionName = CName::NONE;
	}

	// We cannot change to the state state we are in
	if ( stateName == GetCurrentStateName() )
	{
		// Kill current thread
		KillThread();
		return true;
	}

	// We are changing the state right now
	if ( data->m_stateMachineFlags & SMF_ChangingState )
	{
		SCRIPT_LOG_CONDITION( stack, *stack, TXT( "Recursive ChangeState for '%" ) RED_PRIWs TXT( "' with state '%" ) RED_PRIWs TXT( "'" ), 
			GetStateMachineOwner()->GetClass()->GetName().AsChar(), 
			stateName.AsChar() );
		return false;
	}

	// Mark that we are inside state change process
	data->m_stateMachineFlags |= SMF_ChangingState;

	// Check if we can leave current state
	if ( data->m_currentState )
	{
		if ( !data->m_currentState->OnCanLeaveState( stateName ) )
		{
			data->m_stateMachineFlags &= ~SMF_ChangingState;
			return false;
		}
	}

	// Search for state
	CScriptableState* state = NULL;
	if ( !data->m_spawnedStates.Find( stateName, state ) )
	{
		// Find state class
		const CClass* stateClass = GetStateMachineOwner()->GetClass()->FindStateClass( stateName );
		if ( !stateClass )
		{
			data->m_stateMachineFlags &= ~SMF_ChangingState;
			SCRIPT_LOG_CONDITION( stack, *stack, TXT( "Class '%" ) RED_PRIWs TXT( "' does not have state '%" ) RED_PRIWs TXT( "'. Not entering %" ) RED_PRIWs, 
				GetStateMachineOwner()->GetClass()->GetName().AsChar(), 
				stateName.AsChar(), 
				GetStateMachineOwner()->GetFriendlyName().AsChar() );
			return false;
		}

		// Create state object, note that states are no longer CObjects
		state = stateClass->CreateObject< CScriptableState >();
		if ( state == nullptr )
		{
			data->m_stateMachineFlags &= ~SMF_ChangingState;
			SCRIPT_LOG_CONDITION( stack, *stack, TXT( "Unable to spawn state '%" ) RED_PRIWs TXT( "' in %" ) RED_PRIWs, 
				stateName.AsChar(), 
				GetStateMachineOwner()->GetFriendlyName().AsChar() );
			return false;
		}

		// Bind state to this state machine
		state->BindStateMachine( GetStateMachineOwner() );

		// Add state to state list
		ASSERT( data->m_spawnedStates.FindPtr( stateName ) == nullptr );
		data->m_spawnedStates.Insert( stateName, state );
	}

	// Check if we can enter new state
	ASSERT( state );
	if ( !state->OnCanEnterState( stateName ) )
	{
		data->m_stateMachineFlags &= ~SMF_ChangingState;
		return false;
	}
	// Kill current thread
	if ( data->m_thread.IsValid() )
	{
		data->m_thread->ForceKill();
		data->m_thread = NULL;
	}

	CName previousStateName = CName::NONE;

	// Exit current state
	if ( data->m_currentState != nullptr )
	{
		data->m_currentState->OnLeaveState( stateName );
		previousStateName = data->m_currentState->GetStateName();
	}

	// OnEnterState is allowed to call ChangeState so reset flag
	ASSERT( state->GetStateMachine() == GetStateMachineOwner() );
	data->m_stateMachineFlags &= ~SMF_ChangingState;
	data->m_currentState = state;

	// Enter new state, may call ChangeState
	if ( data->m_currentState )
	{
		data->m_currentState->OnEnterState( previousStateName );
	}

	// State entered
	return true;
}

void CScriptableStateMachine::KillCurrentState()
{
	SActiveData* data = GetStateMachineData(); // do not created data if not there

	// Exit current state
	if ( data && data->m_currentState )
	{
		data->m_stateStack.ClearFast();

		if ( data->m_cleanupFunctionName != CName::NONE )
		{
			CallFunction( data->m_currentState, data->m_cleanupFunctionName );
		}

		data->m_currentState->OnLeaveState( CName::NONE );
		data->m_currentState = NULL;
	}
}

struct SScopedRecursionCounter
{
	Int32*	m_counter;

	SScopedRecursionCounter( Int32* counter )
		: m_counter( counter )
	{
		ASSERT( *m_counter >= 0 );
		(*m_counter)++;
	}

	~SScopedRecursionCounter()
	{
		ASSERT( *m_counter > 0 );
		(*m_counter)--;
	}
};

Bool CScriptableStateMachine::EnterState( const CName stateName, const CFunction* stateEntryFunction, CScriptStackFrame& stack )
{
	static Int32 stateEntryDepth = 0;
	SScopedRecursionCounter recursionCounter( &stateEntryDepth );

	// Failsafe
	if ( stateEntryDepth > 32 )
	{
		WARN_CORE( TXT("FATAL state entry recursion on function '%ls' in state '%ls' called from '%ls'."), stateEntryFunction->GetName().AsString().AsChar(), stateName.AsString().AsChar(), stack.m_function->GetName().AsString().AsChar() );
		return false;
	}

	// Change state to new state
	if ( !ChangeState( stateName, &stack ) )
	{
		// Not entered
		return false;
	}

	// Not activated
	SActiveData* data = GetStateMachineData(); // do not created data if not there
	if ( nullptr == data )
	{
		return false;
	}
	
	// Get the entered state 
	CScriptableState* enteredState = NULL;
	data->m_spawnedStates.Find( stateName, enteredState );
	ASSERT( enteredState );

	// Start executing state function
	if ( enteredState )
	{
		// Create thread
		CScriptThread* newThread = GScriptingSystem->CreateThread( enteredState, stateEntryFunction, stack );
		if ( newThread )
		{
			// Set this thread as a new thread for state machine
			data->m_thread = newThread;
			newThread->SetListener(this);

			// Failsafe
			if ( stateEntryDepth > 16 )
			{
				WARN_CORE( TXT("State entry recursion on function '%ls' in state '%ls' called from '%ls'."), 
					stateEntryFunction->GetName().AsChar(), 
					stateName.AsString().AsChar(), 
					stack.m_function->GetName().AsChar() );
			}
			else
			{
				// Advance it and if it prevails, keep it
				data->m_thread->Advance( 0.0f );
			}
		}
	}

#ifndef RED_FINAL_BUILD
	if( data->m_stateMachineFlags & SMF_DumpEntryFunctionCalls )
	{
		SCRIPT_LOG( stack, TXT( "'% " ) RED_PRIWs TXT( "' starting entry function '%" ) RED_PRIWs TXT( "', callstack below" ), 
			GetStateMachineOwner()->GetFriendlyName().AsChar(), 
			stateEntryFunction->GetName().AsChar() );
		SCRIPT_DUMP_STACK_TO_LOG( stack );
	}
#endif

	// State entered
	return true;
}

void CScriptableStateMachine::OnScriptThreadKilled( CScriptThread* thread, Bool )
{
	SActiveData* data = GetStateMachineData(); // do not created data if not there

	// it can be a different thread because derived classes can create threads and this will be called also
	if ( data && data->m_thread.Get() == thread)
	{
		data->m_thread = NULL;
	}
}

const CFunction* CScriptableStateMachine::FindStateFunction( IScriptable*& context, CName functionName ) const
{
	const SActiveData* data = GetStateMachineData(); // do not created data if not there

	// We have a valid state selected, find function there !
	if ( data && data->m_currentState )
	{
		IScriptable* stateContext = data->m_currentState;
		const CFunction* stateFunction = stateContext->FindFunction( stateContext, functionName );
		if ( stateFunction && ( stateFunction->IsEvent() || stateFunction->IsTimer() ) )
		{
			context = stateContext;
			return stateFunction;
		}
	}

	// Use state function found
	return nullptr;
}

void CScriptableStateMachine::BlockStateMachine( Bool block )
{
	SActiveData* data = GetStateMachineData( true ); // create

	if ( block )
	{
		data->m_stateMachineFlags |= SMF_MachineBlocked;
	}
	else
	{
		data->m_stateMachineFlags &= ~SMF_MachineBlocked;
	}
}

const CFunction* FindStateEntryFunction( const CClass* stateMachineClass, const CName functionName )
{
	ASSERT( stateMachineClass != nullptr );

	// Search in states
	const auto& stateClasses = stateMachineClass->GetStateClasses();
	for ( Uint32 i=0; i<stateClasses.Size(); i++ )
	{
		CClass* stateClass = stateClasses[i];
		if ( stateClass )
		{
			const CFunction* func = stateClass->FindFunction( functionName );
			if ( func && func->IsEntry() )
			{
				return func;
			}
		}
	}

	// Search in base class
	if ( stateMachineClass->HasBaseClass() )
	{
		return FindStateEntryFunction( stateMachineClass->GetBaseClass(), functionName );
	}

	// Not found
	return NULL;
}

const CFunction* CScriptableStateMachine::FindEntryFunction( const CName name )
{
	SActiveData* data = GetStateMachineData( true ); // create

	// Get from cache
	const CFunction* func = NULL;
	if ( data && data->m_entryFunctions.Find( name, func ) )
	{
		return func;
	}

	// Find the function, note: we are using the runtime class of state machine owner
	func = FindStateEntryFunction( GetStateMachineOwner()->GetClass(), name );
	if ( func )
	{
		// Remember
		ASSERT( func->IsEntry() );
		data->m_entryFunctions.Insert( name, func );
	}

	// Return function
	return func;
}

void CScriptableStateMachine::funcPushState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, stateName, CName::NONE );
	FINISH_PARAMETERS;

	PushState( stateName, &stack );

	RETURN_VOID();
}

void CScriptableStateMachine::PushState( CName stateName, CScriptStackFrame* stack )
{
	CScriptableState* destState = GetState( stateName );
	if ( !destState )
	{
		SCRIPT_ERROR( *stack, TXT( "Could not find state %" ) RED_PRIWs TXT( " in %" ) RED_PRIWs, 
			stateName.AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );
		return;
	}

	if ( IsInState( stateName ) )
	{
		SCRIPT_ERROR( *stack, TXT( "Could not push state %" ) RED_PRIWs TXT( " on itself in %" ) RED_PRIWs, 
			stateName.AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );
		return;
	}

	SActiveData* data = GetStateMachineData( true ); // create
	data->m_stateStack.PushBack( stateName );

	if ( !ChangeState( stateName, stack, EStateTransitionType_Push ) )
	{
		PopStateStack( stack );
	}
}

const CName CScriptableStateMachine::PopStateStack( CScriptStackFrame* stack )
{
	RED_UNUSED( stack );

	SActiveData* data = GetStateMachineData(); // do not create

	if ( !data || data->m_stateStack.Empty() )
	{
		SCRIPT_WARNING( *stack, TXT( "Attempted to pop state from an empty stack in %" ) RED_PRIWs, 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );

		return CName::NONE;
	}

	return data->m_stateStack.PopBack();
}

void CScriptableStateMachine::funcPopState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, popAll, false );
	FINISH_PARAMETERS;

	PopState( popAll, false, &stack );

	RETURN_VOID();
}

void CScriptableStateMachine::PopState( Bool popAll, Bool eventsOnly, CScriptStackFrame* stack )
{
	SActiveData* data = GetStateMachineData(); // do not create

	if ( !data )
	{
		SCRIPT_WARNING( *stack, TXT( "Attempted to pop state in %" ) RED_PRIWs TXT( " while not being in any state" ), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );
		return;
	}

	// Get the auto state name (from state machine class)
	const CName autoStateName = GetStateMachineOwner()->GetClass()->GetAutoStateName();

	// Do nothing if we're in the only auto state
	SActiveData::TStateStack& stateStack = data->m_stateStack;
	if ( stateStack.Size() == 1 && stateStack[0] == autoStateName )
	{
		return;
	}

	// Pop all states except for auto state (if found on the bottom of the stack)
	CName oldStateName = CName::NONE;
	while ( !stateStack.Empty() && !( stateStack.Size() == 1 && stateStack[0] == autoStateName ) )
	{
		oldStateName = PopStateStack( stack );
		CScriptableState* oldState = GetState( oldStateName );

		const CName newStateName = stateStack.Empty() ? CName::NONE : stateStack.Back();

		// Notify old state about being popped
		oldState->CallLeaveTransitionEvent( EStateTransitionType_Pop, newStateName );

		// Stop after one step if not asked to pop all states
		if ( !popAll )
		{
			break;
		}
	}

	// Perform final change to target state
	EStateTransitionType autoTransitionType;
	if ( stateStack.Empty() )
	{
		stateStack.PushBack( autoStateName );
		autoTransitionType = EStateTransitionType_Push;
		oldStateName = CName::NONE;
	}
	else
	{
		autoTransitionType = EStateTransitionType_Pop;
	}

	if ( eventsOnly )
	{
		CScriptableState* newState = GetState( stateStack.Back() );
		newState->CallEnterTransitionEvent( autoTransitionType, oldStateName );
	}
	else
	{
		ChangeState( stateStack.Back(), stack, autoTransitionType );
	}
}

void CScriptableStateMachine::funcGotoState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, newState, CName::NONE );
	GET_PARAMETER( Bool, keepStack, false );
	GET_PARAMETER( Bool, forceEvents, false );
	FINISH_PARAMETERS;

	GotoState( newState, keepStack, forceEvents, &stack );

	RETURN_VOID();
}

void CScriptableStateMachine::GotoState( CName newState, Bool keepStack, Bool forceEvents, CScriptStackFrame* stack )
{
	if ( newState == CName::NONE )
	{
		const CName autoStateName = GetStateMachineOwner()->GetClass()->GetAutoStateName();
		newState = autoStateName;
	}
		
	CScriptableState* destState = GetState( newState );
	if ( !destState )
	{
		SCRIPT_WARNING( *stack, TXT( "Could not find state %" ) RED_PRIWs TXT( " in %" ) RED_PRIWs, 
			newState.AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );

		return;
	}

	SActiveData* data = GetStateMachineData( true ); // create
	if ( data->m_stateStack.Empty() )
	{
		ChangeState( newState, stack, EStateTransitionType_Direct );
		data->m_stateStack.PushBack( newState );
		return;
	}
		
	// No keeping stack = state forcing
	if ( !keepStack )
	{
		PopState( true, true, stack );
		
		data->m_stateStack.PushBack( newState );
		if ( !ChangeState( newState, stack, EStateTransitionType_Direct ) )
		{
			const CName autoStateName = GetStateMachineOwner()->GetClass()->GetAutoStateName();
			if( newState != autoStateName )
			{
				GotoStateAuto( stack );
			}
		}
		return;
	}

	// self transitioning
	const CName currState = GetCurrentStateName();
	if ( newState == currState )
	{
		// do nothing
		if ( forceEvents )
		{
			CScriptableState* state = GetState( currState );
			CallFunction( state, CNAME( EndState ), newState );
			CallFunction( state, CNAME( BeginState ), newState );
		}
			
		SCRIPT_WARNING( *stack, TXT( "Changing to the same state %" ) RED_PRIWs TXT( " in %" ) RED_PRIWs, 
			newState.AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );

		return;
	}
	else if ( IsInState( newState ) )
	{
		SCRIPT_WARNING( *stack, TXT( "Trying to transition into state (%" ) RED_PRIWs TXT( ") that is already on the stack in %") RED_PRIWs, 
			newState.AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );

		return;
	}

	if ( ChangeState( newState, stack, EStateTransitionType_Direct ) )
	{
		// Switch states at top of stack
		data->m_stateStack.Remove( currState );
		data->m_stateStack.PushBack( newState );
	}
}

Bool CScriptableStateMachine::IsInState( CName stateName )
{
	const SActiveData* data = GetStateMachineData(); // do not create
	return data && data->m_stateStack.Exist( stateName );
}

void CScriptableStateMachine::funcIsInState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, stateName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsInState( stateName ) );
}

Bool CScriptableStateMachine::ChangeState( const CName newState, CScriptStackFrame* stack, EStateTransitionType transition )
{
	SActiveData* data = GetStateMachineData( true ); // create if missing
	data->m_currentTransitionType = transition;

	return ChangeState( newState, stack );
}

void CScriptableStateMachine::funcGotoStateAuto( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	GotoStateAuto( &stack );

	RETURN_VOID();
}

void CScriptableStateMachine::GotoStateAuto( CScriptStackFrame* stack )
{
	const CName autoStateName = GetStateMachineOwner()->GetClass()->GetAutoStateName();
	if ( !autoStateName.Empty() )
	{
		// Pop all

		PopState( true, false, stack );

		// Still not in auto state? - force it

		if ( GetCurrentStateName() != autoStateName )
		{
			GotoState( autoStateName, false, false, stack );
		}
	}
}

void CScriptableStateMachine::funcLogStates( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	LogStates();

	RETURN_VOID();
}

void CScriptableStateMachine::LogStates( const String& context )
{
	RED_UNUSED( context );

	const SActiveData* data = GetStateMachineData(); // do not create
	if ( nullptr != data )
	{
		const SActiveData::TStateStack& stateStack = data->m_stateStack;

		RED_LOG( RED_LOG_CHANNEL( Script ), TXT( "STATE STACK (size = %d%s) for %s" ), 
			stateStack.Size(), 
			context.Empty() ? TXT( "" ) : (TXT( "; context = " ) + context ).AsChar(), 
			GetStateMachineOwner()->GetFriendlyName().AsChar() );

		for ( Uint32 i = 0; i < stateStack.Size(); i++ )
		{
			RED_LOG( RED_LOG_CHANNEL( Script ), TXT( "State [%d]: %s" ), i, 
				stateStack[ i ].AsString().AsChar() );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// Exports

void CScriptableStateMachine::funcGetCurrentState( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetCurrentState() );
}

void CScriptableStateMachine::funcGetCurrentStateName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( GetCurrentStateName() );
}

void CScriptableStateMachine::funcGetState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, stateName, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetState( stateName ) );
}

CScriptableState* CScriptableStateMachine::GetState( CName stateName )
{
	if ( stateName == CName::NONE )
	{
		const CName autoStateName = GetStateMachineOwner()->GetClass()->GetAutoStateName();
		stateName = autoStateName;
	}

	// Search for state
	CScriptableState* state = NULL;
	SActiveData* data = GetStateMachineData( true ); // create
	data->m_spawnedStates.Find( stateName, state );

	if ( !state )
	{
		// Find state class
		const CClass* stateClass = GetStateMachineOwner()->GetClass()->FindStateClass( stateName );
		if ( !stateClass )
		{
			data->m_stateMachineFlags &= ~SMF_ChangingState;
			RED_LOG_WARNING( RED_LOG_CHANNEL( StateMachine ), TXT( "Class '%" ) RED_PRIWs TXT( "' does not have state '%" ) RED_PRIWs TXT( "'. Not entering %" ) RED_PRIWs, 
				GetStateMachineOwner()->GetClass()->GetName().AsChar(), 
				stateName.AsChar(), 
				GetStateMachineOwner()->GetFriendlyName().AsChar() );
			return state;
		}

		// Spawn state
		state = stateClass->CreateObject< CScriptableState >();
		if ( !state )
		{
			data->m_stateMachineFlags &= ~SMF_ChangingState;
			RED_LOG_WARNING( RED_LOG_CHANNEL( StateMachine ), TXT( "Unable to spawn state '%" ) RED_PRIWs TXT( "' in %" ) RED_PRIWs, 
				stateName.AsChar(), 
				GetStateMachineOwner()->GetFriendlyName().AsChar() );
			return state;
		}

		// Bind state to this state machine
		state->BindStateMachine( GetStateMachineOwner() );

		// Add state to state list
		ASSERT( data->m_spawnedStates.FindPtr( stateName ) == nullptr );
		data->m_spawnedStates.Insert( stateName, state );
	}

	return state;
}

void CScriptableStateMachine::funcChangeState( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, stateName, CName::NONE );
	FINISH_PARAMETERS;

	// Change state
	const Bool state = ChangeState( stateName, &stack );
	RETURN_BOOL( state );
}

void CScriptableStateMachine::funcStop( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	// Kill thread
	SActiveData* data = GetStateMachineData(); // do not create
	if ( data && data->m_thread.IsValid() )
	{
		data->m_thread->ForceKill();
		data->m_thread = NULL;
	}

	RETURN_VOID();
}

void CScriptableStateMachine::funcLockEntryFunction( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	SActiveData* data = GetStateMachineData( true ); // create
	if ( flag )
	{
		data->m_stateMachineFlags |= SMF_EntryFunctionLocked;
	}
	else
	{
		data->m_stateMachineFlags &= ~SMF_EntryFunctionLocked;
	}

	RETURN_VOID();
}

void CScriptableStateMachine::funcSetCleanupFunction( CScriptStackFrame& stack, void* result )
{
	ASSERT( stack.GetContext()->IsA< CScriptableState >() );

	GET_PARAMETER( CName, funcName, CName::NONE );
	FINISH_PARAMETERS;

#ifndef NO_ASSERTS
	const CFunction* func = NULL;
	IScriptable* context = stack.GetContext();
	ASSERT( ::FindFunction( context, funcName, func ) );
	ASSERT( func->IsCleanup() );
	ASSERT( stack.GetEntryFunction() );
#endif

	SActiveData* data = GetStateMachineData( true ); // create
	data->m_cleanupFunctionName = funcName;

	RETURN_VOID();
}

void CScriptableStateMachine::funcClearCleanupFunction( CScriptStackFrame& stack, void* result )
{
	ASSERT( stack.GetContext()->IsA< CScriptableState >() );

	FINISH_PARAMETERS;

	SActiveData* data = GetStateMachineData( true ); // create
	data->m_cleanupFunctionName = CName::NONE;

	RETURN_VOID();
}

void CScriptableStateMachine::funcDebugDumpEntryFunctionCalls( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;

	SActiveData* data = GetStateMachineData( true ); // create
	if ( flag )
	{
		data->m_stateMachineFlags |= SMF_DumpEntryFunctionCalls;
	}
	else
	{
		data->m_stateMachineFlags &= ~SMF_DumpEntryFunctionCalls;
	}

	RETURN_VOID();
}
