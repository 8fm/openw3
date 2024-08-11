#include "build.h"

#include "behTreeNodeAtomicIdle.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicIdleDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeAtomicIdleDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCompleteImmediatelyDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeCompleteImmediatelyDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCompleteImmediatelyInstance
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeCompleteImmediatelyInstance::Update()
{
	Complete( m_reportSuccess ? BTTO_SUCCESS : BTTO_FAILED );
}