/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_METRICS_UTILS_H_
#define _RED_MEMORY_METRICS_UTILS_H_

#include "../include/poolTypes.h"

namespace red
{
namespace memory
{
	struct Block;

	RED_MEMORY_API void AddAllocateMetric( PoolHandle handle, const Block & block );
	RED_MEMORY_API void AddFreeMetric( PoolHandle handle,const Block & block );
	RED_MEMORY_API void AddReallocateMetric( PoolHandle handle,const Block & input, const Block & output );
}
}

#include "metricsUtils.hpp"

#endif
