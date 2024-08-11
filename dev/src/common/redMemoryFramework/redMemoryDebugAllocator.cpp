/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryDebugAllocator.h"
#include "redMemoryAllocatorInfo.h"
#include "redMemoryPageAllocator.h"
#include "..\redThreads\redThreadsThread.h"

#ifdef RED_MEMORY_FRAMEWORK_PLATFORM_WINDOWS_API

namespace Red { namespace MemoryFramework {

	MemoryRegionHandle DebugAllocator::AllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint )	
	{ 
		return MemoryRegionHandle(); 
	}

	EAllocatorFreeResults DebugAllocator::FreeRegion( MemoryRegionHandle handle ) 
	{ 
		return Free_NotOwned; 
	}

	//////////////////////////////////////////////////////////////////////////
	// CTor
	//
	DebugAllocator::DebugAllocator()
		: IAllocator()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// DTor
	//
	DebugAllocator::~DebugAllocator()
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// IncreaseMemoryFootprint
	//
	Red::System::Bool DebugAllocator::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
	{
		RED_UNUSED( areaCallback );
		RED_UNUSED( sizeRequired );
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// ReleaseFreeMemoryToSystem
	//
	Red::System::MemSize DebugAllocator::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
	{
		RED_UNUSED( areaCallback );
		return 0u;
	}

	//////////////////////////////////////////////////////////////////////////
	// RequestAllocatorInfo
	//	
	void DebugAllocator::RequestAllocatorInfo( AllocatorInfo& info )
	{
		info.SetAllocatorBudget( 0 );
		info.SetAllocatorTypeName( TXT( "DebugAlloc" ) );
		info.SetPerAllocationOverhead( 0 );
	}

	//////////////////////////////////////////////////////////////////////////
	// Initialise
	//	
	EAllocatorInitResults DebugAllocator::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
	{
		m_flags = flags;
		RED_UNUSED( parameters );
		return AllocInit_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Release
	//
	void DebugAllocator::Release( )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// Allocate
	//	
	void* DebugAllocator::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{ 
		void * ptr = _aligned_malloc( allocSize, allocAlignment );
		allocatedSize = _aligned_msize(ptr, allocAlignment, 0);
		return ptr;
	}

	//////////////////////////////////////////////////////////////////////////
	// Free
	//	Does nothing
	EAllocatorFreeResults DebugAllocator::Free( const void* ptr )
	{
		_aligned_free( const_cast< void* >( ptr ) );
		return Free_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetAllocationSize
	//	...
	Red::System::MemSize DebugAllocator::GetAllocationSize( const void* ptr ) const
	{
		return _aligned_msize( const_cast< void* >( ptr ), 8, 0 );
	}

	//////////////////////////////////////////////////////////////////////////
	// OwnsPointer
	//	...
	Red::System::Bool DebugAllocator::OwnsPointer( const void* ptr ) const
	{
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// Reallocate
	//	Does nothing, returns a non-null value to indicate success
	void* DebugAllocator::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		
		if( ptr == nullptr && allocSize == 0 )
		{
			return nullptr;
		}

		if( ptr )
		{
			Red::System::MemSize oldSize = _aligned_msize( ptr, allocAlignment, 0 );
			if( oldSize == allocSize )
			{
				freedSize = allocSize;
				allocatedSize = allocSize;
				return ptr;
			}
			else if( allocSize == 0 )
			{
				Free( ptr );
				freedSize =  oldSize;
				allocatedSize = 0;
				return nullptr;
			}
			else
			{
				void * newPtr = Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
				Red::System::MemSize toCopy = oldSize < allocSize ? oldSize : allocSize;
				Red::System::MemoryCopy( newPtr, ptr, toCopy );
				freedSize = oldSize;
				Free( ptr );
				return newPtr;
			}
		}
		else
		{
			freedSize = 0;
			return Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
		}
	}

} } // namespace Red { namespace MemoryFramework {

#endif
