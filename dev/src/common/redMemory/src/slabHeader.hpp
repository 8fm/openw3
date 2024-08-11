/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_HEADER_HPP_
#define _RED_MEMORY_SLAB_HEADER_HPP_

#include "slabConstant.h"
#include "block.h"
#include "../include/utils.h"

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE u32 GetSlabBlockSize( u64 block )
	{
		return GetSlabHeader( block )->blockSize;
	}

	RED_MEMORY_INLINE u32 GetSlabBlockSize( const void * block )
	{
		return GetSlabBlockSize( AddressOf( block ) );
	}

	RED_MEMORY_INLINE SlabHeader * GetSlabHeader( u64 block )
	{
		// Slab are aligned to their size (which is always power of 2).
		// Getting the header is just a matter of chopping the bit.
		const u64 mask = ~static_cast< u64 >( c_slabSize - 1 );
		const u64 headerPosition = block & mask;
		SlabHeader * header = reinterpret_cast< SlabHeader* >( headerPosition );
		return header;
	}

	RED_MEMORY_INLINE SlabHeader * GetSlabHeader( const Block & block )
	{
		return GetSlabHeader( block.address );
	}
}
}

#endif

