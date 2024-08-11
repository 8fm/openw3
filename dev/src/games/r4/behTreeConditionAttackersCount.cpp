/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeConditionAttackersCount.h"

#include "r4BehTreeInstance.h"
#include "combatDataComponent.h"


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionAttackersCountDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionAttackersCountDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}



////////////////////////////////////////////////////////////////////////
// CBehTreeConditionAttackersCountInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeConditionAttackersCountInstance::ConditionCheck()
{
	CCombatDataComponent* targetData = CR4BehTreeInstance::Get( m_owner )->GetCombatTargetData();
	if ( targetData )
	{
		Int32 attackersCount = targetData->GetAttackersCount();
		switch ( m_compare )
		{
		default:
			RED_HALT( "Unexpected enumerat value!" );
		case CF_Equal:
			return attackersCount == m_count;
		case CF_NotEqual:
			return attackersCount != m_count;
		case CF_Less:
			return attackersCount < m_count;
		case CF_LessEqual:
			return attackersCount <= m_count;
		case CF_Greater:
			return attackersCount > m_count;
		case CF_GreaterEqual:
			return attackersCount >= m_count;
		}
	}
	return false;
}