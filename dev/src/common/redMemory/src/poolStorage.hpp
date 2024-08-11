/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_POOL_STORAGE_HPP_
#define _RED_MEMORY_POOL_STORAGE_HPP_

#include "defaultAllocator.h"
#include "metricsUtils.h"
#include "oomHandler.h"

#ifdef RED_MEMORY_USE_DEBUG_ALLOCATOR
#include "debugAllocator.h"
#endif

namespace red { namespace memory { class PoolDefault; } }
namespace red { namespace memory { class PoolLegacy; } }
  
namespace red
{
namespace memory
{
namespace internal
{
	RED_MEMORY_API void PoolDefaultOOMHandler( PoolHandle handle, u32 size, u32 alignment );
	RED_MEMORY_API PoolHandle AcquirePoolHandle();

	template< typename AllocatorType >
	struct AllocatorInitializer
	{
		RED_MEMORY_INLINE static AllocatorType * Get()
		{
			return nullptr;
		}
	};

	template<>
	struct AllocatorInitializer< DefaultAllocator >
	{
		RED_MEMORY_INLINE static DefaultAllocator * Get()
		{
			return &AcquireDefaultAllocator();
		}
	};

	RED_MEMORY_INLINE void HandleAllocateSucceeded( PoolStorage & storage, const Block & block )
	{
		atomic::ExchangeAdd64( &storage.bytesAllocated, block.size );
		
#ifdef RED_MEMORY_ENABLE_METRICS
		memory::AddAllocateMetric( storage.handle, block );
#endif
	}
	
	RED_MEMORY_INLINE void HandleReallocateSucceeded( PoolStorage & storage, const Block & inputBlock, const Block & outputBlock )
	{
		const i64 size = outputBlock.size - inputBlock.size;
		atomic::ExchangeAdd64( &storage.bytesAllocated, size );

#ifdef RED_MEMORY_ENABLE_METRICS
		memory::AddReallocateMetric( storage.handle, inputBlock, outputBlock );
#endif
	}
	
	RED_MEMORY_INLINE void HandleFreeSucceeded( PoolStorage & storage, const Block & block )
	{
		atomic::ExchangeAdd64( &storage.bytesAllocated, -static_cast< i32 >( block.size ) );

#ifdef RED_MEMORY_ENABLE_METRICS
		memory::AddFreeMetric( storage.handle, block );
#endif
	}
	
	void HandleAllocateFailure( PoolStorage & storage, u32 size );
}

	template< typename PoolType >
	struct StaticPoolStorage
	{
		static PoolStorage storage RED_STATIC_PRIORITY( 104 );
	};

	template<>
	struct StaticPoolStorage< PoolDefault >
	{
		static PoolStorage storage RED_STATIC_PRIORITY( 102 );
	};

	template<>
	struct StaticPoolStorage< PoolLegacy >
	{
		static PoolStorage storage RED_STATIC_PRIORITY( 103 );
	};

	template< typename PoolType >
	PoolStorage StaticPoolStorage< PoolType >::storage = 
	{
		internal::AllocatorInitializer< typename PoolType::AllocatorType >::Get(),
		0,
		nullptr,
		internal::AcquirePoolHandle()
	};
	
	template< typename PoolType >
	RED_MEMORY_INLINE Block PoolStorageProxy< PoolType >::Allocate( u32 size )
	{
#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		AllocatorType & allocator = GetAllocator();
#else
		DebugAllocator & allocator = AcquireDebugAllocator();
#endif
		const Block block = allocator.Allocate( size );
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		block.address ? internal::HandleAllocateSucceeded( storage, block ) : internal::HandleAllocateFailure( storage, size );
		return block;
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE Block PoolStorageProxy< PoolType >::AllocateAligned( u32 size, u32 alignment )
	{
#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		AllocatorType & allocator = GetAllocator();
#else
		DebugAllocator & allocator = AcquireDebugAllocator();
#endif
		const Block block = allocator.AllocateAligned( size, alignment );
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		block.address ? internal::HandleAllocateSucceeded( storage, block ) : internal::HandleAllocateFailure( storage, size );
		return block;
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE Block PoolStorageProxy< PoolType >::Reallocate( Block & inputBlock, u32 size )
	{
#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		AllocatorType & allocator = GetAllocator();
#else
		DebugAllocator & allocator = AcquireDebugAllocator();
#endif

		const Block outputBlock = allocator.Reallocate( inputBlock, size );
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		if( outputBlock.address || !size  )
		{
			internal::HandleReallocateSucceeded( storage, inputBlock, outputBlock );
		}
		else
		{
			internal::HandleAllocateFailure( storage, size );
		}

		return outputBlock;
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE void PoolStorageProxy< PoolType >::Free( Block & block )
	{
#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		AllocatorType & allocator = GetAllocator();
#else
		DebugAllocator & allocator = AcquireDebugAllocator();
#endif
		allocator.Free( block );
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		internal::HandleFreeSucceeded( storage, block );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE u64 PoolStorageProxy< PoolType >::GetBlockSize( u64 address )
	{
#ifndef RED_MEMORY_USE_DEBUG_ALLOCATOR
		AllocatorType & allocator = GetAllocator();
#else
		DebugAllocator & allocator = AcquireDebugAllocator();
#endif

		return allocator.GetBlockSize( address );
	}

	template< typename PoolType >
	RED_MEMORY_INLINE typename PoolStorageProxy< PoolType >::AllocatorType & PoolStorageProxy< PoolType >::GetAllocator()
	{
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		return *static_cast< AllocatorType *>( storage.allocator );
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE PoolHandle PoolStorageProxy< PoolType >::GetHandle()
	{
		const PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		return storage.handle;
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE u64 PoolStorageProxy< PoolType >::GetTotalBytesAllocated()
	{
		const PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		return storage.bytesAllocated;
	}

	template< typename PoolType >
	RED_MEMORY_INLINE void PoolStorageProxy< PoolType >::SetAllocator( AllocatorType & allocator )
	{
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		storage.allocator = &allocator;
	}
	
	template< typename PoolType >
	RED_MEMORY_INLINE void PoolStorageProxy< PoolType >::SetOutOfMemoryHandler( OOMHandler * handler )
	{
		PoolStorage & storage = StaticPoolStorage< PoolType >::storage;
		storage.oomHandler = handler;
	}
}
}

#endif
