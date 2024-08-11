#include "build.h"
#include "behTreeTicketAlgorithm.h"

#include "r4BehTreeInstance.h"
#include "behTreeTicketData.h"

IMPLEMENT_ENGINE_CLASS( IBehTreeTicketAlgorithmDefinition );
IMPLEMENT_ENGINE_CLASS( IBehTreeTicketAlgorithmDecoratorDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeTicketAlgorithmRandomizeDefinition );
IMPLEMENT_ENGINE_CLASS( IBehTreeTicketAlgorithmListDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeTicketAlgorithmListSumDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeConstantTicketAlgorithmDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeIfActiveTicketAlgorithmDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeDistanceBasedTicketAlgorithmDefinition );
IMPLEMENT_ENGINE_CLASS( CBehTreeDistanceBasedTicketAlgorithmField );
IMPLEMENT_ENGINE_CLASS( CBehTreeTimeBasedTicketAlgorithmDefinition );

////////////////////////////////////////////////////////////////////////////
// IBehTreeTicketAlgorithmDecorator
////////////////////////////////////////////////////////////////////////////

IBehTreeTicketAlgorithmDecoratorInstance::IBehTreeTicketAlgorithmDecoratorInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmInstance( definition, owner, context )
	, m_baseAlgorithm( NULL )
{
	if ( definition.m_baseAlgorithm )
	{
		m_baseAlgorithm = definition.m_baseAlgorithm->SpawnInstance( owner, context );
	}
}
IBehTreeTicketAlgorithmDecoratorInstance::~IBehTreeTicketAlgorithmDecoratorInstance()
{
	if ( m_baseAlgorithm )
	{
		delete m_baseAlgorithm;
	}
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeTicketAlgorithmRandomize
////////////////////////////////////////////////////////////////////////////
CBehTreeTicketAlgorithmRandomizeDefinition::CBehTreeTicketAlgorithmRandomizeDefinition()
	: m_randMin( 0.95f )
	, m_randMax( 1.05f )											
{
}
IBehTreeTicketAlgorithmInstance* CBehTreeTicketAlgorithmRandomizeDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeTicketAlgorithmRandomizeInstance( *this, owner, context );
}


CBehTreeTicketAlgorithmRandomizeInstance::CBehTreeTicketAlgorithmRandomizeInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmDecoratorInstance( definition, owner, context )
	, m_randMin( definition.m_randMin.GetVal( context ) )
	, m_randMax( definition.m_randMax.GetVal( context ) )
{

}

Float CBehTreeTicketAlgorithmRandomizeInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	if ( m_baseAlgorithm )
	{
		Float f = m_baseAlgorithm->CalculateTicketImportance( node, ticket, count );
		Float r = GEngine->GetRandomNumberGenerator().Get< Float >( m_randMin , m_randMax );
		return f*r;
	}
	return 0.f;
}

////////////////////////////////////////////////////////////////////////////
// IBehTreeTicketAlgorithmList
////////////////////////////////////////////////////////////////////////////

IBehTreeTicketAlgorithmListInstance::IBehTreeTicketAlgorithmListInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmInstance( definition, owner, context )
{
	m_list.Resize( definition.m_list.Size() );
	for( Uint32 i = 0; i < m_list.Size(); ++i )
	{
		m_list[ i ] = definition.m_list[ i ]->SpawnInstance( owner, context );
	}
}
IBehTreeTicketAlgorithmListInstance::~IBehTreeTicketAlgorithmListInstance()
{
	for ( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		delete *it;
	}
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeTicketAlgorithmListSum
////////////////////////////////////////////////////////////////////////////
IBehTreeTicketAlgorithmInstance* CBehTreeTicketAlgorithmListSumDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeTicketAlgorithmListSumInstance( *this, owner, context );
}

Float CBehTreeTicketAlgorithmListSumInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	Float f = 0.f;
	for ( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		f += (*it)->CalculateTicketImportance( node, ticket, count );
	}
	return f;
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeConstantTicketAlgorithm
////////////////////////////////////////////////////////////////////////////
IBehTreeTicketAlgorithmInstance* CBehTreeConstantTicketAlgorithmDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeConstantTicketAlgorithmInstance( *this, owner, context );
}

Float CBehTreeConstantTicketAlgorithmInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	return m_importance;
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeIfActiveTicketAlgorithm
////////////////////////////////////////////////////////////////////////////
IBehTreeTicketAlgorithmInstance* CBehTreeIfActiveTicketAlgorithmDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeIfActiveTicketAlgorithmInstance( *this, owner, context );
}

Float CBehTreeIfActiveTicketAlgorithmInstance::CalculateTicketImportance( IBehTreeNodeInstance* node,  const CBehTreeTicketData& ticket, Uint16& count )
{
	return node->IsActive() ? m_importance : 0.f;
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeDistanceBasedTicketAlgorithm
////////////////////////////////////////////////////////////////////////////
IBehTreeTicketAlgorithmInstance* CBehTreeDistanceBasedTicketAlgorithmDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeDistanceBasedTicketAlgorithmInstance( *this, owner, context );
}

CBehTreeDistanceBasedTicketAlgorithmInstance::CBehTreeDistanceBasedTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmInstance( definition, owner, context )
{
	m_distanceToImportance.Resize( definition.m_distanceToImportance.Size() );
	Float mult = definition.m_importanceMultiplier.GetVal( context );
	for ( Uint32 i = 0, n = m_distanceToImportance.Size(); i != n; ++i )
	{
		m_distanceToImportance[ i ].X = definition.m_distanceToImportance[ i ].m_distance.GetVal( context );
		m_distanceToImportance[ i ].Y = definition.m_distanceToImportance[ i ].m_importance * mult;
	}
}

Float CBehTreeDistanceBasedTicketAlgorithmInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	CBehTreeInstance* owner = node->GetOwner();
	CActor* target = owner->GetCombatTarget().Get();
	if ( !target )
	{
		return 0.f;
	}
	CActor* actor = owner->GetActor();
	if ( !actor )
	{
		return 0.f;
	}

	if ( m_distanceToImportance.Empty() )
	{
		return 0.f;
	}

	Float dist = (target->GetWorldPositionRef() - actor->GetWorldPositionRef()).Mag3();

	if ( dist <= m_distanceToImportance[ 0 ].X )
	{
		return m_distanceToImportance[ 0 ].Y;
	}

	for ( Uint32 i = 1, n = m_distanceToImportance.Size(); i < n; ++i )
	{
		if ( dist <= m_distanceToImportance[ i ].X )
		{
			Float ratio = (dist - m_distanceToImportance[ i-1 ].X) / (m_distanceToImportance[ i ].X - m_distanceToImportance[ i-1 ].X);
			return m_distanceToImportance[ i-1 ].Y + ratio * (m_distanceToImportance[ i ].Y - m_distanceToImportance[ i-1 ].Y);
		}
	}
	return m_distanceToImportance.Back().Y;
}

////////////////////////////////////////////////////////////////////////////
// CBehTreeTimeBasedTicketAlgorithm
////////////////////////////////////////////////////////////////////////////
IBehTreeTicketAlgorithmInstance* CBehTreeTimeBasedTicketAlgorithmDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return new CBehTreeTimeBasedTicketAlgorithmInstance( *this, owner, context );
}

CBehTreeTimeBasedTicketAlgorithmInstance::CBehTreeTimeBasedTicketAlgorithmInstance( const Definition& definition, CBehTreeInstance* owner, CBehTreeSpawnContext& context )
	: IBehTreeTicketAlgorithmInstance( definition, owner, context )
	, m_importanceMultiplier( definition.m_importanceMultiplier.GetVal( context ) )
{}

Float CBehTreeTimeBasedTicketAlgorithmInstance::CalculateTicketImportance( IBehTreeNodeInstance* node, const CBehTreeTicketData& ticket, Uint16& count )
{
	return ticket.GetTimeSinceMyLastAquisition() * m_importanceMultiplier;
}