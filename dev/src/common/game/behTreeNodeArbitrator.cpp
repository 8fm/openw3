#include "build.h"
#include "behTreeNodeArbitrator.h"

IMPLEMENT_ENGINE_CLASS( SArbitratorQueryData );
IMPLEMENT_RTTI_ENUM( EArbitratorPriorities );
IMPLEMENT_RTTI_ENUM( ETopLevelAIPriorities );


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeArbitratorDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance* CBehTreeNodeArbitratorDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new CBehTreeNodeArbitratorInstance( *this, owner, context, parent );
}
CBehTreeNodeArbitratorDefinition::CBehTreeNodeArbitratorDefinition()
{
	m_checkFrequency = 60.f;
	m_useScoring = true;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeArbitratorInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeArbitratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_ArbitratorQuery ) && e.m_eventType == BTET_GameplayEvent )
	{
		SArbitratorQueryData* data = e.m_gameplayEventData.Get< SArbitratorQueryData >();
		if ( data )
		{
			Priority currentPriority = 0;
			if ( m_activeChild != INVALID_CHILD )
			{
				currentPriority = Priority( m_children[ m_activeChild ]->Evaluate() );
			}
			data->m_queryResult = currentPriority < data->m_priority;
		}
		return false;
	}
	return Super::OnEvent( e );
}