/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskManager.h"
#include "taskScheduler.h"
#include "taskSchedList.h"
#include "taskAllocator.h"
#include "task.h"

//////////////////////////////////////////////////////////////////////////
// CTaskManager
//////////////////////////////////////////////////////////////////////////
CTaskManager::CTaskManager()
{
	m_taskAllocator = new CTaskAllocator;

	for ( Uint32 i = 0; i < TSG_Count; ++i )
	{
		m_taskScheduler[ i ] = new CTaskScheduler( *this );
	}
}

Bool CTaskManager::Init( const STaskGroupInitParams& initParams )
{
	for ( Uint32 i = 0; i < TSG_Count; ++i )
	{
		const STaskGroupInitParams::SThreadParams& p = initParams.m_threadParams[ i ];
		if ( p.m_numTaskThreads < 1 )
		{
			continue;
		}

		if ( ! m_taskScheduler[ i ]->Init( p ) )
		{
			return false;
		}
	}

	return true;
}

void CTaskManager::Shutdown()
{
// 	// TBD: Not locking because want to make sure no race condition in submitting tasks from another thread.
 	
	for ( Uint32 i = 0; i < TSG_Count; ++i )
	{
		m_taskScheduler[ i ]->Shutdown();
	}
}

CTaskManager::~CTaskManager()
{
	for ( Uint32 i = 0; i < TSG_Count; ++i )
	{
		delete m_taskScheduler[ i ];
	}

	delete m_taskAllocator;
}

void CTaskManager::Tick()
{
	RED_ASSERT( ::SIsMainThread() );

#if DEBUG_TASKS_LEVEL >= DEBUG_TASKS_LEVEL_MEDIUM
	//CTask::DebugCheckForDeadTasks();
#endif // DEBUG_TASKS_LEVEL

#ifdef USE_CONCURRENCY_VISUALIZER
	CVThreadMarkerSeries.write_flag( Concurrency::diagnostic::high_importance, TXT("TaskManager Tick") );
#endif
}

void CTaskManager::Issue( CTask& task, ETaskSchedulePriority taskSchedulePriority /*= TSP_Normal*/, ETaskSchedulerGroup taskSchedulerGroup /*= TSG_Normal*/ )
{
	m_taskScheduler[ taskSchedulerGroup ]->IssueTask( task, taskSchedulePriority );

#ifdef RED_FULL_DETERMINISM
//.... still check if already run!
#endif
}

void CTaskManager::Issue( CTask* tasks[], Uint32 numTasks, ETaskSchedulePriority taskSchedulePriority /*= TSP_Normal*/, ETaskSchedulerGroup taskSchedulerGroup /*= TSG_Normal*/ )
{
	m_taskScheduler[ taskSchedulerGroup ]->IssueTaskBatch( tasks, numTasks, taskSchedulePriority );

#ifdef RED_FULL_DETERMINISM
	//.... still check if already run!
#endif
}

CTaskAllocator& CTaskManager::GetTaskAllocator()
{
	RED_ASSERT( m_taskAllocator );
	return *m_taskAllocator;
}

const CTaskAllocator& CTaskManager::GetTaskAllocator() const
{
	RED_ASSERT( m_taskAllocator );
	return *m_taskAllocator;
}

const CTaskScheduler& CTaskManager::GetTaskScheduler( ETaskSchedulerGroup taskSchedulerGroup /*= TSG_Normal*/ ) const
{
	RED_ASSERT( m_taskAllocator );
	return *m_taskScheduler[ taskSchedulerGroup ];
}

Uint32 CTaskManager::GetNumDedicatedTaskThreads( ETaskSchedulerGroup taskSchedulerGroup /*= TSG_Normal*/ ) const
{
	return m_taskScheduler[ taskSchedulerGroup ]->GetNumDedicatedTaskThreads();
}

void CTaskManager::Flush( ETaskSchedulerGroup taskSchedulerGroup /*= TSG_Normal*/ )
{
	RED_ASSERT( ::SIsMainThread() );

	m_taskScheduler[ taskSchedulerGroup ]->Flush();
}

//////////////////////////////////////////////////////////////////////////
// GTaskManager
//////////////////////////////////////////////////////////////////////////
CTaskManager* GTaskManager = nullptr;

//////////////////////////////////////////////////////////////////////////
// SCreateTaskManager
//////////////////////////////////////////////////////////////////////////
CTaskManager* SCreateTaskManager()
{
	RED_ASSERT( ! GTaskManager );
	GTaskManager = new CTaskManager;
	return GTaskManager;
}

void SDestroyTaskManager()
{
	if ( GTaskManager )
	{
		delete GTaskManager;
		GTaskManager = nullptr;
	}
}

