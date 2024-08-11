#include "redMemoryRegionAllocator.h"

namespace Red { namespace MemoryFramework { 

	//////////////////////////////////////////////////////////////////////
	// RuntimeAllocate
	void* RegionAllocator::RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		
		RED_UNUSED( allocSize );
		RED_UNUSED( allocAlignment );
		RED_UNUSED( allocatedSize );
		return nullptr;
	}

	//////////////////////////////////////////////////////////////////////
	// RuntimeReallocate
	void* RegionAllocator::RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		RED_UNUSED( ptr );
		RED_UNUSED( allocSize );
		RED_UNUSED( allocAlignment );
		RED_UNUSED( allocatedSize );
		RED_UNUSED( freedSize );
		return nullptr;
	}
	
	//////////////////////////////////////////////////////////////////////
	// RuntimeFree
	EAllocatorFreeResults RegionAllocator::RuntimeFree( void* ptr )
	{
		RED_UNUSED( ptr );
		return Free_NotOwned;
	}

	//////////////////////////////////////////////////////////////////////
	// RuntimeGetAllocationSize
	Red::System::MemSize RegionAllocator::RuntimeGetAllocationSize( void* ptr ) const
	{
		RED_UNUSED( ptr );
		return 0;
	}

	//////////////////////////////////////////////////////////////////////
	// RuntimeOwnsPointer
	Red::System::Bool RegionAllocator::RuntimeOwnsPointer( void* ptr ) const 
	{
		RED_UNUSED( ptr );
		return false;
	}

} }