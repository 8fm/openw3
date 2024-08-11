/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "slabChunk.h"
#include "assert.h"
#include "utils.h"
#include "block.h"

namespace red
{
namespace memory
{
	const u64 c_slabChunkSize = c_slabSize * c_slabMaxCount;
	const u32 c_slabChunkAlignment = c_slabSize;

	SlabChunk * CreateSlabChunk( const SystemBlock & block )
	{
		RED_MEMORY_ASSERT( IsAligned( block.address, c_slabSize ), "Chunk address must be align on the size of a single Slab." );
		RED_MEMORY_ASSERT( block.size == c_slabChunkSize, "Allocated memory block size is incorrect." );  

		SlabChunk * chunk = reinterpret_cast< SlabChunk * >( block.address );
		
		for( u32 index = 0; index != c_slabMaxCount - 1; ++index )
		{
			const u32 offset = c_slabSize * ( index + 1 );
			void * block = reinterpret_cast< u8 *>( chunk ) + offset;
			chunk->slabs[ index ] = static_cast< SlabHeader* >( block );
		}

		chunk->slabCount = c_slabMaxCount - 1;

		return chunk;
	}

	SystemBlock ComputeRequiredSystemBlockForSlabChunk( u64 startingAddress )
	{
		u64 address = AlignAddress( startingAddress, c_slabChunkAlignment );
		u64 size = c_slabChunkSize;
		const SystemBlock block = { address, size };
		return block;
	}

	bool IsSlabChunkInUse( SlabChunk * chunk )
	{
		return chunk->slabCount < c_slabMaxCount - 1;
	}

	bool IsSlabChunkOwnerOfBlock( SlabChunk * chunk, const Block & block )
	{
		u64 chunkValue = reinterpret_cast< u64 >( chunk );
		return block.address - chunkValue < c_slabChunkSize;
	}
}
}
