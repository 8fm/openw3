/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/game/behTreeInstance.h"
#include "../../common/game/moveLocomotion.h"
#include "../../common/game/movePathNavigationQuery.h"
#include "../../common/game/moveSteeringLocomotionSegment.h"
#include "../../common/game/movableRepresentationPathAgent.h"
#include "../../common/engine/pathlibWorld.h"

#include "aiActionMoveTo.h"
#include "r6InteractionComponent.h"
#include "../../common/engine/renderFrame.h"


IMPLEMENT_ENGINE_CLASS( CAIActionMove )

CAIActionMove::CAIActionMove()
	: m_moveType( MT_Walk )
	, m_absSpeed( 0.f )
	, m_toleranceRadius( 0.05f )
	, m_takeToleranceFromInteraction( false )
	, m_isMoving( false )
	, m_targetUpdate( false )
{	
}

Bool CAIActionMove::CanBeStartedOn( CComponent* component ) const
{
	CMovingAgentComponent* mac = Cast< CMovingAgentComponent > ( component );
	if ( nullptr == mac )
	{
		return false;
	}

	if ( false == mac->IsMotionEnabled() )
	{
		return false;
	}

	return true;
}

EAIActionStatus CAIActionMove::StartOn( CComponent* component )
{
	R6_ASSERT( m_status != ACTION_InProgress );
	R6_ASSERT( component && component->IsA< CMovingAgentComponent > () );

	// at this point we are sure this IS a moving agent component, AND it is valid, so no need to use Cast<>()
	CMovingAgentComponent* mac = static_cast< CMovingAgentComponent* > ( component );
	m_component = mac;

	mac->AttachLocomotionController( *this );
	
	m_safetyDelay = GGame->GetEngineTime() + 2.f;

	return TBaseClass::StartOn( component );
}


EAIActionStatus CAIActionMove::Tick( Float timeDelta )
{
	CMovingAgentComponent* mac = m_component.Get();
	if ( nullptr == mac )
	{
		return Cancel( TXT("Moving agent component streamed out.") );
	}

	// check success (maybe we already reached target?)
	Float distSq = ( mac->GetAgentPosition().AsVector2() - m_targetPos.AsVector2() ).SquareMag();
	const Float toleranceRadius = GetRealToleranceRadius(); // we will never stop exactly at designated point, but we may stop very very close to it (0.00001) for example

	if ( distSq <= toleranceRadius * toleranceRadius )
	{
		return Stop( ACTION_Successful );
	}

	// if not moving (we need new path) or if target updated (we need new path)
	if ( !m_isMoving || m_targetUpdate )
	{
		CPathAgent* pathAgent = mac->GetPathAgent();
		if ( nullptr == pathAgent )
		{
			return Stop( ACTION_Failed );
		}

		m_targetUpdate = false;

		// if we have no way to plot path, fail (this shouldn't happen though)		
		EngineTime engineTime = GGame->GetEngineTime();
		if ( engineTime > m_pathPlotingDelay )
		{
			// pathfinding
			switch ( pathAgent->PlotPath( m_targetPos.AsVector3() ) )
			{
			case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
			case PathLib::PATHRESULT_FAILED:
				// cannot get to target (pathfinding failed)
				m_pathPlotingDelay = engineTime + 0.5f;
				break;
			case PathLib::PATHRESULT_SUCCESS:
				m_isMoving = true;
				pathAgent->SetupPathfollowing( GetCurrentToleranceRadius(), CPathAgent::PATHFOLLOW_DEFAULT );
				break;
			case PathLib::PATHRESULT_PENDING:
				break;
			default:
				ASSUME( false );
			}
		}

		// we're still not moving? if we won't start - fail
		if ( !m_isMoving && engineTime > m_safetyDelay )
		{
			return Stop( ACTION_Failed );
		}
	}

	return TBaseClass::Tick( timeDelta );
}

EAIActionStatus CAIActionMove::Stop( EAIActionStatus newStatus )
{
	CMovingAgentComponent* mac = m_component.Get();
	if ( mac )
	{
		mac->CancelMove();
		mac->DetachLocomotionController( *this );
		CPathAgent* pathAgent = mac->GetPathAgent();
		if ( pathAgent )
		{
			pathAgent->StopMovement();
		}
	}

	m_isMoving = false;

	return TBaseClass::Stop( newStatus );	
}

EAIActionStatus CAIActionMove::Reset()
{
	m_component = nullptr;
	m_targetPos = Vector::ZEROS;
	m_safetyDelay = EngineTime::ZERO;
	m_pathPlotingDelay = EngineTime::ZERO;
	m_isMoving = false;
	m_targetUpdate = false;
	return TBaseClass::Reset();
}

// -------------------------------------------------------------------------
// CMoveLocomotion::IController implementation
// -------------------------------------------------------------------------
void CAIActionMove::OnSegmentFinished( EMoveStatus status )
{
	if ( CanUseLocomotion() )
	{
		CMoveLSSteering& steeringSegment = locomotion().PushSteeringSegment();
		steeringSegment.AddTargeter( this );
	}
}

void CAIActionMove::OnControllerAttached()
{
	CMoveLSSteering& steeringSegment = locomotion().PushSteeringSegment();
	steeringSegment.AddTargeter( this );
}

void CAIActionMove::OnControllerDetached()
{

}

// -------------------------------------------------------------------------
// IMovementTargeter interface
// -------------------------------------------------------------------------
void CAIActionMove::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( m_status == ACTION_Successful );

	if ( m_isMoving )
	{
		CPathAgent* pathAgent = m_component.Get()->GetPathAgent();
		R6_ASSERT( pathAgent )

		// get follow position
		Vector3 followPosition;
		if ( !pathAgent->FollowPath( followPosition ) )
		{
			// pathfollowing failed
			m_isMoving = false;
			pathAgent->StopMovement();
			return;
		}

		// get destination position
		Vector3 destinationPosition = pathAgent->GetDestination();
		Bool goingToDestinationNow = pathAgent->IsGoingToLastWaypoint();

		// calculate movement parameters
		const Vector3	position			= pathAgent->GetPosition();
		const Vector2	diff				= followPosition.AsVector2() - position.AsVector2();
		const Float		followDistance		= diff.Mag();
		const Float		destinationDistance = ( destinationPosition.AsVector2() - position.AsVector2() ).Mag();
		const Vector2	velocity			= diff.Normalized();
		const Float		orientation			= EulerAngles::YawFromXY( velocity.X, velocity.Y );

		goal.SetOrientationGoal( agent, orientation );
		goal.SetHeadingGoal( agent, velocity );
		goal.SetGoalPosition( followPosition );
		goal.SetDistanceToGoal( followDistance );
		goal.SetDestinationPosition( destinationPosition );
		goal.SetDistanceToDestination( goingToDestinationNow? destinationDistance : destinationDistance + followDistance ); // TODO make it bigger by follow distance, although actual distance might be even bigger
		goal.MatchOrientationWithHeading( true );
		goal.SnapToMinimalSpeed( true );
	}
}

void CAIActionMove::OnGenerateDebugFragments( CRenderFrame* frame )
{
	Vector dir = m_targetPos - m_component.Get()->GetWorldPosition();
	Float scale = dir.Normalize3();

	Matrix m = Matrix::IDENTITY;
	m.SetTranslation( m_component->GetWorldPosition() );
	frame->AddDebugArrow( m, dir, scale, Color::BROWN );
}



Float CAIActionMove::GetRealToleranceRadius() const
{
	return GetCurrentToleranceRadius() + FLT_EPSILON;
}


Float CAIActionMove::GetCurrentToleranceRadius() const
{
	if( m_takeToleranceFromInteraction )
	{
		// targetNode is an interaction to perform
		CNode* targetNode = FindActionTarget();
		if ( targetNode )
		{
			// check if it's an interaction
			CR6InteractionComponent* interaction = CR6InteractionComponent::CheckNodeForInteraction( targetNode );

			if( interaction )
			{
				return interaction->GetPositionTolerance();
			}
		}
	}

	return m_toleranceRadius;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CAIActionMoveN )

Bool CAIActionMoveN::CanBeStartedOn( CComponent* component ) const 
{
	CNode* targetNode = FindActionTarget();
	if ( nullptr == targetNode )
	{
		return false;
	}

	return TBaseClass::CanBeStartedOn( component );
}

EAIActionStatus CAIActionMoveN::StartOn( CComponent* component )
{
	EAIActionStatus ret = TBaseClass::StartOn( component );

	if ( ret == ACTION_InProgress )
	{
		// target node was checked in CanBeStartedOn(), so it's valid now, 
		// but we can't assume it'll be valid during whole lifetime of this action, so we store it as a handle
		m_targetNode = FindActionTarget();
		m_targetPos = RecomputeTargetPos();
	}

	return ret;
}

EAIActionStatus CAIActionMoveN::Tick( Float timeDelta )
{
	// check if target node moved
	CNode* targetNode = m_targetNode.Get();
	if ( targetNode )
	{
		Float tolRad = GetCurrentToleranceRadius();
		if ( m_targetPos.DistanceSquaredTo( targetNode->GetWorldPositionRef() ) > tolRad * tolRad )
		{
			m_targetUpdate = true;
			m_targetPos = RecomputeTargetPos();
		}
	}
	
	if ( nullptr == targetNode || m_targetPos == Vector::ZEROS )
	{
		return Cancel( TXT("Cannot compute target position!") );
	}

	return TBaseClass::Tick( timeDelta ); 
}

EAIActionStatus CAIActionMoveN::Reset()
{
	m_targetNode = nullptr;
	return TBaseClass::Reset();
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CAIActionMoveToNode )

Vector CAIActionMoveToNode::RecomputeTargetPos() const 
{
	return m_targetNode.Get()->GetWorldPosition().SetW( 1.f );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CAIActionMoveAwayFromNode )

CAIActionMoveAwayFromNode::CAIActionMoveAwayFromNode()
	: m_alertDistance( 5.f )
	, m_awayDistance( 10.f )
	, m_safeZoneRadius( 4.f )
{
}


Bool CAIActionMoveAwayFromNode::CanBeStartedOn( CComponent* component ) const 
{
	CNode* targetNode = FindActionTarget();
	if ( nullptr == targetNode )
	{
		return false;
	}

	// do not start this action if we're already further away than we're required to be
	if ( component->GetWorldPositionRef().DistanceSquaredTo( targetNode->GetWorldPositionRef() ) >= m_alertDistance * m_alertDistance )
	{
		return false;
	}

	return TBaseClass::CanBeStartedOn( component );
}


Vector CAIActionMoveAwayFromNode::RecomputeTargetPos() const 
{
	Vector dangerPos = m_targetNode.Get()->GetWorldPosition();
	Vector escapeZone = dangerPos + ( m_component.Get()->GetWorldPosition() - dangerPos ).Normalized3() * ( m_awayDistance + m_safeZoneRadius + GetCurrentToleranceRadius() + FLT_EPSILON );
	Vector awayPosition( Vector::EW );

	CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();
	if ( pathlib )
	{
		if ( pathlib->FindRandomPositionInRadius( PathLib::INVALID_AREA_ID, escapeZone.AsVector3(), m_safeZoneRadius, m_component.Get()->GetRadius(), 0, awayPosition.AsVector3() ) )
		{
			return awayPosition;
		}
	}

	return Vector::ZEROS;
}

Float CAIActionMoveAwayFromNode::GetRealToleranceRadius() const 
{
	return GetCurrentToleranceRadius() + m_safeZoneRadius;
}

#ifndef NO_EDITOR
RED_DEFINE_STATIC_NAME( alertDistance )
RED_DEFINE_STATIC_NAME( awayDistance )
void CAIActionMoveAwayFromNode::OnPropertyPostChange( IProperty* prop )
{
	if ( prop->GetName() == CNAME( alertDistance ) )
	{
		if ( m_alertDistance > m_awayDistance )
		{
			m_alertDistance = m_awayDistance;
		}
	}

	if ( prop->GetName() == CNAME( awayDistance ) )
	{
		if ( m_alertDistance > m_awayDistance )
		{
			m_awayDistance = m_alertDistance;
		}
	}

	TBaseClass::OnPropertyPostChange( prop );
}
#endif // ifndef NO_EDITOR


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CAIActionMoveToInteraction )

Bool CAIActionMoveToInteraction::CanBeStartedOn( CComponent* component ) const 
{
	const Bool canBeStarted = TBaseClass::CanBeStartedOn( component );
	if ( canBeStarted )
	{
		CNode* actionTarget = FindActionTarget();
		if ( nullptr == actionTarget )
		{
			return false;
		}

		CR6InteractionComponent* interaction = CR6InteractionComponent::CheckNodeForInteraction( actionTarget ); 
		if ( nullptr == interaction )
		{
			return false;
		}

		return true;
	}

	return false;
}

EAIActionStatus CAIActionMoveToInteraction::StartOn( CComponent* component )
{
	const EAIActionStatus status = TBaseClass::StartOn( component );
	if ( status == ACTION_InProgress )
	{
		m_interaction = CR6InteractionComponent::CheckNodeForInteraction( FindActionTarget() ); 
		R6_ASSERT( m_interaction );
	}

	return status;
}

Vector CAIActionMoveToInteraction::RecomputeTargetPos( CR6InteractionComponent* interaction ) const
{
	Vector3 pos = Vector::ZEROS;
	CPathLibWorld* pathlib = GGame->GetActiveWorld()->GetPathLibWorld();

	if ( pathlib )
	{
		pathlib->FindSafeSpot( PathLib::INVALID_AREA_ID, interaction->GetInteractLocationForNode( m_component.Get() ), 0.01f, m_component.Get()->GetRadius(), pos );
	}

	return pos;
}

EAIActionStatus CAIActionMoveToInteraction::Tick( Float timeDelta )
{
	// check if target node moved
	CR6InteractionComponent* interaction = m_interaction.Get();
	if ( interaction )
	{
		Float tolRad = GetCurrentToleranceRadius();
		if ( m_interactionPos.DistanceSquaredTo( interaction->GetWorldPositionRef() ) > tolRad * tolRad )
		{
			m_targetUpdate = true;
			m_interactionPos = interaction->GetWorldPosition();
			m_targetPos = RecomputeTargetPos( interaction );
		}
	}
	
	if ( nullptr == interaction || m_targetPos == Vector::ZEROS )
	{
		return Cancel( TXT("Cannot compute target position!") );
	}

	return TBaseClass::Tick( timeDelta );
}

EAIActionStatus CAIActionMoveToInteraction::Reset()
{
	m_interaction = nullptr;
	m_interactionPos = Vector::ZEROS;

	return TBaseClass::Reset();
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------