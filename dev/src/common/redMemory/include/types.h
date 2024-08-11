/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_TYPES_H_
#define _RED_MEMORY_TYPES_H_

#include "../../redSystem/types.h"

namespace red
{
namespace MemoryFramework
{
	// LEGACY TYPES
	typedef red::Uint32		PoolLabel;			// Used to identify a memory pool
	typedef red::Uint32		MemoryClass;		// Used to identify a type of allocation (used for metrics)
}
}

namespace red
{
namespace memory
{
	typedef red::Int8 i8;
	typedef red::Uint8 u8;

	typedef red::Int16 i16;
	typedef red::Uint16 u16;

	typedef red::Int32 i32;
	typedef red::Uint32 u32;

	typedef red::Int64 i64;
	typedef red::Uint64 u64;
}
}

#if defined( RED_COMPILER_MSC ) && ( defined( RED_DLL ) || defined( RED_WITH_DLL ) )

	#define RED_MEMORY_INLINE __forceinline
	
#else

	#define RED_MEMORY_INLINE inline

#endif

#endif
