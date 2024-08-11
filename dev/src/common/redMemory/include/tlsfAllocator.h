/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_INCLUDE_TLSF_ALLOCATOR_H_
#define _RED_MEMORY_INCLUDE_TLSF_ALLOCATOR_H_

#include "redMemoryInternal.h"
#include "defaultAllocator.h"

#include "../src/dynamicTlsfAllocator.h"
#include "../src/lockingDynamicTlsfAllocator.h"
#include "../src/staticTlsfAllocator.h"

namespace red
{
namespace memory
{
	class DynamicTLSFAllocator;
	class LockingDynamicTLSFAllocator;
	class StaticTLSFAllocator;
}
}

#endif
