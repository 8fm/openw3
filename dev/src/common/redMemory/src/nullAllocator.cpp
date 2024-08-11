/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "nullAllocator.h"
#include "assert.h"

namespace red
{
namespace memory
{
	NullAllocator::NullAllocator()
	{}

	NullAllocator::~NullAllocator()
	{}

	Block NullAllocator::Allocate( u32 )
	{
		return NullBlock();
	}
	
	Block NullAllocator::AllocateAligned( u32, u32 )
	{
		return NullBlock();
	}
	
	void NullAllocator::Free( Block & block )
	{
		RED_MEMORY_ASSERT( OwnBlock( block.address ), "Block is not owned by allocator." );
		RED_UNUSED( block );
	}
	
	Block NullAllocator::Reallocate( Block & block, u32 /*size*/ )
	{
		RED_MEMORY_ASSERT( OwnBlock( block.address ), "Block is not owned by allocator." );
		RED_UNUSED( block );
		return NullBlock();
	}
	
	bool NullAllocator::OwnBlock( u64 block ) const
	{
		return block == 0;
	}

	u64 NullAllocator::GetBlockSize( u64 ) const
	{
		return 0;
	}

	NullAllocator & AcquireNullAllocator()
	{
		static NullAllocator allocator;
		return allocator;
	}
}
}
