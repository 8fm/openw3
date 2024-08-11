#include "build.h"
#include "behTreeNodeRiderIdleRoot.h"

////////////////////////////////////
// CBehTreeNodeRiderIdleDynamicRootDefinition
CName CBehTreeNodeRiderIdleDynamicRootDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return CNAME( AI_Load_RiderIdleRoot );
}
IAITree* CBehTreeNodeRiderIdleDynamicRootDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return context.GetVal< IAITree* >( CNAME( riderIdleTree ), m_defaultIdleTree.Get() );
}
IBehTreeNodeInstance* CBehTreeNodeRiderIdleDynamicRootDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
