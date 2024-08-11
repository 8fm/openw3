#include "build.h"
#include "questTriggerCondition.h"
#include "../../common/engine/triggerActivatorComponent.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CQuestTriggerCondition )

CQuestTriggerCondition::CQuestTriggerCondition()
	: m_tag( CNAME( PLAYER ) )
	, m_isFulfilled( false )
	, m_wasRegistered( false )
{
}

CQuestTriggerCondition::~CQuestTriggerCondition()
{
	RegisterCallback( false );
}

void CQuestTriggerCondition::OnActivate()
{
	TBaseClass::OnActivate();

	CollectTriggers();
	CollectActivators();

	m_wasRegistered = false;
	RegisterCallback( true );
}

void CQuestTriggerCondition::OnDeactivate()
{
	RegisterCallback( false );
	m_wasRegistered = false;

	while ( m_triggers.Size() > 0 )
	{
		RemoveTrigger( m_triggers[0] );
	}

	while ( m_activators.Size() > 0 )
	{
		RemoveActivator( m_activators[0] );
	}

	TBaseClass::OnDeactivate();
}

Bool CQuestTriggerCondition::OnIsFulfilled()
{
	if ( !m_isFulfilled && !m_wasRegistered )
	{
		RegisterCallback( true );
	}	
	RemoveUnusedComponents();
	return m_isFulfilled;
}

void CQuestTriggerCondition::RemoveUnusedComponents()
{
	{
		TTriggers::iterator itEnd = m_triggersToRemove.End();
		for ( TTriggers::iterator it = m_triggersToRemove.Begin(); it != itEnd; ++it )
		{
			RemoveTrigger( *it );
		}
		m_triggersToRemove.ClearFast();
	}

	{
		TActivators::iterator itEnd = m_activatorsToRemove.End();
		for ( TActivators::iterator it = m_activatorsToRemove.Begin(); it != itEnd; ++it )
		{
			RemoveActivator( *it );
		}
		m_activatorsToRemove.ClearFast();
	}
}

Bool CQuestTriggerCondition::CollectTriggers()
{
	CWorld *world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( world == nullptr )
	{
		return false;
	}
	m_triggers.ClearFast();

	m_collectingTriggers = true;
	world->GetTagManager()->IterateTaggedNodes( m_triggerTag, *this );

	return !m_triggers.Empty();
}

Bool CQuestTriggerCondition::CollectActivators()
{
	CWorld *world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( world == nullptr )
	{
		return false;
	}
	m_activators.ClearFast();

	m_collectingTriggers = false;
	world->GetTagManager()->IterateTaggedNodes( m_tag, *this );

	return m_activators.Empty();
}

Bool CQuestTriggerCondition::AddTrigger( const TTrigger& trigger )
{
	TTriggerPtr triggerPtr = trigger.Get();
	if ( triggerPtr == nullptr )
	{
		return false;
	}
	if ( m_triggers.PushBackUnique( trigger ) )
	{
		triggerPtr->AddIncludedChannel( TC_Quest );
		triggerPtr->AddListener( this );
		return true;
	}
	return false;
}

Bool CQuestTriggerCondition::RemoveTrigger( const TTrigger& trigger )
{
	if ( m_triggers.Exist( trigger ) )
	{
		TTriggerPtr triggerPtr = trigger.Get();
		if ( triggerPtr != nullptr )
		{
			triggerPtr->RemoveListener( this );
		}
		return m_triggers.Remove( trigger );
	}
	return false;
}

Bool CQuestTriggerCondition::AddActivator( const TActivator& activator )
{
	TActivatorPtr activatorPtr = activator.Get();
	if ( activatorPtr == nullptr )
	{
		return false;
	}
	if ( activatorPtr->IsA< CMovingAgentComponent >() )
	{
		if ( m_activators.PushBackUnique( activator ) )
		{
			static_cast< CMovingAgentComponent* >( activatorPtr )->AddTriggerActivatorChannel( TC_Quest );
			return true;
		}
	}
	else if ( activatorPtr->IsA< CTriggerActivatorComponent >() )
	{
		if ( m_activators.PushBackUnique( activator ) )
		{
			static_cast< CTriggerActivatorComponent* >( activatorPtr )->AddTriggerChannel( TC_Quest );
			return true;
		}
	}		
	return false;
}

Bool CQuestTriggerCondition::RemoveActivator( const TActivator& activator )
{
	return m_activators.Remove( activator );
}

void CQuestTriggerCondition::AddActivatorEntity( CEntity* activatorEntity )
{
	RED_ASSERT( activatorEntity != nullptr );
	CComponent* component = activatorEntity->FindComponent< CMovingAgentComponent >();
	if ( component == nullptr )
	{
		component = activatorEntity->FindComponent< CTriggerActivatorComponent >();
	}
	if ( component == nullptr )
	{
		component = static_cast< CTriggerActivatorComponent* >( activatorEntity->CreateComponent( CTriggerActivatorComponent::GetStaticClass(), SComponentSpawnInfo() ) );
	}
	if ( component != nullptr )
	{
		AddActivator( component );
	}
}

void CQuestTriggerCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( !GGame->IsActive() )
	{
		return;
	}

	if ( eventType == GET_TriggerCreated )
	{
		CComponent* component = param.Get< CComponent* >();
		CTriggerAreaComponent* trigger = component != nullptr ? Cast< CTriggerAreaComponent >( component ) : nullptr;
		if ( trigger != nullptr && trigger->GetParent()->IsA< CEntity >() && trigger->GetEntity()->GetTags().HasTag( m_triggerTag ) )
		{
			if ( AddTrigger( trigger ) )
			{
				OnTriggerAdded( trigger );
			}
		}
	}
	else if ( eventType == GET_TriggerRemoved )
	{
		CComponent* component = param.Get< CComponent* >();
		CTriggerAreaComponent* trigger = component != nullptr ? Cast< CTriggerAreaComponent >( component ) : nullptr;
		if ( trigger != nullptr )
		{
			if ( RemoveTrigger( trigger ) )
			{
				OnTriggerRemoved( trigger );
			}
		}
	}
	else if ( eventType == GET_TriggerActivatorCreated )
	{
		CComponent* activator = param.Get< CComponent* >();
		if ( activator != nullptr && activator->GetParent()->IsA< CEntity >() && activator->GetEntity()->GetTags().HasTag( m_tag ) )
		{
			if ( AddActivator( activator ) )
			{
				OnActivatorAdded( activator );
			}
		}
	}
	else if ( eventType == GET_TriggerActivatorRemoved )
	{
		CComponent* activator = param.Get< CComponent* >();
		if ( activator != nullptr )
		{
			if ( RemoveActivator( activator ) )
			{
				OnActivatorRemoved( activator );
			}
		}
	}
	else if ( eventType == GET_TagAdded )
	{
		if ( param.Get< CName >() == m_tag )
		{
			TDynArray< CEntity* > entities;
			GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( m_tag, entities );
			TDynArray< CEntity* >::iterator itEnd = entities.End();
			for ( TDynArray< CEntity* >::iterator it = entities.Begin(); it != itEnd; ++it )
			{
				AddActivatorEntity( *it );
			}
		}
	}
}

Bool CQuestTriggerCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddListener( GEC_Trigger, this );
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_tag );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveListener( GEC_Trigger, this );
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_tag );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

Bool CQuestTriggerCondition::IsTriggerActivated( const TTrigger& trigger, const TActivator& skipActivator /* = nullptr */ )
{
	if ( trigger.Get() == nullptr )
	{
		return false;
	}
	TActivators::iterator activatorItEnd = m_activators.End();
	for ( TActivators::iterator activatorIt = m_activators.Begin(); activatorIt != activatorItEnd; ++activatorIt )
	{
		if ( skipActivator == *activatorIt )
		{
			continue;
		}
		if ( activatorIt->Get() == nullptr )
		{
			m_activatorsToRemove.PushBackUnique( *activatorIt );
			continue;
		}
		if ( IsInside( trigger, *activatorIt ) )
		{
			return true;
		}
	}
	return false;
}

Bool CQuestTriggerCondition::IsInside( const TTrigger& trigger, const TActivator& activator )
{
	TTriggerPtr triggerPtr = trigger.Get();
	TActivatorPtr activatorPtr = activator.Get();
	if ( triggerPtr == nullptr || activatorPtr == nullptr )
	{
		return false;
	}
	if ( activatorPtr->IsA< CMovingAgentComponent >() )
	{
		return static_cast< CMovingAgentComponent* >( activatorPtr )->IsInsideTrigger( trigger->GetTrigggerObject() );
	}
	else if ( activator->IsA< CTriggerActivatorComponent >() )
	{
		return static_cast< CTriggerActivatorComponent* >( activatorPtr )->IsInsideTrigger( trigger->GetTrigggerObject() );
	}	
	return false;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestInsideTriggerCondition );

CQuestInsideTriggerCondition::CQuestInsideTriggerCondition()
	: m_isInside( true )
{
}

void CQuestInsideTriggerCondition::OnActivate()
{
	TBaseClass::OnActivate();
	UpdateCondition();
}

void CQuestInsideTriggerCondition::UpdateCondition()
{
	TTriggers::iterator triggerItEnd = m_triggers.End();
	for ( TTriggers::iterator triggerIt = m_triggers.Begin(); triggerIt != triggerItEnd; ++triggerIt )
	{
		if ( triggerIt->Get() == nullptr )
		{
			m_triggersToRemove.PushBackUnique( *triggerIt );
			continue;
		}
		if ( IsTriggerActivated( *triggerIt ) == m_isInside )
		{
			m_isFulfilled = true;
			return;
		}
	}
}

void CQuestInsideTriggerCondition::OnTriggerAdded( const TTrigger& trigger )
{
	if ( !m_activators.Empty() && IsTriggerActivated( trigger ) == m_isInside )
	{
		m_isFulfilled = true;
	}
}

void CQuestInsideTriggerCondition::OnActivatorAdded( const TActivator& activator )
{
	UpdateCondition();
}

void CQuestInsideTriggerCondition::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( m_activators.FindPtr( activator ) != nullptr )
	{
		// after each OnAreaEnter trigger is activated
		m_isFulfilled = m_isInside;
	}
}

void CQuestInsideTriggerCondition::OnAreaExit( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( m_activators.FindPtr( activator ) != nullptr )
	{
		// if after leaving area trigger is no more activated
		if ( !IsTriggerActivated( area, activator ) )
		{
			m_isFulfilled = !m_isInside;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestEnterTriggerCondition );

CQuestEnterTriggerCondition::CQuestEnterTriggerCondition()
	: m_onAreaEntry( true )
{
}

void CQuestEnterTriggerCondition::OnActivate()
{
	TBaseClass::OnActivate();

	RED_ASSERT( m_states.Size() == 0 );

	TTriggers::iterator triggerItEnd = m_triggers.End();
	for ( TTriggers::iterator triggerIt = m_triggers.Begin(); triggerIt != triggerItEnd; ++triggerIt )
	{
		m_states.Insert( *triggerIt, IsTriggerActivated( *triggerIt ) );
	}
}

void CQuestEnterTriggerCondition::OnTriggerAdded( const TTrigger& trigger )
{
	m_states.Insert( trigger, IsTriggerActivated( trigger ) );
}

void CQuestEnterTriggerCondition::OnTriggerRemoved( const TTrigger& trigger )
{
	TTriggersStates::iterator it = m_states.Find( trigger );
	if ( it != m_states.End() )
	{
		m_states.Erase( it );
	}
}

void CQuestEnterTriggerCondition::OnActivatorAdded( const TActivator& activator )
{
	TTriggers::iterator triggerItEnd = m_triggers.End();
	for ( TTriggers::iterator triggerIt = m_triggers.Begin(); triggerIt != triggerItEnd; ++triggerIt )
	{
		TTriggersStates::iterator it = m_states.Find( *triggerIt );
		if ( it == m_states.End() )
		{
			// we shouldn't be here
			RED_ASSERT( false );
			m_states.Insert( *triggerIt, IsTriggerActivated( *triggerIt ) );
		}
		else
		{
			Bool isInside = IsTriggerActivated( *triggerIt );
			if ( m_onAreaEntry && isInside && !it->m_second )
			{
				m_isFulfilled = true;
			}
			else if ( !m_onAreaEntry && !isInside && it->m_second )
			{
				m_isFulfilled = true;
			}
			it->m_second = isInside;
		}
	}
}

void CQuestEnterTriggerCondition::RemoveUnusedComponents()
{
	TTriggers::iterator itEnd = m_triggersToRemove.End();
	for ( TTriggers::iterator it = m_triggersToRemove.Begin(); it != itEnd; ++it )
	{
		m_states.Erase( *it );
	}
	TBaseClass::RemoveUnusedComponents();
}

void CQuestEnterTriggerCondition::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( m_activators.FindPtr( activator ) != nullptr )
	{
		TTriggersStates::iterator it = m_states.Find( area );
		// we have such a trigger
		if ( it != m_states.End() )
		{
			// it wasn't activated before and condition is fulfilled on entry
			if ( m_onAreaEntry && !it->m_second )
			{
				m_isFulfilled = true;
			}
			it->m_second = true;
		}
		else
		{
			RED_ASSERT( false );
		}
	}
}

void CQuestEnterTriggerCondition::OnAreaExit( CTriggerAreaComponent* area, CComponent* activator )
{
	if ( m_activators.FindPtr( activator ) != nullptr )
	{
		TTriggersStates::iterator it = m_states.Find( area );
		// we have such a trigger
		if ( it != m_states.End() )
		{
			Bool isInside = IsTriggerActivated( area, activator );
			// it was activated before and condition is fulfilled on exit
			if ( !m_onAreaEntry && !isInside && it->m_second )
			{
				m_isFulfilled = true;
			}
			it->m_second = isInside;
		}
	}
}
