/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_LOCKLESS_SLAB_ALLOCATOR_HPP_
#define _RED_MEMORY_LOCKLESS_SLAB_ALLOCATOR_HPP_

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE bool LocklessSlabAllocator::OwnBlock( u64 block ) const
	{
		return block - m_reservedMemoryRange.start < GetVirtualRangeSize( m_reservedMemoryRange ); 
	}
}
}

#endif
