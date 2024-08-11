#include "build.h"

#include "isConsciousAtWork.h"
#include "behTreeInstance.h"

IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsConsciousAtWorkDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeNodeConditionIsConsciousAtWorkInstance::ConditionCheck()
{
	CBehTreeWorkData* workData = m_workData.Get();
	return workData && workData->IsBeingPerformed() && workData->GetIsConscious();
}

IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsWorkingDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

Bool CBehTreeNodeConditionIsWorkingInstance::ConditionCheck()
{
	CBehTreeWorkData* workData = m_workData.Get();
	return workData && workData->IsBeingPerformed();
}