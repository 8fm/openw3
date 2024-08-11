/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/behaviorGraphAnimationSlotNode.h"
#include "../../common/engine/behaviorGraphStack.h"

ActorActionPlayAnimation::ActorActionPlayAnimation( CActor* actor )
	: ActorAction( actor, ActorAction_Animation )
	, m_animationInProgress( false )
{
}

ActorActionPlayAnimation::~ActorActionPlayAnimation()
{
}

Bool ActorActionPlayAnimation::Start( const CName& slotName, const CName& animationName, Float blendIn, Float blendOut, Bool continuePlaying )
{
	// Get animated component
	CAnimatedComponent* animated = m_actor->GetRootAnimatedComponent();
	if( !animated )
	{
		SET_ERROR_STATE( m_actor, TXT("ActionPlayAnimations: no animated component") );
		return false;
	}

	// Play animation on slot
	if ( !animated->GetBehaviorStack() )
	{
		SET_ERROR_STATE( m_actor, TXT("ActionPlayAnimations: no behavior stack") );
		return false;
	}

	// Setup slot
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendIn = blendIn;
	slotSetup.m_blendOut = blendOut;
	slotSetup.m_listener = this;

	// Play animation
	if ( !animated->GetBehaviorStack()->PlaySlotAnimation( slotName, animationName, &slotSetup ) )
	{
		SET_ERROR_STATE( m_actor, String::Printf( TXT("ActionPlayAnimations: unable to play animation %s"), animationName.AsString().AsChar() ) );
		return false;
	}

	// Setup state
	m_animationName = animationName;
	m_animationSlot = slotName;
	m_animationInProgress = true;
	m_continuePlaying = continuePlaying;

	// We are animating
	return true;
}

Bool ActorActionPlayAnimation::Update( Float timeDelta )
{
	return m_animationInProgress;
}

void ActorActionPlayAnimation::Stop()
{
	// Get animated component
	CAnimatedComponent* animated = m_actor->GetRootAnimatedComponent();
	if( !animated )
	{
		SET_ERROR_STATE( m_actor, TXT("ActionPlayAnimations: no animated component") );
		return;
	}

	// Stop animation
	if ( animated->GetBehaviorStack() )
	{
		animated->GetBehaviorStack()->StopSlotAnimation( m_animationSlot );
	}
}

void ActorActionPlayAnimation::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
{
	m_animationInProgress = false;

	// Set waiting time for sync, 0.5s
	if ( m_continuePlaying )
	{
		//sender->WaitForNextAnimation( 0.5f );
	}
	
	// We've reached our target
	if( status == ISlotAnimationListener::S_Finished || status == ISlotAnimationListener::S_BlendOutStarted )
	{
		m_actor->ActionEnded( GetType(), ActorActionResult_Succeeded );
	}
	else
	{
		m_actor->ActionEnded( GetType(), ActorActionResult_Failed );
	}
}

Bool CActor::ActionPlaySlotAnimation( const CName& slotName, const CName& animationName, 
									  Float blendIn /* = 0.2f */, Float blendOut /* = 0.2f  */,
									  Bool continuePlaying /* = false */ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel other action
	ActionCancelAll();

	// Start action
	if ( !m_actionAnimation.Start( slotName, animationName, blendIn, blendOut, continuePlaying ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionAnimation;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Animation;

	OnActionStarted( ActorAction_Animation );

	return true;
}

void CActor::funcActionPlaySlotAnimationAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( Float, blendIn, 0.2f );
	GET_PARAMETER_OPT( Float, blendOut, 0.2f );
	GET_PARAMETER_OPT( Bool, continuePlaying, false );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	Bool actionResult = ActionPlaySlotAnimation( slotName, animationName, blendIn, blendOut, continuePlaying );

	RETURN_BOOL( actionResult );
}

extern Bool GLatentFunctionStart;

void CActor::funcActionPlaySlotAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER_OPT( Float, blendIn, 0.2f );
	GET_PARAMETER_OPT( Float, blendOut, 0.2f );
	GET_PARAMETER_OPT( Bool, continuePlaying, false );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionPlaySlotAnimation( slotName, animationName, blendIn, blendOut, continuePlaying );
		if ( !actionResult )
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
