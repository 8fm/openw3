/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeCondition.h"

class CBehTreeNodeConditionIsTargetThePlayerInstance;

class CBehTreeNodeConditionIsTargetThePlayerDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsTargetThePlayerDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsTargetThePlayerInstance, ConditionIsTargetThePlayer )
protected:
	Bool					m_useCombatTarget;

	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionIsTargetThePlayerDefinition()
		: m_useCombatTarget( true )											{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsTargetThePlayerDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat/action target.") )
END_CLASS_RTTI()

class CBehTreeNodeConditionIsTargetThePlayerInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool					m_useCombatTarget;

	Bool					ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsTargetThePlayerDefinition Definition;

	CBehTreeNodeConditionIsTargetThePlayerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_useCombatTarget( def.m_useCombatTarget )						{}
};