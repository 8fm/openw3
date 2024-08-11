#ifndef _CORE_MEMORY_MACROS_H_
#define _CORE_MEMORY_MACROS_H_
#pragma once

#ifdef CORE_USES_DEBUG_ALLOCATOR
namespace CoreMemory
{
	Bool ShouldUseDebugAllocator( Red::MemoryFramework::PoolLabel pool, Red::MemoryFramework::MemoryClass memClass );
}
#endif

RED_INLINE size_t CalculateDefaultAlignment( size_t size );

// External OOM Callbacks
#define RED_MEMORY_REGISTER_OOM_CALLBACK( callback )	Memory::RegisterOOMCallback( callback )
#define RED_MEMORY_UNREGISTER_OOM_CALLBACK( callback )	Memory::UnregisterOOMCallback( callback );

// Get the type of a pool
#define RED_MEMORY_POOL_TYPE( PoolName )		INTERNAL_RED_MEMORY_POOL_TYPE( CoreMemory, PoolName )

// Metrics dumps
#define RED_MEMORY_DUMP_CLASS_MEMORY_REPORT( Title )	\
	{     \
		Memory::DumpClassMemoryReport( Title );	\
	}
#define RED_MEMORY_DUMP_POOL_MEMORY_REPORT( Title )	\
	{     \
		Memory::DumpPoolMemoryReport( Title );	\
	}

#ifndef RED_USE_NEW_MEMORY_SYSTEM

// Allocate to a pool
#define RED_MEMORY_ALLOCATE( PoolName, MemoryClass, Size )	INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, MemoryClass, Size, CalculateDefaultAlignment( Size ) )
#define RED_MEMORY_ALLOCATE_ALIGNED( PoolName, MemoryClass, Size, Align )	INTERNAL_RED_MEMORY_ALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, MemoryClass, Size, Align )

// Realloc to a pool 
#define RED_MEMORY_REALLOCATE( PoolName, Ptr, MemoryClass, Size )	INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, Ptr, MemoryClass, Size, CalculateDefaultAlignment( Size ) )
#define RED_MEMORY_REALLOCATE_ALIGNED( PoolName, Ptr, MemoryClass, Size, Align )	INTERNAL_RED_MEMORY_REALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, Ptr, MemoryClass, Size, Align )

// Free memory from a pool
#define RED_MEMORY_FREE( PoolName, MemoryClass, Ptr ) INTERNAL_RED_MEMORY_FREE( SRedMemory::GetInstance(), CoreMemory, PoolName, MemoryClass, Ptr )

// Those macro are use when memory pool can't be resolve at compile time
#define RED_MEMORY_RUNTIME_ALLOCATE( PoolName, MemoryClass, Size, Align )	INTERNAL_RED_MEMORY_RUNTIME_ALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, MemoryClass, Size, Align )

#define RED_MEMORY_RUNTIME_FREE( PoolName, MemoryClass, Ptr ) INTERNAL_RED_MEMORY_RUNTIME_FREE( SRedMemory::GetInstance(), CoreMemory, PoolName, MemoryClass, Ptr )

#define RED_MEMORY_RUNTIME_REALLOCATE( PoolName, Ptr, MemoryClass, Size, Align )	INTERNAL_RED_MEMORY_RUNTIME_REALLOCATE_ALIGNED( SRedMemory::GetInstance(), CoreMemory, PoolName, Ptr, MemoryClass, Size, Align )

// Get a pointer to a pool / allocator directly								
#define RED_MEMORY_GET_ALLOCATOR( PoolName )	INTERNAL_RED_MEMORY_GET_ALLOCATOR( SRedMemory::GetInstance(), CoreMemory, PoolName )

#else

// Allocate to a pool
#define RED_MEMORY_ALLOCATE( PoolName, MemoryClass, Size )					Memory::Allocate< PoolName >( Size, (EMemoryClass)MemoryClass )
#define RED_MEMORY_ALLOCATE_ALIGNED( PoolName, MemoryClass, Size, Align )	Memory::AllocateAligned< PoolName >( Size, Align, (EMemoryClass)MemoryClass )

// Realloc to a pool 
#define RED_MEMORY_REALLOCATE( PoolName, Ptr, MemoryClass, Size )					Memory::ReallocateAligned< PoolName >( Ptr, Size, 8,(EMemoryClass) MemoryClass )
#define RED_MEMORY_REALLOCATE_ALIGNED( PoolName, Ptr, MemoryClass, Size, Align )	Memory::ReallocateAligned< PoolName >( Ptr, Size, Align, (EMemoryClass)MemoryClass )

// Free memory from a pool
#define RED_MEMORY_FREE( PoolName, MemoryClass, Ptr )	Memory::Free< PoolName >( Ptr,(EMemoryClass) MemoryClass )

// Those macro are use when memory pool can't be resolve at compile time
#define RED_MEMORY_RUNTIME_ALLOCATE( PoolName, MemoryClass, Size, Align )	RED_MEMORY_ALLOCATE_ALIGNED( PoolName, (EMemoryClass)MemoryClass, Size, Align )

#define RED_MEMORY_RUNTIME_FREE( PoolName, MemoryClass, Ptr ) RED_MEMORY_FREE( PoolName, (EMemoryClass)MemoryClass, Ptr )

#define RED_MEMORY_RUNTIME_REALLOCATE( PoolName, Ptr, MemoryClass, Size, Align )	RED_MEMORY_REALLOCATE_ALIGNED( PoolName, Ptr, (EMemoryClass)MemoryClass, Size, Align )

// Get a pointer to a pool / allocator directly								
#define RED_MEMORY_GET_ALLOCATOR( PoolName ) PoolName::GetAllocator()


#endif


#endif
