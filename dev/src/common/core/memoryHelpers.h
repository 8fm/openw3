/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_CORE_MEMORY_HELPERS_H_
#define _RED_CORE_MEMORY_HELPERS_H_

namespace Memory
{
	Uint64 GetTotalBytesAllocated();

	Uint32 GetPoolCount();

	Uint64 GetTotalAllocations( Uint32 poolLabel );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetPoolTotalBytesAllocated();

	Uint64 GetPoolTotalBytesAllocated( Uint32 poolLabel );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetPoolTotalBytesAllocatedPeak();

	Uint64 GetPoolTotalBytesAllocatedPeak( Uint32 poolLabel );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetPoolTotalAllocations( );

	Uint64 GetPoolTotalAllocations( Uint32 poolLabel );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetPoolBudget();

	Uint64 GetPoolBudget( Uint32 label );

	template< RED_CONTAINER_POOL_TYPE pool >
	const AnsiChar * GetPoolName();
	const AnsiChar * GetPoolName( Uint32 poolLabel );

	const AnsiChar * GetMemoryClassName( Uint32 memClass );

	void GetMemoryClassName( Uint32 memClass, Red::System::AnsiChar* buffer, Red::System::Uint32 maxCharacters );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass );

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass, Uint32 poolLabel );

	Uint64 GetAllocatedBytesPerMemoryClass( Uint32 memClass );
	
	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetAllocatedBytesPerMemoryClassPeak( Uint32 memClass );
	
	Uint64 GetAllocationPerMemoryClass( Uint32 memClass, Uint32 poolLabel );

	template< RED_CONTAINER_POOL_TYPE pool >
	Uint64 GetBlockSize( const void * ptr );

	Uint64 ReleaseFreeMemoryToSystem();

	void OnFrameStart();
	void OnFrameEnd();

	typedef bool (*OutOfMemoryCallback)( Uint32 poolLabel );
	void RegisterOOMCallback( OutOfMemoryCallback callback );
	void UnregisterOOMCallback( OutOfMemoryCallback callback );

	void DumpClassMemoryReport( const Red::System::AnsiChar* title  );
	void DumpPoolMemoryReport( const Red::System::AnsiChar* title );

	void RegisterCurrentThread();

	void Initialize( CoreMemory::CMemoryPoolParameters * param ); 

	Red::MemoryFramework::AllocationMetricsCollector & GetMetricsCollector();

	void RegisterPoolName( Uint32 label, const AnsiChar* poolName );

	void RegisterClassName( Uint32 memoryClass, const AnsiChar* className );

	void RegisterClassGroup( const AnsiChar* groupName, Uint32* memClasses, Uint32 count );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * AllocateHybrid( size_t size, EMemoryClass memClass );
	
	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * AllocateAlignedHybrid( size_t size, size_t alignment, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void FreeHybrid( const void * ptr, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * ReallocateAlignedHybrid( void * inputPtr, size_t size, size_t alignment, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * Allocate( size_t size, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * AllocateAligned( size_t size, size_t alignment, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void Free( const void * ptr, EMemoryClass memClass );

	template< RED_CONTAINER_POOL_TYPE PoolType >
	void * ReallocateAligned( void * inputPtr, size_t size, size_t alignment, EMemoryClass memClass );
}

#define RED_MEMORY_ALLOCATE_HYBRID( Pool, MemClass, size ) \
	Memory::AllocateHybrid< Pool >( size, MemClass )

#define RED_MEMORY_ALLOCATE_ALIGNED_HYBRID( Pool, MemClass, size, alignment ) \
	Memory::AllocateAlignedHybrid< Pool >( size, alignment, MemClass )

#define RED_MEMORY_FREE_HYBRID( Pool, MemClass, Ptr ) \
	Memory::FreeHybrid< Pool >( Ptr, MemClass )

#define RED_MEMORY_REALLOCATE_ALIGNED_HYBRID( Pool, Ptr, MemClass, size, alignment ) \
	Memory::ReallocateAlignedHybrid< Pool >( Ptr, size, alignment, MemClass )

#ifndef RED_USE_NEW_MEMORY_SYSTEM
	#define RED_MEMORY_INITIALIZE( Param ) \
		Param _param; Memory::Initialize( &_param )
#else
	#define RED_MEMORY_INITIALIZE( Param ) \
		Memory::Initialize( nullptr )
#endif

#include "memoryHelpers.hpp"

#endif
