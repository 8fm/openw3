/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "task.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CTaskAllocator
//////////////////////////////////////////////////////////////////////////
class CTaskAllocator : private Red::System::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Task, MC_Engine, RED_MAX_CACHE_ALIGNMENT );

private:
#ifdef RED_ARCH_X64
	static const size_t					SMALL_TASK_SIZE = 256;
#else
	static const size_t					SMALL_TASK_SIZE = 128;
#endif

	static_assert( SMALL_TASK_SIZE >= RED_MAX_CACHE_ALIGNMENT, "Task size is too small" );
	static_assert( SMALL_TASK_SIZE != 0 && (SMALL_TASK_SIZE & (SMALL_TASK_SIZE-1)) == 0, "Not pow2" );

public:

	CTaskAllocator();
	~CTaskAllocator();

	void*									AllocateTask( size_t size );
	void									DeallocateTask( void* ptr );

private:

	CPool m_taskPool;
	Uint8 * m_buffer;
	Red::Threads::CSpinLock m_mutex;
};