#include "build.h"
#include "behTreeDecoratorSetBehaviourVariable.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "../../common/game/behTreeInstance.h"


/////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorSetBehaviorVariableDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableInstance
Bool CBehTreeDecoratorSetBehaviorVariableInstance::Activate()
{
	if ( Super::Activate() == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( m_setVarActivate )
	{
		CActor*const actor = m_owner->GetActor();
		actor->SetBehaviorVariable( m_varName, m_valueActivate );
	}

	return true;
}

void CBehTreeDecoratorSetBehaviorVariableInstance::Deactivate()
{
	if ( m_setVarDeactivate )
	{
		CActor*const actor	= m_owner->GetActor();
		if ( actor )
		{
			actor->SetBehaviorVariable( m_varName, m_valueDeactivate );
		}
	}
	Super::Deactivate();
}


/////////////////////////////////////////////////////////////
// CBehTreeDecoratorSetBehaviorVariableDefinition
IBehTreeNodeDecoratorInstance* CBehTreeDecoratorOverrideBehaviorVariableDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

/////////////////////////////////////////////////////////////
// CBehTreeDecoratorOverrideBehaviorVariableInstance
Bool CBehTreeDecoratorOverrideBehaviorVariableInstance::Activate()
{
	if ( Super::Activate() == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	CNewNPC* const npc							= GetOwner()->GetNPC();
	m_previousValue = npc->GetBehaviorFloatVariable( m_varName );
	npc->SetBehaviorVariable( m_varName, m_value );
	return true;
}

void CBehTreeDecoratorOverrideBehaviorVariableInstance::Deactivate()
{
	CNewNPC*const npc							= GetOwner()->GetNPC();
	npc->SetBehaviorVariable( m_varName, m_previousValue );
	Super::Deactivate();
}