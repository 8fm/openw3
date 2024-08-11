/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionMoveTo.h"

#include "../engine/pathlibWorld.h"
#include "../engine/tagManager.h"

#include "moveLocomotion.h"
#include "moveGlobalPathPlanner.h"
#include "movePathIterator.h"
#include "moveNavigationPath.h"
#include "moveSteeringLocomotionSegment.h"
#include "movableRepresentationPathAgent.h"

///////////////////////////////////////////////////////////////////////////////

#define MOVE_THRESHOLD										0.1f	// 10 cm

ActorActionMoveTo::ActorActionMoveTo( CActor* actor, EActorActionType type /*= ActorAction_Moving*/ )
	: ActorAction( actor, type )
	, m_activeSegment( nullptr )
	, m_isMoving( false )
	, m_targetWasUpdated( false )
	, m_pathfindingTolerance( 0.f )
{
}

ActorActionMoveTo::~ActorActionMoveTo()
{

}

void ActorActionMoveTo::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

const Vector& ActorActionMoveTo::GetTarget() const
{
	return m_targetPos;
}

Bool ActorActionMoveTo::IsCancelingAllowed() const
{
	CMovingAgentComponent* ac =  m_actor->GetMovingAgentComponent();
	return ac->CanCancelMovement();
}

Bool ActorActionMoveTo::StartMove( const CNode * target, Bool withHeading, EMoveType moveType, Float absSpeed, Float radius, Float tolerance, EMoveFailureAction failureAction, Uint16 pathfollowFlags )
{
	PC_SCOPE( AI_ActorActionMoveTo );

	return StartMove( target->GetWorldPosition(), withHeading ? target->GetWorldYaw() : CEntity::HEADING_ANY, moveType, absSpeed, radius, tolerance, failureAction, pathfollowFlags );
}

void ActorActionMoveTo::ChangeTarget( const Vector& target, EMoveType moveType, Float absSpeed, Float radius )
{
	m_targetPos			= target; 
	m_targetWasUpdated	= true;
	m_toleranceRadius	= radius;

	CMovingAgentComponent* mac = GetMAC();
	if ( !mac || mac->IsMotionEnabled() == false )
	{
		return;
	}
	

	SetMoveSpeed( *mac, moveType, absSpeed );
}


Bool ActorActionMoveTo::StartMove( const Vector& target, Float heading, EMoveType moveType, Float absSpeed, Float radius, Float tolerance, EMoveFailureAction failureAction, Uint16 pathfollowFlags )
{
	PC_SCOPE( AI_ActorActionMoveTo );

	CMovingAgentComponent* mac = GetMAC();
	if ( !mac || mac->IsMotionEnabled() == false )
	{
		// the component is turned off - therefore we can't use any move-related functionality
		return false;
	}

	m_targetPos = target;
	m_pathfindingTolerance = tolerance;

	SetMoveSpeed( *mac, moveType, absSpeed );

	m_moveFlags = pathfollowFlags;

	m_toleranceRadius = radius;
	m_isMoving = false;
	mac->AttachLocomotionController( *this );

	m_safetyDelay = GGame->GetEngineTime() + 2.f;
	m_pathPlotingDelay = EngineTime::ZERO;

	return true;
}

Bool ActorActionMoveTo::StartMoveAway( const CNode * position, Float distance, EMoveType moveType, Float absSpeed, Float radius, EMoveFailureAction failureAction )
{
	Vector dangerPos = position->GetWorldPosition();
	Vector escapeZone = dangerPos + ( m_actor->GetWorldPosition() - dangerPos ).Normalized3() * distance;
	static Float escapeZoneRadius = 4.0f;
	Vector awayPosition;

	CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	if ( pathlib )
	{
		if ( !pathlib->FindRandomPositionInRadius( PathLib::INVALID_AREA_ID, escapeZone.AsVector3(), escapeZoneRadius, radius, 0, awayPosition.AsVector3() ) )
		{
			return false;
		}
	}

	Float heading = 0.0f;
	StartMove( awayPosition, heading, moveType, absSpeed, 0.1f, 0.f, failureAction );

	return true;	
}

Bool ActorActionMoveTo::StartMoveAwayFromLine( const Vector& positionA, const Vector& positionB, Float distance, Bool makeMinimalMovement, 
											   EMoveType moveType, Float absSpeed, Float radius, EMoveFailureAction failureAction )
{
	Vector lineDir = ( positionB - positionA ).Normalized3();
	Vector perpToLine = Vector::Cross( lineDir, Vector::EZ );

	Float escapeDir = ( GEngine->GetRandomNumberGenerator().Get< Uint16 >( 100 ) < 50 ) ? -1.0f : 1.0f;
	Vector escapePos = m_actor->GetWorldPosition() + perpToLine * distance * escapeDir;
	static Float escapeZoneRadius = 1.0f;

	Vector awayPosition;

	CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	if ( pathlib )
	{
		if ( !pathlib->FindRandomPositionInRadius( PathLib::INVALID_AREA_ID, escapePos.AsVector3(), escapeZoneRadius, radius, 0, awayPosition.AsVector3() ) )
		{
			return false;
		}
	}
	

	Float heading = 0.0f;
	StartMove( awayPosition, heading, moveType, absSpeed, 0.1f, 0.f, failureAction );
 
	return true;	
}


void ActorActionMoveTo::Stop()
{
	m_isMoving = false;

	CMovingAgentComponent* mac = GetMAC();
	if ( mac )
	{
		mac->DetachLocomotionController( *this );
		CPathAgent* pathAgent = mac->GetPathAgent();
		if ( pathAgent )
		{
			pathAgent->StopMovement();
		}
	}

	Cleanup();
	
}

//! Stop during update (finish action, etc)
void ActorActionMoveTo::OnStopDuringMovementUpdate( EActorActionResult result )
{
	m_actor->ActionEnded( ActorAction_Moving, result );
}

Bool ActorActionMoveTo::Update( Float timeDelta )
{
	// check success (maybe we already reached target?)
	if ( !m_targetWasUpdated && m_activeSegment && m_activeSegment->IsDestinationReached() )
	{
		Stop();
		OnStopDuringMovementUpdate( ActorActionResult_Succeeded );
		Cleanup();
		return false;
	}

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	CPathAgent* pathAgent = mac->GetPathAgent();
	
	if ( pathAgent->UpdateMetalinkAwareness() )
	{
		return true;
	}

	// if not moving (we need new path) or if target updated (we need new path)
	if ( !m_isMoving || m_targetWasUpdated )
	{
		const Vector& agentPos = mac->GetAgentPosition();
		// success check
		Float distSq = (agentPos.AsVector2() - m_targetPos.AsVector2()).SquareMag();
		if ( distSq <= m_toleranceRadius*m_toleranceRadius && Abs(agentPos.Z - m_targetPos.Z) < 2.f )
		{
			Stop();
			OnStopDuringMovementUpdate( ActorActionResult_Succeeded );
			Cleanup();
			return false;
		}

		// if we have no way to plot path, fail (this shouldn't happen though)
		if ( m_targetWasUpdated && pathAgent->UpdatePathDestination( m_targetPos.AsVector3() ) )
		{
			m_targetWasUpdated = false;
			m_isMoving = true;
		}
		else
		{
			EngineTime engineTime = GGame->GetEngineTime();
			if ( engineTime > m_pathPlotingDelay )
			{
				m_targetWasUpdated = false;
				Bool pathfindingIsPending = false;
				// pathfinding
				switch ( pathAgent->PlotPath( m_targetPos.AsVector3(), m_pathfindingTolerance ) )
				{
				case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
				case PathLib::PATHRESULT_FAILED:
					// cannot get to target (pathfinding failed)
					m_isMoving = false;
					m_pathPlotingDelay = engineTime + 0.5f;
					break;
				case PathLib::PATHRESULT_SUCCESS:
					m_isMoving = true;
					pathAgent->SetupPathfollowing( m_toleranceRadius, CPathAgent::PATHFOLLOW_DEFAULT );
					break;
				case PathLib::PATHRESULT_PENDING:
					m_targetWasUpdated = true;
					pathfindingIsPending = true;
					break;
				default:
					ASSUME( false );
				}

				// we're still not moving? if we won't start - fail
				if ( !m_isMoving && !pathfindingIsPending && engineTime > m_safetyDelay )
				{
					Stop();
					OnStopDuringMovementUpdate( ActorActionResult_Failed );
					Cleanup();
					return false;
				}
			}
		}
	}

	return true;
}

void ActorActionMoveTo::Cleanup()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
}

CMovingAgentComponent* ActorActionMoveTo::GetMAC() const
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	
	// MG: I'm commenting this assert, as that situation happens often after delayed actions removal.
	if ( !mac )
	{
		//ASSERT( mac != NULL && "The actor doesn't have a moving agent component, and therefore can't move" )
		return NULL;
	}

	return mac;
}

CPathAgent* ActorActionMoveTo::GetPathAgent() const
{
	return m_actor->GetMovingAgentComponent()->GetPathAgent();
}

void ActorActionMoveTo::OnSegmentFinished( EMoveStatus status )
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
	if ( CanUseLocomotion() )
	{
		m_activeSegment = &locomotion().PushSteeringSegment();
		m_activeSegment->AddTargeter( this );
		m_activeSegment->AddRef();
	}
}
void ActorActionMoveTo::OnControllerAttached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
	}
	m_activeSegment = &locomotion().PushSteeringSegment();
	m_activeSegment->AddTargeter( this );
	m_activeSegment->AddRef();
}
void ActorActionMoveTo::OnControllerDetached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
}
void ActorActionMoveTo::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( false );

	if ( m_isMoving )
	{
		CPathAgent* pathAgent = GetPathAgent();
		if ( !pathAgent )
		{
			return;
		}

		// get follow position
		Vector3 followPosition;
		if ( !pathAgent->FollowPath( followPosition ) )
		{
			// pathfollowing failed
			m_isMoving = false;
			pathAgent->StopMovement();
			return;
		}

		CNode* road = pathAgent->GetUsedRoad();
		if ( road )
		{
			goal.TSetFlag( CNAME( Road ), road );
		}

		// get destination position
		Vector3 destinationPosition = pathAgent->GetDestination();
		Bool goingToDestinationNow = pathAgent->IsGoingToLastWaypoint();

		// calculate movement parameters
		Vector2 velocity;
		Float orientation;
		Vector3 position = pathAgent->GetPosition();
		Vector2 diff = followPosition.AsVector2() - position.AsVector2();
		Float followDistance = diff.Mag();
		Float destinationDistance = ( destinationPosition.AsVector2() - position.AsVector2() ).Mag();

		velocity = diff.Normalized();
		orientation = EulerAngles::YawFromXY( velocity.X, velocity.Y );

		if ( m_moveFlags & FLAG_PRECISE_ARRIVAL )
		{
			goal.SetFlag( CNAME( PreciseArrival ), true );
		}

		goal.SetOrientationGoal( agent, orientation );
		goal.SetHeadingGoal( agent, velocity );
		goal.SetGoalPosition( followPosition );
		goal.SetGoalTolerance( m_toleranceRadius );
		goal.SetDistanceToGoal( followDistance );
		goal.SetDestinationPosition( destinationPosition );
		if ( Abs( destinationPosition.Z - position.Z ) < 2.f )
		{
			goal.SetDistanceToDestination( goingToDestinationNow? destinationDistance : destinationDistance + followDistance ); // TODO make it bigger by follow distance, although actual distance might be even bigger
		}
		goal.MatchOrientationWithHeading( true );
		goal.SnapToMinimalSpeed( true );
	}
}

// -------------------------------------------------------------------------
// Debugging
// -------------------------------------------------------------------------

void ActorActionMoveTo::GenerateDebugFragments( CRenderFrame* frame )
{
}

void ActorActionMoveTo::DebugDraw( IDebugFrame& debugFrame ) const
{
}

String ActorActionMoveTo::GetDescription() const
{
	return TXT("MoveTo");
}

///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionMoveTo( const CNode * target, Bool withHeading, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */,
						  Float radius/* =0.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */, Uint16 actionFlags /* = 0 */, Float tolerance /* = 0.f*/ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}
	
	// Cancel actions
	ActionCancelAll();

	Float dist = GetWorldPosition().DistanceTo( target->GetWorldPosition() );
	if ( dist > MOVE_THRESHOLD )
	{
		// Start move to action
		if ( !m_actionMoveTo.StartMove( target, withHeading, moveType, absSpeed, radius, tolerance, failureAction, actionFlags ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionMoveTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Moving;

		OnActionStarted( ActorAction_Moving );
	}
	else
	{
		if ( !m_actionSlideTo.StartSlide( target->GetWorldPosition(), 0.1f ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionSlideTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Sliding;

		OnActionStarted( ActorAction_Sliding );
	}

	return true;
}

Bool CActor::ActionMoveToChangeTarget( const Vector& target, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, Float radius/* =0.0f */ )
{
	Bool theSameAction = ( m_action == &m_actionMoveTo );

	if( theSameAction )
	{
		m_actionMoveTo.ChangeTarget( target, moveType, absSpeed, radius );
		return true;
	}
	else
	{
		return ActionMoveTo( target, moveType, absSpeed, radius );
	}	
}
Bool CActor::ActionMoveTo( const Vector& target, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, 
						   Float radius/* =0.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */, Uint16 actionFlags /* = 0 */, Float tolerance /*= 0.f*/ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}
	
	// Cancel actions
	ActionCancelAll();

	Float dist = GetWorldPosition().DistanceTo( target );
	if ( dist > MOVE_THRESHOLD )
	{
		// Start move to action
		if ( !m_actionMoveTo.StartMove( target, CEntity::HEADING_ANY, moveType, absSpeed, radius, tolerance, failureAction, actionFlags ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionMoveTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Moving;

		OnActionStarted( ActorAction_Moving );
	}
	else
	{
		if ( !m_actionSlideTo.StartSlide( target, 0.1f ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionSlideTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Sliding;

		OnActionStarted( ActorAction_Sliding );
	}

	return true;
}

Bool CActor::ActionMoveTo( const Vector& target, Float heading, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, 
						   Float radius/* =0.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */, Uint16 actionFlags /* = 0 */, Float tolerance /*= 0.f*/ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	Float dist = GetWorldPosition().DistanceTo( target );
	if ( dist > MOVE_THRESHOLD )
	{
		// Start move to action
		if ( !m_actionMoveTo.StartMove( target, heading, moveType, absSpeed, radius, tolerance, failureAction, actionFlags ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionMoveTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Moving;

		OnActionStarted( ActorAction_Moving );
	}
	else
	{
		if ( !m_actionSlideTo.StartSlide( target, heading, 0.1f, SR_Nearest ) )
		{
			return false;
		}

		// Start action
		m_action = &m_actionSlideTo;
		m_latentActionResult = ActorActionResult_InProgress;
		m_latentActionType   = ActorAction_Sliding;

		OnActionStarted( ActorAction_Sliding );
	}

	return true;

}

Bool CActor::ActionMoveAwayFrom( const CNode * position, Float distance, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, 
								 Float radius/* =2.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move away action
	if ( !m_actionMoveTo.StartMoveAway( position, distance, moveType, absSpeed, radius, failureAction ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionMoveTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Moving;

	OnActionStarted( ActorAction_Moving );

	return true;
}

Bool CActor::ActionMoveAwayFromLine( const Vector& positionA, const Vector& positionB, Float distance, Bool makeMinimalMovement, 
									 EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, 
									 Float radius/* =2.0f */, EMoveFailureAction failureAction /* = MFA_REPLAN */ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move away action
	if ( !m_actionMoveTo.StartMoveAwayFromLine( positionA, positionB, distance, makeMinimalMovement, moveType, absSpeed, radius, failureAction ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionMoveTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Moving;

	OnActionStarted( ActorAction_Moving );

	return true;
}

//////////////////////////////////////////////////////////////////////////

extern Bool GLatentFunctionStart;

void CActor::funcActionMoveToNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	CNode *pTarget = target.Get();

	// Starting !
	if ( GLatentFunctionStart )
	{
		if ( pTarget == NULL )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Start action
		Bool actionResult = ActionMoveTo( pTarget, false, moveType, absSpeed, range, failureAction );
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

void CActor::funcActionMoveToNodeWithHeading( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	CNode *pTarget = target.Get();
	// Starting !
	if ( GLatentFunctionStart )
	{
		if ( pTarget == NULL )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Start action
		Bool actionResult = ActionMoveTo( pTarget, true, moveType, absSpeed, range, failureAction );
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

void CActor::funcActionMoveToChangeTargetAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );	
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionMoveToChangeTarget( target, moveType, absSpeed, range );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionMoveTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 1.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionMoveTo( target, moveType, absSpeed, range, failureAction );
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


void CActor::funcActionMoveToWithHeading( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionMoveTo( target, heading, moveType, absSpeed, range, failureAction );
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

void CActor::funcActionMoveAwayFromNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, position, NULL );
	GET_PARAMETER( Float, distance, 10.0f );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		CNode *pPosition = position.Get();
		if ( pPosition == NULL )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Start action
		Bool actionResult = ActionMoveAwayFrom( pPosition, distance, moveType, absSpeed, range, failureAction );
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

void CActor::funcActionMoveAwayFromLine( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, positionA, Vector::ZEROS );
	GET_PARAMETER( Vector, positionB, Vector::ZEROS );
	GET_PARAMETER( Float, distance, 10.0f );
	GET_PARAMETER( Bool, makeMinimalMovement, true );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionMoveAwayFromLine( positionA, positionB, distance, makeMinimalMovement, moveType, absSpeed, range, failureAction );
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


//////////////////////////////////////////////////////////////////////////

void CActor::funcActionMoveToNodeAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	CNode *pTarget = target.Get();

	if ( pTarget == NULL )
	{
		RETURN_BOOL( false );
		return;
	}

	Bool actionResult = ActionMoveTo( pTarget, false, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionMoveToNodeWithHeadingAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;
	
	CNode *pTarget = target.Get();

	if ( pTarget == NULL )
	{
		RETURN_BOOL( false );
		return;
	}

	Bool actionResult = ActionMoveTo( pTarget, true, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionMoveToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionMoveTo( target, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}


void CActor::funcActionMoveToWithHeadingAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionMoveTo( target, heading, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}


void CActor::funcActionMoveAwayFromNodeAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, position, NULL );
	GET_PARAMETER( Float, distance, 10.0f );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 1.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	CNode *pPosition = position.Get();

	if ( pPosition == NULL )
	{
		RETURN_BOOL( false );
		return;
	}

	Bool actionResult = ActionMoveAwayFrom( pPosition, distance, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}


void CActor::funcActionMoveAwayFromLineAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, positionA, Vector::ZEROS );
	GET_PARAMETER( Vector, positionB, Vector::ZEROS );
	GET_PARAMETER( Float, distance, 10.0f );
	GET_PARAMETER( Bool, makeMinimalMovement, true );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 1.0f );
	GET_PARAMETER_OPT( EMoveFailureAction, failureAction, MFA_REPLAN );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionMoveAwayFromLine( positionA, positionB, distance, makeMinimalMovement, moveType, absSpeed, range, failureAction );
	RETURN_BOOL( actionResult );
}