/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_MACROS_H
#define _RED_MEMORY_MACROS_H

// Set the maximum size of the CRT heap for platforms where it is supported
// This must be called in global scope
#ifdef RED_PLATFORM_ORBIS
#define RED_MEMORY_SET_MAXIMUM_CRT_SIZE(maxSize)	size_t sceLibcHeapSize = maxSize;
#else
#define RED_MEMORY_SET_MAXIMUM_CRT_SIZE(maxSize)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Note: Any macro with the INTERNAL_ prefix should not be used from engine / game code directly, they should be 
// wrapped (so we can avoid passing the memory manager around)
// Notes: PoolName should be an enum defined somewhere. Category is used to scope pools into different managers.

// This must precede any calls to register pools for a category. If you receive any strange looking template specialisation
// compile errors, this line is probably missing somewhere
#define INTERNAL_RED_MEMORY_BEGIN_POOL_TYPES( Category )	\
	namespace Category { template < Red::MemoryFramework::PoolLabel PoolName > class PoolTypeResolver;

#define INTERNAL_RED_MEMORY_END_POOL_TYPES	}

// Use this in a header to define an allocator type for a particular pool. Specializes template in a particular scope
#define INTERNAL_RED_MEMORY_DECLARE_POOL( PoolName, Type )	\
	template <> class PoolTypeResolver< PoolName > { public: typedef Type PoolType; };

// Use this to get the allocator type from a pool label. 
#define INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName )											\
	Category::PoolTypeResolver< PoolName >::PoolType

// Register pool names with the memory manager for metrics (Category is simply for internal consistency)
#define INTERNAL_RED_MEMORY_REGISTER_POOL_NAME( MemoryMan, Category, PoolName )						\
	MemoryMan.RegisterPoolName(PoolName, #PoolName)

// Register class names with the memory manager for metrics
#define INTERNAL_RED_MEMORY_REGISTER_MEMORY_CLASS( MemoryMan, MemoryClass)							\
	MemoryMan.RegisterClassName(MemoryClass, #MemoryClass)

// Adds an instance of a pool to the memory manager
#define INTERNAL_RED_MEMORY_CREATE_POOL( MemoryMan, Category, PoolName, CreationParams, Flags )				\
	MemoryMan.AddNamedPool< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, CreationParams, Flags )

// Destroy a named pool
#define INTERNAL_RED_MEMORY_DESTROY_POOL( MemoryMan, Category, PoolName )										\
	MemoryMan.DestroyNamedPool( PoolName )

// Get a pointer to a pool / allocator directly								
#define INTERNAL_RED_MEMORY_GET_ALLOCATOR( MemoryMan, Category, PoolName )										\
	static_cast< INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName )* >( MemoryMan.GetPool( PoolName ) )

// Allocate to a pool
#define INTERNAL_RED_MEMORY_ALLOCATE( MemoryMan, Category, PoolName, MemoryClass, Size )						\
	MemoryMan.Allocate< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, MemoryClass, Size, sizeof(size_t) )

// Allocate to a pool with alignment
#define INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( MemoryMan, Category, PoolName, MemoryClass, Size, Align )			\
	MemoryMan.Allocate< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, MemoryClass, Size, Align )

// Realloc to a pool 
#define INTERNAL_RED_MEMORY_REALLOCATE( MemoryMan, Category, PoolName, Ptr, MemoryClass, Size )	\
	MemoryMan.Reallocate< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Ptr, MemoryClass, Size, sizeof(size_t) )

#define INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED( MemoryMan, Category, PoolName, Ptr, MemoryClass, Size, Align )	\
	MemoryMan.Reallocate< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Ptr, MemoryClass, Size, Align )

// Free memory from a pool
#define INTERNAL_RED_MEMORY_FREE( MemoryMan, Category, PoolName, MemoryClass, Ptr )				\
	MemoryMan.Free< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, MemoryClass, Ptr )

// Allocate a moveable region of memory
#define INTERNAL_RED_MEMORY_ALLOCATE_REGION( MemoryMan, Category, PoolName, MemoryClass, Size, Align, Lifetime )			\
	MemoryMan.AllocateRegion< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, MemoryClass, Size, Align, Lifetime )

// Free a moveable region of memory
#define INTERNAL_RED_MEMORY_FREE_REGION( MemoryMan, Category, PoolName, Region )			\
	MemoryMan.FreeRegion< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Region )

// Unlock a moveable region of memory
#define INTERNAL_RED_MEMORY_UNLOCK_REGION( MemoryMan, Category, PoolName, Region )			\
	MemoryMan.UnlockRegion< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Region )

// Unlock a moveable region of memory
#define INTERNAL_RED_MEMORY_LOCK_REGION( MemoryMan, Category, PoolName, Region )			\
	MemoryMan.LockRegion< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Region )

// Pool can't be resolve at compile time.

#define INTERNAL_RED_MEMORY_RUNTIME_ALLOCATE_ALIGNED( MemoryMan, Category, PoolName, MemoryClass, Size, Align )			\
	MemoryMan.RuntimeAllocate( PoolName, MemoryClass, Size, Align )

#define INTERNAL_RED_MEMORY_RUNTIME_FREE( MemoryMan, Category, PoolName, MemoryClass, Ptr )				\
	MemoryMan.RuntimeFree( PoolName, MemoryClass, Ptr )

#define INTERNAL_RED_MEMORY_RUNTIME_REALLOCATE_ALIGNED( MemoryMan, Category, PoolName, Ptr, MemoryClass, Size, Align ) \
	MemoryMan.RuntimeReallocate( PoolName, Ptr, MemoryClass, Size, Align )

// Split a moveable region of memory
#define INTERNAL_RED_MEMORY_SPLIT_REGION( MemoryMan, Category, PoolName, Region, Offset, Align )			\
	MemoryMan.SplitRegion< typename INTERNAL_RED_MEMORY_POOL_TYPE( Category, PoolName ) >( PoolName, Region, Offset, Align )

#endif