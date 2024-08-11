/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_CONSTANT_H_
#define _RED_MEMORY_POOL_CONSTANT_H_

#include "../include/poolTypes.h"

namespace red
{
namespace memory
{	
	const PoolHandle c_poolNodeInvalid = ~0U;	
	const u32 c_poolMaxCount = 128;
}
}

#endif
