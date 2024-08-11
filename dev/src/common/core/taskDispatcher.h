/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "taskDebug.h"
#include "taskSchedList.h"
#include "taskAllocator.h"
#include "task.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CTaskManager;

//////////////////////////////////////////////////////////////////////////
// CTaskDispatcher
//////////////////////////////////////////////////////////////////////////
class CTaskDispatcher : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Task, MC_Engine, RED_MAX_CACHE_ALIGNMENT );

private:
	TTaskDeque								m_taskDeque;
	CTaskManager*							m_manager;

public:
											CTaskDispatcher( CTaskManager& manager );
	virtual									~CTaskDispatcher();

public:
	CTask*									PopTask();
	CTask*									StealTask();

public:
	// No child tasks here so that task stealing can better still bigger tasks
	Bool									IssueFront( CTask& task );
	Bool									IssueBack( CTask& task );
};