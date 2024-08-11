/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionIsInCameraViewInstance;

class CBehTreeNodeConditionIsInCameraViewDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsInCameraViewDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsInCameraViewInstance, ConditionIsInCameraView );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsInCameraViewDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionIsInCameraViewInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Bool							ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsInCameraViewDefinition Definition;

	CBehTreeNodeConditionIsInCameraViewInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}
};


