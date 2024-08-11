/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskDeque.h"
#include "taskSched.h"
#include "engineTime.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTask;

//////////////////////////////////////////////////////////////////////////
// CTaskSchedList
//////////////////////////////////////////////////////////////////////////
class CTaskSchedList
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Task, MC_Engine );

private:
	struct SHead
	{
		TTaskDequeNTS						m_taskDequeNTS;
		EngineTime							m_age;
	};

private:
	EngineTime								m_schedTime;

private:
	SHead									m_taskPriQueue[ TSP_Count ];

public:
	void									SetSchedTime( const EngineTime& schedTime );
	
public:
	void									PushFront( CTask& task, ETaskSchedulePriority taskSchedulePriority );
	void									PushBack( CTask& task, ETaskSchedulePriority taskSchedulePriority );
	CTask*									PopTask();
	CTask*									StealTask();
};