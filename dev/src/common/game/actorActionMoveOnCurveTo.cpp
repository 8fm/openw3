// Todo: currently based on sliding, might be changed to accommodate another type of motion

#include "build.h"
#include "actorActionMoveOnCurveTo.h"

#include "movementTargeter.h"
#include "actionAreaComponent.h"
#include "moveCurveLocomotionSegment.h"


ActorActionMoveOnCurveTo::ActorActionMoveOnCurveTo( CActor* actor, EActorActionType type )
	: ActorAction( actor, type )
	, m_status( ActorActionResult_Succeeded )
{
}

ActorActionMoveOnCurveTo::~ActorActionMoveOnCurveTo()
{
}

void ActorActionMoveOnCurveTo::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

Float ActorActionMoveOnCurveTo::GetMoveOnCurveToDuration( Float dist ) const
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return 0.0f;
	}

	Float speed = mac->GetAbsoluteMoveSpeed();
	if ( speed > 0 )
	{
		return dist / speed;
	}
	else
	{
		return FLT_MAX;
	}
}

Bool ActorActionMoveOnCurveTo::StartMoveOnCurveTo( const Vector& target, Float duration, Bool rightShift )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	Float dist = m_actor->GetWorldPosition().DistanceTo( target );
	if ( dist < 1e-1 )
	{
		return true;
	}

	// take control over the locomotion
	mac->AttachLocomotionController( *this );

	mac->SetUseExtractedMotion( true );

	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;
		locomotion().PushSegment( new CMoveLSCurve( target, duration, rightShift ) );
		return true;
	}
	else
	{
		return false;
	}
}

void ActorActionMoveOnCurveTo::ResetAgentMovementData()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}
	mac->SetMoveTarget( Vector::ZERO_3D_POINT );
	mac->SetMoveHeading( CEntity::HEADING_ANY );
	mac->ForceSetRelativeMoveSpeed( 0.f );
	mac->SetMoveRotation( 0.f );
	mac->SetMoveRotationSpeed( 0.f );

	mac->SetUseExtractedMotion( true );
};

void ActorActionMoveOnCurveTo::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}
	m_status = ActorActionResult_Failed;
	mac->DetachLocomotionController( *this );
	ResetAgentMovementData();
}

Bool ActorActionMoveOnCurveTo::Update( Float timeDelta )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac  || m_status == ActorActionResult_InProgress )
	{
		return true;
	}

	ResetAgentMovementData();
	m_actor->ActionEnded( ActorAction_Sliding, m_status );

	if ( CanUseLocomotion() )
	{
		mac->DetachLocomotionController( *this );
	}
	return false;
}

void ActorActionMoveOnCurveTo::OnSegmentFinished( EMoveStatus status )
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

void ActorActionMoveOnCurveTo::OnControllerAttached()
{
	// nothing to do here
}

void ActorActionMoveOnCurveTo::OnControllerDetached()
{
	if ( m_status == ActorActionResult_InProgress )
	{
		m_status = ActorActionResult_Failed;
	}
}

///////////////////////////////////////////////////////////////////////////////
Bool CActor::ActionMoveOnCurveTo( const Vector& target, Float duration, Bool rightShift  )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionMoveOnCurveTo.StartMoveOnCurveTo( target, duration, rightShift ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionMoveOnCurveTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
extern Bool GLatentFunctionStart;

void CActor::funcActionMoveOnCurveTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, duration, 1.0f );
	GET_PARAMETER( Bool, rightShift, false );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionMoveOnCurveTo( target, duration, rightShift );
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

void CActor::funcActionMoveOnCurveToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, duration, 1.0f );
	GET_PARAMETER( Bool, rightShift, false);
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionMoveOnCurveTo( target, duration, rightShift );
	RETURN_BOOL( actionResult );
}
