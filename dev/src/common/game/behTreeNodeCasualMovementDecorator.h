/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecoratorSteeringGraph.h"

class CBehTreeNodeCasualMovementDecoratorInstance;

class CBehTreeNodeCasualMovementDecoratorDefinition : public CBehTreeDecoratorSteeringGraphDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCasualMovementDecoratorDefinition, CBehTreeDecoratorSteeringGraphDefinition, CBehTreeNodeCasualMovementDecoratorInstance, CasualMovement );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCasualMovementDecoratorDefinition );
	PARENT_CLASS( CBehTreeDecoratorSteeringGraphDefinition );
END_CLASS_RTTI();

class CBehTreeNodeCasualMovementDecoratorInstance : public CBehTreeDecoratorSteeringGraphInstance
{
	typedef CBehTreeDecoratorSteeringGraphInstance Super;
public:
	typedef CBehTreeDecoratorSteeringGraphDefinition Definition;

	CBehTreeNodeCasualMovementDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )												{}

	Bool			Activate() override;
	void			Deactivate() override;
};