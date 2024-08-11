/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskSched.h"
#include "taskGroup.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTaskAllocator;
class CTaskDispatcher;
class CTaskScheduler;
class CTaskThread;
class CTask;

//////////////////////////////////////////////////////////////////////////
// CTaskManager
//////////////////////////////////////////////////////////////////////////
class CTaskManager : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Task, MC_Engine, RED_MAX_CACHE_ALIGNMENT );

private:
	CTaskAllocator*							m_taskAllocator;
	CTaskScheduler*							m_taskScheduler[ TSG_Count ];

public:
											CTaskManager();
											~CTaskManager();

	Bool									Init( const STaskGroupInitParams& initParams );
	void									Shutdown();
	void									Tick();

public:
	CTaskAllocator&							GetTaskAllocator();
	const CTaskAllocator&					GetTaskAllocator() const;

	const CTaskScheduler&					GetTaskScheduler( ETaskSchedulerGroup taskSchedulerGroup = TSG_Normal ) const;

public:
	void									Issue( CTask& task, ETaskSchedulePriority taskSchedulePriority = TSP_Normal, ETaskSchedulerGroup taskSchedulerGroup = TSG_Normal );
	void									Issue( CTask* tasks[], Uint32 numTasks, ETaskSchedulePriority taskSchedulePriority = TSP_Normal, ETaskSchedulerGroup taskSchedulerGroup = TSG_Normal );

public:
	//! If you think you need to Flush(), think again please.
	void									Flush( ETaskSchedulerGroup taskSchedulerGroup = TSG_Normal );
		
public:
	Uint32									GetNumDedicatedTaskThreads( ETaskSchedulerGroup taskSchedulerGroup = TSG_Normal ) const;
};

//////////////////////////////////////////////////////////////////////////
extern CTaskManager* GTaskManager;