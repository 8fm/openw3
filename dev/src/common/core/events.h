/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_EDITOR_EVENT_SYSTEM

#include "names.h"
#include "hashmap.h"
#include "hashset.h"

/// Editor event data
class IEdEventData
{
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_SmallObjects, MC_Editor );
public:
	virtual ~IEdEventData() {};
};

/// Editor event hook
class IEdEventListener
{
public:
	virtual ~IEdEventListener() {};
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data )=0;
};

/// Data wrapper for given type. Use CreateEventData / GetEventData to manipulate on wrapped data.
template< class T >
class TEdEventData : public IEdEventData
{
private:
	T m_value;

	TEdEventData( const T& value )
		: m_value( value )
	{};

	template< typename T > friend struct EventDataCreator;
	template< typename T > friend struct EventDataExtractor;
};

template < typename T >
struct EventDataCreator
{
	static IEdEventData* Create( const T& data )
	{ 
		return new TEdEventData< T >( data );
	}
};

template < typename T >
struct EventDataCreator< T* >
{
	static IEdEventData* Create( const T* data )
	{
		static_assert( std::is_base_of< IReferencable, T >::value, "Cannot create EventData from pointer to non-IReferancable class" );
		return new TEdEventData< THandle< IReferencable > >( data );
	}
};

template < typename T >
struct EventDataExtractor
{
	static T Extract( IEdEventData* data ) 
	{ 
		TEdEventData< T >* tdata = static_cast< TEdEventData< T >* >( data );
		return tdata->m_value;
	}
};

template < typename T >
struct EventDataExtractor< T* >
{
	static T* Extract( IEdEventData* data ) 
	{ 
		TEdEventData< THandle< IReferencable > >* tdata = static_cast< TEdEventData< THandle< IReferencable > >* >( data );
		return Cast< T >( tdata->m_value ).Get();
	}
};

/// Create event data. T should be either a copyable type or a pointer to IReferencable-derived class.
template < typename T >
RED_INLINE IEdEventData* CreateEventData( const T& data )
{
	return EventDataCreator< T >::Create( data );
}

/// Extract event data. For T being a pointer, returns nullptr on mismatched type.
template < typename T >
RED_INLINE T GetEventData( IEdEventData* data )
{
	return EventDataExtractor< T >::Extract( data );
}

/// System event manager
class CEdEventManager
{
public:
	CEdEventManager();
	~CEdEventManager();

	void Clear();
	void ProcessPendingEvens();
	void RegisterListener( const CName& name, IEdEventListener* listener );
	void UnregisterListener( const CName& name, IEdEventListener* listener );
	void UnregisterListener( IEdEventListener* listener );
	void QueueEvent( const CName& name, IEdEventData* data );
	void DispatchEvent( const CName& name, IEdEventData* data );
	void DumpRegisteredListeners();
	void Enable( const Bool enable );

private:
	/// Deferred event
	struct EventInfo
	{
		CName			m_name;
		IEdEventData*	m_data;

		RED_INLINE EventInfo()
			: m_data( NULL )
		{};

		RED_INLINE EventInfo( const CName& name, IEdEventData* data )
			: m_name( name )
			, m_data( data )
		{};
	};

	typedef THashSet< IEdEventListener* > TListenerSet;
	typedef THashMap< CName, TListenerSet > TListeners;

	TListeners											m_listeners;
	TListenerSet										m_invalidListeners;
	TListeners											m_invalidEventListeners;
	TDynArray< EventInfo >								m_events;
	Red::Threads::CMutex								m_lock;
	Bool												m_enabled;
};

/// Event manager
typedef TSingleton<CEdEventManager> SEvents;

#	define EDITOR_QUEUE_EVENT( name, data )	SEvents::GetInstance().QueueEvent( name, data )
#	define EDITOR_DISPATCH_EVENT( name, data )	SEvents::GetInstance().DispatchEvent( name, data )
#else
#	define EDITOR_QUEUE_EVENT( name, data )
#	define EDITOR_DISPATCH_EVENT( name, data )
#endif
