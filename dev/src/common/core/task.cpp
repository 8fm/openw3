/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "task.h"
#include "taskDispatcher.h"
#include "taskRunner.h"
#include "taskManager.h"

CTask::CTask( Uint32 taskFlags /*=0*/ )
	: CTaskSchedNode()
	, m_taskFlags( taskFlags )
	, m_lifeRefCount( 1 )
	, m_taskDispatcher( nullptr )
{
}

CTask::~CTask()
{
}

Int32 CTask::AddRef()
{
	const Int32 newRefCount = m_lifeRefCount.Increment();
	RED_ASSERT( newRefCount > 0 );

	return newRefCount;
}

Int32 CTask::Release()
{
	const Int32 newRefCount = m_lifeRefCount.Decrement();
	RED_ASSERT( newRefCount >= 0 );

	if ( newRefCount == 0 )
	{
		delete this;
	}

	return newRefCount;
}

void CTask::TryCancel()
{
	(void)MarkCancelled();
}

void* CTask::operator new( size_t size, ETaskRoot )
{ 
	RED_ASSERT( GTaskManager );

	CTaskAllocator& allocator = GTaskManager->GetTaskAllocator();
	void* ptr = allocator.AllocateTask( size );

	return ptr;
}

void CTask::operator delete( void* ptr )
{
	RED_ASSERT( GTaskManager );

	CTaskAllocator& allocator = GTaskManager->GetTaskAllocator();
	allocator.DeallocateTask( ptr );
}
