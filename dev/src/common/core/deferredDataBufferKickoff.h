/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_DEFERRED_DATA_BUFFER_KICKOFF_H_
#define _CORE_DEFERRED_DATA_BUFFER_KICKOFF_H_

#include "hashset.h"

/// Helper class that implements a APC like interface for things that may need periodic "nagging"
/// The DDBs are budgeted and tightly coupled with IO so it makes perfect sense update internal state
/// of the DDBs that are in the loading queue every time something important happens on the AsyncIO/DecompressionThread.
/// Doing this instead of using normal "Job" allows us to lower the CPU usage and latency A LOT in the whole system.
/// Basically, the goal is that the moment the IO resources are freed by some work that has just finished we try to "push" more work
/// into the system by looking at the list of DDBs that are not yet loaded.

/// Helper class that holds the references to internal stuff related to DDB that may need periodic "nagging"
class CDeferredDataBufferKickOffList
{
public:
	CDeferredDataBufferKickOffList();

	// register/unregister the callback
	void RegisterCallback( class BufferAsyncData* data );
	void UnregisterCallback( class BufferAsyncData* data );

	// refcounting for callbacks crash fix
	Uint32 ReleaseCallbackRef( Red::Threads::CAtomic< Int32 >* refCount, class BufferAsyncData* data );

	// kick jobs
	// NOTE: do not call UnregisterCallback from inside the Kick
	void KickNewJobs();

private:
	Red::Threads::CMutex			m_lock;
	THashSet< class BufferAsyncData* >	m_callbacks;
	TDynArray< class BufferAsyncData* >	m_toRemove;
	Bool								m_isProcessing;
	Red::Threads::CAtomic< Bool >		m_isKickingNewJobs;
};

/// Global interface :(
typedef TSingleton< CDeferredDataBufferKickOffList, TNoDestructionLifetime > SDeferredDataBufferKickOffList;

#endif
