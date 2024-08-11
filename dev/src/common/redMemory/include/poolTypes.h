/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_TYPES_H_
#define _RED_MEMORY_POOL_TYPES_H_

#include "types.h"

namespace red
{
namespace memory
{
	typedef u32 PoolHandle;

	struct PoolStorage;

	struct PoolParameter
	{
		const char * name;
		PoolStorage * storage;
		u64 budget;
		PoolHandle parentHandle;
	};
}
}

#endif
