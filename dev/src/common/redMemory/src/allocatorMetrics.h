/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_ALLOCATOR_METRICS_H_
#define _RED_MEMORY_ALLOCATOR_METRICS_H_

namespace red
{
namespace memory
{
	struct AllocatorMetrics
	{
		u64 consumedSystemMemoryBytes;
		u64 consumedMemoryBytes;
		u64 bookKeepingBytes;
		u64 smallestBlockSize;
		u64 largestBlockSize;
	};	
}
}

#endif
