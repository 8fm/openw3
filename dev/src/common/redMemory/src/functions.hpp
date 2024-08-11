/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_FUNCTIONS_HPP_
#define _RED_MEMORY_FUNCTIONS_HPP_

#include "hookUtils.h"
#include "poolStorage.h"
#include "pool.h"

#ifdef RED_MEMORY_USE_DEBUG_ALLOCATOR
#include "debugAllocator.h"
#endif

namespace red
{
namespace memory
{
	template< typename PoolType >
	RED_MEMORY_INLINE void * Allocate( u32 size )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;

		ProcessPreAllocateHooks< PoolType >( size);
		Block block = ProxyType::Allocate( size );
		ProcessPostAllocateHooks< PoolType >( block );
		return reinterpret_cast< void* >( block.address );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE void * Allocate( Proxy & proxy, u32 size )
	{
		ProcessPreAllocateHooks( proxy, size );

#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		Block block = proxy.Allocate( size );
#else
		Block block = AcquireDebugAllocator().Allocate( size );
		RED_UNUSED( proxy );
#endif
		ProcessPostAllocateHooks( proxy, block );		
		return reinterpret_cast< void* >( block.address );
	}

	RED_MEMORY_INLINE void * Allocate( Pool & pool, u32 size )
	{
		ProcessPreAllocateHooks( pool, size );
		Block block = pool.Allocate( size );
		ProcessPostAllocateHooks( pool, block );		
		return reinterpret_cast< void* >( block.address );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void * AllocateAligned( u32 size, u32 alignment )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;

		ProcessPreAllocateHooks< PoolType >( size );
		Block block = ProxyType::AllocateAligned( size, alignment );
		ProcessPostAllocateHooks< PoolType >( block );		
		return reinterpret_cast< void* >( block.address );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE void * AllocateAligned( Proxy & proxy, u32 size, u32 alignment )
	{
		ProcessPreAllocateHooks( proxy, size );

#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		Block block = proxy.AllocateAligned( size, alignment );
#else
		Block block = AcquireDebugAllocator().AllocateAligned( size, alignment );
		RED_UNUSED( proxy );
#endif
		ProcessPostAllocateHooks( proxy, block );
		return reinterpret_cast< void* >( block.address );
	}

	RED_MEMORY_INLINE void * AllocateAligned( Pool & pool, u32 size, u32 alignment )
	{
		ProcessPreAllocateHooks( pool, size );
		Block block = pool.AllocateAligned( size, alignment );
		ProcessPostAllocateHooks( pool, block );
		return reinterpret_cast< void* >( block.address );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void Free( const void * ptr )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;

		Block block = { reinterpret_cast< u64 >( ptr ), 0 };
		ProcessPreFreeHooks< PoolType >( block );
		ProxyType::Free( block );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE void Free( Proxy & proxy, const void * ptr )
	{
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreFreeHooks( proxy, block );

#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		proxy.Free( block );
#else
		AcquireDebugAllocator().Free( block );
		RED_UNUSED( proxy );
#endif
	}

	RED_MEMORY_INLINE void Free( Pool & pool, const void * ptr )
	{
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };
		ProcessPreFreeHooks( pool, block );
		pool.Free( block );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void * Reallocate( void * ptr, u32 size )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks< PoolType >( block, size );
		Block result = ProxyType::Reallocate( block, size );
		ProcessPostReallocateHooks< PoolType >( block, result );
	
		return reinterpret_cast< void* >( result.address );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE void * Reallocate( Proxy & proxy, void * ptr, u32 size )
	{
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks( proxy, block, size );

#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR

		Block result = proxy.Reallocate( block, size );
#else
		Block result = AcquireDebugAllocator().Reallocate( block, size );
		RED_UNUSED( proxy );
#endif
		ProcessPostReallocateHooks( proxy, block, result );
		
		return reinterpret_cast< void* >( result.address );
	}

	RED_MEMORY_INLINE void * Reallocate( Pool & pool, void * ptr, u32 size )
	{
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks( pool, block, size );
		Block result = pool.Reallocate( block, size );
		ProcessPostReallocateHooks( pool, block, result );
	
		return reinterpret_cast< void* >( result.address );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void * ReallocateAligned( void * ptr, u32 size, u32 alignment )
	{
		RED_FATAL_ASSERT( alignment <= 16, "ReallocateAligned do not support alignment more than 16." );

		// Ok, this is tricky. Allocators are all aligned on 16. 
		// Only one exception, the SlabAllocator: 
		// If alloc size is less than 128, it is aligned on 8 if multiple of 8, 16 if multiple of 16. 
		// Simple round up operation will do the trick.

		typedef PoolStorageProxy< PoolType > ProxyType;
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks< PoolType >( block, size );
		Block result = ProxyType::Reallocate( block, RoundUp( size, alignment ) );
		ProcessPostReallocateHooks< PoolType >( block, result );

		return reinterpret_cast< void* >( result.address );
	}

	template< typename Proxy >
	RED_MEMORY_INLINE void * ReallocateAligned( Proxy & proxy, void * ptr, u32 size, u32 alignment )
	{
		RED_FATAL_ASSERT( alignment <= 16, "ReallocateAligned do not support alignment more than 16." );

		// Ok, this is tricky. Allocators are all aligned on 16. 
		// Only one exception, the SlabAllocator: 
		// If alloc size is less than 128, it is aligned on 8 if multiple of 8, 16 if multiple of 16. 
		// Simple round up operation will do the trick.
		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks( proxy, block, size );

#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		Block result = proxy.Reallocate( block, RoundUp( size, alignment ) );
#else
		Block result = AcquireDebugAllocator().Reallocate( block, RoundUp( size, alignment ) );
		RED_UNUSED( proxy );
#endif
		ProcessPostReallocateHooks( proxy, block, result );
		
		return reinterpret_cast< void* >( result.address );
	}

	RED_MEMORY_INLINE void * ReallocateAligned( Pool & pool, void * ptr, u32 size, u32 alignment )
	{
		RED_FATAL_ASSERT( alignment <= 16, "ReallocateAligned do not support alignment more than 16." );

		// Ok, this is tricky. Allocators are all aligned on 16. 
		// Only one exception, the SlabAllocator: 
		// If alloc size is less than 128, it is aligned on 8 if multiple of 8, 16 if multiple of 16. 
		// Simple round up operation will do the trick.

		Block block = { reinterpret_cast< u64 >( ptr ), 0 };

		ProcessPreReallocateHooks( pool, block, size );
		Block result = pool.Reallocate( block, RoundUp( size, alignment ) );
		ProcessPostReallocateHooks( pool, block, result );
		
		return reinterpret_cast< void* >( result.address );
	}
}
}

#endif
