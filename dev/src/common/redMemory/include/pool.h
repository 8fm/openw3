/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_INCLUDE_POOL_H_
#define _RED_MEMORY_INCLUDE_POOL_H_

#include "redMemoryInternal.h"

#include "../src/poolDeclaration.h"

#define RED_MEMORY_POOL_STATIC( poolUniqueName, allocatorType )													\
	_INTERNAL_RED_MEMORY_POOL( poolUniqueName, allocatorType, RED_MEMORY_EMPTY() )		

#if defined( RED_DLL ) || defined( RED_WITH_DLL )

#define RED_MEMORY_POOL( poolUniqueName, allocatorType, dllTag )												\
	class poolUniqueName;																						\
	dllTag##_TEMPLATE template struct dllTag red::memory::StaticPoolStorage< poolUniqueName  >;					\
	_INTERNAL_RED_MEMORY_POOL( poolUniqueName, allocatorType, dllTag )

#else

#define RED_MEMORY_POOL( poolUniqueName, allocatorType, dllTag )												\
	RED_MEMORY_POOL_STATIC( poolUniqueName, allocatorType )

#endif // RED_DLL || RED_WITH_DLL

#endif
