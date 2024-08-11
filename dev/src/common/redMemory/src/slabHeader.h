/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_HEADER_H_
#define _RED_MEMORY_SLAB_HEADER_H_

#include "slabList.h"

namespace red
{
namespace memory
{
	struct Block;

	struct SlabHeader
	{
		// 0 byte; first cache line.
		SlabFreeList ** freeListTail; // Local allocator free list. For Lockless implementation.
		SlabList listNode; 
		u64 lastBlock;
		u32 blockSize;
		u8 padding[ 28 ];

		// 64 byte; second cache line.
		u64 firstFree;
		u32 blockUsed;
		u32 emptyFlag;
		// 80 byte; slab data need to be aligned on 16
		void * data; 
	};

	SlabHeader * GetSlabHeader( u64 block );
	SlabHeader * GetSlabHeader( const Block & block );
	RED_MEMORY_API u32 GetSlabBlockSize( u64 block );
	u32 GetSlabBlockSize( const void * block );
	void PushBlockToFreeList( Block & block );
	void PushBlockToFreeList( u64 block );
	void PushBlockToFreeList( u64 block, SlabHeader * header );
}
}

#include "slabHeader.hpp"

#endif
