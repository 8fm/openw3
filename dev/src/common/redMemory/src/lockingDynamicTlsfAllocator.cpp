/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "lockingDynamicTlsfAllocator.h"
#include "tlsfBlock.h"

namespace red
{
namespace memory
{
#ifdef RED_PLATFORM_ORBIS
	AdaptiveMutex::AdaptiveMutex()
	{
		ScePthreadMutexattr attr;
		scePthreadMutexattrInit( &attr );
		scePthreadMutexattrSettype( &attr, SCE_PTHREAD_MUTEX_ADAPTIVE );	
		scePthreadMutexattrSetprotocol( &attr, SCE_PTHREAD_PRIO_INHERIT );		
		scePthreadMutexInit( &m_mutex, &attr, nullptr );						
		scePthreadMutexattrDestroy( &attr );						
	}

	AdaptiveMutex::~AdaptiveMutex()
	{
		scePthreadMutexDestroy( &m_mutex );
	}

	void AdaptiveMutex::Acquire()
	{
		scePthreadMutexLock( &m_mutex );
	}

	void AdaptiveMutex::Release()
	{
		scePthreadMutexUnlock( &m_mutex );
	}
#endif

	LockingDynamicTLSFAllocator::LockingDynamicTLSFAllocator()
	{}

	LockingDynamicTLSFAllocator::~LockingDynamicTLSFAllocator()
	{}

	void LockingDynamicTLSFAllocator::Initialize( const DynamicTLSFAllocatorParameter & parameter )
	{
		CScopedLock< LockPrimitive > lock( m_lock );
		m_allocator.Initialize( parameter );
	}
	
	void LockingDynamicTLSFAllocator::Uninitialize()
	{
		CScopedLock< LockPrimitive > lock( m_lock );
		m_allocator.Uninitialize();
	}

	Block LockingDynamicTLSFAllocator::Allocate( u32 size )
	{
		CScopedLock< LockPrimitive > lock( m_lock );
		return m_allocator.Allocate( size );
	}
	
	Block LockingDynamicTLSFAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		CScopedLock< LockPrimitive > lock( m_lock );
		return m_allocator.AllocateAligned( size, alignment );
	}
	
	Block LockingDynamicTLSFAllocator::Reallocate( Block & block, u32 size )
	{
		CScopedLock< LockPrimitive > lock( m_lock );
		return m_allocator.Reallocate( block, size );
	}
	
	void LockingDynamicTLSFAllocator::Free( Block & block )
	{
		if( block.address )
		{
			CScopedLock< LockPrimitive > lock( m_lock );
			m_allocator.Free( block );
		}
	}

	bool LockingDynamicTLSFAllocator::OwnBlock( u64 block ) const
	{
		return m_allocator.OwnBlock( block );
	}

	u64 LockingDynamicTLSFAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );
		return GetTLSFBlockSize( address );
	}

	void LockingDynamicTLSFAllocator::BuildMetrics( TLSFAllocatorMetrics & metrics )
	{
		m_allocator.BuildMetrics( metrics );
	}
}
}
