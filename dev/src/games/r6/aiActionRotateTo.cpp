/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiActionRotateTo.h"
#include "r6behTreeInstance.h"
#include "r6InteractionComponent.h"
#include "../../common/game/moveSteeringLocomotionSegment.h"

IMPLEMENT_ENGINE_CLASS( CAIActionRotateMACToNode )

CAIActionRotateMACToNode::CAIActionRotateMACToNode()
	: m_angularTolerance( 0.5f )
	, m_takeToleranceFromInteraction( false )
	, m_component( nullptr )
	, m_targetYaw( 0.f )
	, m_isRotating( false )
{	
}

Bool CAIActionRotateMACToNode::CanBeStartedOn( CComponent* component ) const
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

	CNode* targetNode = FindActionTarget();
	if ( nullptr == targetNode )
	{
		return false;
	}

	const Float targetYaw = ( targetNode->GetWorldPosition() - mac->GetWorldPosition() ).ToEulerAngles().Yaw;
	const Float angDist = EulerAngles::AngleDistance( mac->GetWorldYaw(), targetYaw );
	if ( MAbs( angDist ) < GetCurrentAngularTolerance() )
	{
		return false; // no need to rotate
	}

	return true;
}

EAIActionStatus CAIActionRotateMACToNode::StartOn( CComponent* component )
{
	m_targetNode = FindActionTarget();
	if ( nullptr == m_targetNode.Get() )
	{
		return ACTION_Failed;
	}

	R6_ASSERT( m_status != ACTION_InProgress );
	R6_ASSERT( component && component->IsA< CMovingAgentComponent > () );

	// at this point we are sure this IS a moving agent component, AND it is valid, so no need to use Cast<>()
	CMovingAgentComponent* mac = static_cast< CMovingAgentComponent* > ( component );
	m_component = mac;
	mac->AttachLocomotionController( *this );

	// calculate target yaw
	m_targetYaw = ( m_targetNode.Get()->GetWorldPosition() - mac->GetWorldPosition() ).ToEulerAngles().Yaw;

	// shouldn't be rotating already
	R6_ASSERT( false == m_isRotating && false == mac->IsRotationTargetEnabled() );

	R6_ASSERT( CanUseLocomotion() );
	if ( CanUseLocomotion() )
	{
		// create a new locomotion segment
		CMoveLSSteering& segment = locomotion().PushSteeringSegment();
		segment.AddTargeter( this );

		m_isRotating = true;
		return TBaseClass::StartOn( component );
	}

	return ACTION_Failed;
}


EAIActionStatus CAIActionRotateMACToNode::Tick( Float timeDelta )
{
	if ( nullptr == m_targetNode.Get() )
	{
		return Cancel( TXT("target node despawned") );
	}

	CMovingAgentComponent* mac = m_component.Get();
	if ( nullptr == mac )
	{
		return Cancel( TXT("Moving agent component despawned.") );
	}

	m_targetYaw = RecomputeTargetYaw();
	const Float angDist = EulerAngles::AngleDistance( mac->GetWorldYaw(), m_targetYaw );
	if ( MAbs( angDist ) < GetCurrentAngularTolerance() )
	{
		// no need to rotate more
		return Stop( ACTION_Successful );
	}
	
	return TBaseClass::Tick( timeDelta );
}

EAIActionStatus CAIActionRotateMACToNode::Stop( EAIActionStatus newStatus )
{
	m_isRotating = false;

	CMovingAgentComponent* mac = m_component.Get();
	if ( mac && mac->IsAttached() )
	{
		mac->DetachLocomotionController( *this );
	}

	return TBaseClass::Stop( newStatus );	
}

EAIActionStatus CAIActionRotateMACToNode::Reset()
{
	m_component = nullptr;
	m_targetNode = nullptr;
	m_targetYaw = 0.f;
	m_isRotating = false;

	return TBaseClass::Reset();
}

Float CAIActionRotateMACToNode::RecomputeTargetYaw() const
{
	return ( m_targetNode.Get()->GetWorldPosition() - m_component.Get()->GetWorldPosition() ).ToEulerAngles().Yaw;
}

// -------------------------------------------------------------------------
// CMoveLocomotion::IController implementation
// -------------------------------------------------------------------------
void CAIActionRotateMACToNode::OnSegmentFinished( EMoveStatus status )
{
	if ( m_isRotating )
	{
		if ( status == MS_Completed )
		{
			Stop( ACTION_Successful );
		}
		Stop( ACTION_Failed );
	}
}

void CAIActionRotateMACToNode::OnControllerAttached()
{
	// noting to do here
}

void CAIActionRotateMACToNode::OnControllerDetached()
{
	if ( m_isRotating )
	{
		Stop( ACTION_Failed );
	}
}




Float CAIActionRotateMACToNode::GetCurrentAngularTolerance() const
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
				return interaction->GetRotationTolerance();
			}
		}
	}

	return m_angularTolerance;
}




// -------------------------------------------------------------------------
// IMovementTargeter interface
// -------------------------------------------------------------------------
void CAIActionRotateMACToNode::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	const Float agentOrientation = agent.GetWorldYaw();
	goal.SetFulfilled( false );
	goal.SetOrientationGoal( agent, m_targetYaw );
	goal.MatchMoveDirectionWithOrientation( true );
}

// -------------------------------------------------------------------------
// CAIActionRotateMACToInteraction class
// -------------------------------------------------------------------------
IMPLEMENT_ENGINE_CLASS( CAIActionRotateMACToInteraction )

Bool CAIActionRotateMACToInteraction::CanBeStartedOn( CComponent* component ) const 
{
	CNode* targetNode = FindActionTarget();	
	if ( nullptr == targetNode )
	{
		return false;
	}

	CR6InteractionComponent* interaction = CR6InteractionComponent::CheckNodeForInteraction( targetNode );
	if ( nullptr == interaction )
	{
		return false;
	}

	const Float targetRot = interaction->GetInteractRotationForNode( component ).Yaw;
	if ( CheckInteractRotation( targetRot, component->GetWorldRotation().Yaw, GetCurrentAngularTolerance() ) )
	{
		// already rotated
		return false;
	}

	return TBaseClass::CanBeStartedOn( component );
}

EAIActionStatus CAIActionRotateMACToInteraction::StartOn( CComponent* component )
{
	EAIActionStatus status = TBaseClass::StartOn( component );
	if ( ACTION_InProgress == status )
	{
		m_targetNode = CR6InteractionComponent::CheckNodeForInteraction( m_targetNode.Get() );
		R6_ASSERT( m_targetNode.Get() );
	}

	return status;
}

RED_INLINE Bool CAIActionRotateMACToInteraction::CheckInteractRotation( Float rot1, Float rot2, Float tol ) const
{
	return MAbs( EulerAngles::AngleDistance( rot1, rot2 ) ) <= tol;
}

Float CAIActionRotateMACToInteraction::RecomputeTargetYaw() const 
{
	if ( nullptr == m_targetNode.Get() )
	{
		return m_targetYaw;
	}

	const CR6InteractionComponent* interaction = Cast< const CR6InteractionComponent > ( m_targetNode.Get() );
	if( !interaction )
	{
		R6_ASSERT( false );
		return m_targetYaw;
	}

	return interaction->GetInteractRotationForNode( m_component.Get() ).Yaw;
}
