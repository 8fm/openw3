/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_DEBUG_ALLOCATOR_H_
#define _RED_MEMORY_DEBUG_ALLOCATOR_H_

#include "allocator.h"
#include "block.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API DebugAllocator
	{
	public:

		RED_MEMORY_DECLARE_ALLOCATOR( DebugAllocator, 0xDBBADF0D, 16 );

		DebugAllocator();
		~DebugAllocator();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );
		Block Reallocate( Block & block, u32 size );
		void Free( Block & block );

		bool OwnBlock( u64 block ) const;
		u64 GetBlockSize( u64 address ) const;
	
	private:

		DebugAllocator( const DebugAllocator& );
		DebugAllocator & operator=( const DebugAllocator& );
	};

	DebugAllocator & AcquireDebugAllocator();
}
}

#endif
