/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_OPERATORS_H_
#define _RED_MEMORY_OPERATORS_H_

#include "redMemoryInternal.h" // should be included via local precompiled header. However, for the sake of standalone include, I'm making sure it is included. 
#include "../src/operatorsInternal.h"
#include "../src/macroUtils.h"

//////////////////////////////////////////////////////////////////////////

#ifdef RED_COMPILER_MSC
	#define RED_NEW( ... )			RED_MEMORY_CONCAT( RED_MEMORY_OVERLOAD( _INTERNAL_RED_NEW_, __VA_ARGS__ )( __VA_ARGS__ ), RED_MEMORY_EMPTY() )		
	#define RED_NEW_ARRAY( ... )	RED_MEMORY_CONCAT( RED_MEMORY_OVERLOAD( _INTERNAL_RED_NEW_ARRAY_, __VA_ARGS__ )( __VA_ARGS__ ), RED_MEMORY_EMPTY() )

	#define RED_DELETE( ... )		RED_MEMORY_CONCAT( RED_MEMORY_OVERLOAD( _INTERNAL_RED_DELETE_, __VA_ARGS__ )( __VA_ARGS__ ), RED_MEMORY_EMPTY() )
	#define RED_DELETE_ARRAY( ... ) RED_MEMORY_CONCAT( RED_MEMORY_OVERLOAD( _INTERNAL_RED_DELETE_ARRAY_, __VA_ARGS__ )( __VA_ARGS__ ), RED_MEMORY_EMPTY() )		
#else
	#define RED_NEW( ... )			RED_MEMORY_OVERLOAD( _INTERNAL_RED_NEW_, __VA_ARGS__ )( __VA_ARGS__ )	
	#define RED_NEW_ARRAY( ... )	RED_MEMORY_OVERLOAD( _INTERNAL_RED_NEW_ARRAY_, __VA_ARGS__ )( __VA_ARGS__ )
	
	#define RED_DELETE( ... )		RED_MEMORY_OVERLOAD( _INTERNAL_RED_DELETE_, __VA_ARGS__ )( __VA_ARGS__ )
	#define RED_DELETE_ARRAY( ... )	RED_MEMORY_OVERLOAD( _INTERNAL_RED_DELETE_ARRAY_, __VA_ARGS__ )( __VA_ARGS__ )
#endif
			
//////////////////////////////////////////////////////////////////////////

#define RED_ALLOCATE( proxy, size )						red::memory::internal::AllocateHelper( proxy(), static_cast< red::memory::u32 >( size ) )
#define RED_ALLOCATE_ALIGNED( proxy, size, alignment )	red::memory::internal::AllocateAlignedHelper( proxy(), static_cast< red::memory::u32 >( size ), static_cast< red::memory::u32 >( alignment ) )

#define RED_REALLOCATE( proxy, ptr, size )						red::memory::internal::ReallocateHelper( proxy(), ptr, static_cast< red::memory::u32 >( size ) )
#define RED_REALLOCATE_ALIGNED( proxy, ptr, size, alignment )	red::memory::internal::ReallocateAlignedHelper( proxy(), ptr, static_cast< red::memory::u32 >( size ), static_cast< red::memory::u32 >( alignment ) )

#define RED_FREE( proxy, ptr ) red::memory::internal::FreeHelper( proxy(), ptr )

//////////////////////////////////////////////////////////////////////////

#endif
