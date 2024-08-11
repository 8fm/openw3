/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "task.h"
#include "taskDispatcher.h"
#include "taskManager.h"
#include "taskRunner.h"
#include "taskScheduler.h"
#include "taskThread.h"
#include "math.h"

#define SLOW_WORK_SEMAPHORE

//////////////////////////////////////////////////////////////////////////
// CTaskScheduler::SWorkToken
//////////////////////////////////////////////////////////////////////////
CTaskScheduler::SWorkToken::SWorkToken()
	: m_count( 0 )
	, m_waiting( 0 )
	, m_lock( 0, INT_MAX )
{
}

CTaskScheduler::SWorkToken::~SWorkToken()
{
}

namespace Helper
{
	static RED_FORCE_INLINE bool DecrementIfPositive( Red::Threads::CAtomic< Int32 >& ref )
	{
		while ( 1 )
		{
			int val = ref.GetValue();
			
		}
	}
}

void CTaskScheduler::SWorkToken::WaitForWork()
{
#ifdef SLOW_WORK_SEMAPHORE
	m_lock.Acquire();
#else
	int counter = m_count.Decrement();
	if ( counter < 0 )
	{
		m_waiting.Increment();
		m_lock.Acquire();
	}
#endif
}

void CTaskScheduler::SWorkToken::SignalWork( const Uint32 count /*= 1*/ )
{
#ifdef SLOW_WORK_SEMAPHORE
	m_lock.Release( count );
#else
	for ( Uint32 i=0; i<count; ++i )
	{
		int counter = m_count.Increment();
		if ( counter <= 0 )
		{
			m_lock.Release(1);
			m_waiting.Decrement();
		}
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// CTaskScheduler
//////////////////////////////////////////////////////////////////////////
CTaskScheduler::CTaskScheduler( CTaskManager& taskManager )
	: m_taskManager( taskManager )
{
}

CTaskScheduler::~CTaskScheduler()
{
}

Bool CTaskScheduler::Init( const STaskGroupInitParams::SThreadParams& threadParams )
{
	RED_ASSERT( threadParams.m_numTaskThreads > 0 );
	if ( threadParams.m_numTaskThreads < 1 )
	{
		return false;
	}

#ifndef RED_FULL_DETERMINISM
	InitTaskThreads( threadParams );
#endif

	return true;
}

void CTaskScheduler::InitTaskThreads( const STaskGroupInitParams::SThreadParams& threadParams )
{
	RED_ASSERT( threadParams.m_numTaskThreads > 0 );
	
	LOG_CORE(TXT("CTaskScheduler initializing with %u task threads"), threadParams.m_numTaskThreads );

	m_taskThreadList.Reserve( threadParams.m_numTaskThreads );
	for ( Uint32 i = 0; i < threadParams.m_numTaskThreads; ++i )
	{
		CTaskDispatcher* taskDispatcher = new CTaskDispatcher( m_taskManager );
		CTaskThread* taskThread = new CTaskThread( *this, *taskDispatcher, i );

		m_taskThreadList.PushBack( taskThread );
		m_taskDispatcherList.PushBack( taskDispatcher );
	}

	// Post-init threads so dispatcher list at final size for task stealing
	for ( Uint32 i = 0; i < threadParams.m_numTaskThreads; ++i )
	{
		CTaskThread* taskThread = m_taskThreadList[ i ];
		RED_ASSERT( taskThread );

		taskThread->InitThread();
		taskThread->SetPriority(threadParams.m_priority);

		if ( threadParams.m_affinityMasks )
		{
			const Red::Threads::TAffinityMask mask = threadParams.m_affinityMasks[ i ];
			taskThread->SetAffinityMask( mask );
		}
	}
}

// The request to shutdown should come after anything may issue additional requests,
// so shut down any taks issuing threads before calling this
void CTaskScheduler::Shutdown()
{
	if ( ! ::SIsMainThread() )
	{
		RED_HALT( "Only shutdown from the main thread!" );
	}
	
	// Request task exit before broadcasting work so work won't
	// be reset by a woken thread
	for ( Uint32 i = 0; i < m_taskThreadList.Size(); ++i )
	{
		CTaskThread* taskThread = m_taskThreadList[ i ];
		taskThread->RequestShutdown();
	}

	m_workToken.SignalWork(100); // Wake up and exit

	for ( Uint32 i = 0; i < m_taskThreadList.Size(); ++i )
	{
		CTaskThread* taskThread = m_taskThreadList[ i ];
		RED_ASSERT( taskThread );
		taskThread->JoinThread();
	}

	m_taskThreadList.ClearPtrFast();

	for( ;; )
	{
		CTask* task = m_taskSchedListBacklog.PopTask();
		if ( ! task )
		{
			break;
		}
		RED_VERIFY( task->MarkCancelled() );
		task->Release();
	}
}

CTask* CTaskScheduler::PopTask()
{
	// Try to get something from the backlog
	CTask* task = m_taskSchedListBacklog.PopTask();

	return task;
}

void CTaskScheduler::IssueTask( CTask& task, ETaskSchedulePriority taskSchedulePriority )
{
	Bool issuedDirectly = false;
	{
		IssueTask_NoWorkSignal( task, taskSchedulePriority, issuedDirectly );
	}

	if ( !issuedDirectly )
	{
		m_workToken.SignalWork();
	}
}

void CTaskScheduler::IssueTaskBatch( CTask* tasks[], Uint32 numTasks, ETaskSchedulePriority taskSchedulePriority )
{
	RED_ASSERT( ( tasks && numTasks > 0 ) || ( ! tasks && numTasks == 0 ) );

	Uint32 numToSignal = 0;
	for ( Uint32 i = 0; i < numTasks; ++i )
	{
		RED_ASSERT( tasks[i] );
		CTask& task = *tasks[i];
		Bool issuedDirectly = false;
		IssueTask_NoWorkSignal( task, taskSchedulePriority, issuedDirectly );
		numToSignal += ( issuedDirectly ? 0 : 1 );
	}

	m_workToken.SignalWork( numToSignal );
}

void CTaskScheduler::IssueTask_NoWorkSignal( CTask& task, ETaskSchedulePriority taskSchedulePriority, Bool& issuedDirectly )
{
	issuedDirectly = false;

	extern RED_TLS Bool GIsTaskThread;
	if ( task.m_taskDispatcher )
	{
		task.m_taskDispatcher->IssueFront( task );
		issuedDirectly = true;
	}
	else if ( task.MarkScheduled() )
	{		
		task.AddRef();
		m_taskSchedListBacklog.PushBack( task, taskSchedulePriority );
	}
	else
	{
		RED_FATAL( "Failed to issue task!" );
	}	
}

CTaskManager& CTaskScheduler::GetTaskManager()
{
	return m_taskManager;
}

const CTaskManager& CTaskScheduler::GetTaskManager() const
{
	return m_taskManager;
}

CTaskScheduler::SWorkToken& CTaskScheduler::GetWorkToken()
{
	return m_workToken;
}

const CTaskScheduler::SWorkToken& CTaskScheduler::GetWorkToken() const
{
	return m_workToken;
}

const CTaskScheduler::TTaskThreadList& CTaskScheduler::GetThreadList() const
{
	return m_taskThreadList;
}

CTask* CTaskScheduler::StealTask()
{
	// TBD: Randomize or use victim score
	// TBD: Steal up to half of tasks at once

	//FIXME: Shit reading two TLS, could just have index start at invalid value.
	extern RED_TLS Uint32 GTaskThreadIndex;
	Uint32 thiefIndex = GTaskThreadIndex;

	RED_ASSERT( thiefIndex < m_taskDispatcherList.Size() );
	for ( Uint32 i = 0; i < thiefIndex; ++i )
	{
		CTaskDispatcher* taskDispatcher = m_taskDispatcherList[ i ];
		CTask* task = taskDispatcher->StealTask();
		if ( task )
		{
			m_workToken.SignalWork(); // for latency, assuming there should be more work. TBD: could check if more tasks left
			return task;
		}
	}

	for ( Uint32 i = thiefIndex + 1; i < m_taskDispatcherList.Size(); ++i )
	{
		CTaskDispatcher* taskDispatcher = m_taskDispatcherList[ i ];
		CTask* task = taskDispatcher->StealTask();
		if ( task )
		{
			return task;
		}
	}

	return nullptr;
}

// This cannot guarantee there won't be any tasks issued right after if you
// could be processing tasks inline on another non-task or non-main thread.
// Restricting the thread to run tasks inline is pointless since arbitrary threads
// can issue a task. To truly flush you'd want to flush any thread that can issue tasks
// flush tasks and any tasks they can create
void CTaskScheduler::Flush()
{
	if ( ! ::SIsMainThread() )
	{
		RED_FATAL( "Only flush from the main thread!!! FIX THIS NOW." );
		return;
	}
	
	typedef Red::Threads::CAtomic< Uint32 > TAtomicCounter;

	class CTaskFence : public CTask
	{
	private:
		TAtomicCounter& m_barrierCount;
		TAtomicCounter& m_finishCount;
		Uint32 m_numThreads;

	public:
		CTaskFence( TAtomicCounter& barrierCount, TAtomicCounter& finishCount, Uint32 numThreads )
			: m_barrierCount( barrierCount )
			, m_finishCount( finishCount )
			, m_numThreads( numThreads )
		{
		}

	public:
		virtual void Run() override
		{
			// Tie up this thread so the other fence tasks will get picked up by another
			m_barrierCount.Increment();
			while ( m_barrierCount.GetValue() < m_numThreads )
			{
				continue;
			}

			// Increment this after checking the barrier above. This way the flush doesn't return (and invalidate the stack based variables)
			// before we're done checking them!
			m_finishCount.Increment();
		}

		virtual const Char* GetDebugName() const override { return TXT("Flush"); }
		virtual Uint32 GetDebugColor() const override { return Color::WHITE.ToUint32(); }
	};

	TAtomicCounter barrierCount( 0 );
	TAtomicCounter finishCount( 0 );

	const Uint32 numThreads = m_taskThreadList.Size();
	if ( numThreads < 1 )
	{
		return;
	}

	const size_t size = sizeof(CTask*) * numThreads;
	CTask** taskFenceBatch = static_cast< CTask**>( RED_ALLOCA( size ) );
	RED_FATAL_ASSERT( taskFenceBatch, "Failed to alloca %llu bytes", size );

	for ( Uint32 i = 0; i < numThreads; ++i )
	{
		CTask* taskFence = new ( CTask::Root ) CTaskFence( barrierCount, finishCount, numThreads );
		taskFenceBatch[i] = taskFence;
	}

	IssueTaskBatch( taskFenceBatch, numThreads, TSP_Low );

	for ( Uint32 i = 0;  i < m_taskThreadList.Size(); ++i )
	{
		taskFenceBatch[ i ]->Release();
		taskFenceBatch[ i ] = nullptr;
	}

	//TODO: Just busy wait for Flush(), which really shouldn't be used anyway.
	while ( finishCount.GetValue() < numThreads )
	{
#ifdef RED_PLATFORM_WINPC
		::YieldProcessor(); // Busy wait
#endif
	}
}
