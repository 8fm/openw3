/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_DEFAULT_ALLOCATOR_HPP_
#define _RED_MEMORY_DEFAULT_ALLOCATOR_HPP_

#include "slabHeader.h"
#include "tlsfBlock.h"

namespace red
{
namespace memory
{
	RED_INLINE bool DefaultAllocator::OwnBlock( u64 block ) const
	{
		return m_slabAllocator.OwnBlock( block ) || m_tlsfAllocator.OwnBlock( block );
	}

	RED_INLINE u64 DefaultAllocator::GetBlockSize( u64 address ) const
	{
		RED_MEMORY_ASSERT( OwnBlock( address ), "Block is not own by this allocator." );

		return m_slabAllocator.OwnBlock( address ) ? GetSlabBlockSize( address ) : GetTLSFBlockSize( address );
	}
}
}

#endif
