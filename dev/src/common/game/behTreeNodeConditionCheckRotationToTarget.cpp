#include "build.h"

#include "behTreeNodeConditionCheckRotationToTarget.h"
#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCheckRotationToCombatTargetDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionCheckRotationToCombatTargetDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCheckRotationToCombatTargetInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionCheckRotationToCombatTargetInstance::ConditionCheck()
{
	CActor* combatTarget = m_owner->GetCombatTarget().Get();
	if( combatTarget )
	{
		CActor* actor = m_owner->GetActor();
		if( actor->IsRotatedTowards( combatTarget->GetWorldPositionRef(), m_rotationTolerance ) )
		{
			return true;
		}
	}

	return false;
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCheckRotationToActionTargetDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionCheckRotationToActionTargetDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionCheckRotationToActionTargetInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionCheckRotationToActionTargetInstance::ConditionCheck()
{
	CNode* actionTarget = m_owner->GetActionTarget().Get();
	if ( actionTarget )
	{
		CActor* actor = m_owner->GetActor();
		if( actor->IsRotatedTowards( actionTarget->GetWorldPositionRef(), m_rotationTolerance ) )
		{
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////
// Named Target
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionCheckRotationToNamedTargetDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

String CBehTreeNodeConditionCheckRotationToNamedTargetDefinition::GetNodeCaption() const
{	
	String baseCaption = TBaseClass::GetNodeCaption();
	return baseCaption + TXT(" ( ") + m_targetName.GetValue().AsString() + TXT(" )");
}

Bool CBehTreeNodeConditionCheckRotationToNamedTargetInstance::ConditionCheck()
{
	CNode* actionTarget = m_owner->GetNamedTarget( m_targetName ).Get();
	if ( actionTarget )
	{
		CActor* actor = m_owner->GetActor();
		if( actor->IsRotatedTowards( actionTarget->GetWorldPositionRef(), m_rotationTolerance ) )
		{
			return true;
		}
	}

	return false;
}