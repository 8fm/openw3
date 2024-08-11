/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

ActorActionRaiseEvent::ActorActionRaiseEvent( CActor* actor )
	: ActorAction( actor, ActorAction_RaiseEvent )
	, m_inProgress( false )
{
}

ActorActionRaiseEvent::~ActorActionRaiseEvent()
{
}

Bool ActorActionRaiseEvent::Start( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout, Bool isForce /* = false  */)
{
	// Get animated component
	CAnimatedComponent* animated = m_actor->GetRootAnimatedComponent();
	if( !animated )
	{
		SET_ERROR_STATE( m_actor, TXT("ActorActionRiseEvent: no animated component") );
		return false;
	}

	// Get behavior graph instance
	if ( !animated->GetBehaviorStack() )
	{
		SET_ERROR_STATE( m_actor, TXT("ActorActionRiseEvent: no behavior stack") );
		return false;
	}

	Bool ret = isForce ? animated->GetBehaviorStack()->GenerateBehaviorForceEvent( eventName ) : animated->GetBehaviorStack()->GenerateBehaviorEvent( eventName );
	if ( ! ret )
	{
		// Event wasn't processed
		return false;
	}

	m_notificationName = notificationName;
	m_notificationType = notificationType;
	m_timer = timeout;
	m_inProgress = true;

	// Done
	return true;
}

Bool ActorActionRaiseEvent::Update( Float timeDelta )
{
	m_timer -= timeDelta;

	CAnimatedComponent* ac = m_actor->GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		if ( ( m_notificationType == BGNT_Activation && ac->GetBehaviorStack()->ActivationNotificationReceived( m_notificationName ) ) ||
			 ( m_notificationType == BGNT_Deactivation && ac->GetBehaviorStack()->DeactivationNotificationReceived( m_notificationName ) ) )
		{
			m_inProgress = false;
		}
	}

	if ( m_timer < 0.f )
	{
		m_inProgress = false;
		m_actor->ActionEnded( GetType(), ActorActionResult_Failed );
	}
	else if ( !m_inProgress )
	{
		m_actor->ActionEnded( GetType(), ActorActionResult_Succeeded );
	}

	return m_inProgress;
}

void ActorActionRaiseEvent::Stop()
{
	m_inProgress = false;
}

Bool CActor::ActionRaiseEvent( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel other action
	ActionCancelAll();

	// Start action
	if ( !m_actionRaiseEvent.Start( eventName, notificationName, notificationType, timeout ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionRaiseEvent;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_RaiseEvent;

	OnActionStarted( ActorAction_RaiseEvent );

	return true;
}

Bool CActor::ActionRaiseForceEvent( const CName& eventName, const CName& notificationName, EBehaviorGraphNotificationType notificationType, Float timeout )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel other action
	ActionCancelAll();

	// Start action
	if ( !m_actionRaiseEvent.Start( eventName, notificationName, notificationType, timeout, true ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionRaiseEvent;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_RaiseEvent;

	OnActionStarted( ActorAction_RaiseEvent );

	return true;
}

extern Bool GLatentFunctionStart;

void CActor::funcActionRaiseEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER_OPT( CName, notificationName, CName::NONE );
	GET_PARAMETER_OPT( EBehaviorGraphNotificationType, notificationType, BGNT_Deactivation );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	if ( notificationName == CName::NONE )
	{
		String defNotName = eventName.AsString() + TXT("_f");
		notificationName = CName( defNotName );
	}

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool result = ActionRaiseEvent( eventName, notificationName, notificationType, timeout );
		if ( !result )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionRaiseForceEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER_OPT( CName, notificationName, CName::NONE );
	GET_PARAMETER_OPT( EBehaviorGraphNotificationType, notificationType, BGNT_Deactivation );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	if ( notificationName == CName::NONE )
	{
		String defNotName = eventName.AsString() + TXT("_f");
		notificationName = CName( defNotName );
	}

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool result = ActionRaiseForceEvent( eventName, notificationName, notificationType, timeout );
		if ( !result )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionRaiseEventAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionRaiseEvent( eventName, CName::NONE, BGNT_Deactivation, 10.f );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionRaiseForceEventAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionRaiseForceEvent( eventName, CName::NONE, BGNT_Deactivation, 10.f );
	RETURN_BOOL( actionResult );
}
