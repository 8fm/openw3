/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#define ENABLE_VALIDATE_ON_DEFRAG

#ifdef RED_PLATFORM_CONSOLE
	#define ENABLE_REGION_LOCK_PROTECTION
#endif

namespace Red { namespace MemoryFramework {

	// How much free space are we willing to leave at the end of an allocation (helps to reduce fragmentation)
	const Uint32 k_maximumWastedSpacePerRegion = 1024 * 4;		

	namespace GpuAllocatorImpl
	{
		///////////////////////////////////////////////////////////////////////
		// Node is used to represent a non-moveable region made public to the user
		// Locked node = node cannot be moved during defrag, or freed
		class RegionNode : public Utils::ListNode< RegionNode >, public MemoryRegion
		{
		public:
			RED_INLINE Bool IsFree() const										{ return ( m_packedAlignmentAndStatus & c_packedFree ) == 0 ? false : true; }
			RED_INLINE Bool IsLocked() const									{ return ( m_packedAlignmentAndStatus & c_packedLock ) == 0 ? false : true; }
			RED_INLINE Red::System::Uint32 GetAlignment() const					{ return m_packedAlignmentAndStatus & ~c_packMask; }
			RED_INLINE Red::System::MemSize GetEndAddress() const				{ return m_alignedAddress + m_alignedSize; }
			RED_INLINE void SetAlignedAddress( Red::System::MemUint address)	{ m_alignedAddress = address; }
			RED_INLINE void SetAlignedSize( Red::System::MemSize size )			{ m_alignedSize = static_cast< Red::System::Uint32 >( size ); }
			RED_INLINE void SetIsFree( Bool isFree )	
			{
				if( isFree )
				{
					m_packedAlignmentAndStatus |= c_packedFree; 
				}
				else
				{
					m_packedAlignmentAndStatus &= ~c_packedFree;
				}
			}
			RED_INLINE void SetLockStatus( Bool isLocked )
			{
				if( isLocked )
				{
					m_packedAlignmentAndStatus |= c_packedLock; 
				}
				else
				{
					m_packedAlignmentAndStatus &= ~c_packedLock;
				}
			}

			RED_INLINE void SetAlignment( Red::System::Uint32 alignment )		
			{ 
				m_packedAlignmentAndStatus = ( m_packedAlignmentAndStatus & c_packMask ) | ( alignment & ~c_packMask );
			}

			RED_INLINE void SetAlignmentAndFreeStatus( Red::System::Uint32 alignment, Bool isFree )
			{
				Uint32 maskedLock = m_packedAlignmentAndStatus & c_packedLock;
				Uint32 maskedFree = isFree ? c_packedFree : 0;
				m_packedAlignmentAndStatus = maskedFree | maskedLock | ( alignment & ~c_packMask );
			}
			

		private:
			static const Uint32 c_packedFree = ( 1 << 31 );
			static const Uint32 c_packedLock = ( 1 << 30 );
			static const Uint32 c_packMask = c_packedFree | c_packedLock;
			union
			{
				struct  
				{
					Bool isFree : 1;
					Bool isLocked : 1;
					Uint32 alignment : 30;
				} helper;
				Uint32 m_packedAlignmentAndStatus;
			};
		};

		///////////////////////////////////////////////////////////////////////
		// ValidateNode
		//	Test that the linked list is still ok and memory is physically contiguous
		RED_INLINE void ValidateNode(RegionNode* theNode)
		{
			if( theNode->GetPrevious() )
			{
				RED_MEMORY_ASSERT( theNode->GetPrevious()->GetEndAddress() == theNode->GetAddress(), "Nodes are not physically contiguous!" );
				RED_MEMORY_ASSERT( theNode->GetPrevious()->GetNext() == theNode, "Linked list is broken" );
			}
			if( theNode->GetNext() )
			{
				RED_MEMORY_ASSERT( theNode->GetNext()->GetAddress() == theNode->GetEndAddress(), "Nodes are not physically contiguous!" );
				RED_MEMORY_ASSERT( theNode->GetNext()->GetPrevious() == theNode, "Linked list is broken" );
			}
			if( !theNode->IsFree() )
			{
				RED_MEMORY_ASSERT( theNode->GetAlignment() != 0, "Alignment not set on used node" );
				RED_MEMORY_ASSERT( ( theNode->GetAddress() & ( theNode->GetAlignment() - 1 ) ) == 0, "Misaligned used region" );
			}
		}
	}

	///////////////////////////////////////////////////////////////////////
	// CTor
	template< typename TSyncLock >
	GpuAllocator< TSyncLock >::GpuAllocator()
		: m_gpuMemoryBlock( nullptr )
		, m_cpuMemoryBlock( nullptr )
		, m_freeRegionHead( nullptr )
		, m_freeRegionTail( nullptr )
		, m_bottomPhysicalHead( nullptr )
		, m_bottomPhysicalTail( nullptr )
		, m_topPhysicalHead( nullptr )
		, m_topPhysicalTail( nullptr )
		, m_fragmentationScope( 0 )
	{
	}

	///////////////////////////////////////////////////////////////////////
	// DTor
	template< typename TSyncLock >
	GpuAllocator< TSyncLock >::~GpuAllocator()
	{
	}

	///////////////////////////////////////////////////////////////////////
	// DefragmentTopRegions
	//	Defrag a set of regions. Region addresses are fixed up, but no data is moved (this is the job of the userCallback)
	//	Returns number of bytes moved
	template< typename TSyncLock >
	Uint32 GpuAllocator< TSyncLock >::DefragmentTopRegions( const DefragSettings& settings, Red::System::MemUint& recommendedStartingPoint )
	{
		// Any empty nodes created during defrag are in an initially "locked" state. Allocations will not be made in locked
		// nodes, so we won't start loading a new mesh into a chunk of memory that may still be being copied from.
		// AfterDefragmentationMemoryFinished() must be called when all copies are finished, to unlock those locked empty nodes.

		// Skip over empty nodes smaller than this. Not entirely sure why, but when this gets very small (possibly around
		// the alignment size), it can lead to data corruption. So keeping it at 4KB, which has apparently been safe so far.
		const MemSize smallestGapToFill = 4 * 1024;

		GpuAllocatorImpl::RegionNode* currentNode = m_topPhysicalTail;
		MemSize bytesToMoveAccumulated = 0;
		MemSize bytesMoved = 0;

		MemSize baseAddress = currentNode != nullptr ? currentNode->GetAddress() : 0;
		MemSize amountToSkip = settings.startingPoint;

		while( currentNode != nullptr )
		{
			const Red::System::MemSize nodeSize = currentNode->GetSize();

			if ( amountToSkip < nodeSize )
			{
				break;
			}
			amountToSkip -= nodeSize;
			currentNode = currentNode->GetPrevious();
		}

		while( currentNode != nullptr )
		{
			const Red::System::MemSize nodeSize = currentNode->GetSize();

			if( bytesMoved >= settings.maxMemoryToMove )
			{
				MemUint endAddr = currentNode->GetAddress() + bytesToMoveAccumulated;
				RED_MEMORY_ASSERT( endAddr <= baseAddress, "" );
				recommendedStartingPoint = baseAddress - endAddr;

				if( bytesToMoveAccumulated > 0 )	// There is a hole to be filled with a new free block
				{
					GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( currentNode->GetEndAddress(), bytesToMoveAccumulated, true );
					Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
					GpuAllocatorImpl::ValidateNode( newFreeNode );
					CoalesceFreeNode( m_topPhysicalHead, m_topPhysicalTail, newFreeNode );	// Merge with other free nodes
				}
				return static_cast< Uint32 >( bytesMoved );
			}
			else if( currentNode->IsFree() && nodeSize >= smallestGapToFill )
			{
				RED_MEMORY_ASSERT( !currentNode->IsLocked(), "Locked nodes cannot be free!" );

				// We remove all free nodes, and accumulate the memory footprint we regained
				bytesToMoveAccumulated += nodeSize;
				GpuAllocatorImpl::RegionNode* nextNode = currentNode->GetPrevious();
				Utils::RemoveFromList( m_topPhysicalHead, m_topPhysicalTail, currentNode );
				ReleaseFreeRegion( currentNode );
				currentNode = nextNode;
				continue;
			}
			else if( bytesToMoveAccumulated > 0 )
			{
				if( currentNode->IsLocked() || nodeSize > settings.blockSizeLimit )
				{
					RED_MEMORY_ASSERT( !currentNode->IsLocked() || !currentNode->IsFree(), "Locked nodes cannot be free!" );

					// Insert a free block rather than moving anything
					MemUint newBaseAddress = currentNode->GetEndAddress();
					GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, bytesToMoveAccumulated, true );
					Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
					ValidateNode( newFreeNode );
					bytesToMoveAccumulated = 0;
				}
				else
				{
					// New base address may not fit exactly where last free block was
					MemUint requiredAlignment = currentNode->GetAlignment();
					MemUint newBaseAddress = currentNode->GetAddress() + bytesToMoveAccumulated;
					MemUint alignedAddress = requiredAlignment == 0 ? newBaseAddress : ( newBaseAddress & ~( requiredAlignment - 1 ) );		// Align down (i.e. may need end-padding)
					if( newBaseAddress != alignedAddress )
					{
						// We need to insert a free block to pick up the slack from the alignment
						MemUint freeBlockSize = newBaseAddress - alignedAddress;
						GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( alignedAddress + nodeSize, freeBlockSize, true );
						Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
						bytesToMoveAccumulated -= freeBlockSize;	// Remove the 'slack' from the free block
					}
					if( alignedAddress != currentNode->GetAddress() )
					{
						settings.moveRequestCb( reinterpret_cast< void* >( currentNode->GetAddress() ), reinterpret_cast< void* >( alignedAddress ), nodeSize, settings.userData );
						currentNode->SetAlignedAddress( alignedAddress );
						bytesMoved += nodeSize;
					}
				}
			}

			currentNode = currentNode->GetPrevious();
		}

		// Fill end with an empty block. Can't just leave it out, because we don't want to allocate something new here.
		// NOTE: Not 100% sure about this, we aren't actually using "top" regions for anything right now...
		if( bytesToMoveAccumulated > 0 )
		{
			GpuAllocatorImpl::RegionNode* firstNode = m_bottomPhysicalHead;
			if ( firstNode != nullptr ) // is this needed? probably not, but just being safe...
			{
				// We need to insert a free block to pick up the slack from the alignment
				MemUint freeBlockSize = bytesToMoveAccumulated;
				GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( firstNode->GetAddress() - freeBlockSize, freeBlockSize, true );
				Utils::InsertAfter( m_bottomPhysicalHead, m_bottomPhysicalTail, firstNode, newFreeNode );
				GpuAllocatorImpl::ValidateNode( newFreeNode );
			}
		}

		// If we reached the end of the pool, recommend starting over.
		recommendedStartingPoint = 0;

		return static_cast< Uint32 >( bytesMoved );
	}

	///////////////////////////////////////////////////////////////////////
	// DefragmentBottomRegions
	//	Defrag a set of regions. Regions are fixed up, but no data is moved (this is the job of the userCallback)
	//	Returns amount of memory moved
	template< typename TSyncLock >
	Uint32 GpuAllocator< TSyncLock >::DefragmentBottomRegions( const DefragSettings& settings, Red::System::MemUint& recommendedStartingPoint )
	{
		// Any empty nodes created during defrag are in an initially "locked" state. Allocations will not be made in locked
		// nodes, so we won't start loading a new mesh into a chunk of memory that may still be being copied from.
		// AfterDefragmentationMemoryFinished() must be called when all copies are finished, to unlock those locked empty nodes.

		// Skip over empty nodes smaller than this. Not entirely sure why, but when this gets very small (possibly around
		// the alignment size), it can lead to data corruption. So keeping it at 4KB, which has apparently been safe so far.
		const MemSize smallestGapToFill = 4 * 1024;

		GpuAllocatorImpl::RegionNode* currentNode = m_bottomPhysicalHead;
		MemSize bytesToMoveAccumulated = 0;
		MemSize regionMemoryMoved = 0;
		// Track whether we've moved actual data (and not just accumulated empty space). When it's time to add an empty
		// node where it was moved from, then if we've moved data the empty node should be locked. If we haven't moved
		// anything (for example, an empty gap just before a locked allocated node), then the empty node doesn't need to
		// be locked.
		Bool didMoveAnythingInRun = false;

		MemSize baseAddress = currentNode != nullptr ? currentNode->GetAddress() : 0;
		MemSize amountToSkip = settings.startingPoint;

		while( currentNode != nullptr )
		{
			const Red::System::MemSize nodeSize = currentNode->GetSize();

			if ( amountToSkip < nodeSize )
			{
				break;
			}
			currentNode = currentNode->GetNext();
			amountToSkip -= nodeSize;
		}

		const MemUint endAddress =  m_bottomPhysicalTail ? m_bottomPhysicalTail->GetEndAddress() : 0;

		while( currentNode != nullptr )
		{
			const Red::System::MemSize nodeSize = currentNode->GetSize();

			if( regionMemoryMoved >= settings.maxMemoryToMove || currentNode->GetAddress() >= endAddress )
			{
				MemUint endAddr = currentNode->GetAddress() - bytesToMoveAccumulated;
				RED_MEMORY_ASSERT( endAddr >= baseAddress, "" );
				recommendedStartingPoint = endAddr - baseAddress;

				if( bytesToMoveAccumulated > 0 )	// There is a hole to be filled with a new free block
				{
					const Bool lockRegion = didMoveAnythingInRun;
					GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( currentNode->GetAddress() - bytesToMoveAccumulated, bytesToMoveAccumulated, lockRegion );
					Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
					GpuAllocatorImpl::ValidateNode( newFreeNode );
					CoalesceFreeNode( m_bottomPhysicalHead, m_bottomPhysicalTail, newFreeNode );	// Merge with other free nodes
				}
				return static_cast< Uint32 >( regionMemoryMoved );
			}
			else if( currentNode->IsFree() && !currentNode->IsLocked() && nodeSize >= smallestGapToFill )
			{
				RED_MEMORY_ASSERT( !currentNode->IsLocked(), "Locked nodes cannot be free!" );

				// We remove all free nodes, and accumulate the memory footprint we regained
				bytesToMoveAccumulated += nodeSize;
				GpuAllocatorImpl::RegionNode* nextNode = currentNode->GetNext();
				Utils::RemoveFromList( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode );
				ReleaseFreeRegion( currentNode );
				currentNode = nextNode;
				continue;
			}
			else if( bytesToMoveAccumulated > 0 )
			{
				if( currentNode->IsLocked() || nodeSize > settings.blockSizeLimit )
				{
					RED_MEMORY_ASSERT( !currentNode->IsLocked() || !currentNode->IsFree(), "Locked nodes cannot be free!" );
					// We can't move this node; instead we add a free block which fills the accumulated movement, then we continue
					const Bool lockRegion = didMoveAnythingInRun;
					MemUint newBaseAddress = currentNode->GetAddress() - bytesToMoveAccumulated;
					GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, bytesToMoveAccumulated, lockRegion );
					Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
					ValidateNode( newFreeNode );
					bytesToMoveAccumulated = 0;
					didMoveAnythingInRun = false;
				}
				else
				{
					// If it's free and we're moving it (because it's smaller than the threshold, allowing for better
					// batching of copies), make sure to lock it. Don't want to allocate from it while a copy is still
					// in flight there.
					if ( currentNode->IsFree() )
					{
						currentNode->SetLockStatus( true );
					}

					// New base address may not fit exactly where last free block was, 
					MemUint requiredAlignment = currentNode->GetAlignment();
					MemUint newBaseAddress = currentNode->GetAddress() - bytesToMoveAccumulated;
					MemUint alignedAddress = requiredAlignment == 0 ? newBaseAddress : ( ( newBaseAddress + ( requiredAlignment - 1 ) ) & ~( requiredAlignment - 1 ) );

					// Insert free block to pick up the slack if there is an alignment hole
					if( alignedAddress != newBaseAddress )
					{
						const Bool locked = didMoveAnythingInRun;
						// We need to insert a free block to pick up the slack from the alignment
						MemUint freeBlockSize = alignedAddress - newBaseAddress;
						GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, freeBlockSize, locked );
						Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
						bytesToMoveAccumulated -= freeBlockSize;	// Remove the 'slack' from the free block
					}

					// Finally, the block can be updated + data move requested
					if( alignedAddress != currentNode->GetAddress() )
					{
						settings.moveRequestCb( reinterpret_cast< void* >( currentNode->GetAddress() ), reinterpret_cast< void* >( alignedAddress ), nodeSize, settings.userData );
						currentNode->SetAlignedAddress( alignedAddress );
						regionMemoryMoved += static_cast< Uint32 >( nodeSize );
						didMoveAnythingInRun = true;
					}
				}
			}

			currentNode = currentNode ? currentNode->GetNext() : m_bottomPhysicalHead;
		}

		// If we've moved anything at the end of the memory region, add in a locked empty block. Can't just leave it out
		// because we don't want to allocate something new here. If we haven't moved anything, then it's okay to use
		// that memory.
		if( bytesToMoveAccumulated > 0 && didMoveAnythingInRun )
		{
			GpuAllocatorImpl::RegionNode* lastNode = m_bottomPhysicalTail;
			if ( lastNode != nullptr ) // is this needed? probably not, but just being safe...
			{
				MemUint freeBlockSize = bytesToMoveAccumulated;
				GpuAllocatorImpl::RegionNode* newFreeNode = MakeFreeRegion( lastNode->GetEndAddress(), freeBlockSize, true );
				Utils::InsertAfter( m_bottomPhysicalHead, m_bottomPhysicalTail, lastNode, newFreeNode );
				GpuAllocatorImpl::ValidateNode( newFreeNode );
			}
		}

		// If we reached the end of the pool, recommend starting over.
		recommendedStartingPoint = 0;

		return static_cast< Uint32 >( regionMemoryMoved );
	}

	///////////////////////////////////////////////////////////////////////
	// RequestDefragmentation
	//	Internal regions are moved and patched, but the data is left to the user to deal with
	//	Once this completes, the user MUST do the data moving, and then call FinaliseDefragmentation
	template< typename TSyncLock >
	Red::System::MemUint GpuAllocator< TSyncLock >::RequestDefragmentation( DefragMode defragMode, const DefragSettings& settings )
	{
		m_syncLock.Acquire();		// Lock so nobody else can try to use this pool during defrag
		Int32 defragScope = m_fragmentationScope++;

		Red::System::MemUint recommendedStartingPoint = 0;

		RED_FATAL_ASSERT( defragScope == 0, "Recursive defrag, or attempting to run multiple defrags at once" );
		if( defragScope == 0 )
		{
			Uint32 bytesMoved = 0;
			if( defragMode == DefragShortLivedRegions )
			{
				bytesMoved = DefragmentBottomRegions( settings, recommendedStartingPoint );
			}
			else if( defragMode == DefragLongLivedRegions )
			{
				bytesMoved = DefragmentTopRegions( settings, recommendedStartingPoint );
			}

#ifdef ENABLE_VALIDATE_ON_DEFRAG
			// Validate list is still valid (contiguous, no gaps)
			GpuAllocatorImpl::RegionNode* currentNode = m_bottomPhysicalHead;
			while( currentNode != nullptr )
			{
				GpuAllocatorImpl::ValidateNode( currentNode );
				currentNode = currentNode->GetNext();
			}

			// Validate list is still valid (contiguous, no gaps)
			currentNode = m_topPhysicalHead;
			while( currentNode != nullptr )
			{
				GpuAllocatorImpl::ValidateNode( currentNode );
				currentNode = currentNode->GetNext();
			}
#endif
		}

		return recommendedStartingPoint;
	}

	///////////////////////////////////////////////////////////////////////
	// FinaliseDefragmentation
	//	The user must call this once all move requests have been processed. The allocator will then be available for use again
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::FinaliseDefragmentation()
	{
		// ... 

		--m_fragmentationScope;

		m_syncLock.Release();		// Allocator is ready for use again
	}

	///////////////////////////////////////////////////////////////////////
	// AfterDefragmentationMemoryFinished
	//	The user must call this after all move requests are finished. Until then, the empty spaces created by the defrag will
	//	not be available. This must be called before the next RequestDefragmentation.
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::AfterDefragmentationMemoryFinished( DefragMode defragMode )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		// Go through all region nodes, and unlock any free ones. These were locked on creation during the defrag
		// to make sure something new didn't get added where old data is still copying out of.
		if( defragMode == DefragShortLivedRegions )
		{
			GpuAllocatorImpl::RegionNode* currentNode = m_bottomPhysicalHead;
			while( currentNode != nullptr )
			{
				if ( currentNode->IsFree() && currentNode->IsLocked() )
				{
					currentNode->SetLockStatus( false );
					currentNode = CoalesceFreeNode( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode );
				}
				currentNode = currentNode->GetNext();
			}

			// If the tail node is free, it can be removed from the list.
			if ( m_bottomPhysicalTail != nullptr && m_bottomPhysicalTail->IsFree() )
			{
				RED_MEMORY_ASSERT( !m_bottomPhysicalTail->IsLocked(), "Free tail node is locked, but we've just unlocked everything!" );
				GpuAllocatorImpl::RegionNode* oldTail = m_bottomPhysicalTail;
				Utils::RemoveFromList( m_bottomPhysicalHead, m_bottomPhysicalTail, oldTail );
				ReleaseFreeRegion( oldTail );
				RED_MEMORY_ASSERT( m_bottomPhysicalTail == nullptr || !m_bottomPhysicalTail->IsFree(), "Have a free tail, but we just removed a free tail! Didn't coalesce properly?" );
			}
		}
		else if( defragMode == DefragLongLivedRegions )
		{
			GpuAllocatorImpl::RegionNode* currentNode = m_topPhysicalHead;
			while( currentNode != nullptr )
			{
				if ( currentNode->IsFree() && currentNode->IsLocked() )
				{
					currentNode->SetLockStatus( false );
					currentNode = CoalesceFreeNode( m_topPhysicalHead, m_topPhysicalTail, currentNode );
				}
				currentNode = currentNode->GetNext();
			}
		}
	}


	///////////////////////////////////////////////////////////////////////
	// Initialise
	template< typename TSyncLock >
	EAllocatorInitResults GpuAllocator< TSyncLock >::Initialise( const IAllocatorCreationParameters& parameters, Red::System::Uint32 flags )
	{
		const CreationParameters& params = static_cast< const CreationParameters& >( parameters );
		m_creationParameters = params;
		m_flags = flags;

		RED_MEMORY_ASSERT( params.m_gpuBudget != 0, "No gpu budget" );
		RED_MEMORY_ASSERT( params.m_cpuBudget != 0, "No cpu budget" );

		// Allocate our main GPU and CPU blocks
		Red::System::MemSize gpuAllocated = 0;
		m_gpuMemoryBlock = PageAllocator::GetInstance().GetPagedMemory( params.m_gpuBudget, gpuAllocated, params.m_gpuFlags );
		RED_MEMORY_ASSERT( m_gpuMemoryBlock, "Failed to allocate gpu memory" );
		if( !m_gpuMemoryBlock )
		{
			return AllocInit_OutOfMemory;
		}

		Red::System::MemSize cpuAllocated = 0;
		m_cpuMemoryBlock = PageAllocator::GetInstance().GetPagedMemory( params.m_cpuBudget, cpuAllocated, params.m_cpuFlags );
		RED_MEMORY_ASSERT( m_cpuMemoryBlock, "Failed to allocate system memory" );
		if( !m_cpuMemoryBlock )
		{
			return AllocInit_OutOfMemory;
		}

		InitialiseRegionPool();

		return AllocInit_OK;
	}

	///////////////////////////////////////////////////////////////////////
	// InitialiseRegionPool
	//	Just builds our initial free-list of region nodes (NOT assigned addresses)
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::InitialiseRegionPool()
	{
		Red::System::Uint32 regionPoolSize = static_cast< Red::System::Uint32 >( m_creationParameters.m_cpuBudget / sizeof( GpuAllocatorImpl::RegionNode ) );
		GpuAllocatorImpl::RegionNode* theNode = (GpuAllocatorImpl::RegionNode*)m_cpuMemoryBlock;
		m_freeRegionHead = theNode;
		theNode->SetPrevious( nullptr );
		theNode->SetNext( theNode + 1 );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetAlignedAddress( (Red::System::MemUint)-1 );
		theNode->SetAlignedSize( (Red::System::MemSize)-1 );
		++theNode;
		for( Red::System::Uint32 i=1; i<regionPoolSize - 1; ++i )
		{
			theNode->SetPrevious( theNode - 1 );
			theNode->SetNext( theNode + 1 );
			theNode->SetAlignmentAndFreeStatus( 0, true );
			theNode->SetAlignedAddress( (Red::System::MemUint)-1 );
			theNode->SetAlignedSize( (Red::System::MemSize)-1 );
			++theNode;
		}
		theNode->SetPrevious( theNode - 1 );
		theNode->SetNext( nullptr );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetAlignedAddress( (Red::System::MemUint)-1 );
		theNode->SetAlignedSize( (Red::System::MemSize)-1 );
		m_freeRegionTail = theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// Release
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::Release( )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		m_freeRegionHead = nullptr;
		m_freeRegionTail = nullptr;
		m_bottomPhysicalHead = nullptr;
		m_bottomPhysicalTail = nullptr;
		m_topPhysicalHead = nullptr;
		m_topPhysicalTail = nullptr;
		PageAllocator::GetInstance().FreePagedMemory( m_gpuMemoryBlock, m_creationParameters.m_gpuBudget, m_creationParameters.m_gpuFlags );
		PageAllocator::GetInstance().FreePagedMemory( m_cpuMemoryBlock, m_creationParameters.m_cpuBudget, m_creationParameters.m_cpuFlags );
	}

	///////////////////////////////////////////////////////////////////////
	// NodeCanFitRequest
	//	Can the node be used to allocate this size / alignment?
	template< typename TSyncLock >
	Bool GpuAllocator< TSyncLock >::NodeCanFitRequest( GpuAllocatorImpl::RegionNode* node, Red::System::MemSize size, Red::System::MemSize alignment )
	{
		if( node->GetSize() >= size )	// Region is big enough?
		{
			// Is it big enough to handle the aligned buffer?
			Red::System::MemUint baseAddr = node->GetAddress();
			Red::System::MemUint alignedUpAddr = (baseAddr + (alignment-1)) & ~(alignment-1);
			if( node->GetSize() >= ( alignedUpAddr - baseAddr ) + size )
			{
				return true;
			}
		}
		return false;
	}

	///////////////////////////////////////////////////////////////////////
	// FindEmptyNodeInList
	//	Returns the smallest empty node that can satisfy the request (best-fit, O(n))
	template< typename TSyncLock >
	GpuAllocatorImpl::RegionNode* GpuAllocator< TSyncLock >::FindEmptyNodeInList( Red::System::MemSize size, Red::System::MemSize alignment, GpuAllocatorImpl::RegionNode* head, GpuAllocatorImpl::RegionNode* tail )
	{
		Red::System::MemSize smallFittingNodeSize = 0xffffffffffffffff;
		GpuAllocatorImpl::RegionNode* smallestFittingNode = nullptr;
		GpuAllocatorImpl::RegionNode* theNode = head;
		while( theNode )
		{
			if( theNode->IsFree() && !theNode->IsLocked() && NodeCanFitRequest( theNode, size, alignment ) )
			{
				// If the fit is exact, get out now
				if( theNode->GetSize() == size && ( theNode->GetAddress() & ( alignment - 1 ) ) == 0 )
				{
					return theNode;
				}
				else if( theNode->GetSize() < smallFittingNodeSize )
				{
					smallFittingNodeSize = theNode->GetSize();
					smallestFittingNode = theNode;
				}
			}
			theNode = theNode->GetNext();
		}

		return smallestFittingNode;
	}

	///////////////////////////////////////////////////////////////////////
	// GetUnusedAvailable
	//	Calculate how much free space there is between the bottom and top lists
	template< typename TSyncLock >
	Red::System::MemSize GpuAllocator< TSyncLock >::GetUnusedAvailable() const
	{
		Red::System::MemUint baseAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : reinterpret_cast< Red::System::MemUint >( m_gpuMemoryBlock );
		Red::System::MemUint topAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : reinterpret_cast< Red::System::MemUint >( m_gpuMemoryBlock ) + m_creationParameters.m_gpuBudget;
		return topAddr - baseAddr;
	}

	///////////////////////////////////////////////////////////////////////
	// MakeFreeRegion
	//	Get a new node header from the pool
	template< typename TSyncLock >
	GpuAllocatorImpl::RegionNode* GpuAllocator< TSyncLock >::MakeFreeRegion( Red::System::MemUint address, Red::System::MemSize size, Bool initiallyLocked )
	{
		RED_FATAL_ASSERT( m_freeRegionHead, "Free region pool is full! Give this pool more CPU budget" );
		GpuAllocatorImpl::RegionNode* theNode = Utils::PopFront( m_freeRegionHead, m_freeRegionTail );
		theNode->SetAlignedAddress( address );
		theNode->SetAlignedSize( size );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetLockStatus( initiallyLocked );
		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// ReleaseFreeRegion
	//	Release a region header back to the pool
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::ReleaseFreeRegion( GpuAllocatorImpl::RegionNode* node )
	{
		// Put a node back on the free node pool
		node->SetAlignedAddress( (Red::System::MemUint)-1 );
		node->SetAlignedSize( (Red::System::MemSize)-1 );
		node->SetAlignment( (Uint32)0 );
		Utils::PushFront( m_freeRegionHead, m_freeRegionTail, node );
	}

	///////////////////////////////////////////////////////////////////////
	// SplitFreeNode
	//	Takes a free region, splits it at the offset, returns the new node at the end + fixes up the list (the original node is now the 'front' of the split)
	template< typename TSyncLock >
	GpuAllocatorImpl::RegionNode* GpuAllocator< TSyncLock >::SplitNode( GpuAllocatorImpl::RegionNode*& listHead, GpuAllocatorImpl::RegionNode*& listTail, GpuAllocatorImpl::RegionNode* sourceNode, Red::System::MemSize offset )
	{
		GpuAllocatorImpl::RegionNode* newNode = MakeFreeRegion( sourceNode->GetAddress() + offset, sourceNode->GetSize() - offset, sourceNode->IsLocked() );
		Utils::InsertAfter( listHead, listTail, sourceNode, newNode );
		sourceNode->SetAlignedSize( offset );
		GpuAllocatorImpl::ValidateNode( newNode );
		GpuAllocatorImpl::ValidateNode( sourceNode );
		return newNode;
	}

	///////////////////////////////////////////////////////////////////////
	// PrepareAllocatedNode
	//	This takes a node with a region used for an allocation. We chop off the alignment padding, and end space,
	//  and add new free regions for them
	template< typename TSyncLock >
	GpuAllocatorImpl::RegionNode* GpuAllocator< TSyncLock >::PrepareAllocatedNode( GpuAllocatorImpl::RegionNode*& listHead, GpuAllocatorImpl::RegionNode*& listTail, GpuAllocatorImpl::RegionNode* theNode,  Red::System::MemSize size, Red::System::MemSize alignment )
	{
		Red::System::MemUint baseAddress = theNode->GetAddress();
		Red::System::MemUint alignedAddress = (baseAddress + (alignment-1)) & ~(alignment-1);
		if( alignedAddress != baseAddress )
		{
			GpuAllocatorImpl::RegionNode* splitNodeFront = SplitNode( listHead, listTail, theNode, alignedAddress - baseAddress );
			theNode->SetIsFree( true );			// foundNode ptr contains the padding bit, we free it then continue
			theNode = splitNodeFront;
		}

		// The node may have space at the end that we can reclaim (if we recycle a previous region for example)
		if( theNode->GetSize() > size + k_maximumWastedSpacePerRegion )
		{
			GpuAllocatorImpl::RegionNode* splitNodeFront = SplitNode( listHead, listTail, theNode, size );
			splitNodeFront->SetIsFree( true );
		}

#ifdef ENABLE_REGION_LOCK_PROTECTION
		theNode->SetLockStatus( true );		// Allocated nodes are locked by default
#endif

		GpuAllocatorImpl::ValidateNode( theNode );

		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegionBottom
	//	Allocate from low address space
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::AllocateRegionBottom( Red::System::MemSize size, Red::System::MemSize alignment )
	{
		// FindEmptyNodeInList is the bottleneck here; if it gets too slow we can try first-fit (huge performance increase, but more fragmentation potentially)
		GpuAllocatorImpl::RegionNode* foundNode = FindEmptyNodeInList( size, alignment, m_bottomPhysicalHead, m_bottomPhysicalTail );
		if( !foundNode )
		{
			// Allocate a new region big enough to handle the alignment
			Red::System::MemSize availableMemory = GetUnusedAvailable();
			Red::System::MemUint baseAddress = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : reinterpret_cast< Red::System::MemUint >( m_gpuMemoryBlock );
			Red::System::MemUint alignedAddress = (baseAddress + (alignment-1)) & ~(alignment-1);
			Red::System::MemSize regionTotalSize = alignedAddress + size - baseAddress;
			if( availableMemory >= regionTotalSize )
			{
				foundNode = MakeFreeRegion( baseAddress, regionTotalSize, false );
				Utils::PushBack( m_bottomPhysicalHead, m_bottomPhysicalTail, foundNode );
			}
			else
			{
				return nullptr;
			}
		}
		GpuAllocatorImpl::ValidateNode( foundNode );

		// We have a region big enough, chop off the padding and free it back to the allocator
		foundNode = PrepareAllocatedNode( m_bottomPhysicalHead, m_bottomPhysicalTail, foundNode, size, alignment );
		foundNode->SetAlignmentAndFreeStatus( static_cast< Uint32 >( alignment ), false );
		RED_FATAL_ASSERT( ( foundNode->GetAddress() & (alignment-1) ) == 0, "Misaligned region" );

		return static_cast< MemoryRegionHandle >( foundNode );
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegionTop
	//	Allocate from the high address space
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::AllocateRegionTop( Red::System::MemSize size, Red::System::MemSize alignment )
	{
		// FindEmptyNodeInList is the bottleneck here; if it gets too slow we can try first-fit (huge performance increase, but more fragmentation potentially)
		GpuAllocatorImpl::RegionNode* foundNode = FindEmptyNodeInList( size, alignment, m_topPhysicalHead, m_topPhysicalTail );
		if( !foundNode )
		{
			// Allocate a new region big enough to handle the alignment
			Red::System::MemSize availableMemory = GetUnusedAvailable();
			Red::System::MemUint baseAddress = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : reinterpret_cast< Red::System::MemUint >( m_gpuMemoryBlock ) + m_creationParameters.m_gpuBudget;
			Red::System::MemUint lowestAddress = baseAddress - ( size + ( alignment-1 ) );
			Red::System::MemUint alignedAddress = (lowestAddress + (alignment-1)) & ~(alignment-1);
			Red::System::MemSize regionTotalSize = baseAddress - alignedAddress;
			if( availableMemory >= regionTotalSize )
			{
				foundNode = MakeFreeRegion( alignedAddress, regionTotalSize, false );
				Utils::PushFront( m_topPhysicalHead, m_topPhysicalTail, foundNode );
			}
			else
			{
				return nullptr;
			}
		}
		GpuAllocatorImpl::ValidateNode( foundNode );

		// We have a region big enough, chop off the padding and free it back to the allocator
		foundNode = PrepareAllocatedNode( m_topPhysicalHead, m_topPhysicalTail, foundNode, size, alignment );
		foundNode->SetAlignmentAndFreeStatus( static_cast< Uint32 >( alignment ), false );
		RED_FATAL_ASSERT( ( foundNode->GetAddress() & (alignment-1) ) == 0, "Misaligned region" );

		return static_cast< MemoryRegionHandle >( foundNode );
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegion
	//
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::AllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Allocate during defrag" );

		switch( lifetimeHint )
		{
		case Region_Shortlived:
			return AllocateRegionBottom( size, alignment );
			break;
		case Region_Longlived:
			return AllocateRegionTop( size, alignment );
			break;
		}
		return nullptr;
	}

	///////////////////////////////////////////////////////////////////////
	// CoalesceFreeNode
	//	Takes a node, merges it with its neighbours (removing them from the list)
	//	and returns a merged node
	template< typename TSyncLock >
	GpuAllocatorImpl::RegionNode* GpuAllocator< TSyncLock >::CoalesceFreeNode( GpuAllocatorImpl::RegionNode*& head, GpuAllocatorImpl::RegionNode*& tail, GpuAllocatorImpl::RegionNode* theNode )
	{
		const Bool thisIsLocked = theNode->IsLocked();
		// Coalesce previous node
		GpuAllocatorImpl::RegionNode* previousNode = theNode->GetPrevious();
		if( previousNode != nullptr && previousNode->IsFree() && ( previousNode->IsLocked() == thisIsLocked ) )
		{
			previousNode->SetAlignedSize( previousNode->GetSize() + theNode->GetSize() );
			Utils::RemoveFromList( head, tail, theNode );

			// the old node is now invalid, push it back to the free node pool
			ReleaseFreeRegion( theNode );

			// carry on, using the merged node
			theNode = previousNode;
		}
		GpuAllocatorImpl::ValidateNode( theNode );

		// Coalesce next node
		GpuAllocatorImpl::RegionNode* nextNode = theNode->GetNext();
		if( nextNode != nullptr && nextNode->IsFree() && ( nextNode->IsLocked() == thisIsLocked ) )
		{
			theNode->SetAlignedSize( theNode->GetSize() + nextNode->GetSize() );
			Utils::RemoveFromList( head, tail, nextNode );

			// The merged 'next' node is now gone, release it back to the pool
			ReleaseFreeRegion( nextNode );
		}
		GpuAllocatorImpl::ValidateNode( theNode );

		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegionBottom
	//
	template< typename TSyncLock >
	EAllocatorFreeResults GpuAllocator< TSyncLock >::FreeRegionBottom( GpuAllocatorImpl::RegionNode* theNode )
	{
		theNode->SetIsFree( true );
		theNode->SetLockStatus( false );
		CoalesceFreeNode( m_bottomPhysicalHead, m_bottomPhysicalTail, theNode );

		// If the tail of the list is free, we remove the final block to free up the middle
		if( m_bottomPhysicalTail->IsFree() && !m_bottomPhysicalTail->IsLocked() )
		{
			GpuAllocatorImpl::RegionNode* endNode = m_bottomPhysicalTail;
			Utils::RemoveFromList( m_bottomPhysicalHead, m_bottomPhysicalTail, endNode );
			ReleaseFreeRegion( endNode );
		}

		return Free_OK;
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegionTop
	//
	template< typename TSyncLock >
	EAllocatorFreeResults GpuAllocator< TSyncLock >::FreeRegionTop( GpuAllocatorImpl::RegionNode* theNode )
	{
		theNode->SetIsFree( true );
		theNode->SetLockStatus( false );
		CoalesceFreeNode( m_topPhysicalHead, m_topPhysicalTail, theNode );

		// If the tail of the list is free, we remove the final block to free up the middle
		if( m_topPhysicalHead->IsFree() && !m_topPhysicalHead->IsLocked() )
		{
			GpuAllocatorImpl::RegionNode* endNode = m_topPhysicalHead;
			Utils::RemoveFromList( m_topPhysicalHead, m_topPhysicalTail, endNode );
			ReleaseFreeRegion( endNode );
		}

		return Free_OK;
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegion
	//
	template< typename TSyncLock >
	EAllocatorFreeResults GpuAllocator< TSyncLock >::FreeRegion( MemoryRegionHandle handle )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Free during defrag" );

		// First of all, we need to get hold of the node that matches the user region
		GpuAllocatorImpl::RegionNode* theNode = const_cast< GpuAllocatorImpl::RegionNode* >( static_cast< const GpuAllocatorImpl::RegionNode* >( handle.GetRegionInternal() ) );
		RED_MEMORY_ASSERT( !theNode->IsFree(), "This memory region is already free!" );
		if( theNode->IsFree() )
		{
			return Free_AlreadyFree;
		}

#ifdef ENABLE_REGION_LOCK_PROTECTION		
		RED_MEMORY_ASSERT( !theNode->IsLocked(), "Locked regions should not be freed!" );
#endif

		GpuAllocatorImpl::ValidateNode( theNode );

		// Check the region address to figure out if it is allocated in the top or bottom memory
		Red::System::MemSize bottomBaseAddr = m_bottomPhysicalHead != nullptr ? m_bottomPhysicalHead->GetAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize bottomTailAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize topBaseAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize topTailAddr = m_topPhysicalTail != nullptr ? m_topPhysicalTail->GetEndAddress() : (Red::System::MemSize)-1;
		
		if( theNode->GetAddress() >= bottomBaseAddr && theNode->GetEndAddress() <= bottomTailAddr )
		{
			return FreeRegionBottom( theNode );
		}
		else if( theNode->GetAddress() >= topBaseAddr && theNode->GetEndAddress() <= topTailAddr )
		{
			return FreeRegionTop( theNode );
		}
		else
		{
			RED_MEMORY_ASSERT( false, "Region is not owned by this allocator, or bookkeeping is broken" );
			return Free_NotOwned;
		}
	}

	///////////////////////////////////////////////////////////////////////
	// IncreaseMemoryFootprint
	//	Can't do anything in this pool
	template< typename TSyncLock >
	Red::System::Bool GpuAllocator< TSyncLock >::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
	{
		RED_UNUSED( areaCallback );
		RED_UNUSED( sizeRequired );
		return false;
	}

	///////////////////////////////////////////////////////////////////////
	// ReleaseFreeMemoryToSystem
	//	Can't do anything (except *maybe* defrag)
	template< typename TSyncLock >
	Red::System::MemSize GpuAllocator< TSyncLock >::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
	{
		RED_UNUSED( areaCallback );
		return 0;
	}

	///////////////////////////////////////////////////////////////////////
	// RequestAllocatorInfo
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::RequestAllocatorInfo( AllocatorInfo& info )
	{
		info.SetAllocatorTypeName( TXT( "GpuAllocator" ) );
		info.SetAllocatorBudget( m_creationParameters.m_gpuBudget );
		info.SetPerAllocationOverhead( sizeof( GpuAllocatorImpl::RegionNode ) );
	}

	///////////////////////////////////////////////////////////////////////
	// WalkAllocator
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::WalkAllocator( AllocatorWalker* theWalker )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Walking during defrag" );
		
		// We only expose the 'gpu area' to the walker, since the cpu area is just bookkeeping data
		theWalker->OnMemoryArea( reinterpret_cast< Red::System::MemUint >( m_gpuMemoryBlock ), m_creationParameters.m_gpuBudget );
	}

	///////////////////////////////////////////////////////////////////////
	// WalkPoolArea
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker )
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );

		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Walking during defrag" );

		// We basically ignore the start / size as we cannot trust them, but we already know that we only have 1 large memory space to deal with
		RED_UNUSED( startAddress );
		RED_UNUSED( size );
		
		GpuAllocatorImpl::RegionNode* theNode = m_bottomPhysicalHead;
		while( theNode )
		{
			if( theNode->IsFree() )
			{
				theWalker->OnFreeArea( theNode->GetAddress(), theNode->GetSize() );
			}
			else
			{
				theWalker->OnUsedArea( theNode->GetAddress(), theNode->GetSize(), theNode->GetMemoryClass() );
			}
			if( theNode->IsLocked() )
			{
				theWalker->OnLockedArea( theNode->GetAddress(), theNode->GetSize() );
			}
			theNode = theNode->GetNext();
		}

		// Treat the middle as one huge unused block
		Red::System::MemSize unusedStart = m_bottomPhysicalHead ? m_bottomPhysicalHead->GetEndAddress() : reinterpret_cast< Red::System::MemSize >( m_gpuMemoryBlock );
		Red::System::MemSize unusedEnd = m_topPhysicalHead ? m_topPhysicalHead->GetAddress() : ( reinterpret_cast< Red::System::MemSize >( m_gpuMemoryBlock ) + m_creationParameters.m_gpuBudget );
		theWalker->OnFreeArea( unusedStart, unusedEnd - unusedStart );

		theNode = m_topPhysicalHead;
		while( theNode )
		{
			if( theNode->IsFree() )
			{
				theWalker->OnFreeArea( theNode->GetAddress(), theNode->GetSize() );
			}
			else
			{
				theWalker->OnUsedArea( theNode->GetAddress(), theNode->GetSize(), 0 );
			}
			if( theNode->IsLocked() )
			{
				theWalker->OnLockedArea( theNode->GetAddress(), theNode->GetSize() );
			}
			theNode = theNode->GetNext();
		}
	}

	///////////////////////////////////////////////////////////////////////
	// DumpDebugOutput
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::DumpDebugOutput()
	{
		typename TSyncLock::TScopedLock lock( &m_syncLock );
		RED_MEMORY_LOG( TXT( "--------------------------------------------------------------------" ) );
		RED_MEMORY_LOG( TXT( "GpuAllocator Debug Info" ) );
		RED_MEMORY_LOG( TXT( "\tGPU Budget: %lld, CPU Budget: %lld (%d nodes max)" ), m_creationParameters.m_gpuBudget
																					, m_creationParameters.m_cpuBudget
																					, m_creationParameters.m_cpuBudget / sizeof( GpuAllocatorImpl::RegionNode ) );

		RED_MEMORY_LOG( TXT( "CPU Address Space: %p - %p. GPU Address Space: %p - %p" ), m_cpuMemoryBlock, (Red::System::MemUint)m_cpuMemoryBlock + m_creationParameters.m_cpuBudget
																					   , m_gpuMemoryBlock, (Red::System::MemUint)m_gpuMemoryBlock + m_creationParameters.m_gpuBudget );

		// Count # free header blocks
		Red::System::Uint32 freeHeaderNodeCount = 0;
		GpuAllocatorImpl::RegionNode* theNode = m_freeRegionHead;
		while( theNode )
		{
			++freeHeaderNodeCount;
			theNode = theNode->GetNext();
		}
		RED_MEMORY_LOG( TXT( "\t%d Node headers free" ), freeHeaderNodeCount );
		RED_MEMORY_LOG( TXT( "\t%lld bytes unused" ), GetUnusedAvailable() );

		if( m_bottomPhysicalHead != nullptr && m_bottomPhysicalTail != nullptr )
		{
#ifdef RED_LOGGING_ENABLED
			Red::System::MemSize addressSpaceSize = m_bottomPhysicalTail->GetEndAddress() - m_bottomPhysicalHead->GetAddress();
			RED_MEMORY_LOG( TXT( "Bottom address space size (shortlived): %d" ), (Uint32)addressSpaceSize );
#endif

			theNode = m_bottomPhysicalHead;
			while( theNode )
			{
				if( theNode->IsFree() )
				{
					RED_MEMORY_LOG( TXT( "Free block %p, size=%d" ), theNode->GetAddress(), theNode->GetSize() );
				}
#ifdef ENABLE_MEMORY_REGION_DEBUG_STRINGS
				else
				{
					RED_MEMORY_LOG( TXT( "Used block: %p, size=%d, locked=%hs, dbgString=%hs" ), theNode->GetAddress(), theNode->GetSize(), theNode->IsLocked() ? "yarp" : "narp", theNode->GetDebugString() );
				}
#endif
				theNode = theNode->GetNext();
			}
		}

		if( m_topPhysicalHead != nullptr && m_topPhysicalTail != nullptr )
		{
#ifdef RED_LOGGING_ENABLED
			Red::System::MemSize addressSpaceSize = m_topPhysicalTail->GetEndAddress() - m_topPhysicalHead->GetAddress();
			RED_MEMORY_LOG( TXT( "Top address space size: %d (longlived)" ), (Uint32)addressSpaceSize );
#endif

			theNode = m_topPhysicalHead;
			while( theNode )
			{
				if( theNode->IsFree() )
				{
					RED_MEMORY_LOG( TXT( "Free block %p, size=%d" ), theNode->GetAddress(), theNode->GetSize() );
				}
				theNode = theNode->GetNext();
			}
		}
	}

	/////////////////////////////////////////////////////////////////
	// UnlockRegion
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::UnlockRegion( MemoryRegionHandle handle )
	{
#ifdef ENABLE_REGION_LOCK_PROTECTION	
		if( handle.IsValid() )
		{
			// Do we even own this?
			RED_MEMORY_ASSERT( handle.GetAddress() >= reinterpret_cast< MemUint >( m_gpuMemoryBlock ) && 
				( handle.GetAddress() + handle.GetSize() ) <= ( reinterpret_cast< MemUint >( m_gpuMemoryBlock ) + m_creationParameters.m_gpuBudget ),
				"Handle does not belong to this pool!" );

			typename TSyncLock::TScopedLock lock( &m_syncLock );
			GpuAllocatorImpl::RegionNode* theNode = const_cast< GpuAllocatorImpl::RegionNode* >( static_cast< const GpuAllocatorImpl::RegionNode* >( handle.GetRegionInternal() ) );
			RED_MEMORY_ASSERT( theNode->IsLocked(), "Attempting to unlock a node multiple times?!" );
			theNode->SetLockStatus( false );
		}
#endif
	}

	/////////////////////////////////////////////////////////////////
	// LockRegion
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::LockRegion( MemoryRegionHandle handle )
	{
#ifdef ENABLE_REGION_LOCK_PROTECTION	
		if( handle.IsValid() )
		{
			// Do we even own this?
			RED_MEMORY_ASSERT( handle.GetAddress() >= reinterpret_cast< MemUint >( m_gpuMemoryBlock ) && 
				( handle.GetAddress() + handle.GetSize() ) <= ( reinterpret_cast< MemUint >( m_gpuMemoryBlock ) + m_creationParameters.m_gpuBudget ),
				"Handle does not belong to this pool!" );

			typename TSyncLock::TScopedLock lock( &m_syncLock );
			GpuAllocatorImpl::RegionNode* theNode = const_cast< GpuAllocatorImpl::RegionNode* >( static_cast< const GpuAllocatorImpl::RegionNode* >( handle.GetRegionInternal() ) );
			RED_MEMORY_ASSERT( !theNode->IsLocked(), "Attempting to lock a node multiple times?!" );
			theNode->SetLockStatus( true );
		}
#endif
	}

	/////////////////////////////////////////////////////////////////
	// SplitRegion
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::SplitRegion( MemoryRegionHandle baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitBlockAlignment )
	{
		// invalid block
		if ( !baseRegion.IsValid() || (splitPosition >= baseRegion.GetSize()) || (0==splitPosition) )
			return MemoryRegionHandle();

		// we can split only allocated regions
		GpuAllocatorImpl::RegionNode* theNode = const_cast< GpuAllocatorImpl::RegionNode* >( static_cast< const GpuAllocatorImpl::RegionNode* >( baseRegion.GetRegionInternal() ) );
		if ( theNode->IsFree() )
			return MemoryRegionHandle();

#ifdef ENABLE_REGION_LOCK_PROTECTION	
		RED_FATAL_ASSERT( theNode->IsLocked(), "Only locked regions may be split" );
#endif

		// the split block alignment must be correct upon calling thing (its too dangerous to do manually)
		RED_FATAL_ASSERT( ( ( theNode->GetAddress() + splitPosition ) & ( splitBlockAlignment - 1 ) ) == 0, "Split offset would result in misaligned region" );

		typename TSyncLock::TScopedLock lock( &m_syncLock );
		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Split during defrag" );

		// Check the region address to figure out if it is allocated in the top or bottom memory
		Red::System::MemSize bottomBaseAddr = m_bottomPhysicalHead != nullptr ? m_bottomPhysicalHead->GetAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize bottomTailAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize topBaseAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : (Red::System::MemSize)-1;
		Red::System::MemSize topTailAddr = m_topPhysicalTail != nullptr ? m_topPhysicalTail->GetEndAddress() : (Red::System::MemSize)-1;

		GpuAllocatorImpl::RegionNode* splitRegion;
		if( theNode->GetAddress() >= bottomBaseAddr && theNode->GetEndAddress() <= bottomTailAddr )
		{
			splitRegion = SplitNode( m_bottomPhysicalHead, m_bottomPhysicalTail, theNode, splitPosition );
		}
		else if( theNode->GetAddress() >= topBaseAddr && theNode->GetEndAddress() <= topTailAddr )
		{
			splitRegion = SplitNode( m_topPhysicalHead, m_topPhysicalTail, theNode, splitPosition );
		}
		else
		{
			return MemoryRegionHandle();
		}

		// the split block is NOT FREE
		RED_FATAL_ASSERT( splitRegion != nullptr, "Splitting the memory block failed" );
		splitRegion->SetAlignmentAndFreeStatus( static_cast< Uint32 >( splitBlockAlignment ), false );

#ifdef ENABLE_REGION_LOCK_PROTECTION		
		splitRegion->SetLockStatus( true );		// Split regions are always locked
#endif
		GpuAllocatorImpl::ValidateNode( splitRegion );

		// Create memory region wrapper for the split block
		return static_cast< MemoryRegionHandle >( splitRegion );
	}

	/////////////////////////////////////////////////////////////////
	// OnOutOfMemory
	//
	template< typename TSyncLock >
	void GpuAllocator< TSyncLock >::OnOutOfMemory()
	{
		DumpDebugOutput();
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeAllocateRegion
	//	Runtime interface
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::RuntimeAllocateRegion( Red::System::MemSize size, Red::System::MemSize alignment, RegionLifetimeHint lifetimeHint )
	{
		return AllocateRegion( size, alignment, lifetimeHint );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeFreeRegion
	//	Runtime interface
	template< typename TSyncLock >
	EAllocatorFreeResults GpuAllocator< TSyncLock >::RuntimeFreeRegion( MemoryRegionHandle handle )
	{
		return FreeRegion( handle );
	}

	/////////////////////////////////////////////////////////////////
	// RuntimeFreeRegion
	//	Runtime interface
	template< typename TSyncLock >
	MemoryRegionHandle GpuAllocator< TSyncLock >::RuntimeSplitRegion( MemoryRegionHandle baseRegion, Red::System::MemSize splitPosition, Red::System::MemSize splitBlockAlignment )
	{
		return SplitRegion( baseRegion, splitPosition, splitBlockAlignment );
	}

} }
