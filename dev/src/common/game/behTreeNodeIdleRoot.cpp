#include "build.h"
#include "behTreeNodeIdleRoot.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeIdleDynamicRootDefinition )

/////////////////////////////////////////////////////////////
// CBehTreeNodeBaseIdleDynamicRootInstance
void CBehTreeNodeBaseIdleDynamicRootInstance::RequestChildDespawn()
{
	m_isBeingInterrupted = true;
}
Bool CBehTreeNodeBaseIdleDynamicRootInstance::IsAvailable()
{
	if ( m_childNode == nullptr || m_isBeingInterrupted )
	{
		DebugNotifyAvailableFail();
		return false;
	}
	return true;
}

Int32 CBehTreeNodeBaseIdleDynamicRootInstance::Evaluate()
{
	if ( m_childNode == nullptr || m_isBeingInterrupted )
	{
		DebugNotifyAvailableFail();
		return -1;
	}
	return m_priority;
}
void CBehTreeNodeBaseIdleDynamicRootInstance::Deactivate()
{
	if ( m_isBeingInterrupted )
	{
		m_isBeingInterrupted = false;
		Super::Deactivate();

		if ( m_childNode )
		{
			m_childNode->RegisterForDeletion();
			m_childNode = nullptr;
		}

		return;
	}
	Super::Deactivate();
}

////////////////////////////////////////////////////////////
// CBehTreeNodeIdleDynamicRootDefinition
CName CBehTreeNodeIdleDynamicRootDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return CNAME( AI_Load_IdleRoot );
}
IAITree* CBehTreeNodeIdleDynamicRootDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return context.GetVal< IAITree* >( CNAME( idleTree ), m_defaultIdleTree.Get() );
}
