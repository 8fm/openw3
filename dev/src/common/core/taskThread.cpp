/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskThread.h"
#include "taskDispatcher.h"
#include "taskScheduler.h"

//////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////
RED_TLS Uint32 GTaskThreadIndex;

//////////////////////////////////////////////////////////////////////////
// CTaskThread
//////////////////////////////////////////////////////////////////////////
const AnsiChar* CTaskThread::s_threadNamePool[] = {
	"red Task Thread 1",
	"red Task Thread 2",
	"red Task Thread 3",
	"red Task Thread 4",
	"red Task Thread 5",
	"red Task Thread 6",
	"red Task Thread 7",
	"red Task Thread 8",
	"red Task Thread 9",
	"red Task Thread 10",
	"red Task Thread 11",
	"red Task Thread 12",
};

CTaskThread::CTaskThread( CTaskScheduler& taskScheduler, CTaskDispatcher& taskDispatcher, Uint32 threadIndex )
	: Red::Threads::CThread( threadIndex < ARRAY_COUNT_U32(s_threadNamePool) ? s_threadNamePool[ threadIndex ] : "red Task Thread..." )
	, m_taskScheduler( taskScheduler )
	, m_taskDispatcher( taskDispatcher )
	, m_threadIndex( threadIndex )
	, m_shutdownFlag( false )
{
}

CTaskThread::~CTaskThread()
{
//	m_taskDispatcher.Shutdown();
}

CTaskDispatcher& CTaskThread::GetTaskDispatcher()
{
	return m_taskDispatcher;
}

const CTaskDispatcher& CTaskThread::GetTaskDispatcher() const
{
	return m_taskDispatcher;
}

void CTaskThread::ThreadFunc()
{
	Memory::RegisterCurrentThread();

	GTaskThreadIndex = m_threadIndex;

	for( ;; )
	{
#if DEBUG_TASKS_LEVEL >= DEBUG_TASKS_LEVEL_MEDIUM
		m_debugTaskThreadLastActiveTime.SetNow();
#endif // DEBUG_TASKS_LEVEL

		RunAllTasks();

		// Run all tasks *again* now to account for the race condition of work signaled right before resetting it
		// There can be no tasks on the thread-local queue anymore, so loop while there are still tasks on the scheduler.
		//
		// (If a task *were* being pushed on the local queue directly (subtask or by thread id) then we would still be
		// in the first RunAllTasks() - although it would be an error if a task could go directly onto the local queue
		// from another thread.)
		//
		// While it would be enough to just RunAllTasks() here before waiting for work, let's avoid the case where
		// we run an entire batch of tasks on just one thread

		if ( m_shutdownFlag.GetValue() )
		{
#ifdef USE_CONCURRENCY_VISUALIZER
			CVThreadMarkerSeries.write_alert( TXT("Shutdown request received!") );
#endif
			// Undo resetting work and exit ThreadFunc.
			// Shutdown flag originally set before broadcasting work in order to wake
			// any threads up. Checked after resetting work to not swallow a broadcast. There's also nothing in
			// the local taskdispatcher at this point.
			m_taskScheduler.GetWorkToken().SignalWork(4);
			return;
		}

		while ( RunOneSchedulerTask_BroadcastWork() )
		{
			RunAllTasks();
		}

		m_taskScheduler.GetWorkToken().WaitForWork();
	}
}

void CTaskThread::RunAllTasks()
{
	CTaskRunner taskRunner;
	CTask* task = nullptr;
	do
	{
		task = m_taskDispatcher.PopTask();

		if ( ! task )
		{
			task = m_taskScheduler.PopTask();
		}

		if ( ! task )
		{
			task = m_taskScheduler.StealTask();
		}

		if ( task )
		{
			taskRunner.RunTask( *task, m_taskDispatcher );
			task->Release();
		}

	} while( task );
}

Bool CTaskThread::RunOneSchedulerTask_BroadcastWork()
{
	CTask* task = m_taskScheduler.PopTask();
	if ( task )
	{
#ifdef USE_CONCURRENCY_VISUALIZER
		CVThreadMarkerSeries.write_message( Concurrency::diagnostic::high_importance, TXT("Rebroadcasting work!") );
#endif
		m_taskScheduler.GetWorkToken().SignalWork(1);
		CTaskRunner taskRunner;
		taskRunner.RunTask( *task, m_taskDispatcher );
		task->Release();

		return true;
	}
	return false;
}

