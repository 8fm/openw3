/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_SYSTEM_ALLOCATOR_TYPE_H_
#define _RED_MEMORY_SYSTEM_ALLOCATOR_TYPE_H_

#if defined( RED_PLATFORM_WINPC )
#include "systemAllocatorWin.h"
#elif defined( RED_PLATFORM_DURANGO )
#include "systemAllocatorDurango.h"
#elif defined( RED_PLATFORM_ORBIS )
#include "systemAllocatorOrbis.h"
#include "flexibleSystemAllocator.h"
#endif

namespace red
{
namespace memory
{
#if defined( RED_PLATFORM_WINPC )
	typedef SystemAllocatorWin PlatformSystemAllocator;
	typedef SystemAllocatorWin PlatformFlexibleAllocator;
#elif defined( RED_PLATFORM_DURANGO )
	typedef SystemAllocatorDurango PlatformSystemAllocator;
	typedef SystemAllocatorDurango PlatformFlexibleAllocator;
#elif defined( RED_PLATFORM_ORBIS )
	typedef SystemAllocatorOrbis PlatformSystemAllocator;
	typedef FlexibleSystemAllocator PlatformFlexibleAllocator;
#endif

	RED_MEMORY_API SystemAllocator & AcquireSystemAllocator();
	RED_MEMORY_API SystemAllocator & AcquireFlexibleSystemAllocator();
}
}

#endif
