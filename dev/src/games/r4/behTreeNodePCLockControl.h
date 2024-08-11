/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDecorator.h"

class CBehTreeNodePCLockControlDecoratorInstance;

class CBehTreeNodePCLockControlDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodePCLockControlDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodePCLockControlDecoratorInstance, PC_LockControl );
protected:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodePCLockControlDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();

class CBehTreeNodePCLockControlDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef CBehTreeNodePCLockControlDecoratorDefinition Definition;

	CBehTreeNodePCLockControlDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )										{}

	Bool Activate() override;
	void Deactivate() override;
};

