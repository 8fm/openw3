#pragma once


#include "../../common/game/behTreeNodeForced.h"

class CBehTreeNodeRiderForcedBehaviorInstance;


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderForcedBehaviorDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeRiderForcedBehaviorDefinition : public CBehTreeNodeBaseForcedBehaviorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeRiderForcedBehaviorDefinition, CBehTreeNodeBaseForcedBehaviorDefinition, CBehTreeNodeRiderForcedBehaviorInstance, RiderForcedBehavior );
public:
	CBehTreeNodeRiderForcedBehaviorDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;

	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	CName GetCancelEventName( ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeRiderForcedBehaviorDefinition );
	PARENT_CLASS( CBehTreeNodeBaseForcedBehaviorDefinition );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeRiderForcedBehaviorInstance : public CBehTreeNodeBaseForcedBehaviorInstance
{
private:
	typedef CBehTreeNodeBaseForcedBehaviorInstance Super;

public:
	typedef CBehTreeNodeRiderForcedBehaviorDefinition Definition;

	CBehTreeNodeRiderForcedBehaviorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};