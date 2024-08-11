/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_H_
#define _RED_MEMORY_POOL_H_

#include "block.h"
#include "proxy.h"
#include "proxyTypeId.h"

namespace red
{
namespace memory
{
	class RED_MEMORY_API Pool
	{
	public:

		RED_MEMORY_DECLARE_PROXY( Pool, 8 );
		RED_MEMORY_PROXY_TYPE_ID( 0 );
		
		RED_MOCKABLE Block Allocate( u32 size ) const;
		RED_MOCKABLE Block AllocateAligned( u32 size, u32 alignment ) const;
		RED_MOCKABLE Block Reallocate( Block & block, u32 size ) const;
		RED_MOCKABLE void Free( Block & block ) const;
		u64 GetBlockSize( u64 address ) const;

	protected:

		Pool();
		~Pool();

	private:
	
		virtual Block OnAllocate( u32 size ) const = 0;
		virtual Block OnAllocateAligned( u32 size, u32 alignment ) const = 0;
		virtual Block OnReallocate( Block & block, u32 size ) const = 0;
		virtual void OnFree( Block & block ) const = 0;
		virtual u64 OnGetBlockSize( u64 address ) const = 0;
	};
}
}

#include "pool.hpp"

#endif
