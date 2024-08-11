/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_DEFAULT_ALLOCATOR_H_
#define _RED_MEMORY_DEFAULT_ALLOCATOR_H_

#include "allocator.h"
#include "locklessSlabAllocator.h"
#include "lockingDynamicTlsfAllocator.h"

namespace red
{
namespace memory
{
	class SystemAllocator;
	class ThreadMonitor;

	struct DefaultAllocatorMetrics
	{
		AllocatorMetrics metrics;
		LocklessSlabAllocatorMetrics locklessSlabAllocatorMetrics;
		TLSFAllocatorMetrics tlsfAllocatorMetrics;
	};

	struct DefaultAllocatorParameter
	{
		SystemAllocator * systemAllocator;
		ThreadMonitor * threadMonitor;
	};

	class RED_MEMORY_API DefaultAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( DefaultAllocator, 0x1811CCDB, 8 );

		DefaultAllocator();
		~DefaultAllocator();

		void Initialize( const DefaultAllocatorParameter & parameter );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 address ) const;

		void RegisterCurrentThread();

		void BuildMetrics( DefaultAllocatorMetrics & metrics );

		// FOR UNIT TEST ONLY
		bool InternalSlabAllocatorOwnBlock( const Block & block ) const;
		bool InternalTLSFAllocatorOwnBlock( const Block & block ) const;
		void InternalMarkSlabAllocatorAsFull();

	private:

		DefaultAllocator( const DefaultAllocator & );
		DefaultAllocator & operator=( const DefaultAllocator& );

		bool IsInitialized() const;
		Block ReallocateFromSlabAllocator( Block & block, u32 size );
		Block ReallocateFromTLSFAllocator( Block & block, u32 size );

		RED_ALIGN( 64 ) LocklessSlabAllocator m_slabAllocator;
		RED_ALIGN( 64 ) LockingDynamicTLSFAllocator m_tlsfAllocator;

		SystemAllocator * m_systemAllocator;
	};

	RED_MEMORY_API DefaultAllocator & AcquireDefaultAllocator();
}
}

#include "defaultAllocator.hpp"

#endif
