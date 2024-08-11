#include "build.h"
#include "inGameConfigRefreshEvent.h"

namespace InGameConfig
{
	void CRefreshEventDispatcher::DispatchIfExists(const CName& eventName)
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Dispatching in-game config events can be done only on main thread!" );

		if( DoesExist( eventName ) )
		{
			Dispatch( eventName );
		}
	}

	Bool CRefreshEventDispatcher::DoesExist(const CName& eventName) const
	{
		return m_listenerBuckets.KeyExist( eventName );
	}

	void CRefreshEventDispatcher::Dispatch(const CName& eventName)
	{
		ListenerBucket& listeners = m_listenerBuckets[ eventName ];
		for( IRefreshEventListener* listener : listeners )
		{
			listener->OnRequestRefresh( eventName );
		}
	}

	void CRefreshEventDispatcher::RegisterForEvent(const CName& eventName, IRefreshEventListener* listener)
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Registering for in-game config events can be done only on main thread!" );

		ListenerBucket& bucket = GetOrCreateListenerBucket( eventName );
		if( bucket.Exist( listener ) == false )
		{
			bucket.PushBack( listener );
		}
	}

	void CRefreshEventDispatcher::UnregisterFromAllEvents(IRefreshEventListener* listener)
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Unregistering from in-game config events can be done only on main thread!" );

		for( ListenerBucketMap::iterator it = m_listenerBuckets.Begin(); it != m_listenerBuckets.End(); )
		{
			ListenerBucket& bucket = it.Value();
			bucket.Remove( listener );

			if( bucket.Size() == 0 )
			{
				ListenerBucketMap::iterator eraseIt = it;
				++it;
				m_listenerBuckets.Erase( eraseIt );
			}
			else
			{
				++it;
			}
		}
	}

	void CRefreshEventDispatcher::UnregisterFromEvent(const CName& eventName, IRefreshEventListener* listener)
	{
		RED_FATAL_ASSERT( ::SIsMainThread(), "Unregistering from in-game config events can be done only on main thread!" );

		if( DoesExist( eventName ) == true )
		{
			ListenerBucket& bucket = m_listenerBuckets[eventName];
			bucket.Remove( listener );

			if( bucket.Size() == 0 )
			{
				m_listenerBuckets.Erase( eventName );
			}
		}
	}

	CRefreshEventDispatcher::ListenerBucket& CRefreshEventDispatcher::GetOrCreateListenerBucket(const CName& eventName)
	{
		if( DoesExist( eventName ) == false )
		{
			m_listenerBuckets.Insert( eventName, ListenerBucket() );
		}

		return m_listenerBuckets[eventName];
	}

}
