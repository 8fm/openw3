/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeCondition.h"

class CBehTreeNodeConditionHasAbilityInstance;


class CBehTreeNodeConditionHasAbilityDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionHasAbilityDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionHasAbilityInstance, HasAbility )
protected:
	CName				m_abilityName;

	virtual IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionHasAbilityDefinition()														{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionHasAbilityDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
	PROPERTY_EDIT( m_abilityName, TXT("Ability name") )
END_CLASS_RTTI()

class CBehTreeNodeConditionHasAbilityInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CName				m_abilityName;

	virtual Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionHasAbilityDefinition Definition;

	CBehTreeNodeConditionHasAbilityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_abilityName( def.m_abilityName )														{}
};