/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeCondition.h"

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsPlayerInstance;
class CBehTreeNodeConditionIsPlayerDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsPlayerDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsPlayerInstance, ConditionIsPlayer )
	DECLARE_AS_R6_ONLY

protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionIsPlayerDefinition() { }
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsPlayerDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionIsPlayerInstance : public CBehTreeNodeConditionInstance
{
protected:
	typedef CBehTreeNodeConditionInstance Super;

public:
	typedef CBehTreeNodeConditionIsPlayerDefinition Definition;

	CBehTreeNodeConditionIsPlayerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

protected:
	Bool ConditionCheck() override;
};