/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_ALLOCATOR_H_
#define _RED_MEMORY_SLAB_ALLOCATOR_H_

#include "../include/utils.h"

#include "slabConstant.h"
#include "slabList.h"
#include "virtualRange.h"
#include "block.h"

#include "allocatorMetrics.h"

namespace red
{
namespace memory
{
	class SlabChunkAllocatorInterface;
	struct SlabChunk;
	struct SlabHeader;

	struct SlabAllocatorMetrics
	{
		AllocatorMetrics metrics;
	};

	struct SlabAllocatorParameter
	{
		VirtualRange reservedVirtualRange;
		SlabChunkAllocatorInterface * chunkAllocator;
	};

	class RED_MEMORY_API SlabAllocator
	{	
	public:
	
		SlabAllocator();
		~SlabAllocator();

		void Initialize( const SlabAllocatorParameter & parameter );
		void Uninitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignement );
		void Free( Block & block );

		Block Reallocate( Block & block, u32 size );

		bool OwnBlock( u64 block ) const;

		// For Lockless implementation. Do not use.
		void SetId( u32 id );
		void ForceProcessFreeList();

		void BuildMetrics( SlabAllocatorMetrics & metrics );

		// UNIT TEST ONLY
		u32 InternalGetAvailableSlabCount();
		const SlabChunk * InternalGetChunk();
		void InternalMarkChunkAsEmpty();
		void InternalMarkSlabAsEmpty( u32 allocSize );
		const SlabFreeList * InternalGetFreeListTail() const;

	private:

		SlabAllocator( const SlabAllocator & );
		SlabAllocator& operator=( const SlabAllocator & );

		SlabList & GetSlabList( u32 size );

		Block TakeBlockFromSlab( SlabList & slab );
		void AllocateSlab( SlabList & slab, u32 size );
		SlabHeader * TakeSlabFromChunk();
		void InitializeSlab( SlabList & slab, u32 size, SlabHeader * header );

		void FreeLocal( u64 block, SlabHeader * header );
		void FreeSlab( SlabHeader * header );
		void FreeChunk( SlabChunk * chunk );
		void PushSlabToEmptyList( SlabHeader * header );
		void ProcessFreeList();

		bool OwnSlabHeader( SlabHeader * header );

		bool IsInitialized() const;

		// 0 byte. 16 first bucket are aligned on 8 byte, 24 other are aligned on 16
		SlabList m_slabs[ 40 ];

		// 640 byte
		SlabList m_emptySlabList;
		SlabChunk * m_chunk;
		SlabFreeList * m_freeListHead;
		SlabFreeList m_freeListSentinel;
		VirtualRange m_reservedMemoryRange;
		u32 m_memoryReserved;
		u32 m_memoryUsed;

		// 704 byte, new cache line
		// free list tail must not be on same cache line than head to reduce false sharing.
		SlabFreeList * m_freeListTail;

		SlabChunkAllocatorInterface * m_chunkAllocator;
		u32 m_id;

		u8 m_padding2[ 44 ];
	};
}
}

#include "slabAllocator.hpp"

#endif
