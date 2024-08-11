#pragma once

// Helper class for making MultiAllocator objects in Core Memory

namespace CoreMemory
{
	// Helper class to make setting up a multi-allocator much simpler
	template< Red::MemoryFramework::PoolLabel TSmallAllocPool, Red::MemoryFramework::MemoryClass TSmallAllocMemClass,
		Red::MemoryFramework::PoolLabel TLargeAllocPool, Red::MemoryFramework::MemoryClass TLargeAllocMemClass >
	class MultiAllocatorCreationHelper
	{
	public:
		static void* SmallAlloc( Red::System::MemSize size, Red::System::MemSize align ) 
		{ 
			return RED_MEMORY_ALLOCATE_ALIGNED( TSmallAllocPool, TSmallAllocMemClass, size, align );
		}
		static Red::MemoryFramework::EAllocatorFreeResults SmallFree( void* ptr ) 
		{ 
			return RED_MEMORY_FREE( TSmallAllocPool, TSmallAllocMemClass, ptr );
		}
		static Red::System::MemSize SmallSize( void* ptr )
		{
			Red::MemoryFramework::IAllocator* smallPool = SRedMemory::GetInstance().GetPool( TSmallAllocPool );
			auto typedSmallPool = static_cast< typename RED_MEMORY_POOL_TYPE( TSmallAllocPool )* >( smallPool );
			return typedSmallPool->GetAllocationSize( ptr );
		}
		static Bool SmallOwnership( void* ptr )
		{
			Red::MemoryFramework::IAllocator* smallPool = SRedMemory::GetInstance().GetPool( TSmallAllocPool );
			auto typedSmallPool = static_cast< typename RED_MEMORY_POOL_TYPE( TSmallAllocPool )* >( smallPool );
			return typedSmallPool->OwnsPointer( ptr );
		}
		static void* LargeAlloc( Red::System::MemSize size, Red::System::MemSize align ) 
		{ 
			return RED_MEMORY_ALLOCATE_ALIGNED( TLargeAllocPool, TLargeAllocMemClass, size, align );
		}
		static Red::MemoryFramework::EAllocatorFreeResults LargeFree( void* ptr ) 
		{ 
			return RED_MEMORY_FREE( TLargeAllocPool, TLargeAllocMemClass, ptr );
		}
		static Red::System::MemSize LargeSize( void* ptr )
		{
			Red::MemoryFramework::IAllocator* largePool = SRedMemory::GetInstance().GetPool( TLargeAllocPool );
			auto typedlargePool = static_cast< typename RED_MEMORY_POOL_TYPE( TLargeAllocPool )* >( largePool );
			return typedlargePool->GetAllocationSize( ptr );
		}
		static Bool LargeOwnership( void* ptr )
		{
			Red::MemoryFramework::IAllocator* largePool = SRedMemory::GetInstance().GetPool( TLargeAllocPool );
			auto typedLargePool = static_cast< typename RED_MEMORY_POOL_TYPE( TLargeAllocPool )* >( largePool );
			return typedLargePool->OwnsPointer( ptr );
		}
	};
}