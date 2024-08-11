/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_PAGE_ALLOCATOR_TYPE_H_
#define _RED_MEMORY_PAGE_ALLOCATOR_TYPE_H_

#if defined( RED_PLATFORM_WINPC )
#include "pageAllocatorWin.h"
#elif defined( RED_PLATFORM_DURANGO )
#include "pageAllocatorDurango.h"
#elif defined( RED_PLATFORM_ORBIS )
#include "pageAllocatorOrbis.h"
#endif

namespace red
{
namespace memory
{
#if defined( RED_PLATFORM_WINPC )
	typedef PageAllocatorWin PlatformPageAllocator;
#elif defined( RED_PLATFORM_DURANGO )
	typedef PageAllocatorDurango PlatformPageAllocator;
#elif defined( RED_PLATFORM_ORBIS )
	typedef PageAllocatorOrbis PlatformPageAllocator;
#endif
}
}

#endif
