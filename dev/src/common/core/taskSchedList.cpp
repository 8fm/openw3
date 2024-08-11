/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskSchedList.h"
#include "task.h"

//////////////////////////////////////////////////////////////////////////
// CTaskSchedList
//////////////////////////////////////////////////////////////////////////
CTask* CTaskSchedList::PopTask()
{
	CTask* task = nullptr;
	Uint32 i = 0;
	for (; i < TSP_Count; ++i )
	{
		CTask* node = nullptr;
		TTaskDequeNTS& taskDeque = m_taskPriQueue[ i ].m_taskDequeNTS;
		Uint32 newSize = 0;
		if ( taskDeque.PopFront( node ) )
		{
			task = node;
			break;
		}
	}

	return task;
}

CTask* CTaskSchedList::StealTask()
{
	CTask* node = nullptr;
	for ( Uint32 i = 0; i < TSP_Count; ++i )
	{
		if ( m_taskPriQueue[ i ].m_taskDequeNTS.PopFront( node ) )
		{
			return static_cast< CTask* >( node );
		}
	}

	return nullptr;
}

void CTaskSchedList::PushFront( CTask& schedNode, ETaskSchedulePriority taskSchedulePriority )
{
	const Uint32 index = static_cast< Uint32 >( taskSchedulePriority );
	m_taskPriQueue[ index ].m_taskDequeNTS.PushBack( &schedNode );
}

void CTaskSchedList::PushBack( CTask& schedNode, ETaskSchedulePriority taskSchedulePriority )
{
	const Uint32 index = static_cast< Uint32 >( taskSchedulePriority );
	m_taskPriQueue[ index ].m_taskDequeNTS.PushBack( &schedNode );
}
