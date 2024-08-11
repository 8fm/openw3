/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeAtomicSteerWithTarget.h"

#include "behTreeInstance.h"
#include "movementGoal.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeAtomicSteerWithTargetDefinition );
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeAtomicSteerWithCustomTargetDefinition );


//////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicSteerWithTargetDefinition
//////////////////////////////////////////////////////////////////////////

void CBehTreeAtomicSteerWithTargetInstance::Update()
{
}


Bool CBehTreeAtomicSteerWithTargetInstance::IsFinished() const
{
	return false;
}

void CBehTreeAtomicSteerWithTargetInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	CNode* target = m_useCombatTarget ? m_owner->GetCombatTarget().Get() : m_owner->GetActionTarget().Get();
	goal.SetGoalTargetNode( target );
	goal.SetFulfilled( false );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicSteerWithCustomTargetInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeAtomicSteerWithCustomTargetInstance::CBehTreeAtomicSteerWithCustomTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_ptr( owner )
{}

void CBehTreeAtomicSteerWithCustomTargetInstance::Update()
{
}


Bool CBehTreeAtomicSteerWithCustomTargetInstance::IsFinished() const
{
	return false;
}

void CBehTreeAtomicSteerWithCustomTargetInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetDestinationPosition( m_ptr->GetTarget() );
	goal.SetFulfilled( false );
}