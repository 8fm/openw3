/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_STORAGE_H_
#define _RED_MEMORY_POOL_STORAGE_H_

#include "../include/poolTypes.h"
#include "block.h"

namespace red
{
namespace memory
{
	class OOMHandler;

	struct PoolStorage
	{
		void * allocator;
		i64 bytesAllocated;
		OOMHandler * oomHandler;
		PoolHandle handle;
		u32 padding;
	};

	static_assert( sizeof( PoolStorage ) == 32, "PoolStorage size must be 32." );

	template< typename PoolType >
	struct PoolStorageProxy
	{
		typedef typename PoolType::AllocatorType AllocatorType;

		static Block Allocate( u32 size );
		static Block AllocateAligned( u32 size, u32 alignment );
		static Block Reallocate( Block & block, u32 size );
		static void Free( Block & block );
		static u64 GetBlockSize( u64 address );
		
		static AllocatorType & GetAllocator();
		static PoolHandle GetHandle();
		static u64 GetTotalBytesAllocated();

		static void SetAllocator( AllocatorType & allocator );
		static void SetOutOfMemoryHandler( OOMHandler * handler );
	};
}
}

#include "poolStorage.hpp"

#endif
