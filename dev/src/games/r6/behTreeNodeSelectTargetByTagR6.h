/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeSelectTargetByTag.h"

class CBehTreeNodeSelectTargetByTagDecoratorR6Instance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagDecoratorR6Definition : public CBehTreeNodeSelectTargetByTagDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDecoratorR6Definition, CBehTreeNodeSelectTargetByTagDecoratorDefinition, CBehTreeNodeSelectTargetByTagDecoratorR6Instance, SelectTargetByTagDecoratorR6 );
	DECLARE_AS_R6_ONLY
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectTargetByTagDecoratorR6Definition );
	PARENT_CLASS( CBehTreeNodeSelectTargetByTagDecoratorDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagDecoratorR6Instance : public CBehTreeNodeSelectTargetByTagDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeNodeSelectTargetByTagDecoratorR6Definition Definition;

	CBehTreeNodeSelectTargetByTagDecoratorR6Instance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeSelectTargetByTagDecoratorInstance( def, owner, context, parent )
	{
	}

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;
};

