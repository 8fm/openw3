#pragma once

#include "behTreeDecorator.h"

class CBehTreeDecoratorUninterruptableInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorUninterruptableDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorUninterruptableDefinition, IBehTreeNodeDecoratorDefinition,CBehTreeDecoratorUninterruptableInstance, Uninterruptable );

public:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeDecoratorUninterruptableDefinition);
PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeDecoratorUninterruptableInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
	typedef CBehTreeDecoratorUninterruptableDefinition Definition;

public:
	CBehTreeDecoratorUninterruptableInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent )	{}

	Bool Interrupt() override;
};
