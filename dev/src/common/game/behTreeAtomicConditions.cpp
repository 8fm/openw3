#include "build.h"
#include "behTreeAtomicConditions.h"

IMPLEMENT_ENGINE_CLASS( IBehTreeAtomicCondition );
IMPLEMENT_ENGINE_CLASS( IBehTreeAtomicBinaryCondition );
IMPLEMENT_ENGINE_CLASS( CBehTreeAtomicANDCondition );
IMPLEMENT_ENGINE_CLASS( CBehTreeAtomicORCondition );
IMPLEMENT_ENGINE_CLASS( CBehTreeAtomicNOTCondition );
IMPLEMENT_ENGINE_CLASS( CBehTreeAtomicTestSubtreeCondition );

////////////////////////////////////////////////////////////////////////
// IBehTreeAtomicCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicConditionInstance* IBehTreeAtomicCondition::SpawnInstance( CBehTreeSpawnContext& context )
{
	return NULL;
}

Bool IBehTreeAtomicConditionInstance::ConditionCheck( CBehTreeNodeComplexConditionInstance* node )
{
	return true;
}

////////////////////////////////////////////////////////////////////////
// IBehTreeAtomicBinaryCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicBinaryConditionInstance::IBehTreeAtomicBinaryConditionInstance( const Definition* def, CBehTreeSpawnContext& context )
	: Super( def, context )
	, m_condition1( NULL )
	, m_condition2( NULL )
{
	if ( def->m_condition1 )
	{
		m_condition1 = def->m_condition1->SpawnInstance( context );
	}
	if ( def->m_condition2 )
	{
		m_condition2 = def->m_condition2->SpawnInstance( context );
	}
}

////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicANDCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicConditionInstance* CBehTreeAtomicANDCondition::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CBehTreeAtomicANDConditionInstance( this, context );
}
Bool CBehTreeAtomicANDConditionInstance::ConditionCheck( CBehTreeNodeComplexConditionInstance* node )
{
	return m_condition1 && m_condition1->ConditionCheck( node ) && m_condition2 && m_condition2->ConditionCheck( node );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicORCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicConditionInstance* CBehTreeAtomicORCondition::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CBehTreeAtomicORConditionInstance( this, context );
}
Bool CBehTreeAtomicORConditionInstance::ConditionCheck( CBehTreeNodeComplexConditionInstance* node )
{
	return (m_condition1 && m_condition1->ConditionCheck( node )) || (m_condition2 && m_condition2->ConditionCheck( node ));
}
////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicNOTCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicConditionInstance* CBehTreeAtomicNOTCondition::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CBehTreeAtomicNOTConditionInstance( this, context );
}

CBehTreeAtomicNOTConditionInstance::CBehTreeAtomicNOTConditionInstance( const Definition* def, CBehTreeSpawnContext& context )
	: Super( def, context )
	, m_child( def->m_child ? def->m_child->SpawnInstance( context ) : NULL )
{
}

Bool CBehTreeAtomicNOTConditionInstance::ConditionCheck( CBehTreeNodeComplexConditionInstance* node )
{
	return m_child && !m_child->ConditionCheck( node );
}
////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicTestSubtreeCondition
////////////////////////////////////////////////////////////////////////
IBehTreeAtomicConditionInstance* CBehTreeAtomicTestSubtreeCondition::SpawnInstance( CBehTreeSpawnContext& context )
{
	return new CBehTreeAtomicTestSubtreeConditionInstance( this, context );
}
Bool CBehTreeAtomicTestSubtreeConditionInstance::ConditionCheck( CBehTreeNodeComplexConditionInstance* node )
{
	return node->GetDecoratorChild()->IsAvailable();
}