/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_FUNCTIONS_H_
#define _RED_MEMORY_FUNCTIONS_H_

namespace red
{
namespace memory
{
	class Pool;

	template< typename PoolType >
	void * Allocate( u32 size );

	template< typename Proxy >
	void * Allocate( Proxy & proxy, u32 size );

	void * Allocate( Pool & pool, u32 size );

	template< typename PoolType >
	void * AllocateAligned( u32 size, u32 alignment );

	template< typename Proxy >
	void * AllocateAligned( Proxy & proxy, u32 size, u32 alignment );

	void * AllocateAligned( Pool & pool, u32 size, u32 alignment );

	template< typename Pool >
	void * Reallocate( void * block, u32 size );

	template< typename Proxy >
	void * Reallocate( Proxy & proxy, void * block, u32 size );

	void * Reallocate( Pool & pool, u32 size );

	template< typename PoolType >
	void * ReallocateAligned( void * block, u32 size, u32 alignment );

	template< typename Proxy >
	void * ReallocateAligned( Proxy & proxy, void * block, u32 size, u32 alignment );

	void * ReallocateAligned( Pool & pool, void * block, u32 size, u32 alignment );

	template< typename PoolType >
	void Free( const void * block );

	template< typename Proxy >
	void Free( Proxy & allocator, const void * block );

	void Free( Pool & pool, const void * block );
}
}

#include "functions.hpp"

#endif
