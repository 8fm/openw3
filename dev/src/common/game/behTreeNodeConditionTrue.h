/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionTrueInstance;

class CBehTreeNodeConditionTrueDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionTrueDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionTrueInstance, ConditionTrue );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionTrueDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionTrueInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool							ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionTrueDefinition Definition;

	CBehTreeNodeConditionTrueInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}
};