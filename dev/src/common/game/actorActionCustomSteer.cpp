/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionCustomSteer.h"

#include "../game/movementTargeter.h"
#include "../game/moveSteeringLocomotionSegment.h"
#include "../game/movableRepresentationPathAgent.h"


ActorActionCustomSteer::ActorActionCustomSteer( CActor* actor, EActorActionType type /*= ActorActionCustomSteer*/ )
	: ActorAction( actor, type )
	, m_isMoving( false )
	, m_targeter( NULL )
	, m_absSpeed( 0.0f )
	, m_moveType( MT_AbsSpeed )
{
}

ActorActionCustomSteer::~ActorActionCustomSteer()
{
}

void ActorActionCustomSteer::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

Bool ActorActionCustomSteer::IsCancelingAllowed() const
{
	CMovingAgentComponent* ac =  m_actor->GetMovingAgentComponent();
	return ac->CanCancelMovement();
}

Bool ActorActionCustomSteer::StartMove( IMovementTargeter* targeter, EMoveType moveType, Float absSpeed, EMoveFailureAction failureAction )
{
	PC_SCOPE( AI_ActorActionCustomSteer);

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	if( targeter )
	{
		m_targeter = targeter;
	}
	else
	{
		return false;
	}

	// if the target's got a CMovingAgentComponent, cash its instance for future reference
	m_moveType = moveType;
	m_absSpeed = absSpeed;
	SetMoveSpeed( *mac, moveType, absSpeed );

	mac->AttachLocomotionController( *this );

	return true;
}

void ActorActionCustomSteer::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( mac )
	{
		mac->DetachLocomotionController( *this );
	}

	Cleanup();
}

Bool ActorActionCustomSteer::Update( Float timeDelta )
{
	PC_SCOPE( AI_ActorActionCustomSteer );

	if( m_targeter )
	{
		//SetMoveSpeed( *m_actor->GetMovingAgentComponent(), m_moveType, m_absSpeed );
		if( m_targeter->IsFinished() )
		{
			m_actor->ActionEnded( ActorAction_CustomSteer, ActorActionResult_Succeeded );
			CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
			if ( mac )
			{
				mac->DetachLocomotionController( *this );
			}
			Cleanup();
			return false;
		}
	}
	else
	{
		m_actor->ActionEnded( ActorAction_CustomSteer, ActorActionResult_Failed );
		CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
		if ( mac )
		{
			mac->DetachLocomotionController( *this );
		}
		Cleanup();
		return false;
	}

	return true;
}

void ActorActionCustomSteer::Cleanup()
{
	m_isMoving = false;
	m_targeter = NULL;
	m_absSpeed = 0.0f;
	m_moveType = MT_AbsSpeed;
}

void ActorActionCustomSteer::OnSegmentFinished( EMoveStatus status )
{
	if ( CanUseLocomotion() )
	{
		if ( m_targeter && !m_targeter->IsFinished() )
		{
			CMoveLSSteering& steeringSegment = locomotion().PushSteeringSegment();
			steeringSegment.AddTargeter( m_targeter );
		}
		
	}
}
void ActorActionCustomSteer::OnControllerAttached()
{
	CMoveLSSteering& steeringSegment = locomotion().PushSteeringSegment();
	steeringSegment.SetUseFootstepHandler( false );
	steeringSegment.AddTargeter( m_targeter );
}
void ActorActionCustomSteer::OnControllerDetached()
{

}

// -------------------------------------------------------------------------
// Debugging
// -------------------------------------------------------------------------

void ActorActionCustomSteer::GenerateDebugFragments( CRenderFrame* frame )
{
}

void ActorActionCustomSteer::DebugDraw( IDebugFrame& debugFrame ) const
{
}

String ActorActionCustomSteer::GetDescription() const
{
	return String::Printf( TXT( "Custom steering" ) );
}

///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionCustomSteer( IMovementTargeter* targeter, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionCustomSteer.StartMove( targeter, moveType, absSpeed, failureAction ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionCustomSteer;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_CustomSteer;

	OnActionStarted( ActorAction_CustomSteer);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//extern Bool GLatentFunctionStart;
//
//void CActor::funcActionMoveToDynamicNode( CScriptStackFrame& stack, void* result )
//{
//	GET_PARAMETER( THandle<CNode>, target, NULL );
//	GET_PARAMETER( Int32, moveType, MT_Walk );
//	GET_PARAMETER( Float, absSpeed, 1.0f );
//	GET_PARAMETER( Float, range, 0.0f );
//	GET_PARAMETER_OPT( Bool, keepDistance, false );
//	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
//	FINISH_PARAMETERS;
//
//	// Only in threaded code
//	ASSERT( stack.m_thread );
//
//	// Start test
//	ACTION_START_TEST;
//
//	CNode *pTarget = target.Get();
//
//	// Starting !
//	if ( GLatentFunctionStart )
//	{
//		if ( pTarget == NULL )
//		{
//			// Already failed
//			RETURN_BOOL( false );
//			return;
//		}
//
//		// Start action
//		Bool actionResult = ActionMoveToDynamicNode( pTarget, (EMoveType)moveType, absSpeed, range, keepDistance, failureAction );
//		if ( !actionResult )
//		{
//			// Already failed
//			RETURN_BOOL( false );
//			return;
//		}
//
//		// Bump the latent stuff
//		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();
//
//		// Yield the thread to pause execution
//		stack.m_thread->ForceYield();
//		return;
//	}
//
//	// Action still in progress
//	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
//	{
//		if ( m_latentActionResult == ActorActionResult_InProgress )
//		{
//			// Yield the thread to pause execution
//			stack.m_thread->ForceYield();
//			return;
//		}
//	}
//
//	// Get the state
//	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
//	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );
//
//	// Reset
//	if ( ! canceled )
//	{
//		m_latentActionResult = ActorActionResult_Failed;
//		m_latentActionType = ActorAction_None;
//		m_latentActionIndex = 0;
//	}
//
//	// Return state
//	RETURN_BOOL( succeeded );
//}
//
//void CActor::funcActionMoveToDynamicNodeAsync( CScriptStackFrame& stack, void* result )
//{
//	GET_PARAMETER( THandle<CNode>, target, NULL );
//	GET_PARAMETER( Int32, moveType, MT_Walk );
//	GET_PARAMETER( Float, absSpeed, 1.0f );
//	GET_PARAMETER( Float, range, 0.0f );
//	GET_PARAMETER_OPT( Bool, keepDistance, false );
//	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
//	FINISH_PARAMETERS;
//
//	// Start test
//	ACTION_START_TEST;
//
//	CNode *pTarget = target.Get();
//
//	if ( pTarget == NULL )
//	{
//		RETURN_BOOL( false );
//		return;
//	}
//
//	Bool actionResult = ActionMoveToDynamicNode( pTarget, (EMoveType)moveType, absSpeed, range, keepDistance, failureAction );
//	RETURN_BOOL( actionResult );
//}

///////////////////////////////////////////////////////////////////////////////
