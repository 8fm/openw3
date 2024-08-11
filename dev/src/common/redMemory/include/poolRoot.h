/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_INCLUDE_POOL_ROOT_H_
#define _RED_MEMORY_INCLUDE_POOL_ROOT_H_

#include "redMemoryInternal.h"
#include "pool.h"
#include "defaultAllocator.h"

namespace red
{
namespace memory
{
	RED_MEMORY_POOL( PoolRoot, NullAllocator, RED_MEMORY_API );
		RED_MEMORY_POOL( PoolCPU, NullAllocator, RED_MEMORY_API );
			RED_MEMORY_POOL( PoolDefault, DefaultAllocator, RED_MEMORY_API );	// Allocation made without explicit Pool will be routed to PoolDefault Pool.
			RED_MEMORY_POOL( PoolLegacy, DefaultAllocator, RED_MEMORY_API );	// Allocation made via global new/delete operator will be routed through this pool.
		RED_MEMORY_POOL( PoolGPU, NullAllocator, RED_MEMORY_API );
		RED_MEMORY_POOL( PoolFlexible, NullAllocator, RED_MEMORY_API );
}
}

#endif
