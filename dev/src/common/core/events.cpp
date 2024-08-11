/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "events.h"
#include "profiler.h"

#ifndef NO_EDITOR_EVENT_SYSTEM

CEdEventManager::CEdEventManager()
	: m_enabled( true )
{
}

CEdEventManager::~CEdEventManager()
{
}

void CEdEventManager::Enable( const Bool enable )
{
	m_enabled = enable;

	if ( !enable )
		Clear();
}

void CEdEventManager::Clear()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_listeners.Clear();
	m_events.Clear();
}

void CEdEventManager::ProcessPendingEvens()
{
	if ( !m_enabled )
		return;

	// Get events
	TDynArray< EventInfo > copyEvents;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		copyEvents = m_events;
		m_events.Clear();
	}

	// Dispatch each event
	if ( copyEvents.Size() )
	{
		CTimeCounter timerCounter;
		for ( Uint32 i=0; i<copyEvents.Size(); i++ )
		{
			const EventInfo &systemEvent = copyEvents[i];
			DispatchEvent( systemEvent.m_name, systemEvent.m_data );
		}

		// Log if longer than 1ms
		const Float time = timerCounter.GetTimePeriod();
		if ( time > 0.001f )
		{
			WARN_CORE( TXT("PERFORMANCE HOG: %i pending events took %1.2fms"), copyEvents.Size(), time * 1000.0f );
		}
	}

	// Clear invalid listeners
	m_invalidListeners.Clear();
	m_invalidEventListeners.Clear();
}

void CEdEventManager::RegisterListener( const CName& name, IEdEventListener* listener )
{
	if ( !m_enabled )
		return;

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_listeners.GetRef( name ).Insert( listener );
}

void CEdEventManager::UnregisterListener( const CName& name, IEdEventListener* listener )
{
	if ( !m_enabled )
		return;

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Get listeners array
	TListenerSet* listeners = m_listeners.FindPtr( name );
	if ( listeners )
	{
		listeners->Erase( listener );
	}

	// Add listener to invalid listeners
	m_invalidEventListeners[name].Insert( listener );
}

void CEdEventManager::UnregisterListener( IEdEventListener* listener )
{
	if ( !m_enabled )
		return;

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Remove listener from all events
	for ( TListeners::iterator i = m_listeners.Begin(); i != m_listeners.End(); ++i )
	{
		TListenerSet& listeners = i->m_second;
		listeners.Erase( listener );
	}

	// Add listener to invalid listeners
	m_invalidListeners.Insert( listener );
}

void CEdEventManager::QueueEvent( const CName& name, IEdEventData* data )
{
	if ( !m_enabled )
	{
		delete data;
		return;
	}

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
	m_events.PushBack( EventInfo( name, data ) );
}

void CEdEventManager::DispatchEvent( const CName& name, IEdEventData* data )
{
	if ( !m_enabled )
	{
		delete data;
		return;
	}

	// Get listeners for this event
	TListenerSet listeners;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
		m_listeners.Find( name, listeners );
		// listeners stays empty if not found
	}

	// Dispatch events
	for ( auto it = listeners.Begin(); it != listeners.End(); ++it )
	{
		// Make sure this listener hasn't been invalidated by a previously called event listener
		bool valid;
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );
			valid = (*it) != nullptr && !m_invalidListeners.Exist( *it );
			if ( valid && m_invalidEventListeners.KeyExist( name ) )
			{
				valid = !m_invalidEventListeners[name].Exist( *it );
			}
		}
		
		// Note: do not lock while the event is dispatched - this way the handler may alter the EventManager state without deadlocking it
		if ( valid )
		{
			(*it)->DispatchEditorEvent( name, data );
		}
	}

	// Delete event data
	delete data;
}

void CEdEventManager::DumpRegisteredListeners()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	LOG_CORE( TXT("=========================================") );
	LOG_CORE( TXT("CEdEventManager::DumpRegisteredListeners:") );

	// Remove listener from all events
	for ( TListeners::iterator i = m_listeners.Begin(); i != m_listeners.End(); ++i )
	{
		Uint32 cnt = 0;
		TListenerSet& eventListeners = i->m_second;
		for( auto el = eventListeners.Begin(); el != eventListeners.End(); ++el )
		{
			if( *el )
			{
				++cnt;
			}
		}

		if( cnt > 0 )
		{
			LOG_CORE( TXT("Event: %s, num listeners: %d"), i->m_first.AsString().AsChar(), cnt );
		}
	}

	LOG_CORE( TXT("=========================================") );
}

#endif