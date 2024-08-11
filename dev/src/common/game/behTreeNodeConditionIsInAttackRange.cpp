#include "build.h"

#include "behTreeNodeConditionIsInAttackRange.h"
#include "behTreeInstance.h"

#include "attackRange.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsInAttackRangeDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionIsInAttackRangeInstance::CBehTreeNodeConditionIsInAttackRangeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
	, m_attackRangeName( def.m_attackRangeName.GetVal( context ) )
	, m_predictPositionInTime( def.m_predictPositionInTime.GetVal( context ) )
{}

Bool CBehTreeNodeConditionIsInAttackRangeInstance::ConditionCheck()
{
	CActor* actor = m_owner->GetActor();
	const CAIAttackRange* attackRange = m_attackRange.Get();
	if ( !attackRange )
	{
		attackRange = CAIAttackRange::Get( actor, m_attackRangeName );
		if ( !attackRange )
		{
			return false;
		}
		m_attackRange = const_cast< CAIAttackRange* >( attackRange );
	}
	CActor* target = m_owner->GetCombatTarget().Get();
	if ( target && attackRange->Test( actor, target, m_predictPositionInTime ) )
	{
		return true;
	}

	return false;
}