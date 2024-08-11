/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _LISTENER_H
#define	_LISTENER_H

template< class _EventType >
class CEventNotifier;

template< class _EventType >
class IEventHandler
{
public:
	virtual ~IEventHandler() {} 

	virtual void HandleEvent( const _EventType &event ) = 0;

	virtual void OnHandlerAttached( CEventNotifier< _EventType >* /*notifier*/ )
	{
	}

	virtual void OnHandlerDetached( CEventNotifier< _EventType >* /*notifier*/ )
	{
	}
};


template< class _EventType >
class CEventNotifier
{
	typedef TDynArray< IEventHandler<_EventType >* >	tHandlersList;

	tHandlersList			m_eventHandlers;
	tHandlersList			m_delayedUnregistration;
	Bool					m_iterating;
	CObject*				m_owner;

public:
			CEventNotifier();
		   ~CEventNotifier();

		    CEventNotifier( CObject* owner );

	void	RegisterHandler( IEventHandler< _EventType > *handler );
	void	UnregisterHandler( IEventHandler< _EventType > *handler );

	void	BroadcastEvent( const _EventType &event );

	Bool	IsHandlerRegistered( IEventHandler< _EventType > *handler ) const;
	
	void	UnregisterAllHandlers();

	RED_INLINE CObject* GetOwner() const { return m_owner; }

	RED_INLINE void SetOwner( CObject* owner ) { m_owner = owner; }

};

#include "listener.inl"

#endif /* _LISTENER_H */