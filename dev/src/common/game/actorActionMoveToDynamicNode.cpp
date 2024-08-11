/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionMoveToDynamicNode.h"

#include "movementTargeter.h"
#include "moveSteeringLocomotionSegment.h"
#include "movableRepresentationPathAgent.h"
#include "../engine/renderFrame.h"


///////////////////////////////////////////////////////////////////////////////

namespace // anonymous
{
	class CMoveTRGPursueMovingAgent : public IMoveTRGManaged
	{
	private:
		THandle< CMovingAgentComponent >		m_pursuedAgent;
		Float									m_distance;

		mutable Vector							m_velocity;

	public:
		CMoveTRGPursueMovingAgent( const THandle< CMovingAgentComponent >& pursuedAgent, Float distance )
			: m_pursuedAgent( pursuedAgent )
			, m_distance( distance )
		{
		}

		virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
		{
			CMovingAgentComponent* pursuedAgent = m_pursuedAgent.Get();
			if ( !pursuedAgent )
			{
				// the target agent got unstreamed - exit
				goal.SetFulfilled( true );
				return;
			}

			const Vector& targetPos = pursuedAgent->GetWorldPositionRef();

			Vector velocity;
			Float orientation;
			Float distToTarget = targetPos.DistanceTo2D( agent.GetAgentPosition() );
			if ( distToTarget <= m_distance )
			{
				velocity = Flee( agent, targetPos );
			}
			else
			{
				velocity = Pursue( agent, *pursuedAgent );
			}

			orientation = FaceTarget( agent, targetPos );

			goal.SetOrientationGoal( agent, orientation );
			goal.SetHeadingGoal( agent, velocity.AsVector2() );
			goal.SetDistanceToGoal( m_distance < 0 ? FLT_MAX : m_distance );

			// annotations
			goal.SetFulfilled( false );
		}

		virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame )
		{
			Vector agentPos = agent.GetAgentPosition();
			frame->AddDebugLineWithArrow( agentPos, agentPos + m_velocity, 1.0f, 0.1f, 0.1f, Color::BLUE );
		}

		virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const
		{
		}
	};

	// ------------------------------------------------------------------------

	class CMoveTRGPursueDynamicNode : public IMoveTRGManaged
	{
	private:
		THandle< CNode >						m_target;
		Float									m_distance;

		mutable Vector							m_velocity;

	public:
		CMoveTRGPursueDynamicNode( const THandle< CNode >& target, Float distance )
			: m_target( target )
			, m_distance( distance )
		{
		}

		virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
		{
			CNode* target = m_target.Get();
			if ( !target )
			{
				// the target node got unstreamed - exit
				goal.SetFulfilled( true );
				return;
			}

			const Vector& targetPos = target->GetWorldPositionRef();

			Vector velocity;
			Float orientation;
			Float distToTarget = targetPos.DistanceTo2D( agent.GetAgentPosition() );
			if ( distToTarget <= m_distance )
			{
				velocity = Flee( agent, targetPos );
			}
			else
			{
				velocity = Seek( agent, targetPos );
			}

			orientation = FaceTarget( agent, targetPos );

			goal.SetOrientationGoal( agent, orientation );
			goal.SetHeadingGoal( agent, velocity.AsVector2() );
			goal.SetDistanceToGoal( m_distance < 0 ? FLT_MAX : m_distance );

			// annotations
			goal.SetFulfilled( false );
		}

		virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame )
		{
			Vector agentPos = agent.GetAgentPosition();
			frame->AddDebugLineWithArrow( agentPos, agentPos + m_velocity, 1.0f, 0.1f, 0.1f, Color::BLUE );
		}

		virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const
		{
		}
	};

	// ------------------------------------------------------------------------

	class CMoveTRGWait : public IMoveTRGManaged
	{
	public:
		virtual void UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
		{
			goal.SetFulfilled( false );
		}

		virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame )
		{
		}

		virtual void GenerateDebugPage( TDynArray< String >& debugLines ) const
		{
		}
	};

} // anonymous


///////////////////////////////////////////////////////////////////////////////

ActorActionMoveToDynamicNode::ActorActionMoveToDynamicNode( CActor* actor, EActorActionType type /*= ActorAction_DynamicMoving*/ )
	: ActorAction( actor, type )
	, m_activeSegment( nullptr )
	, m_targetMAC( nullptr )
	, m_desiredDistance( 0.f )
	, m_pathfindingTolerance( 0.f )
	, m_keepDistance( false )
	, m_isMoving( false )
	
{
}

ActorActionMoveToDynamicNode::~ActorActionMoveToDynamicNode()
{
}

void ActorActionMoveToDynamicNode::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

Bool ActorActionMoveToDynamicNode::IsCancelingAllowed() const
{
	CMovingAgentComponent* ac =  m_actor->GetMovingAgentComponent();
	return ac->CanCancelMovement();
}

Bool ActorActionMoveToDynamicNode::StartMove( const CNode* target, EMoveType moveType, Float absSpeed, Float radius, Float tolerance, Bool keepDistance, EMoveFailureAction failureAction )
{
	PC_SCOPE( AI_ActorActionMoveTo );

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	// if the target's got a CMovingAgentComponent, cash its instance for future reference
	m_targetMAC = NULL;
	const CEntity* targetEntity = Cast< CEntity >( target );
	if ( targetEntity )
	{
		for( ComponentIterator< CMovingAgentComponent > it( targetEntity ); it; ++it )
		{
			// get the first target MAC there is
			m_targetMAC = *it;
			if ( m_targetMAC )
			{
				break;
			}
		}
	}
	SetMoveSpeed( *mac, moveType, absSpeed );

	m_keepDistance = keepDistance;
	m_desiredDistance = radius;
	m_pathfindingTolerance = tolerance;
	m_target = const_cast< CNode* >( target );
	m_lastTargetUpdate = EngineTime::ZERO;

	mac->AttachLocomotionController( *this );
	
	return true;
}

void ActorActionMoveToDynamicNode::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
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

Bool ActorActionMoveToDynamicNode::Update( Float timeDelta )
{
	PC_SCOPE( AI_ActorActionMoveToDynamicNode );

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	CPathAgent* pathAgent = mac->GetPathAgent();
	CNode* targetNode = m_target.Get();
	if ( !pathAgent || !targetNode )
	{
		m_actor->ActionEnded( ActorAction_DynamicMoving, ActorActionResult_Failed );
		Stop();
		Cleanup();
		return false;
	}

	// check success 
	if ( m_activeSegment && m_activeSegment->IsDestinationReached() )
	{
		// movement completed
		m_actor->ActionEnded( ActorAction_DynamicMoving, ActorActionResult_Succeeded );

		Stop();

		Cleanup();

		return false;
	}

	if ( pathAgent->UpdateMetalinkAwareness() )
	{
		return true;
	}
	
	if ( !m_isMoving )
	{
		Vector3 targetPos;
		if ( m_targetMAC )
		{
			targetPos = m_targetMAC->GetAgentPosition().AsVector3();
		}
		else
		{
			targetPos = targetNode->GetWorldPosition().AsVector3();
		}

		// success check
		const Vector& actorPos = mac->GetAgentPosition();
		Float distSq = (actorPos.AsVector2() - targetPos.AsVector2()).SquareMag();
		if ( distSq <= m_desiredDistance*m_desiredDistance && Abs( actorPos.Z - targetPos.Z ) < 2.f )
		{
			// movement completed
			m_actor->ActionEnded( ActorAction_DynamicMoving, ActorActionResult_Succeeded );

			Stop();

			Cleanup();

			return false;
		}

		switch ( pathAgent->PlotPath( targetPos, m_pathfindingTolerance ) )
		{
		case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
		case PathLib::PATHRESULT_FAILED:
			// cannot get to target (pathfinding failed)
			m_actor->ActionEnded( ActorAction_DynamicMoving, ActorActionResult_Failed );
			Stop();
			Cleanup();
			return false;
		case PathLib::PATHRESULT_SUCCESS:
			m_isMoving = true;
			pathAgent->SetupPathfollowing( m_desiredDistance, CPathAgent::PATHFOLLOW_DEFAULT );
			break;
		case PathLib::PATHRESULT_PENDING:
			break;
		default:
			ASSUME( false );
		}
	}

	//if ( m_isMoving )
	//{
	//	// TODO: check whether we should update path destination
	//	Vector3 currentDestination = pathAgent->GetDestination();
	//	// update current target
	//	if ( !pathAgent->UpdatePathDestination( currentDestination ) )
	//	{
	//		// pathfollowing failed
	//		m_isMoving = false;
	//		pathAgent->StopMovement();

	//	}
	//}
	return true;
}

void ActorActionMoveToDynamicNode::Cleanup()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}

	m_target = nullptr;
	m_targetMAC = nullptr;
	m_isMoving = false;
}

CPathAgent* ActorActionMoveToDynamicNode::GetPathAgent() const
{
	return m_actor->GetMovingAgentComponent()->GetPathAgent();
}
void ActorActionMoveToDynamicNode::OnSegmentFinished( EMoveStatus status )
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
void ActorActionMoveToDynamicNode::OnControllerAttached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
	m_activeSegment = &locomotion().PushSteeringSegment();
	m_activeSegment->AddTargeter( this );
	m_activeSegment->AddRef();
}
void ActorActionMoveToDynamicNode::OnControllerDetached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
}
void ActorActionMoveToDynamicNode::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	struct Local
	{
		static void Stop( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal )
		{
			goal.SetHeadingGoal( agent, Vector2( 0, 0 ) );
			goal.SetDistanceToGoal( 0.f );
		}
	};
	goal.SetFulfilled( false );

	if ( m_isMoving )
	{
		CPathAgent* pathAgent = GetPathAgent();
		if ( !pathAgent )
		{
			return;
		}

		CNode* targetNode = m_target.Get();
		if ( !targetNode )
		{
			goal.SetFulfilled( true );
			return;
		}

		Vector3 targetPos;
		if ( m_targetMAC )
		{
			targetPos = m_targetMAC->GetAgentPosition().AsVector3();
		}
		else
		{
			targetPos = targetNode->GetWorldPosition().AsVector3();
		}
		
		// TODO: check whether we should update path destination
		// update current target
		if ( !pathAgent->UpdatePathDestination( targetPos ) )
		{
			// pathfollowing failed
			m_isMoving = false;
			Local::Stop( agent, goal );
			pathAgent->StopMovement();
			return;
		}

		// get follow position
		Vector3 followPosition;
		if ( !pathAgent->FollowPath( followPosition ) )
		{
			// pathfollowing failed
			m_isMoving = false;
			Local::Stop( agent, goal );
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

		velocity = (followDistance > 0.f) ? diff * (1.f / followDistance) : Vector2( 0.f, 0.f );
		orientation = EulerAngles::YawFromXY( velocity.X, velocity.Y );

		goal.SetOrientationGoal( agent, orientation );
		goal.SetHeadingGoal( agent, velocity );
		goal.SetGoalPosition( followPosition );
		goal.SetGoalTolerance( m_desiredDistance );
		goal.SetDistanceToGoal( followDistance );
		goal.SetDestinationPosition( destinationPosition );
		if ( Abs( destinationPosition.Z - position.Z ) < 2.f )
		{
			goal.SetDistanceToDestination( goingToDestinationNow? destinationDistance : destinationDistance + followDistance ); // TODO make it bigger by follow distance, although actual distance might be even bigger
		}
		goal.MatchOrientationWithHeading( true );
		goal.SnapToMinimalSpeed( true );
		goal.SetGoalTargetNode( m_target );
	}
}

// -------------------------------------------------------------------------
// Debugging
// -------------------------------------------------------------------------

void ActorActionMoveToDynamicNode::GenerateDebugFragments( CRenderFrame* frame )
{
}

void ActorActionMoveToDynamicNode::DebugDraw( IDebugFrame& debugFrame ) const
{
}

String ActorActionMoveToDynamicNode::GetDescription() const
{
	CNode* target = m_target.Get();
	if ( target )
	{
		return String::Printf( TXT( "Dynamically following node %s" ), target->GetFriendlyName().AsChar() );
	}
	else
	{
		return TXT( "Dynamically following UNSTREAMED node!!!" );
	}
}

///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionMoveToDynamicNode( const CNode* target, EMoveType moveType /* = MT_Walk */, Float absSpeed /* = 1.0f */, 
	Float radius/* =0.0f */, Bool keepDistance /* = false */, EMoveFailureAction failureAction /* = MFA_REPLAN */, Float tolerance /* = 0.f */ )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();


	// Start move to action
	if ( !m_actionMoveToDynamicNode.StartMove( target, moveType, absSpeed, radius, tolerance, keepDistance, failureAction ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionMoveToDynamicNode;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_DynamicMoving;

	OnActionStarted( ActorAction_DynamicMoving );

	return true;
}

//////////////////////////////////////////////////////////////////////////

extern Bool GLatentFunctionStart;

void CActor::funcActionMoveToDynamicNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( Bool, keepDistance, false );
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
		Bool actionResult = ActionMoveToDynamicNode( pTarget, moveType, absSpeed, range, keepDistance, failureAction );
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

void CActor::funcActionMoveToDynamicNodeAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CNode>, target, NULL );
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	GET_PARAMETER( Float, absSpeed, 1.0f );
	GET_PARAMETER( Float, range, 0.0f );
	GET_PARAMETER_OPT( Bool, keepDistance, false );
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

	Bool actionResult = ActionMoveToDynamicNode( pTarget, moveType, absSpeed, range, keepDistance, failureAction );
	RETURN_BOOL( actionResult );
}

///////////////////////////////////////////////////////////////////////////////
