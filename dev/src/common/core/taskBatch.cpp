/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "taskBatch.h"
#include "taskDispatcher.h"
#include "taskRunner.h"
#include "taskManager.h"
#include "profiler.h"

//////////////////////////////////////////////////////////////////////////
// CTaskBatch::SSyncToken
//////////////////////////////////////////////////////////////////////////
STaskBatchSyncToken::STaskBatchSyncToken()
	: m_syncCount( 0 )
	, m_waitCount( 0 )
{
}

STaskBatchSyncToken::~STaskBatchSyncToken()
{
#ifdef RED_ASSERTS_ENABLED
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	RED_ASSERT( m_syncCount >= m_waitCount, TXT("CTaskSync destructed before finishing sync!") );
#endif
}

void STaskBatchSyncToken::Sync()
{
	Bool wake = false;

#ifdef RED_ASSERTS_ENABLED
	Int32 debugSyncCount = 0;
	Int32 debugWaitCount = 0;
#endif

	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

		++m_syncCount;
		wake = ( m_syncCount == m_waitCount );

#ifdef RED_ASSERTS_ENABLED
		debugSyncCount = m_syncCount;
		debugWaitCount = m_waitCount;
#endif

		// Gah! Spurious wakeup from waiting, so sees syncCount done and exits before waking here... hm.
		// -> Bullshit? Sync token may be destructed after this wake, so never put this inside the scoped mutex lock
		if ( wake )
		{
			m_condVar.WakeAny();
		}
	}

	RED_FATAL_ASSERT( debugSyncCount <= debugWaitCount, "Sync count '%d' exceeds expected max '%d'. Expect crashes if the sync token is destructed!", debugSyncCount, debugWaitCount );
}

void STaskBatchSyncToken::Wait()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

	while ( m_syncCount < m_waitCount )
	{
		m_condVar.Wait( m_mutex );
	}

	// Reset after a wait in case task batch reused.
	m_syncCount = 0;
	m_waitCount = 0;
}

//////////////////////////////////////////////////////////////////////////
// CTaskBatch
//////////////////////////////////////////////////////////////////////////
void CTaskBatch::IssueAndWait( ETaskSchedulePriority taskSchedulePriority /*= TSP_Normal*/ )
{
	if ( m_taskStack.Empty() )
	{
		return;
	}

	DoIssue( taskSchedulePriority );

	{
		PC_SCOPE_PIX( TaskBatchWait );

#ifdef USE_CONCURRENCY_VISUALIZER
		//TBD: Maybe use category for filters
		Concurrency::diagnostic::span spanRunTaskWait( CVThreadMarkerSeries, TXT("Task batch wait for finish") );
#endif
		m_syncToken.Wait();
	}
}

void CTaskBatch::Issue( ETaskSchedulePriority taskSchedulePriority /*= TSP_Normal */ )
{
	if ( m_taskStack.Empty() )
	{
		return;
	}

	DoIssue( taskSchedulePriority );
}

void CTaskBatch::IssueWithoutInlining( ETaskSchedulePriority taskSchedulePriority /*= TSP_Normal */ )
{
	if ( m_taskStack.Empty() )
	{
		return;
	}

	DoIssueWithoutInlining( taskSchedulePriority );
}

void CTaskBatch::FinishTasksAndWait()
{
	RunTasksInline();
	m_syncToken.Wait();
}

void CTaskBatch::DoIssueWithoutInlining( ETaskSchedulePriority taskSchedulePriority )
{
	// TODO: Make sure not on a task thread and that the task can't recursively call Wait() with its own tasksync!
	// Although if on a task thread, the tasks you're synching must also be on the same thread

	PC_SCOPE_PIX( TaskBatchIssueWithoutInlining );

#ifdef USE_CONCURRENCY_VISUALIZER
	const Uint32 debugLargeTaskBatchSize = 100;
	if ( m_taskStack.Size() > debugLargeTaskBatchSize )
	{
		CVThreadMarkerSeries.write_alert( TXT("Large task batch of size %u"), debugLargeTaskBatchSize );
	}
#endif

	GTaskManager->Issue( m_taskStack.TypedData(), m_taskStack.Size(), taskSchedulePriority );
}

void CTaskBatch::DoIssue( ETaskSchedulePriority taskSchedulePriority )
{
	// TODO: Make sure not on a task thread and that the task can't recursively call Wait() with its own tasksync!
	// Although if on a task thread, the tasks you're synching must also be on the same thread

	PC_SCOPE_PIX( TaskBatchIssue );

#ifdef USE_CONCURRENCY_VISUALIZER
	const Uint32 debugLargeTaskBatchSize = 100;
	if ( m_taskStack.Size() > debugLargeTaskBatchSize )
	{
		CVThreadMarkerSeries.write_alert( TXT("Large task batch of size %u"), debugLargeTaskBatchSize );
	}
#endif

	GTaskManager->Issue( m_taskStack.TypedData(), m_taskStack.Size(), taskSchedulePriority );

	RunTasksInline();
}

void CTaskBatch::RunTasksInline()
{
#ifdef USE_CONCURRENCY_VISUALIZER
	//TBD: Maybe use category for filters
	CVThreadMarkerSeries.write_flag( Concurrency::diagnostic::normal_importance, TXT("Task batch run inline") );
#endif

	// Use a stack with the intent of spending more time processing tasks than to iterate through tasks already running.
	RED_ASSERT( GTaskManager );
	CTaskDispatcher taskDispatcher( *GTaskManager );
	CTaskRunner taskRunner;

	while ( ! m_taskStack.Empty() )
	{
		CTask* task = m_taskStack.PopBackFast();
		RED_ASSERT( task );
		taskRunner.RunTask( *task, taskDispatcher );
		task->Release();
		for ( ;; ) 
		{
			task = taskDispatcher.PopTask();
			if ( !task )
			{
				break;
			}

			taskRunner.RunTask( *task, taskDispatcher );
			task->Release();
		}
	}
}

void CTaskBatch::Add( CTask& task )
{
	task.AddRef();
	
	m_taskStack.PushBack( &task );

	++m_syncToken.m_waitCount;
}

CTaskBatch::CTaskBatch(  Uint32 reserveCount )
{
	m_taskStack.Reserve( reserveCount );
}

CTaskBatch::~CTaskBatch()
{
	RED_FATAL_ASSERT( m_taskStack.Empty(), "Task stack not empty on batch destruction" );
}
