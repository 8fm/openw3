/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "engineTime.h"

/// A dedicated queue for handling the OOM cases
class DeferredDataOOMQueue
{
public:
	DeferredDataOOMQueue();
	~DeferredDataOOMQueue();

	/// Get number of buffers in the queue (debug only)
	RED_FORCE_INLINE const Uint32 GetQueuedBufferCount() const { return m_oomQueue.Size(); }

	/// Get total size of failed memory allocations
	RED_FORCE_INLINE const Uint64 GetQueuedDataSize() const { return m_oomMemorySize; }

	/// Retry the deferred buffers that failed to load due to OOM
	void Reschedule();

private:
	/// Register failed async data in the OOM queue for later loading
	void Register( BufferAsyncData* data );

	/// The queue
	TDynArray< BufferAsyncData* >			m_oomQueue;
	Red::Threads::CMutex					m_oomQueueLock;

	/// Debug state
	Uint64									m_oomMemorySize;

	/// Last refresh time
	EngineTime								m_nextRefreshTime;

	friend class BufferAsyncData;
};

/// The queue of buffers that failed loading due to the OOM condition
typedef TSingleton< DeferredDataOOMQueue > SDeferredDataOOMQueue;