/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionHasCombatTargetInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionHasCombatTargetDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionHasCombatTargetDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionHasCombatTargetInstance, HasCombatTarget );
protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionHasCombatTargetDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionHasCombatTargetInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionHasCombatTargetDefinition Definition;

	CBehTreeNodeConditionHasCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};