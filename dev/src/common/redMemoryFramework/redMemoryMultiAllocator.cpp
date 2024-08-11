/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryMultiAllocator.h"
#include "redMemoryAllocatorInfo.h"

namespace Red { namespace MemoryFramework {

	///////////////////////////////////////////////////////////////////
	// CTor
	MultiAllocator::MultiAllocator()
	{
	}

	///////////////////////////////////////////////////////////////////
	// DTor
	MultiAllocator::~MultiAllocator()
	{
	}

	///////////////////////////////////////////////////////////////////
	// RequestAllocatorInfo
	void MultiAllocator::RequestAllocatorInfo( AllocatorInfo& info )
	{
		info.SetAllocatorTypeName( TXT( "MultiAllocator" ) );
		info.SetAllocatorBudget( 0 );
	}

	///////////////////////////////////////////////////////////////////
	// Initialise
	EAllocatorInitResults MultiAllocator::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
	{
		const CreationParameters& creationParams = static_cast< const CreationParameters& >( parameters );
		RED_MEMORY_ASSERT( creationParams.m_smallOwnershipFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_smallSizeFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_smallAllocateFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_smallFreeFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_largeOwnershipFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_largeSizeFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_largeAllocateFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_largeFreeFn, "All function pointers must be set" );
		RED_MEMORY_ASSERT( creationParams.m_smallAllocationCutoff > 0, "Small cutoff must be > 0" );
		m_creationParameters = creationParams;
		return AllocInit_OK;
	}

	///////////////////////////////////////////////////////////////////
	// Release
	//	Since this just routes allocations, we can't really do anything here
	void MultiAllocator::Release( )
	{
	}

	///////////////////////////////////////////////////////////////////
	// ReleaseFreeMemoryToSystem
	//	Since this just routes allocations, we can't really do anything here
	Red::System::MemSize MultiAllocator::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
	{
		RED_UNUSED( areaCallback );
		return 0;
	}

	///////////////////////////////////////////////////////////////////
	// IncreaseMemoryFootprint
	//	Since this just routes allocations, we can't really do anything here
	Red::System::Bool MultiAllocator::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
	{
		RED_UNUSED( areaCallback );
		RED_UNUSED( sizeRequired );
		return false;
	}

	///////////////////////////////////////////////////////////////////
	// Allocate
	//	Route allocation through one of the registered pools
	void* MultiAllocator::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		void* returnPtr = nullptr;
		if( allocSize <= m_creationParameters.m_smallAllocationCutoff )
		{
			returnPtr = m_creationParameters.m_smallAllocateFn( allocSize, allocAlignment );
			allocatedSize = m_creationParameters.m_smallSizeFn( returnPtr );
		}
		else
		{
			returnPtr = m_creationParameters.m_largeAllocateFn( allocSize, allocAlignment );
			allocatedSize = m_creationParameters.m_largeSizeFn( returnPtr );
		}
		
		return returnPtr;
	}

	///////////////////////////////////////////////////////////////////
	// Reallocate
	void* MultiAllocator::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		if( ptr != nullptr && !OwnsPointer( ptr ) )		// We only want to deal with memory we own
		{
			allocatedSize = 0;
			freedSize = 0;
			return nullptr;
		}

		Red::System::MemSize currentSize = 0;
		void* newPtr = nullptr;
		if( ptr != nullptr )
		{
			currentSize = GetAllocationSize( ptr );
		}
	
		// Allocate + Copy the data if required
		if( allocSize != 0 )
		{
			newPtr = Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
			if( !newPtr )
			{
				allocatedSize = 0;
				freedSize = 0;
				return nullptr;		// OOM
			}
			else if( currentSize > 0 )
			{
				Red::System::MemSize copySize = currentSize < allocSize ? currentSize : allocSize;
				Red::System::MemoryCopy( newPtr, ptr, copySize );
			}
		}

		// Free the old ptr
		if( ptr != nullptr && newPtr != ptr )
		{
			Free( ptr );		// This passes a 'free' metrics marker so we don't update freedSize ourselves
			freedSize = currentSize;
		}

		return newPtr;
	}

	///////////////////////////////////////////////////////////////////
	// Free
	//	Try to free from the small pool first, assuming it is the faster implementation
	EAllocatorFreeResults MultiAllocator::Free( const void* ptr )
	{
		if( m_creationParameters.m_smallOwnershipFn( ptr ) )
		{
			return m_creationParameters.m_smallFreeFn( ptr );
		}
		else if( m_creationParameters.m_largeOwnershipFn( ptr ) )
		{
			return m_creationParameters.m_largeFreeFn( ptr );
		}
		else
		{
			return Free_NotOwned;
		}
	}

	///////////////////////////////////////////////////////////////////
	// GetAllocationSize
	Red::System::MemSize MultiAllocator::GetAllocationSize( const void* ptr ) const
	{
		if( m_creationParameters.m_smallOwnershipFn( ptr ) )
		{
			return m_creationParameters.m_smallSizeFn( ptr );
		}
		else if( m_creationParameters.m_largeOwnershipFn( ptr ) )
		{
			return m_creationParameters.m_largeSizeFn( ptr );
		}
		else
		{
			return 0;
		}
	}

	///////////////////////////////////////////////////////////////////
	// OwnsPointer
	Red::System::Bool MultiAllocator::OwnsPointer( const void* ptr ) const
	{
		if( m_creationParameters.m_smallOwnershipFn( ptr ) )
		{
			return true;
		}
		else if( m_creationParameters.m_largeOwnershipFn( ptr ) )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	///////////////////////////////////////////////////////////////////
	// WalkAllocator
	void MultiAllocator::WalkAllocator( AllocatorWalker* theWalker )
	{
		RED_UNUSED( theWalker );
	}

	///////////////////////////////////////////////////////////////////
	// WalkPoolArea
	void MultiAllocator::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker )
	{
		RED_UNUSED( startAddress );
		RED_UNUSED( size );
		RED_UNUSED( theWalker );
	}

	///////////////////////////////////////////////////////////////////
	// OnOutOfMemory
	void MultiAllocator::OnOutOfMemory()
	{
	}

	///////////////////////////////////////////////////////////////////
	// DumpDebugOutput
	void MultiAllocator::DumpDebugOutput()
	{
	}

} }