/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

namespace Red { namespace MemoryFramework {

	/////////////////////////////////////////////////////////////////
	// CTor
	//
	template< typename TSyncLock >
	VirtualAllocWrapperAllocator< TSyncLock >::VirtualAllocWrapperAllocator()
		: IAllocator()
		, m_allocRecordPoolHead( nullptr )
		, m_allocRecordPoolTail( nullptr )
		, m_activeAllocsHead( nullptr )
		, m_activeAllocsTail( nullptr )
		, m_flags( 0 )
	{

	}

	/////////////////////////////////////////////////////////////////
	// DTor
	//
	template< typename TSyncLock >
	VirtualAllocWrapperAllocator< TSyncLock >::~VirtualAllocWrapperAllocator()
	{

	}

	/////////////////////////////////////////////////////////////////
	// Allocate
	//	We have no special case for alignment, just assume its < granularity
	template< typename TSyncLock >
	void*	VirtualAllocWrapperAllocator< TSyncLock >::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		ActiveAllocation* allocRecord = Utils::PopFront( m_allocRecordPoolHead, m_allocRecordPoolTail );	// Get a record first
		if( allocRecord == nullptr )
		{
			RED_MEMORY_LOG( TXT( "Ran out of allocation records in VirtualAllocWrapperAllocator" ) );
			return nullptr;
		}

		void* allocatedPages = PageAllocator::GetInstance().GetPagedMemory( allocSize, allocatedSize, m_flags );
		RED_MEMORY_ASSERT( ( reinterpret_cast< MemUint >( allocatedPages ) & ( allocAlignment - 1 ) ) == 0, "Requested alignment cannot be serviced by this allocator" );

		if( allocatedPages )
		{
			allocRecord->m_startAddress = reinterpret_cast< MemUint >( allocatedPages );
			allocRecord->m_totalSize = allocatedSize;
			Utils::PushFront( m_activeAllocsHead, m_activeAllocsTail, allocRecord );	// Record is active
		}

		return allocatedPages;
	}

	/////////////////////////////////////////////////////////////////
	// Reallocate
	//	We don't need it, don't use it!
	template< typename TSyncLock >
	void*	VirtualAllocWrapperAllocator< TSyncLock >::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		RED_MEMORY_ASSERT( false, "NOT IMPLEMENTED" );
		return nullptr;
	}

	/////////////////////////////////////////////////////////////////
	// Free
	//
	template< typename TSyncLock >
	EAllocatorFreeResults VirtualAllocWrapperAllocator< TSyncLock >::Free( const void* ptr )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		// Find the matching record (slow! don't allocate too much in here)
		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			if( theAlloc->m_startAddress == reinterpret_cast< MemUint >( ptr ) )
			{
				PageAllocator::GetInstance().FreePagedMemory( reinterpret_cast< void* >( theAlloc->m_startAddress ), theAlloc->m_totalSize, m_flags );
				Utils::RemoveFromList( m_activeAllocsHead, m_activeAllocsTail, theAlloc );			// Remove from active list
				Utils::PushFront( m_allocRecordPoolHead, m_allocRecordPoolTail, theAlloc );			// Release record back to pool
				return Free_OK;
			}
			theAlloc = theAlloc->GetNext();
		}

		return Free_NotOwned;
	}

	/////////////////////////////////////////////////////////////////
	// GetAllocationSize
	//
	template< typename TSyncLock >
	Red::System::MemSize VirtualAllocWrapperAllocator< TSyncLock >::GetAllocationSize( const void* ptr ) const
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		// Find the matching record
		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			if( theAlloc->m_startAddress == reinterpret_cast< MemUint >( ptr ) )
			{
				return theAlloc->m_totalSize;
			}
			theAlloc = theAlloc->GetNext();
		}

		return 0;
	}

	/////////////////////////////////////////////////////////////////
	// OwnsPointer
	//
	template< typename TSyncLock >
	Red::System::Bool VirtualAllocWrapperAllocator< TSyncLock >::OwnsPointer( const void* ptr ) const
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		// Find the matching record
		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			if( theAlloc->m_startAddress == reinterpret_cast< MemUint >( ptr ) )
			{
				return true;
			}
			theAlloc = theAlloc->GetNext();
		}

		return false;
	}

	/////////////////////////////////////////////////////////////////
	// Initialise
	//	
	template< typename TSyncLock >
	EAllocatorInitResults VirtualAllocWrapperAllocator< TSyncLock >::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		// Set up the pool list
		MemSize poolSize;
		m_allocRecordPool = (ActiveAllocation*) PageAllocator::GetInstance().GetPagedMemory( sizeof( ActiveAllocation ) * c_maxAllocRecords, poolSize, m_flags );
		RED_MEMORY_ASSERT( m_allocRecordPool, "Failed to allocate memory for bookkeeping" );
		RED_MEMORY_ASSERT( poolSize >= sizeof( ActiveAllocation ) * c_maxAllocRecords, "Failed to allocate memory for bookkeeping" );

		m_allocRecordPoolHead = nullptr;
		m_allocRecordPoolTail = nullptr;
		for( Uint32 i=0; i < c_maxAllocRecords; ++i )
		{
			m_allocRecordPool[i].SetPrevious( nullptr );
			m_allocRecordPool[i].SetNext( nullptr );
			Utils::PushFront( m_allocRecordPoolHead, m_allocRecordPoolTail, &m_allocRecordPool[ i ] );
		}

		return AllocInit_OK;
	}

	/////////////////////////////////////////////////////////////////
	// Release
	//	Free up any memory still used
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::Release( )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			PageAllocator::GetInstance().FreePagedMemory( reinterpret_cast< void* >( theAlloc->m_startAddress ), theAlloc->m_totalSize, m_flags );
			theAlloc = theAlloc->GetNext();
		}

		PageAllocator::GetInstance().FreePagedMemory( m_allocRecordPool, sizeof( ActiveAllocation ) * c_maxAllocRecords, m_flags );
	}

	/////////////////////////////////////////////////////////////////
	// IncreaseMemoryFootprint
	//	Do nothing
	template< typename TSyncLock >
	Red::System::Bool VirtualAllocWrapperAllocator< TSyncLock >::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		return false;
	}

	/////////////////////////////////////////////////////////////////
	// ReleaseFreeMemoryToSystem
	//	Do nothing
	template< typename TSyncLock >
	Red::System::MemSize VirtualAllocWrapperAllocator< TSyncLock >::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		return 0;
	}

	/////////////////////////////////////////////////////////////////
	// OnOutOfMemory
	//	Output proper debug info
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::OnOutOfMemory()
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		DumpDebugOutput();
	}

	/////////////////////////////////////////////////////////////////
	// RequestAllocatorInfo
	//
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::RequestAllocatorInfo( AllocatorInfo& info )
	{
		info.SetAllocatorTypeName( TXT( "VirtualAllocWrapper" ) );
		info.SetAllocatorBudget( 0 );
		info.SetPerAllocationOverhead( 0 );
	}

	/////////////////////////////////////////////////////////////////
	// WalkAllocator
	//	Returns each area that has been allocated from the chunk allocator. They are NOT in address order
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::WalkAllocator( AllocatorWalker* theWalker )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		
		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			theWalker->OnMemoryArea( theAlloc->m_startAddress, theAlloc->m_totalSize );
			theAlloc = theAlloc->GetNext();
		}
	}

	/////////////////////////////////////////////////////////////////
	// WalkPoolArea
	//	Since we don't track free blocks, all this can do is return that an area was, in fact allocated
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		theWalker->OnUsedArea( startAddress, size, 0 );	/// A bit dangerous, but the only thing calling this should be coming from WalkAllocator()
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeAllocate
	//	Just call through to 'static' version
	template< typename TSyncLock >
	void*  VirtualAllocWrapperAllocator< TSyncLock >::RuntimeAllocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
	{
		return Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeReallocate
	//	Just call through to 'static' version
	template< typename TSyncLock >
	void*  VirtualAllocWrapperAllocator< TSyncLock >::RuntimeReallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
	{
		return Reallocate( ptr, allocSize, allocAlignment, allocatedSize, freedSize, memoryClass );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeFree
	//	Just call through to 'static' version
	template< typename TSyncLock >
	EAllocatorFreeResults  VirtualAllocWrapperAllocator< TSyncLock >::RuntimeFree( void* ptr )
	{
		return Free( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeGetAllocationSize
	//	Just call through to 'static' version
	template< typename TSyncLock >
	Red::System::MemSize  VirtualAllocWrapperAllocator< TSyncLock >::RuntimeGetAllocationSize( void* ptr ) const
	{
		return GetAllocationSize( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeOwnsPointer
	//	Just call through to 'static' version
	template< typename TSyncLock >
	Red::System::Bool  VirtualAllocWrapperAllocator< TSyncLock >::RuntimeOwnsPointer( void* ptr ) const
	{
		return OwnsPointer( ptr );
	}

	/////////////////////////////////////////////////////////////////
	// DumpDebugInfoVerbose
	//	Logs loads of debug info in case something goes wrong
	template< typename TSyncLock >
	void VirtualAllocWrapperAllocator< TSyncLock >::DumpDebugOutput()
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		RED_MEMORY_LOG( TXT( "Active allocations: " ) );

		ActiveAllocation* theAlloc = m_activeAllocsHead;
		while( theAlloc != nullptr )
		{
			RED_MEMORY_LOG( TXT( "%p - %d bytes" ), theAlloc->m_startAddress, theAlloc->m_totalSize );
			theAlloc = theAlloc->GetNext();
		}
	}
} }
