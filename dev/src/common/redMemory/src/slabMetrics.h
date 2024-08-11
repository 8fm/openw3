/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_SLAB_METRICS_H_
#define _RED_MEMORY_SLAB_METRICS_H_

namespace red
{
namespace memory
{
	struct SlabMetrics
	{
		u32 chunkCount;
		u32 chunkCountPeak;
		u32 slabCountPerSize[ c_slabSizeStepCount ];
		u32 blockAllocatedPerSize[ c_slabSizeStepCount ];
	};
}
}

#endif
