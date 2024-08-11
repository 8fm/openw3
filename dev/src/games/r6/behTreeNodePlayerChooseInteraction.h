/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeSelectTargetByTag.h"

class CBehTreeNodePlayerChooseInteractionInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerChooseInteractionDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePlayerChooseInteractionDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodePlayerChooseInteractionInstance, ChooseInteractionDecorator );
	DECLARE_AS_R6_ONLY

public:
	CBehTreeNodePlayerChooseInteractionDefinition();
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodePlayerChooseInteractionDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodePlayerChooseInteractionInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

public:
	typedef CBehTreeNodePlayerChooseInteractionDefinition Definition;

	CBehTreeNodePlayerChooseInteractionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	virtual Bool IsAvailable() override;
};

