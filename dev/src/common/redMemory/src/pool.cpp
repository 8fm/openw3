/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "pool.h"

namespace red
{
namespace memory
{
	Block Pool::Allocate( u32 size ) const
	{
		return OnAllocate( size );
	}

	Block Pool::AllocateAligned( u32 size, u32 alignment ) const
	{
		return OnAllocateAligned( size, alignment );
	}
	
	Block Pool::Reallocate( Block & block, u32 size ) const
	{
		return OnReallocate( block, size );
	}
	
	void Pool::Free( Block & block ) const
	{
		OnFree( block );
	}

	u64 Pool::GetBlockSize( u64 address ) const
	{
		return OnGetBlockSize( address );
	}
}
}
