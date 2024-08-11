/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_CHUNK_ALLOCATOR_INTERFACE_H_
#define _RED_MEMORY_SLAB_CHUNK_ALLOCATOR_INTERFACE_H_

namespace red
{
namespace memory
{
	struct SlabChunk;

	class SlabChunkAllocatorInterface
	{
	public:
		virtual SlabChunk * Allocate( u32 allocatorId ) = 0;
		virtual void Free( u32 allocatorId, SlabChunk * chunk ) = 0;

	protected:
		virtual ~SlabChunkAllocatorInterface(){}
	};
}
}

#endif
