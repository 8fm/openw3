/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_LOCKING_DYNAMIC_TLSF_ALLOCATOR_H_
#define _RED_MEMORY_LOCKING_DYNAMIC_TLSF_ALLOCATOR_H_

#include "allocator.h"
#include "dynamicTlsfAllocator.h"

namespace red
{
namespace memory
{

#ifdef RED_PLATFORM_ORBIS

	class AdaptiveMutex
	{
	public:

		AdaptiveMutex();
		~AdaptiveMutex();
	
		void Acquire();
		void Release();
	
	private:
		ScePthreadMutex m_mutex;
	};

#endif


	class LockingDynamicTLSFAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( LockingDynamicTLSFAllocator, 0x14cff314, DynamicTLSFAllocator::DefaultAlignmentType::value );

		LockingDynamicTLSFAllocator();
		~LockingDynamicTLSFAllocator();

		void Initialize( const DynamicTLSFAllocatorParameter & parameter );
		void Uninitialize();	

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 address ) const;

		void BuildMetrics( TLSFAllocatorMetrics & metrics );

	private:

#ifndef RED_PLATFORM_ORBIS
		typedef Red::Threads::CMutex LockPrimitive;
#else
		typedef AdaptiveMutex LockPrimitive;
#endif

		DynamicTLSFAllocator m_allocator;
		LockPrimitive m_lock;
	};
}
}

#endif
