/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeDecoratorPassMetaobstaclesInstance;

class CBehTreeNodeDecoratorPassMetaobstaclesDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorPassMetaobstaclesDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorPassMetaobstaclesInstance, PassMetaobstacles );

protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorPassMetaobstaclesDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
END_CLASS_RTTI();


class CBehTreeNodeDecoratorPassMetaobstaclesInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

public:
	typedef CBehTreeNodeDecoratorPassMetaobstaclesDefinition Definition;

	CBehTreeNodeDecoratorPassMetaobstaclesInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )										{}

	Bool Activate() override;
	void Deactivate() override;
};