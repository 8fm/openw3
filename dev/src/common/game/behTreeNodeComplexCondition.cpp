#include "build.h"
#include "behTreeNodeComplexCondition.h"

#include "behTreeAtomicConditions.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeComplexConditionDefinition
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeComplexConditionDefinition::IsValid() const
{
	if ( m_condition == nullptr )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
IBehTreeNodeDecoratorInstance*	CBehTreeNodeComplexConditionDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeNodeComplexConditionInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeComplexConditionInstance::ConditionCheck()
{
	if ( m_condition )
	{
		return m_condition->ConditionCheck( this );
	}
	return false;
}

CBehTreeNodeComplexConditionInstance::CBehTreeNodeComplexConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_condition( NULL )
{
	if ( def.m_condition )
	{
		m_condition = def.m_condition->SpawnInstance( context );
	}
}
