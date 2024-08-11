/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_HOOK_UTILS_HPP_
#define _RED_MEMORY_HOOK_UTILS_HPP_

#include "hookTypes.h"
#include "poolConstant.h"
#include "pool.h"
#include "poolStorage.h"

namespace red
{
namespace memory
{
namespace internal
{
	RED_MEMORY_API void ProcessPreHooks( HookPreParameter & param );
	RED_MEMORY_API void ProcessPostHooks( HookPostParameter & param );
}

#ifdef RED_MEMORY_ENABLE_HOOKS

	template< typename PoolType >
	RED_MEMORY_INLINE HookProxyParameter GenerateProxyParameter()
	{
		typedef typename PoolType::AllocatorType AllocatorType;
		static_assert( ProxyHasTypeId< AllocatorType >::value, "Allocator needs Type Id to be use with hooks." );
		typedef PoolStorageProxy< PoolType > ProxyType;

		HookProxyParameter param = 
		{
			PoolType::GetHandle(),
			AllocatorType::TypeId,
			AddressOf( &ProxyType::GetAllocator() )
		};

		return param;
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE HookProxyParameter GenerateProxyParameter( ProxyType & proxy )
	{
		static_assert( ProxyHasTypeId< ProxyType >::value, "Proxy needs Type Id to be use with hooks." );

		HookProxyParameter param = 
		{
			c_poolNodeInvalid,
			ProxyType::TypeId,
			AddressOf( proxy )
		};

		return param;
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPreAllocateHooks( u32 & size )
	{
		Block block = NullBlock();
		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter< PoolType >()
		};

		internal::ProcessPreHooks( param );
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreAllocateHooks( const ProxyType & proxy, u32 & size )
	{
		Block block = NullBlock();
		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter( proxy )
		};

		internal::ProcessPreHooks( param );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPreFreeHooks( Block & block )
	{
		if( !block.address )
			return;

		typedef PoolStorageProxy< PoolType > ProxyType;
		block.size = ProxyType::GetBlockSize( block.address );
		u32 size = 0;

		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter< PoolType >()
		};

		internal::ProcessPreHooks( param );
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreFreeHooks( const ProxyType & proxy, Block & block )
	{
		if( !block.address )
			return;

		block.size = proxy.GetBlockSize( block.address );
		u32 size = 0;

		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter( proxy )
		};

		internal::ProcessPreHooks( param );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE  void ProcessPreReallocateHooks( Block & block, u32 & size )
	{
		if( block.address )
		{
			typedef PoolStorageProxy< PoolType > ProxyType;
			block.size = ProxyType::GetBlockSize( block.address );
		}

		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter< PoolType >()
		};

		internal::ProcessPreHooks( param );
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreReallocateHooks( const ProxyType & proxy, Block & block, u32 & size )
	{
		if( block.address )
		{
			block.size = proxy.GetBlockSize( block.address );
		}

		HookPreParameter param = 
		{
			&block,
			&size,
			GenerateProxyParameter( proxy )
		};

		internal::ProcessPreHooks( param );
	}


	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPostAllocateHooks( Block & block )
	{
		static_assert( !std::is_same< PoolType, Pool >::value, "Provider Pool Type can be bas pool class." );

		Block inputBlock = NullBlock();

		HookPostParameter param = 
		{
			&inputBlock,
			&block,
			GenerateProxyParameter< PoolType >()
		};

		internal::ProcessPostHooks( param );
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPostAllocateHooks( const ProxyType & proxy, Block & block )
	{
		Block inputBlock = NullBlock();

		HookPostParameter param = 
		{
			&inputBlock,
			&block,
			GenerateProxyParameter( proxy )
		};

		internal::ProcessPostHooks( param );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPostReallocateHooks( Block & inputBlock, Block & outputBlock )
	{
		static_assert( !std::is_same< PoolType, Pool >::value, "Provider Pool Type can be bas pool class." );
		
		HookPostParameter param = 
		{
			&inputBlock,
			&outputBlock,
			GenerateProxyParameter< PoolType >()
		};

		internal::ProcessPostHooks( param );
	}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPostReallocateHooks( const ProxyType & proxy, Block & inputBlock, Block & outputBlock )
	{
		HookPostParameter param = 
		{
			&inputBlock,
			&outputBlock,
			GenerateProxyParameter( proxy )
		};

		internal::ProcessPostHooks( param );
	}

#else

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPreAllocateHooks( u32 & )
	{}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreAllocateHooks( const ProxyType & , u32 & )
	{}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPreFreeHooks( Block & )
	{}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreFreeHooks( const ProxyType & , Block & )
	{}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPreReallocateHooks( Block & , u32 & )
	{}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPreReallocateHooks( const ProxyType & , Block & , u32 & )
	{}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPostAllocateHooks( Block & )
	{}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPostAllocateHooks( const ProxyType & , Block & )
	{}

	template< typename PoolType >
	RED_MEMORY_INLINE void ProcessPostReallocateHooks( Block & , Block & )
	{}

	template< typename ProxyType >
	RED_MEMORY_INLINE void ProcessPostReallocateHooks( const ProxyType & , Block & , Block & )
	{}

#endif
}
}

#endif
