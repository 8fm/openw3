/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */


#ifndef _RED_MEMORY_SLAB_ALLOCATOR_HPP_
#define _RED_MEMORY_SLAB_ALLOCATOR_HPP_

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE Block SlabAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( alignment <= 16 || alignment == 64, "Alignment supported are 8, 16 or 64." );
		RED_MEMORY_ASSERT( size <= 64 || alignment <= 16, "SlabAllocator support only alocation size of less than 64 on 64 alignement" );

		return Allocate( RoundUp( size, alignment ) );
	}
}
}

#endif
