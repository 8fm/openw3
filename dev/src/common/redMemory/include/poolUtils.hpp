/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_INCLUDE_POOL_UTILS_HPP_
#define _RED_MEMORY_INCLUDE_POOL_UTILS_HPP_

#include "poolRoot.h"
#include "../src/poolStorage.h"

namespace red
{
namespace memory
{
	template< typename PoolType >
	RED_MEMORY_INLINE void InitializePool( const PoolParameter & param, typename PoolType::AllocatorType & allocator )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;
		RegisterPool( PoolType::GetHandle(), param );
		ProxyType::SetAllocator( allocator ); 
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void SetPoolOOMHandler( OOMHandler * oomHandler )
	{
		typedef PoolStorageProxy< PoolType > ProxyType;
		ProxyType::SetOutOfMemoryHandler( oomHandler );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE u64 GetPoolBudget()
	{
		const PoolHandle handle = PoolType::GetHandle();
		return GetPoolBudget( handle );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE u64 GetPoolTotalBytesAllocated()
	{
		typedef PoolStorageProxy< PoolType > ProxyType;
		return ProxyType::GetTotalBytesAllocated();
	}

	template< typename PoolType >
	RED_MEMORY_INLINE const char * GetPoolName()
	{
		const PoolHandle handle = PoolType::GetHandle();
		return GetPoolName( handle );
	}

	template< typename T >
	struct InternalPoolContextMissing
	{
		typedef void Type;
	};

	template< typename T, typename = void >
	struct InternalHasPoolContext
	{
		enum { Value = false };
	};

	template< typename T >
	struct InternalHasPoolContext< T, typename InternalPoolContextMissing< typename T::_INTERNAL_RED_POOL_CONTEXT >::Type >
	{
		enum { Value = true };
	};

	template< typename T, bool >
	struct InternalPoolResolver
	{
		typedef PoolDefault Type;
		typedef Type::AllocatorType AllocatorType;
	};

	template< typename T >
	struct InternalPoolResolver< T, true >
	{
		typedef typename T::_INTERNAL_RED_POOL_CONTEXT Type;
		typedef typename Type::AllocatorType AllocatorType;
	};

	template< typename Type >
	struct PoolResolver
	{
		typedef InternalPoolResolver< Type, InternalHasPoolContext< Type >::Value > _InternalResolver;

		typedef typename _InternalResolver::Type PoolType;
	};
}
}


#endif
