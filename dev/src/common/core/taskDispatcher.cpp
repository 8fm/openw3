/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskDispatcher.h"
#include "taskManager.h"
#include "taskScheduler.h"
#include "task.h"

//////////////////////////////////////////////////////////////////////////
// CTaskDispatcher
//////////////////////////////////////////////////////////////////////////
Bool CTaskDispatcher::IssueFront( CTask& task )
{
	// CompareExchange in case there's Wait() or TryCancel() on this task
	if ( task.MarkScheduled() )
	{
		task.AddRef();
		m_taskDeque.PushBack( &task ); // hacked
		return true;
	}

	return false;
}

Bool CTaskDispatcher::IssueBack( CTask& task )
{
	// CompareExchange in case there's Wait() or TryCancel() on this task
	if ( task.MarkScheduled() )
	{
		task.AddRef();
		m_taskDeque.PushBack( &task );
		return true;
	}

	return false;
}

CTaskDispatcher::CTaskDispatcher( CTaskManager& manager )
	: m_manager( &manager )
{
}

CTaskDispatcher::~CTaskDispatcher()
{
}

CTask* CTaskDispatcher::PopTask()
{
	CTask* node = nullptr;
	m_taskDeque.PopFront( node );
	return node;
}

CTask* CTaskDispatcher::StealTask()
{
	CTask* node = nullptr;
	m_taskDeque.PopFront( node );
	return nullptr;
}
