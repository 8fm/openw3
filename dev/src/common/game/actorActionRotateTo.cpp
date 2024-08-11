/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionRotateTo.h"

#include "moveSteeringLocomotionSegment.h"
#include "movementTargeter.h"

ActorActionRotateTo::ActorActionRotateTo( CActor* actor )
	: ActorAction( actor, ActorAction_Rotating )
	, m_status( ActorActionResult_Succeeded )
	, m_angleTolerance( 3.f )
	, m_targetYaw( 0.0f )
{
}

ActorActionRotateTo::~ActorActionRotateTo()
{
}

ActorActionRotateTo::EResult ActorActionRotateTo::StartRotateTo( const Vector& target, Float angleTolerance )
{
	const Vector dir		= target - m_actor->GetWorldPosition();
	const Float targetYaw	= dir.ToEulerAngles().Yaw;
	return StartOrienting( targetYaw, angleTolerance );
}
Bool ActorActionRotateTo::UpdateRotateTo( const Vector& target )
{
	Vector dir	= target - m_actor->GetWorldPosition();
	m_targetYaw = dir.ToEulerAngles().Yaw;
	Float angDist = EulerAngles::AngleDistance( m_actor->GetWorldYaw(), m_targetYaw );
	return MAbs( angDist ) > m_angleTolerance;
}
ActorActionRotateTo::EResult ActorActionRotateTo::StartOrienting( Float orientation, Float angleTolerance )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return R_Failed;
	}

	// fail the action immediately if there's an active rotation target in use
	if ( mac->IsRotationTargetEnabled() )
	{
		return R_Failed;
	}

	Float angDist = EulerAngles::AngleDistance( m_actor->GetWorldYaw(), orientation );
	if ( MAbs( angDist ) < angleTolerance )
	{
		return R_NoRotationNeeded;
	}
	m_targetYaw = orientation;
	m_angleTolerance = angleTolerance;

	// take control of the locomotion
	mac->AttachLocomotionController( *this );
	ASSERT( CanUseLocomotion() );
	if ( CanUseLocomotion() )
	{
		// create a new locomotion segment
		CMoveLSSteering& segment = locomotion().PushSteeringSegment();
		segment.AddTargeter( this );
	}

	m_status = ActorActionResult_InProgress;

	return R_Started;
}
Bool ActorActionRotateTo::UpdateOrienting( Float orientation )
{
	m_targetYaw = orientation;
	Float angDist = EulerAngles::AngleDistance( m_actor->GetWorldYaw(), orientation );
	if ( MAbs( angDist ) < m_angleTolerance )
	{
		return false;
	}

	return true;
}

void ActorActionRotateTo::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->DetachLocomotionController( *this );
	}

	m_status = ActorActionResult_Failed;
}

Bool ActorActionRotateTo::Update( Float timeDelta )
{
	if ( m_status != ActorActionResult_InProgress )
	{
		m_actor->ActionEnded( GetType(), m_status );
		return false;
	}
	else
	{
		return true;
	}
}

void ActorActionRotateTo::OnSegmentFinished( EMoveStatus status )
{
	if ( status == MS_Completed )
	{
		m_status = ActorActionResult_Succeeded;
	}
	else if ( status == MS_Failed )
	{
		m_status = ActorActionResult_Failed;
	}
}

void ActorActionRotateTo::OnControllerAttached()
{
	// noting to do here
}

void ActorActionRotateTo::OnControllerDetached()
{
	m_status = ActorActionResult_Failed;
}

void ActorActionRotateTo::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( false );
	goal.SetOrientationGoal( agent, m_targetYaw );
	goal.MatchMoveDirectionWithOrientation( true );
}


///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionRotateTo( const Vector& target, Bool endOnRotationFinished, Float angleTolerance )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	ActorActionRotateTo::EResult result = m_actionRotateTo.StartRotateTo( target, angleTolerance );

	if ( result == ActorActionRotateTo::R_Failed )
	{
		m_latentActionResult = ActorActionResult_Failed;
		return false;
	}
	if ( result == ActorActionRotateTo::R_Started || endOnRotationFinished == false ) // if endOnRotationFinished = false and no rotation needed, continue rotating
	{		
		// Start action
		m_action = &m_actionRotateTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Rotating;

		OnActionStarted( ActorAction_Rotating );
		return true;
	}
	else // result == ActorActionRotateTo::R_NoRotationNeeded
	{
		ASSERT( result == ActorActionRotateTo::R_NoRotationNeeded );
		m_latentActionResult = ActorActionResult_Succeeded;
		return true;
	}
}
Bool CActor::ActionRotateTo_Update( const Vector& target, Bool endOnRotationFinished )
{
	const Bool result = m_actionRotateTo.UpdateRotateTo( target );
	if ( endOnRotationFinished )
	{
		return result;
	}
	return true;
}
Bool CActor::ActionSetOrientation( Float orientation, Bool endOnRotationFinished, Float angleTolerance )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	ActorActionRotateTo::EResult result = m_actionRotateTo.StartOrienting( orientation, angleTolerance );

	if ( result == ActorActionRotateTo::R_Failed )
	{
		m_latentActionResult = ActorActionResult_Failed;
		return false;
	}
	if ( result == ActorActionRotateTo::R_Started || endOnRotationFinished == false )
	{		
		// Start action
		m_action = &m_actionRotateTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Rotating;

		OnActionStarted( ActorAction_Rotating );
		return true;
	}
	else // result == ActorActionRotateTo::R_NoRotationNeeded
	{
		ASSERT( result == ActorActionRotateTo::R_NoRotationNeeded );
		m_latentActionResult = ActorActionResult_Succeeded;
		return true;
	}
}
Bool CActor::ActionSetOrientation_Update( Float orientation, Bool endOnRotationFinished )
{
	const Bool result = m_actionRotateTo.UpdateOrienting( orientation );
	if ( endOnRotationFinished )
	{
		return result;
	}
	return true;
}
extern Bool GLatentFunctionStart;

void CActor::funcActionRotateTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionRotateTo( target );
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

void CActor::funcActionRotateToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionRotateTo( target );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionSetOrientation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, orientation, 0.0f );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionSetOrientation( orientation );
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
