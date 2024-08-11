/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_DEBUG_ALLOCATOR_CONSTANT_H_
#define _RED_MEMORY_DEBUG_ALLOCATOR_CONSTANT_H_

namespace red
{
namespace memory
{
	const u32 c_debugAllocatorDefaultAlignment = 16;

	const u8 c_debugAllocatorUnitTestAllocFiller = 0xa3;
	const u8 c_debugAllocatorUnitTestFreeFiller = 0xf3;
}
}

#endif
