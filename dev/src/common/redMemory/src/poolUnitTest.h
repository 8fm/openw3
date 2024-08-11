/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_UNIT_TEST_H_
#define _RED_MEMORY_POOL_UNIT_TEST_H_

#include "../include/pool.h"
#include "dynamicFixedSizeAllocator.h"
#include "defaultAllocator.h"

namespace red
{
namespace memory
{
namespace unitTest
{
	// DO NOT USE. Unit Test ONLY!!!
	RED_MEMORY_POOL( UnitTest_DefaultAllocator_Pool, DefaultAllocator, RED_MEMORY_API );
	RED_MEMORY_POOL( UnitTest_Local_Allocator_Pool, DynamicFixedSizeAllocator, RED_MEMORY_API );
	RED_MEMORY_POOL( UnitTest_External_Allocator_Pool, DynamicFixedSizeAllocator, RED_MEMORY_API );

	RED_MEMORY_API void InitializePools( u32 blockSize );
	RED_MEMORY_API void UninitializePools();

	RED_MEMORY_API void * AllocateFromExternalAllocator( u32 blockSize  );
	RED_MEMORY_API void * AllocateAlignedFromExternalAllocator( u32 blockSize );
	RED_MEMORY_API void FreeFromExternalAllocator( void * block );
	RED_MEMORY_API void * ReallocateFromExternalAllocator( u32 blockSize );
}
}
}

#endif
