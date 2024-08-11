/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_CHUNK_H_
#define _RED_MEMORY_SLAB_CHUNK_H_

#include "slabConstant.h"
#include "systemBlock.h"

namespace red
{
namespace memory
{
	struct SlabHeader;
	struct Block;

	struct SlabChunk
	{
		SlabChunk * nextFree; // Chunk Free List. See Lockless implementation.
		u32 slabCount;
		SlabHeader * slabs[ c_slabMaxCount - 1 ];
	};

	RED_MEMORY_API SlabChunk * CreateSlabChunk( const SystemBlock & block );
	RED_MEMORY_API SystemBlock ComputeRequiredSystemBlockForSlabChunk( u64 startingAddress );
	RED_MEMORY_API bool IsSlabChunkInUse( SlabChunk * chunk );
	RED_MEMORY_API bool IsSlabChunkOwnerOfBlock( SlabChunk * chunk, const Block & block );
}
}

#endif
