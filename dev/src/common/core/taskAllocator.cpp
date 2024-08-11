/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "taskAllocator.h"

//////////////////////////////////////////////////////////////////////////
// CTaskAllocator
//////////////////////////////////////////////////////////////////////////

const Uint32 MaxTaskCount = 1280; // 1280 * 256 byte per task = 320 KB

CTaskAllocator::CTaskAllocator()
	: m_buffer( nullptr )
{
	m_buffer = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Task, MC_Task, MaxTaskCount * SMALL_TASK_SIZE, RED_TASK_ALIGNMENT ) );
	m_taskPool.Init( m_buffer, MaxTaskCount * SMALL_TASK_SIZE, SMALL_TASK_SIZE );
}

CTaskAllocator::~CTaskAllocator()
{
	RED_MEMORY_FREE( MemoryPool_Task, MC_Task, m_buffer );
}

void* CTaskAllocator::AllocateTask( size_t size )
{
	const size_t maxBlockAllocSize = SMALL_TASK_SIZE;
	RED_WARNING_ONCE( size <= maxBlockAllocSize,  "Make your task classes smaller %llu > %llu!", size, maxBlockAllocSize );

	//const size_t allocSize = size + sizeof( CTask::STag );
	Uint8* mem = nullptr;
	if ( size <= maxBlockAllocSize )
	{
		Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_mutex );
		mem = static_cast< Uint8* >( m_taskPool.AllocateBlock() );
		RED_WARNING_ONCE( mem != nullptr, "Task Pool is full. Fallback on default allocator. Consider increasing task pool or batching" );
	}

	if( mem == nullptr )
	{
		mem = static_cast< Uint8* >( RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_Task, MC_Task, size, RED_TASK_ALIGNMENT ) );
	}
	
	return mem;
}

void CTaskAllocator::DeallocateTask( void* ptr )
{
	RED_FATAL_ASSERT( ( reinterpret_cast< uintptr_t >( ptr ) & ( RED_TASK_ALIGNMENT-1) ) == 0, "CTask memory not aligned!" );
	Uint8* mem = static_cast< Uint8* >( ptr );

	if( mem >= m_buffer && mem < ( m_buffer + MaxTaskCount * SMALL_TASK_SIZE ) )
	{
		Red::Threads::CScopedLock< Red::Threads::CSpinLock > lock( m_mutex );
		m_taskPool.FreeBlock( mem );	
	}
	else
	{
		RED_MEMORY_FREE(  MemoryPool_Task, MC_Task, mem );
	}

}
