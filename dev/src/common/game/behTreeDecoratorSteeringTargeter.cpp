/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorSteeringTargeter.h"

#include "behTreeInstance.h"
#include "movementGoal.h"
#include "moveSteeringLocomotionSegment.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorSetSteeringCustomPositionDefinition )

///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeDecoratorSteeringTargeterInstance
///////////////////////////////////////////////////////////////////////////////
Bool IBehTreeNodeDecoratorSteeringTargeterInstance::Activate()
{
	CActor* actor = m_owner->GetActor();
	if ( actor == nullptr )
	{
		DebugNotifyActivationFail();
		return false;
	}
	if ( CMovingAgentComponent *const movingAgentComponent = actor->GetMovingAgentComponent() )
	{
		if ( CMoveLocomotion *const locomotion = movingAgentComponent->GetLocomotion() )
		{
			locomotion->AddTargeter_MoveLSSteering( this );
			locomotion->AddMoveLocomotionListener( this );			// if segment is updated we must update targeter as well
		}
	}

	return Super::Activate();
}

void IBehTreeNodeDecoratorSteeringTargeterInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	if ( actor )
	{
		CMovingAgentComponent *const movingAgentComponent = actor->GetMovingAgentComponent();
		if ( movingAgentComponent )
		{
			CMoveLocomotion *const locomotion = movingAgentComponent->GetLocomotion();
			if ( locomotion )
			{
				locomotion->RemoveTargeter_MoveLSSteering( this );
				locomotion->RemoveMoveLocomotionListener( this );
			}
		}
	}

	Super::Deactivate();
}

Bool IBehTreeNodeDecoratorSteeringTargeterInstance::IsFinished() const
{
	return false;
}
void IBehTreeNodeDecoratorSteeringTargeterInstance::OnSegmentPushed( IMoveLocomotionSegment* segment )
{
	// if segment is updated we must update targeter as well
	CMoveLSSteering *const moveLSSteering = segment->AsCMoveLSSteering( );
	if ( moveLSSteering )
	{
		moveLSSteering->AddTargeter( this );
	}
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeDecoratorSetSteeringTargetNodeDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return m_combatTarget
		? static_cast< IBehTreeNodeDecoratorSteeringTargeterInstance* >( new CBehTreeNodeDecoratorSetSteeringCombatTargetNodeInstance( *this, owner, context, parent ) )
		: static_cast< IBehTreeNodeDecoratorSteeringTargeterInstance* >( new CBehTreeNodeDecoratorSetSteeringActionTargetNodeInstance( *this, owner, context, parent ) );
}
///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringCombatTargetNodeInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorSetSteeringCombatTargetNodeInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CActor* combatTarget = m_owner->GetCombatTarget().Get();
	goal.SetGoalTargetNode( combatTarget );
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringActionTargetNodeInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorSetSteeringActionTargetNodeInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CNode* actionTarget = m_owner->GetActionTarget().Get();
	goal.SetGoalTargetNode( actionTarget );
}



///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeDecoratorSetSteeringNamedTargetNodeDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return m_combatTarget
		? static_cast< IBehTreeNodeDecoratorSteeringTargeterInstance* >( new CBehTreeNodeDecoratorSetSteeringNamedTargetWithCombatTargetNodeInstance( *this, owner, context, parent ) )
		: static_cast< IBehTreeNodeDecoratorSteeringTargeterInstance* >( new CBehTreeNodeDecoratorSetSteeringNamedTargetWithActionTargetNodeInstance( *this, owner, context, parent ) );
}
///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringNamedTargetWithCombatTargetNodeInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorSetSteeringNamedTargetWithCombatTargetNodeInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CNode* combatTarget = m_owner->GetCombatTarget().Get();
	goal.TSetFlag( m_targetName, combatTarget );
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringNamedTargetWithActionTargetNodeInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorSetSteeringNamedTargetWithActionTargetNodeInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CNode* actionTarget = m_owner->GetActionTarget().Get();
	goal.TSetFlag( m_targetName, actionTarget );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSetSteeringCustomPositionInstance
///////////////////////////////////////////////////////////////////////////////
void CBehTreeNodeDecoratorSetSteeringCustomPositionInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	const Vector& value = m_customMoveData->GetTarget();
	goal.SetFlag( m_steeringParameterName, value );
}