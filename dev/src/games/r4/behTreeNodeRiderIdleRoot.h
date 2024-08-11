#pragma once

#include "../../common/game/behTreeNodeIdleRoot.h"

class CBehTreeNodeRiderIdleDynamicRootInstance;

class CBehTreeNodeRiderIdleDynamicRootDefinition : public CBehTreeNodeBaseIdleDynamicRootDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRiderIdleDynamicRootDefinition, CBehTreeNodeBaseIdleDynamicRootDefinition, CBehTreeNodeRiderIdleDynamicRootInstance, RiderIdleRootDynamic );
protected:
	
public:
	CBehTreeNodeRiderIdleDynamicRootDefinition() {}

	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	IAITree* GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeRiderIdleDynamicRootDefinition );
	PARENT_CLASS( CBehTreeNodeBaseIdleDynamicRootDefinition );
END_CLASS_RTTI();


class CBehTreeNodeRiderIdleDynamicRootInstance : public CBehTreeNodeBaseIdleDynamicRootInstance
{
	typedef CBehTreeNodeBaseIdleDynamicRootInstance Super;
public:
	typedef CBehTreeNodeRiderIdleDynamicRootDefinition Definition;

	CBehTreeNodeRiderIdleDynamicRootInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeBaseIdleDynamicRootInstance( def, owner, context, parent )	{}
};

