/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeGuardAreaData.h"
#include "../../common/game/behTreeNodeCondition.h"


class CBehTreeNodeConditionShouldPursueCombatTargetInstance;

class CBehTreeNodeConditionShouldPursueCombatTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionShouldPursueCombatTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionShouldPursueCombatTargetInstance, ShouldPursueTarget )
protected:

	CBehTreeValFloat						m_allowPursueInDistance;
	Bool									m_allowPursueInCombat;
	Bool									m_testReachability;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionShouldPursueCombatTargetDefinition()
		: m_allowPursueInDistance( 4.f )
		, m_allowPursueInCombat( true )
		, m_testReachability( true )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionShouldPursueCombatTargetDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
	PROPERTY_EDIT( m_allowPursueInDistance, TXT("Allow pursue from guard area to pursue area if enemy is in given distance. NOTICE: Only taken into consideration if flag 'allowPursueInCombat' is set.") )
	PROPERTY_EDIT( m_allowPursueInCombat, TXT("Allow pursue if we were already in combat with current target") )
	PROPERTY_EDIT( m_testReachability, TXT("Test target pathfinding reachability") )
END_CLASS_RTTI()

class CBehTreeNodeConditionShouldPursueCombatTargetInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:

	CBehTreeGuardAreaDataPtr				m_guardAreaDataPtr;
	Float									m_allowPursueInDistanceSq;
	Float									m_testTimeout;
	Bool									m_lastDecision;
	Bool									m_allowPursueInCombat;
	Bool									m_testReachability;

	Bool							GuardAreaTest();
	Bool							ReachabilityTest();

	Bool							ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionShouldPursueCombatTargetDefinition Definition;

	CBehTreeNodeConditionShouldPursueCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};