/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "behTreeNodePartyWorkSynchronizer.h"
#include "behTreeInstance.h"
#include "encounterCreaturePool.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodePartyWorkSynchronizerDecoratorDefinition )

RED_DEFINE_STATIC_NAME( AI_IsWaitingForOthersToWork );

const Float CBehTreeNodePartyWorkSynchronizerDecoratorInstance::SYNCHRONIZATION_CHECK_TIME_LIMIT_MAX = 1.5f;
const Float CBehTreeNodePartyWorkSynchronizerDecoratorInstance::SYNCHRONIZATION_CHECK_TIME_LIMIT_MIN = 0.5f;

CBehTreeNodePartyWorkSynchronizerDecoratorInstance::CBehTreeNodePartyWorkSynchronizerDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, m_synchronizationEnabled( false )
	, m_prevSynchronizationCheckResult( false )
	, m_cachedPartyMember( nullptr )
	, m_nextSynchronizationTime( 0.f )
{
	m_encounter = CEncounterParameters::GetEncounter( context );

	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_EnablePartyWorkSynchronization );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}

	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_DisablePartyWorkSynchronization );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}


Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	if( e.m_eventName == CNAME( AI_EnablePartyWorkSynchronization ) )
	{
		m_synchronizationEnabled = true;
	}
	else if( e.m_eventName == CNAME( AI_DisablePartyWorkSynchronization ) )
	{
		m_synchronizationEnabled = false;
	}
	
	return Super::OnListenedEvent( e );
}

void CBehTreeNodePartyWorkSynchronizerDecoratorInstance::OnDestruction() 
{
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_EnablePartyWorkSynchronization );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = CNAME( AI_DisablePartyWorkSynchronization );
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::Activate()
{
	if( !m_synchronizationEnabled )
	{
		return Super::Activate();		
	}

	CEncounter* encounter = m_encounter.Get();
	if( !encounter )
	{
		// no encounter, no synchronization ( quest npcs )
		m_synchronizationEnabled = false;
	}

	if( !m_synchronizationEnabled )
	{
		return Super::Activate();		
	}

	CEncounterCreaturePool& creaturePool = m_encounter.Get()->GetCreaturePool();

	const CEncounterCreaturePool::Party* party = creaturePool.GetPartiesManager().GetParty( m_owner->GetActor() );

	// if no party nor synchronization enabled
	if( !party || !m_synchronizationEnabled )
	{
		// no party, no synchronization
		m_synchronizationEnabled = false;

		//activate child
		return Super::Activate();
	}	
	// if other party members are working, this npc can also work
	if( IfRestOfPartyIsWorking() )
	{
		return Super::Activate();		
	}

	// else, postopne activation of child
	return IBehTreeNodeInstance::Activate();	
}

void CBehTreeNodePartyWorkSynchronizerDecoratorInstance::Update()
{
	if( !m_synchronizationEnabled )
	{
		Super::Update();
		return;
	}
	if( IfRestOfPartyIsWorking() )
	{
		// if was activated, just keep updating
		if( m_child->IsActive() )
		{
			Super::Update();			
		}
		else
		{			
			m_child->Activate();
		}
	}
	else
	{		
		if( m_child->IsActive() )
		{
			// some more updates needed
			if( !m_child->Interrupt() )
			{
				m_child->Update();
			}
		}
	}
}

void CBehTreeNodePartyWorkSynchronizerDecoratorInstance::Deactivate()
{
	if( m_child->IsActive() )
	{
		m_child->Deactivate();
	}
	
	IBehTreeNodeInstance::Deactivate();
}

Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::Interrupt()
{
	if( m_child->IsActive() )
	{
		if( !m_child->Interrupt() )
		{
			return false;
		}
	}
	Deactivate();	
	return true;
}

Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_IsWaitingForOthersToWork ) )
	{
		SGameplayEventParamInt* params = e.m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( params )
		{
			params->m_value = 1;
		}
		return false;
	}

	return Super::OnEvent( e );
}

Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::IfRestOfPartyIsWorking()
{
	if( m_nextSynchronizationTime < m_owner->GetLocalTime() )
	{
		m_prevSynchronizationCheckResult = IfRestOfPartyIsWorkingImpl();

		m_nextSynchronizationTime = m_owner->GetLocalTime() + GEngine->GetRandomNumberGenerator().Get< Float >( SYNCHRONIZATION_CHECK_TIME_LIMIT_MIN, SYNCHRONIZATION_CHECK_TIME_LIMIT_MAX );
	}

	return m_prevSynchronizationCheckResult;
}

Bool CBehTreeNodePartyWorkSynchronizerDecoratorInstance::IfRestOfPartyIsWorkingImpl()
{	
	CEncounter* encounter = m_encounter.Get();
	if( !encounter )
	{
		return true;
	}

	CEncounterCreaturePool& creaturePool = encounter->GetCreaturePool();

	const CEncounterCreaturePool::Party* party = creaturePool.GetPartiesManager().GetParty( m_owner->GetActor() );
	if( !party )
	{
		return true;
	}

	for( Uint32 i =0; i<party->Size(); ++i )
	{
		const CEncounterCreaturePool::CPartiesManager::PartyMember& member = party->operator[]( i );
		if( !member.m_actor )
		{
			return false;
		}
		if( !( member.m_actor->IsAtWork() && member.m_actor->SignalGameplayEventReturnInt( CNAME( AI_IsWaitingForOthersToWork ), false ) == 1 ) )
		{
			return false;
		}
	}
	return true;
}