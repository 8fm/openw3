/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskRunner.h"
#include "taskDispatcher.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTaskScheduler;
class CTaskDispatcher;

//////////////////////////////////////////////////////////////////////////
// CTaskThread
//////////////////////////////////////////////////////////////////////////
class CTaskThread : public Red::Threads::CThread
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Task, MC_Engine, RED_MAX_CACHE_ALIGNMENT );

private:
	static const AnsiChar*					s_threadNamePool[];

private:
	CTaskScheduler&							m_taskScheduler;
	CTaskDispatcher&						m_taskDispatcher;
	Uint32									m_threadIndex;
	Red::Threads::CAtomic< Bool >			m_shutdownFlag;

#if DEBUG_TASKS_LEVEL >= DEBUG_TASKS_LEVEL_MEDIUM
public:
	EngineTime								m_debugTaskThreadLastActiveTime;
#endif // DEBUG_TASKS_LEVEL

public:
											CTaskThread( CTaskScheduler& taskScheduler, CTaskDispatcher& taskDispatcher, Uint32 threadIndex );
	virtual									~CTaskThread();

public:
	void									RequestShutdown() { m_shutdownFlag.SetValue( true ); }

public:
	virtual void							ThreadFunc() override;


public:
	CTaskDispatcher&						GetTaskDispatcher();
	const CTaskDispatcher&					GetTaskDispatcher() const;

private:
	void									RunAllTasks();
	bool									RunOneSchedulerTask_BroadcastWork();
};