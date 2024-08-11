#pragma once

#include "behTreeDecorator.h"
#include "behTreeNodeAtomicAction.h"
#include "behTreeNodeSelectWanderingTarget.h"

class CPatrolPointComponent;
class CBehTreeNodeSelectPatrolingTargetDecoratorInstance;

class CBehTreeNodeSelectPatrolingTargetDecoratorDefinition : public CBehTreeNodeRandomWanderingTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectPatrolingTargetDecoratorDefinition, CBehTreeNodeRandomWanderingTargetDefinition, CBehTreeNodeSelectPatrolingTargetDecoratorInstance, SelectPatrollingTargetDecorator );	
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectPatrolingTargetDecoratorDefinition );
	PARENT_CLASS( CBehTreeNodeRandomWanderingTargetDefinition );
END_CLASS_RTTI();


class CBehTreeNodeSelectPatrolingTargetDecoratorInstance : public CBehTreeNodeRandomWanderingTargetInstance
{
	typedef CBehTreeNodeRandomWanderingTargetInstance Super;
public:
	typedef CBehTreeNodeSelectPatrolingTargetDecoratorDefinition Definition;

	CBehTreeNodeSelectPatrolingTargetDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeRandomWanderingTargetInstance( def, owner, context, parent ) {}

	Bool Activate() override;
};
