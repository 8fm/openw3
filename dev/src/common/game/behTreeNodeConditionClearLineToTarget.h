#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionClearLineToTargetInstance;

////////////////////////////////////////////////////////////////////////
// Condition that checks if we have clear line to action or combat
// target. Does both navigation and creature test.
////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeConditionClearLineToTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionClearLineToTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionClearLineToTargetInstance, ConditionClearLineToTarget );
protected:	
	Bool				m_combatTarget;
	Bool				m_navTest;
	Bool				m_creatureTest;
	Bool				m_useAgentRadius;
	CBehTreeValFloat	m_customRadius;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionClearLineToTargetDefinition() : 
		CBehTreeNodeConditionDefinition()
		, m_combatTarget( true )
		, m_navTest( true )
		, m_creatureTest( true )
		, m_useAgentRadius( true )
		, m_customRadius ( 0.5f )										{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionClearLineToTargetDefinition );
	PARENT_CLASS( CBehTreeNodeConditionDefinition );	
	PROPERTY_EDIT( m_combatTarget, TXT("Use combat or action target") );
	PROPERTY_EDIT( m_navTest, TXT("Do navigation test") );
	PROPERTY_EDIT( m_creatureTest, TXT("Do creature test"));
	PROPERTY_EDIT( m_useAgentRadius, TXT("Use agents own radius for test (CMovingAgentComponent 'radius' property)"));
	PROPERTY_EDIT( m_customRadius, TXT("Specify custion radius to be used"));
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionClearLineToTargetInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool	m_combatTarget;
	Bool	m_navTest;
	Bool	m_creatureTest;
	Bool	m_useAgentRadius;
	Float	m_customRadius;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionClearLineToTargetDefinition Definition;

	CBehTreeNodeConditionClearLineToTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_combatTarget( def.m_combatTarget )
		, m_navTest( def.m_navTest )
		, m_creatureTest( def.m_creatureTest )
		, m_useAgentRadius( def.m_useAgentRadius )
		, m_customRadius( def.m_customRadius.GetVal( context ) )		{}
};