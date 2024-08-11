/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_NULL_ALLOCATOR_H_
#define _RED_MEMORY_NULL_ALLOCATOR_H_

#include "block.h"
#include "allocator.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API NullAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( NullAllocator, 0xA181FC8A, 1 );

		NullAllocator();
		~NullAllocator();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		void Free( Block & block );
		Block Reallocate( Block & block, u32 size );
		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 address ) const;

	private:

		NullAllocator( const NullAllocator& );
		NullAllocator & operator=( const NullAllocator& );
	};

	RED_MEMORY_API NullAllocator & AcquireNullAllocator();
}
}

#endif
