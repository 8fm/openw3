/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

template< class _EventType >
CEventNotifier< _EventType >::CEventNotifier()
	: m_iterating( false )
	, m_owner( NULL )
{
}

template< class _EventType >
CEventNotifier< _EventType >::~CEventNotifier()
{
}

template< class _EventType >
void CEventNotifier< _EventType >::RegisterHandler( IEventHandler< _EventType > *handler )
{
	if ( Find( m_eventHandlers.Begin(), m_eventHandlers.End(), handler ) == m_eventHandlers.End() )
	{
		m_eventHandlers.PushBack( handler );
		handler->OnHandlerAttached( this );
	}
}

template< class _EventType >
Bool CEventNotifier< _EventType >::IsHandlerRegistered( IEventHandler< _EventType > *handler ) const
{
	return m_eventHandlers.Exist( handler );
}

template< class _EventType >
void CEventNotifier< _EventType >::UnregisterHandler( IEventHandler< _EventType > *handler )
{
	if ( m_iterating )
	{
		m_delayedUnregistration.PushBack( handler );
		return;
	}

	typename tHandlersList::iterator it = Find( m_eventHandlers.Begin(), m_eventHandlers.End(), handler );
	if ( it != m_eventHandlers.End() )
	{
		(*it)->OnHandlerDetached( this );
		m_eventHandlers.Erase( it );
	}
}

template< class _EventType >
void CEventNotifier< _EventType >::UnregisterAllHandlers()
{
	if ( m_iterating )
	{
		m_delayedUnregistration.PushBackUnique( m_eventHandlers );
		return;
	}

	for( typename tHandlersList::iterator it = m_eventHandlers.Begin();
		it != m_eventHandlers.End();
		++it )
	{
		(*it)->OnHandlerDetached( this );
	}
	m_eventHandlers.Clear();
}


template< class _EventType >
void CEventNotifier< _EventType >::BroadcastEvent( const _EventType &event )
{
	// store previous iteration state, so it can be set after we're done. This is necessary in case of events broadcasted by event handlers.
	Bool previousIterationState = m_iterating;
	m_iterating = true;

	for( typename tHandlersList::iterator it = m_eventHandlers.Begin();
		 it != m_eventHandlers.End();
		 ++it )
	{
		(*it)->HandleEvent( event );
	}

	m_iterating = previousIterationState;

	if ( !m_delayedUnregistration.Empty() && !m_iterating )
	{
		for( typename tHandlersList::iterator it = m_delayedUnregistration.Begin();
			 it != m_delayedUnregistration.End();
			 ++it )
		{
			m_eventHandlers.Remove( *it );
			(*it)->OnHandlerDetached( this );
		}

		m_delayedUnregistration.Clear();
	}
}
