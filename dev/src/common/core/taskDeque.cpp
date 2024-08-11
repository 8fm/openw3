/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskDeque.h"
#include "taskSchedNode.h"

//////////////////////////////////////////////////////////////////////////
// CTaskDeque
//////////////////////////////////////////////////////////////////////////
template< typename TMutex >
CTaskDeque<TMutex>::CTaskDeque()
	: m_put( 0 )
	, m_get( 0 )
	, m_count( 0 )
{
	Red::MemoryZero( &m_tasks, sizeof(m_tasks) );
}

template< typename TMutex>
CTaskDeque<TMutex>::~CTaskDeque()
{
}

template< typename TMutex>
void CTaskDeque<TMutex>::PushBack( CTask* node )
{
	RED_FATAL_ASSERT( node != nullptr, "Cannot push empty task" );
	if ( !node )
		return;

	// Two thread might manipulate m_count... unfortunately we do handle that case very well. 
	// The following is just doing atomic operation, so this is a very very fast lock. And wont have any contention except if PopFront is called.
	Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_lock ); 

	// put item
	const Int32 putIndex = m_put.Increment() % MAX_TASKS;
	const CTask* prev = m_tasks[ putIndex ].Exchange( node );
	RED_FATAL_ASSERT( prev == nullptr, "Overflow of the task deque" );

	// check count
	const Int32 count = m_count.Increment();
	RED_FATAL_ASSERT( count < (Int32) MAX_TASKS, "Overflow of the tasks deque" );
}

template< typename TMutex>
Bool CTaskDeque<TMutex>::PopFront( CTask*& /*[out]*/ outNode )
{
	{
		// Two thread might manipulate m_count... unfortunately we do handle that case very well. 
		// The following is just doing atomic operation, so this is a very very fast lock. And wont have any contention except if PushBack is called.
		Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_lock ); 

		// get count
		const Int32 count = m_count.Decrement();
		if ( count < 0 )
		{
			// no tasks in queue
			m_count.Increment();
			return false;
		}
	}

	// pop the task index and we need to wait for it to be fully written
	// this can happen when more than one thread is adding to the queue
	const Int32 getIndex = m_get.Increment() % MAX_TASKS;
	while ( nullptr == (outNode = m_tasks[ getIndex ].Exchange( nullptr )) ) {};

	// clean
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Explicit Instantations
//////////////////////////////////////////////////////////////////////////
template class CTaskDeque< Red::Threads::CMutex >;
template class CTaskDeque< Red::Threads::CNullMutex >;
