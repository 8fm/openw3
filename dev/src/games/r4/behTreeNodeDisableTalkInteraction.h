/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDecorator.h"

class CBehTreeNodeDecoratorDisableTalkInteractionInstance;

class CBehTreeNodeDecoratorDisableTalkInteractionDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorDisableTalkInteractionDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorDisableTalkInteractionInstance, DisableTalkInteraction )
protected:

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorDisableTalkInteractionDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorDisableTalkInteractionInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	THandle< CInteractionComponent >			m_cachedInteraction;

	CInteractionComponent*			GetInteractionComponent();
public:
	typedef CBehTreeNodeDecoratorDisableTalkInteractionDefinition Definition;

	CBehTreeNodeDecoratorDisableTalkInteractionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )								{}

	Bool Activate() override;
	void Deactivate() override;
};