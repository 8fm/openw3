/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_THREAD_MONITOR_H_
#define _RED_MEMORY_THREAD_MONITOR_H_

#include "threadIdProvider.h"

#include <utility>

namespace red
{
namespace memory
{
	const u32 c_threadMonitorMaxObservers = 1;

	class RED_MEMORY_API ThreadMonitor
	{
	public:

		typedef void (*OnThreadDieCallback)( ThreadId threadId, void * userData );

		ThreadMonitor();
		RED_MOCKABLE ~ThreadMonitor();

		void Uninitialize();

		RED_MOCKABLE void MonitorCurrentThread();
		
		void RegisterOnThreadDiedSignal( OnThreadDieCallback callback, void * userData );
		void UnregisterFromThreadDiedSignal( OnThreadDieCallback callback, void * userData );

		void NotifyCurrentThreadDied();

		// UNIT TEST ONLY
		void InternalSetThreadIdProvider( ThreadIdProvider * provider );
		void InternalForceNotifyThreadDied( ThreadId id );
		void InternalUnregisterAllObservers();

	private:

		typedef std::pair< OnThreadDieCallback, void* > Observer;

		ThreadMonitor( const ThreadMonitor & );
		ThreadMonitor& operator=( const ThreadMonitor& );

		Observer * FindAvailableObserverSlot();

		void OnThreadDied( u32 threadId ) const; 

		Observer m_observers[ c_threadMonitorMaxObservers ];

		ThreadIdProvider * m_threadIdProvider;
		ThreadIdProvider m_threadIdproviderStorage;

		CRWSpinLock m_lock;
	};
}
}

#endif
