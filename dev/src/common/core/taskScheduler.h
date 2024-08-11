/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskSchedList.h"
#include "taskGroup.h"

#include <atomic>

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTaskManager;
class CTask;

//////////////////////////////////////////////////////////////////////////
// CTaskScheduler
//////////////////////////////////////////////////////////////////////////
class CTaskScheduler : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Task, MC_Engine, RED_MAX_CACHE_ALIGNMENT );

public:
	struct SWorkToken : private Red::System::NonCopyable
	{
	private:
		Red::Threads::CSemaphore			m_lock;
		Red::Threads::CAtomic< Int32 >		m_count;
		Red::Threads::CAtomic< Int32 >		m_waiting;

	public:
											SWorkToken();
											~SWorkToken();

	public:
		void								WaitForWork();
		void								SignalWork( const Uint32 workCount = 1 );
	};

private:
	typedef TDynArray< CTaskThread* >		TTaskThreadList;
	typedef TDynArray< CTaskDispatcher* >	TTaskDispatcherList;

private:
	CTaskManager&							m_taskManager;
	CTaskSchedList							m_taskSchedListBacklog;
	TTaskThreadList							m_taskThreadList;
	TTaskDispatcherList						m_taskDispatcherList;

private:
	SWorkToken								m_workToken;

public:
											CTaskScheduler( CTaskManager& taskManager );
 											~CTaskScheduler();
	Bool									Init( const STaskGroupInitParams::SThreadParams& threadParams );
	void									Shutdown();

public:
	CTaskManager&							GetTaskManager();
	const CTaskManager&						GetTaskManager() const;

public:
	SWorkToken&								GetWorkToken();
	const SWorkToken&						GetWorkToken() const;

	const TTaskThreadList&					GetThreadList() const;

public:
	void									IssueTask( CTask& task, ETaskSchedulePriority taskSchedulePriority );
	void									IssueTaskBatch( CTask* tasks[], Uint32 numTasks, ETaskSchedulePriority taskSchedulePriority );

public:
	void									Flush();

public:
	// Called by a task thread
	CTask*									PopTask();
	CTask*									StealTask();

public:
	Uint32									GetNumDedicatedTaskThreads() const { return m_taskThreadList.Size(); }

private:
	void									InitTaskThreads( const STaskGroupInitParams::SThreadParams& threadParams );

private:
	void									IssueTask_NoWorkSignal( CTask& task, ETaskSchedulePriority taskSchedulePriority, Bool& /*[out]*/ issuedDirectly );
};

