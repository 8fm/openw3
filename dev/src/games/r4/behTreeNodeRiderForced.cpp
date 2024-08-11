#include "build.h"
#include "behTreeNodeRiderForced.h"


#include "../../common/game/behTreeDynamicNodeEvent.h"
#include "../../common/game/behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderForcedBehaviorDefinition
////////////////////////////////////////////////////////////////////////
CName CBehTreeNodeRiderForcedBehaviorDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return CNAME( AI_Rider_Load_Forced );
}

CName CBehTreeNodeRiderForcedBehaviorDefinition::GetCancelEventName( ) const
{
	return CNAME( AI_Rider_Forced_Cancel );
}

IBehTreeNodeInstance* CBehTreeNodeRiderForcedBehaviorDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeRiderForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeRiderForcedBehaviorInstance::CBehTreeNodeRiderForcedBehaviorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
{
}