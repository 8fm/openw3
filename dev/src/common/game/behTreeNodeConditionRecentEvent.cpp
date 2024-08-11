/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionRecentEvent.h"

#include "behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance
////////////////////////////////////////////////////////////////////////
Bool IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance::ConditionCheck()
{
	if ( m_lastEvent == 0.f || (m_activationTimeout >= 0.f && m_lastEvent + m_activationTimeout < m_owner->GetLocalTime()) )
	{
		return false;
	}
	return true;
}

IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance::IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_activationTimeout( def.m_activationTimeout )
	, m_lastEvent( 0.f )
{
}

void IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance::Deactivate()
{
	// reset event!
	m_lastEvent = 0.f;

	Super::Deactivate();
}

Bool IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance::OnListenedEvent( CBehTreeEvent& e )
{
	m_lastEvent = m_owner->GetLocalTime();
	return !m_isActive;
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasEventFiredRecentlyDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeConditionWasEventFiredRecentlyDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return ( m_executionTimeout >= 0.f ) ?
		new CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance( *this, owner, context, parent ) :
		new CBehTreeNodeConditionWasEventFiredRecentlyInstance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasEventFiredRecentlyInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeConditionWasEventFiredRecentlyInstance::CBehTreeNodeConditionWasEventFiredRecentlyInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_eventName( def.m_eventName.GetVal( context ) )
{
	if ( !m_eventName.Empty() )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = m_eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		context.AddEventListener( e, this );
	}
}

void CBehTreeNodeConditionWasEventFiredRecentlyInstance::OnDestruction()
{
	if ( !m_eventName.Empty() )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = m_eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
		m_owner->RemoveEventListener( e, this );
	}
	Super::OnDestruction();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance::Update()
{
	if ( m_lastEvent + m_executionTimeout < m_owner->GetLocalTime() )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	Super::Update();
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return ( m_executionTimeout >= 0.f ) ?
		new CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance( *this, owner, context, parent ) :
		new CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance::CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_eventsNames( def.m_eventsNames )
{
	for ( auto it = m_eventsNames.Begin(), end = m_eventsNames.End(); it != end; ++it )
	{
		CName eventName = *it;
		if ( !eventName.Empty() )
		{
			SBehTreeEvenListeningData e;
			e.m_eventName = eventName;
			e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
			context.AddEventListener( e, this );
		}
	}
}
void CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance::OnDestruction()
{
	for ( auto it = m_eventsNames.Begin(), end = m_eventsNames.End(); it != end; ++it )
	{
		CName eventName = *it;
		if ( !eventName.Empty() )
		{
			SBehTreeEvenListeningData e;
			e.m_eventName = eventName;
			e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
			m_owner->RemoveEventListener( e, this );
		}
	}

	Super::OnDestruction();
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance
////////////////////////////////////////////////////////////////////////
void CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance::Update()
{
	if ( m_lastEvent + m_executionTimeout < m_owner->GetLocalTime() )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	Super::Update();
}
