/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "threadMonitor.h"

#include <xutility>

namespace red
{
namespace memory
{
namespace
{
	static void NotifyThreadDied( void * userData )
	{
		ThreadMonitor * monitor = static_cast< ThreadMonitor* >( userData );
		monitor->NotifyCurrentThreadDied();
	}

#ifdef RED_PLATFORM_ORBIS
	
	static ScePthreadOnce s_orbisOnceControl = SCE_PTHREAD_ONCE_INIT;
	static ScePthreadKey s_orbisThreadKey;
	
	void ThreadKeyIntialization()
	{
		auto result = scePthreadKeyCreate( &s_orbisThreadKey, NotifyThreadDied );
		RED_FATAL_ASSERT( result == SCE_OK, "scePthreadKeyCreate failed. Code: %d", result );
		RED_UNUSED( result );
	}

#endif

}

	ThreadMonitor::ThreadMonitor()
		:	m_threadIdProvider( nullptr )
	{
		std::memset( m_observers, 0, sizeof( m_observers ) );
		m_threadIdProvider = &m_threadIdproviderStorage;
	}

	ThreadMonitor::~ThreadMonitor()
	{}

	void ThreadMonitor::Uninitialize()
	{
		m_threadIdProvider = nullptr; // Can't notified allocator anymore.  
	}

	void ThreadMonitor::MonitorCurrentThread()
	{
#ifdef RED_COMPILER_MSC
		auto fls_id = FlsAlloc( NotifyThreadDied );
		FlsSetValue( fls_id, this );
#elif defined( RED_PLATFORM_ORBIS )
		scePthreadOnce( &s_orbisOnceControl, ThreadKeyIntialization ); 
		auto result = scePthreadSetspecific( s_orbisThreadKey, this );
		RED_FATAL_ASSERT( result == SCE_OK, "scePthreadSetspecific failed. Code: %d", result );
		RED_UNUSED( result );
#else
		static_assert( 0, "Unknown Platform. Lockless allocators won't work correctly." );
#endif	
	}

	void ThreadMonitor::RegisterOnThreadDiedSignal( OnThreadDieCallback callback, void * userData )
	{
		CScopedLock< CRWSpinLock > scopedLock( m_lock );
		Observer * observerSlot = FindAvailableObserverSlot();
		RED_FATAL_ASSERT( observerSlot != nullptr, "ThreadMonitor do not have any slot available for new Observer." );
		*observerSlot = std::make_pair( callback, userData );
	}

	void ThreadMonitor::UnregisterFromThreadDiedSignal( OnThreadDieCallback callback, void * userData )
	{
		auto beginIter = std::begin( m_observers );
		auto endIter = std::end( m_observers );
		
		CScopedLock< CRWSpinLock > scopedLock( m_lock );
		auto iter = find( beginIter, endIter, std::make_pair( callback, userData ) );
		if( iter != endIter )
		{
			*iter = std::make_pair( nullptr, nullptr );
		}
	}

	void ThreadMonitor::NotifyCurrentThreadDied()
	{
		if( m_threadIdProvider )
		{
			const ThreadId threadId = m_threadIdProvider->GetCurrentId();
			OnThreadDied( threadId );
		}
	}

	ThreadMonitor::Observer * ThreadMonitor::FindAvailableObserverSlot()
	{
		for( u32 index = 0; index != c_threadMonitorMaxObservers; ++index )
		{
			Observer & observer = m_observers[ index ];

			if( observer.first == nullptr )
			{
				return &observer;
			}
		}

		return nullptr;
	}

	void ThreadMonitor::OnThreadDied( u32 threadId ) const
	{
		for( u32 index = 0; index != c_threadMonitorMaxObservers; ++index )
		{
			const Observer & observer = m_observers[ index ];
			if( observer.first )
			{
				OnThreadDieCallback callback = observer.first;
				void * userData = observer.second;
				callback( threadId, userData );
			}
		}
	}

	void ThreadMonitor::InternalSetThreadIdProvider( ThreadIdProvider * provider )
	{
		m_threadIdProvider = provider;
	}

	void ThreadMonitor::InternalForceNotifyThreadDied( ThreadId id )
	{
		OnThreadDied( id );
	}
}
}
