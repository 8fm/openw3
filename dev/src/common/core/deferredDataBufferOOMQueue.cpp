/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "engineTime.h"

#include "deferredDataBuffer.h"
#include "deferredDataBufferAsync.h"
#include "deferredDataBufferOOMQueue.h"

#include "configVar.h"

namespace Config
{
	TConfigVar< Float >			cvDeferredDataBufferOOMRetryTimeout( "Streaming", "DeferredBufferOOMRetryTimeout", 1.0f );
}


DeferredDataOOMQueue::DeferredDataOOMQueue()
	: m_oomMemorySize( 0 )
	, m_nextRefreshTime( EngineTime::GetNow() )
{
	m_oomQueue.Reserve( 256 );
}

DeferredDataOOMQueue::~DeferredDataOOMQueue()
{
}

void DeferredDataOOMQueue::Reschedule()
{
	// throttle the execution 
	const EngineTime curTime = EngineTime::GetNow();
	if ( curTime > m_nextRefreshTime )
	{
		TDynArray< BufferAsyncData* > queue;
		Uint64 queueMemorySize = 0;

		// get current queue
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_oomQueueLock );
			queue = std::move( m_oomQueue );

			queueMemorySize = m_oomMemorySize;
			m_oomMemorySize = 0;
		}

		// stats
		if ( !queue.Empty() )
		{
			LOG_CORE( TXT("Rescheduling %d OOMed buffers (%1.2fKB total)"), 
				queue.Size(), queueMemorySize / 1024.0f );
		}

		// add it to the kickoff list
		for ( auto* data : queue )
		{
			// only reschedule if the OOM flag was not cleared
			if ( data->ResetOOMFlag() )
			{
				SDeferredDataBufferKickOffList::GetInstance().RegisterCallback( data );
			}

			data->Release(); // get rid of our reference
		}

		// update in a while
		m_nextRefreshTime = curTime + Config::cvDeferredDataBufferOOMRetryTimeout.Get();
	}
}

void DeferredDataOOMQueue::Register( BufferAsyncData* data )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > scopedLock( m_oomQueueLock );

	m_oomQueue.PushBack( data );
	m_oomMemorySize += data->GetSize();

	data->AddRef();
}
