/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef _NOTIFIER_H_
#define _NOTIFIER_H_

namespace Events
{
	//////////////////////////////////////////////////////////////////////////
	// Types
	//////////////////////////////////////////////////////////////////////////
	typedef Uint32 TListenerHandle;

#define LISTENER_HANDLE_STARTING_VALUE 1024
#define LISTENER_HANDLE_PRIORITY_INCREMENT 1024 / 2

	//////////////////////////////////////////////////////////////////////////
	// CNotifier
	//////////////////////////////////////////////////////////////////////////
	enum EPriority
	{
		P_High = 0,
		P_Normal,
		P_Low
	};

	template< typename TEventData >
	class CNotifier
	{
	public:
		typedef std::function< void( const TEventData& ) > TEventCallback;

		CNotifier();
		void Clear();

		template< typename TInstance >
		TListenerHandle RegisterListener( void ( TInstance::*callback )( const TEventData& ), TInstance* instance, EPriority priority = P_Normal );
		TListenerHandle RegisterListener( void ( *callback )( const TEventData& ), EPriority priority = P_Normal );
		TListenerHandle RegisterListener( const TEventCallback& callback, EPriority priority = P_Normal );
		TListenerHandle RegisterListener( TEventCallback&& callback, EPriority priority = P_Normal );

		Bool UnregisterListener( TListenerHandle );

		void Send( const TEventData& data );

	private:
		TListenerHandle CreateHandle( EPriority priority );

		TSortedMap< TListenerHandle, TEventCallback* > m_listeners;
		TListenerHandle m_nextHandle;
	};

	//////////////////////////////////////////////////////////////////////////
	// Function definitions
	//////////////////////////////////////////////////////////////////////////
	template< typename TEventData >
	CNotifier<TEventData>::CNotifier()
	:	m_nextHandle( LISTENER_HANDLE_STARTING_VALUE )
	{

	}

	template< typename TEventData >
	void CNotifier<TEventData>::Clear()
	{
		for( auto iter : m_listeners )
		{
			delete iter.m_second;
		}

		m_listeners.Clear();
	}

	template< typename TEventData >
	void CNotifier<TEventData>::Send( const TEventData& data )
	{
		for( auto iter : m_listeners )
		{
			( *( iter.m_second ) )( data );
		}
	}

	template< typename TEventData >
	Bool CNotifier<TEventData>::UnregisterListener( TListenerHandle handle )
	{
		TEventCallback* callback = nullptr;

		auto iter = m_listeners.Find( handle );

		if( iter != m_listeners.End() )
		{
			delete iter->m_second;

			return m_listeners.Erase( iter );
		}

		return false;
	}

	template< typename TEventData >
	TListenerHandle Events::CNotifier<TEventData>::CreateHandle( EPriority priority )
	{
		TListenerHandle newHandle = m_nextHandle++;

		if( priority == P_High )
		{
			newHandle -= LISTENER_HANDLE_PRIORITY_INCREMENT;

			RED_FATAL_ASSERT( newHandle != LISTENER_HANDLE_STARTING_VALUE, "Too many listeners" );
		}
		else if( priority == P_Low )
		{
			newHandle += LISTENER_HANDLE_PRIORITY_INCREMENT;
		}
		else if( priority == P_Normal )
		{
			RED_FATAL_ASSERT( newHandle != LISTENER_HANDLE_STARTING_VALUE + LISTENER_HANDLE_PRIORITY_INCREMENT, "Too many listeners" );
		}

		return newHandle;
	}

	template< typename TEventData >
	TListenerHandle CNotifier<TEventData>::RegisterListener( TEventCallback&& callback, EPriority priority )
	{
		TEventCallback* callbackPtr = new TEventCallback( std::move( callback ) );

		TListenerHandle handle = CreateHandle( priority );
		m_listeners.Insert( handle, callbackPtr );
		return handle;
	}

	template< typename TEventData >
	TListenerHandle CNotifier<TEventData>::RegisterListener( const TEventCallback& callback, EPriority priority )
	{
		TEventCallback* callbackPtr = new TEventCallback( callback );

		TListenerHandle handle = CreateHandle( priority );
		m_listeners.Insert( handle, callbackPtr );
		return handle;
	}

	template< typename TEventData >
	TListenerHandle CNotifier<TEventData>::RegisterListener( void ( *callback )( const TEventData& ), EPriority priority )
	{
		TEventCallback* callbackPtr = new TEventCallback( callback );

		TListenerHandle handle = CreateHandle( priority );
		m_listeners.Insert( handle, callbackPtr );
		return handle;
	}

	template< typename TEventData >
	template< typename TInstance >
	TListenerHandle CNotifier<TEventData>::RegisterListener( void ( TInstance::*callback )( const TEventData& ), TInstance* instance, EPriority priority )
	{
		// Wrap the call in a lambda instead of using std::bind since it's broken in VS2012 (Switch back once we're using VS2013+
		TInstance* instanceWrapper = instance;
		auto callbackWrapper = callback;
		TEventCallback* callbackPtr = new TEventCallback( [ instanceWrapper, callbackWrapper ]( const TEventData& data ){ ( instanceWrapper->*callbackWrapper )( data ); } );

// 		auto binding = std::bind( callback, instance, std::placeholders::_1 );
// 		TEventCallback* callbackPtr = new TEventCallback( binding );

		TListenerHandle handle = CreateHandle( priority );
		m_listeners.Insert( handle, callbackPtr );
		return handle;
	}
}

#endif // _NOTIFIER_H_
