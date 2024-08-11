/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/


#ifndef _RED_CORE_MEMORY_HELPERS_HPP_
#define _RED_CORE_MEMORY_HELPERS_HPP_

namespace Memory
{

#ifndef RED_USE_NEW_MEMORY_SYSTEM


	template< int pool >
	RED_INLINE EMemoryPoolLabel GetPoolLabel()
	{
		return (EMemoryPoolLabel)pool;
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetPoolTotalBytesAllocated()
	{
		return GetPoolTotalBytesAllocated( pool );
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetPoolTotalBytesAllocatedPeak()
	{
		return GetPoolTotalBytesAllocatedPeak( pool );
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetPoolTotalAllocations( )
	{
		return GetPoolTotalAllocations( pool );
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetPoolBudget()
	{
		Red::MemoryFramework::AllocatorInfo info;
		SRedMemory::GetInstance().GetPool( pool )->RequestAllocatorInfo( info );
		return info.GetBudget();
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass )
	{
		return GetAllocatedBytesPerMemoryClass( pool, memClass );
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetAllocatedBytesPerMemoryClassPeak( Uint32 memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		return SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( pool ).m_totalAllocationsPeak;
#else
		return 0;
#endif
	}

	template< EMemoryPoolLabel pool >
	RED_INLINE Uint64 GetBlockSize( const void * ptr )
	{
		return SRedMemory::GetInstance().GetPool( pool )->RuntimeGetAllocationSize( const_cast< void* >( ptr ) );
	}

	template< RED_CONTAINER_POOL_TYPE pool >
	RED_INLINE const AnsiChar * GetPoolName()
	{
		return GetPoolName( pool );
	}

	template< EMemoryPoolLabel PoolType >
	RED_INLINE void * AllocateHybrid( size_t size, EMemoryClass memClass )
	{
		if( size <= 128 )
		{
			return RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, memClass, size ); 
		}
		else
		{
			return RED_MEMORY_ALLOCATE( PoolType, memClass, size ); 
		}
	}

	template< EMemoryPoolLabel PoolType >
	RED_INLINE void * AllocateAlignedHybrid( size_t size, size_t alignment, EMemoryClass memClass )
	{
		if( size <= 128 )
		{
			return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, memClass, size, alignment ); 
		}
		else
		{
			return RED_MEMORY_ALLOCATE_ALIGNED( PoolType, memClass, size, alignment ); 
		}
	}

	template< EMemoryPoolLabel PoolType >
	RED_INLINE void FreeHybrid( const void * ptr, EMemoryClass memClass )
	{
		auto sbaAllocator = RED_MEMORY_GET_ALLOCATOR( MemoryPool_SmallObjects );
		if( sbaAllocator && sbaAllocator->OwnsPointer( ptr ) )
		{
			RED_MEMORY_FREE( MemoryPool_SmallObjects, memClass, ptr );
		}
		else
		{
			RED_MEMORY_FREE( PoolType, memClass, ptr );
		}
	}

	template< EMemoryPoolLabel PoolType >
	RED_INLINE void * ReallocateAlignedHybrid( void * inputPtr, size_t size, size_t alignment, EMemoryClass memClass )
	{
		if( size == 0 )
		{
			FreeHybrid< PoolType >( inputPtr, memClass );
			return nullptr;
		}

		if( inputPtr == nullptr )
		{
			return AllocateAlignedHybrid< PoolType >( size, alignment, memClass );
		}

		Red::MemoryFramework::PoolLabel allocPool = size <= 128 ? MemoryPool_SmallObjects : PoolType;
		auto sbaAllocator = RED_MEMORY_GET_ALLOCATOR( MemoryPool_SmallObjects );
		Red::MemoryFramework::PoolLabel freePool = ( sbaAllocator && sbaAllocator->OwnsPointer( inputPtr ) ) ?  MemoryPool_SmallObjects : PoolType;
		
		if( allocPool == freePool || sbaAllocator == nullptr )
		{
			return RED_MEMORY_RUNTIME_REALLOCATE( allocPool, inputPtr, memClass, size, alignment );
		}
		else
		{
			// Edge case where pool is different; do a 'manual' realloc
			void* newBuffer = RED_MEMORY_RUNTIME_ALLOCATE( allocPool, memClass, size, alignment );
			size_t inputSize = SRedMemory::GetInstance().GetPool( freePool )->RuntimeGetAllocationSize( inputPtr );
			size_t copySize = inputSize < size ? inputSize : size;
			Red::System::MemoryCopy( newBuffer, inputPtr, copySize );
			RED_MEMORY_RUNTIME_FREE( freePool, memClass, inputPtr );

			return newBuffer;
		}
	}

#else

	template< typename pool >
	RED_INLINE Uint32 GetPoolLabel()
	{
		return pool::GetHandle();
	}
	
	template< typename pool >
	RED_INLINE Uint64 GetPoolTotalBytesAllocated()
	{
		return red::memory::GetPoolTotalBytesAllocated< pool >();
	}

	template< typename pool >
	RED_INLINE Uint64 GetPoolTotalBytesAllocatedPeak()
	{
		return 0; // TODO
	}

	template< typename pool >
	RED_INLINE Uint64 GetPoolTotalAllocations( )
	{
		return 0; // TODO
	}

	template< typename pool >
	RED_INLINE Uint64 GetPoolBudget()
	{
		return red::memory::GetPoolBudget< pool >();
	}

	template< typename pool >
	RED_INLINE Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		red::memory::PoolHandle handle = pool::GetHandle();
		return GetMetricsCollector().GetMetricsForPool( handle ).m_allocatedBytesPerMemoryClass[ memClass ]; 
#else
		return 0;
#endif
	}

	template< typename pool >
	RED_INLINE Uint64 GetAllocatedBytesPerMemoryClassPeak( Uint32 memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		red::memory::PoolHandle handle = pool::GetHandle();
		return GetMetricsCollector().GetMetricsForPool( handle ).m_allocatedBytesPerMemoryClassPeak[ memClass ]; 
#else
		return 0;
#endif
	}

	template< typename pool >
	RED_INLINE Uint64 GetBlockSize( const void * ptr )
	{
		if( ptr )
		{
			return red::memory::PoolStorageProxy< pool >::GetBlockSize( reinterpret_cast< Uint64 >( ptr ) ); 
		}
		else
		{
			return 0;
		}
	}

	template< RED_CONTAINER_POOL_TYPE pool >
	RED_INLINE const AnsiChar * GetPoolName()
	{
		return red::memory::GetPoolName< pool >();
	}

	template< typename PoolType >
	RED_INLINE void * AllocateHybrid( size_t size, EMemoryClass memClass )
	{
		return RED_MEMORY_ALLOCATE( PoolType, memClass, size ); 	
	}

	template< typename PoolType >
	RED_INLINE void * AllocateAlignedHybrid( size_t size, size_t alignment, EMemoryClass memClass )
	{
		return RED_MEMORY_ALLOCATE_ALIGNED( PoolType, memClass, size, alignment ); 
	}

	template< typename PoolType >
	RED_INLINE void FreeHybrid( const void * ptr, EMemoryClass memClass )
	{
		RED_MEMORY_FREE( PoolType, memClass, ptr );
	}

	template< typename PoolType >
	RED_INLINE void * ReallocateAlignedHybrid( void * inputPtr, size_t size, size_t alignment, EMemoryClass memClass )
	{
		return RED_MEMORY_REALLOCATE_ALIGNED( PoolType, inputPtr, memClass, size, alignment );
	}

	template< RED_CONTAINER_POOL_TYPE PoolType >
	RED_INLINE void * Allocate( size_t size, EMemoryClass memClass )
	{
		void * ptr = red::memory::Allocate< PoolType >( static_cast< Uint32 >( size ) );
		if( memClass < MC_No_Memset_DO_NOT_USE_MOVE_OR_REMOVE && ptr )
		{
			Red::System::MemoryZero( ptr, size );
		}

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		GetMetricsCollector().OnAllocation( PoolType::GetHandle(), memClass, ptr, GetBlockSize< PoolType >( ptr ), 8 );
#endif	
		return ptr;
	}

	template< RED_CONTAINER_POOL_TYPE PoolType >
	RED_INLINE void * AllocateAligned( size_t size, size_t alignment, EMemoryClass memClass )
	{
		void * ptr = red::memory::AllocateAligned< PoolType >(  static_cast< Uint32 >( size ), static_cast< Uint32 >( alignment ) );
		if( memClass < MC_No_Memset_DO_NOT_USE_MOVE_OR_REMOVE && ptr )
		{
			Red::System::MemoryZero( ptr, size );
		}
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		GetMetricsCollector().OnAllocation( PoolType::GetHandle(), memClass, ptr, GetBlockSize< PoolType >( ptr ), alignment );
#endif	
		return ptr;
	}

	template< RED_CONTAINER_POOL_TYPE PoolType >
	RED_INLINE void Free( const void * ptr, EMemoryClass memClass )
	{
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		GetMetricsCollector().OnFree( PoolType::GetHandle(), memClass, ptr, Red::MemoryFramework::Free_OK, GetBlockSize< PoolType >( ptr ) );
#endif

		red::memory::Free< PoolType >( ptr );
	}

	template< RED_CONTAINER_POOL_TYPE PoolType >
	RED_INLINE void * ReallocateAligned( void * inputPtr, size_t size, size_t alignment, EMemoryClass memClass )
	{

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
		Uint64 inputSize = GetBlockSize< PoolType >( inputPtr );		
		void * outputPtr = red::memory::ReallocateAligned< PoolType >( inputPtr, static_cast< Uint32 >( size ),  static_cast< Uint32 >( alignment ) );
		Uint64 outputSize = GetBlockSize< PoolType >( outputPtr );
		GetMetricsCollector().OnReallocation( PoolType::GetHandle(), memClass, inputPtr, outputPtr, inputSize, outputSize, alignment );
		if( !inputPtr && outputPtr && memClass < MC_No_Memset_DO_NOT_USE_MOVE_OR_REMOVE )
		{
			Red::System::MemoryZero( outputPtr, size ); // WITCHER HACK. Block needs to be clear.
		}
		return outputPtr;
#else

		void * outputPtr = red::memory::ReallocateAligned< PoolType >( inputPtr, static_cast< Uint32 >( size ),  static_cast< Uint32 >( alignment ) );

		if( !inputPtr && outputPtr && memClass < MC_No_Memset_DO_NOT_USE_MOVE_OR_REMOVE )
		{
			Red::System::MemoryZero( outputPtr, size ); // WITCHER HACK. Block needs to be clear.
		}

		return  outputPtr;
#endif

	}


#endif
}

#endif
