/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "legacyGpuAllocator.h"
#include "assert.h"
#include "systemAllocator.h"
#include "flags.h"

#define ENABLE_VALIDATE_ON_DEFRAG

#ifdef RED_PLATFORM_CONSOLE
	#define ENABLE_REGION_LOCK_PROTECTION
#endif

namespace red
{
namespace memory
{
namespace Utils
{

	template< class NodeType >
	class ListNode
	{
	public:
		ListNode();
		~ListNode();

		NodeType* GetNext() const;
		NodeType* GetPrevious() const;
		void SetNext( NodeType* next );
		void SetPrevious( NodeType* previous );

	private:
		NodeType* m_next;
		NodeType* m_previous;
	};

	template< class NodeType >
	RED_INLINE ListNode< NodeType >::ListNode()
		: m_next( nullptr )
		, m_previous( nullptr )
	{
	}

	template< class NodeType >
	ListNode< NodeType >::~ListNode()
	{
	}

	template< class NodeType >
	RED_INLINE NodeType* ListNode< NodeType >::GetNext() const
	{
		return m_next;
	}

	template< class NodeType >
	RED_INLINE NodeType* ListNode< NodeType >::GetPrevious() const
	{
		return m_previous;
	}

	template< class NodeType >
	RED_INLINE void ListNode< NodeType >::SetNext( NodeType* next )
	{
		m_next = next;
	}

	template< class NodeType >
	RED_INLINE void ListNode< NodeType >::SetPrevious( NodeType* previous )
	{
		m_previous = previous;
	}

	///////////////////////////////////////////////////////////////
	// PushFront
	//	Add node to front of list
	template< class NodeType >
	RED_INLINE void PushFront( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert )
	{
		toInsert->SetNext( headPtr );
		toInsert->SetPrevious( nullptr );
		if( headPtr )
		{
			headPtr->SetPrevious( toInsert );
		}
		else
		{
			tailPtr = toInsert;
		}
		headPtr = toInsert;
	}

	///////////////////////////////////////////////////////////////
	// PushBack
	//	Add node to back of list
	template< class NodeType >
	RED_INLINE void PushBack( NodeType*& headPtr, NodeType*& tailPtr, NodeType* toInsert )
	{
		toInsert->SetNext( nullptr );
		toInsert->SetPrevious( tailPtr );
		if( tailPtr )
		{
			tailPtr->SetNext( toInsert );
		}
		else
		{
			headPtr = toInsert;
		}
		tailPtr = toInsert;
	}

	///////////////////////////////////////////////////////////////
	// PopFront
	//	Remove node from head
	template< class NodeType >
	RED_INLINE NodeType* PopFront( NodeType*& headPtr, NodeType*& tailPtr )
	{
		NodeType* popped = headPtr;
		if( headPtr != nullptr )
		{
			headPtr = headPtr->GetNext();
			if( headPtr != nullptr )
			{
				headPtr->SetPrevious( nullptr );
			}
			else
			{
				tailPtr = nullptr;
			}

			popped->SetNext( nullptr );
			popped->SetPrevious( nullptr );
		}

		return popped;
	}

	///////////////////////////////////////////////////////////////
	// RemoveFromList
	//	Remove node from any point in a list. 
	template< class NodeType >
	RED_INLINE void RemoveFromList( NodeType*& headPtr, NodeType*& tailPtr, NodeType* nodeInList )
	{
		if( nodeInList->GetPrevious() != nullptr )
		{
			nodeInList->GetPrevious()->SetNext( nodeInList->GetNext() );
		}
		else
		{
			RED_MEMORY_ASSERT( headPtr == nodeInList, "Bad head ptr - list is corrupt" );
			headPtr = nodeInList->GetNext();
		}

		if( nodeInList->GetNext() != nullptr )
		{
			nodeInList->GetNext()->SetPrevious( nodeInList->GetPrevious() );
		}
		else
		{
			RED_MEMORY_ASSERT( tailPtr == nodeInList, "Bad tail ptr - list is corrupt" );
			tailPtr = nodeInList->GetPrevious();
		}

		nodeInList->SetNext( nullptr );
		nodeInList->SetPrevious( nullptr );
	}

	///////////////////////////////////////////////////////////////
	// InsertBefore
	//	Insert node immediately before the 'anchor' node
	template< class NodeType >
	RED_INLINE void InsertBefore( NodeType*& headPtr, NodeType*& /*tailPtr*/, NodeType* anchor, NodeType* toInsert )
	{
		toInsert->SetPrevious( anchor->GetPrevious() );
		toInsert->SetNext( anchor );
		if( anchor->GetPrevious() != nullptr )
		{
			anchor->GetPrevious()->SetNext( toInsert );
		}
		else
		{
			RED_MEMORY_ASSERT( headPtr == anchor, "Bad head ptr" );
			headPtr = toInsert;
		}
		anchor->SetPrevious( toInsert );
	}

	///////////////////////////////////////////////////////////////
	// InsertAfter
	//	Insert node immediately after the 'anchor' node
	template< class NodeType >
	RED_INLINE void InsertAfter( NodeType*& /*headPtr*/, NodeType*& tailPtr, NodeType* anchor, NodeType* toInsert )
	{
		toInsert->SetPrevious( anchor );
		toInsert->SetNext( anchor->GetNext() );
		if( anchor->GetNext() != nullptr )
		{
			anchor->GetNext()->SetPrevious( toInsert );
		}
		else
		{
			RED_MEMORY_ASSERT( tailPtr == anchor, "Bad tail ptr" );
			tailPtr = toInsert;
		}
		anchor->SetNext( toInsert );
	}

}
	// How much free space are we willing to leave at the end of an allocation (helps to reduce fragmentation)
	const u32 k_maximumWastedSpacePerRegion = 1024 * 4;		

	
	///////////////////////////////////////////////////////////////////////
	// Node is used to represent a non-moveable region made public to the user
	// Locked node = node cannot be moved during defrag, or freed
	class LegacyGpuAllocator::RegionNode : public Utils::ListNode< RegionNode >, public Region
	{
	public:
		RED_INLINE bool IsFree() const										{ return ( m_packedAlignmentAndStatus & c_packedFree ) == 0 ? false : true; }
		RED_INLINE bool IsLocked() const									{ return ( m_packedAlignmentAndStatus & c_packedLock ) == 0 ? false : true; }
		RED_INLINE u32 GetAlignment() const					{ return m_packedAlignmentAndStatus & ~c_packMask; }
		RED_INLINE u64 GetEndAddress() const				{ return m_alignedAddress + m_alignedSize; }
		RED_INLINE void SetAlignedAddress( u64 address)	{ m_alignedAddress = address; }
		RED_INLINE void SetAlignedSize( u64 size )			{ m_alignedSize = static_cast< u32 >( size ); }
		RED_INLINE void SetIsFree( bool isFree )	
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
		RED_INLINE void SetLockStatus( bool isLocked )
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

		RED_INLINE void SetAlignment( u32 alignment )		
		{ 
			m_packedAlignmentAndStatus = ( m_packedAlignmentAndStatus & c_packMask ) | ( alignment & ~c_packMask );
		}

		RED_INLINE void SetAlignmentAndFreeStatus( u32 alignment, bool isFree )
		{
			u32 maskedLock = m_packedAlignmentAndStatus & c_packedLock;
			u32 maskedFree = isFree ? c_packedFree : 0;
			m_packedAlignmentAndStatus = maskedFree | maskedLock | ( alignment & ~c_packMask );
		}
			
		RED_INLINE void ValidateNode() const
		{
			if( GetPrevious() )
			{
				RED_MEMORY_ASSERT( GetPrevious()->GetEndAddress() == GetAddress(), "Nodes are not physically contiguous!" );
				RED_MEMORY_ASSERT( GetPrevious()->GetNext() == this, "Linked list is broken" );
			}
			if( GetNext() )
			{
				RED_MEMORY_ASSERT( GetNext()->GetAddress() == GetEndAddress(), "Nodes are not physically contiguous!" );
				RED_MEMORY_ASSERT( GetNext()->GetPrevious() == this, "Linked list is broken" );
			}
			if( !IsFree() )
			{
				RED_MEMORY_ASSERT( GetAlignment() != 0, "Alignment not set on used node" );
				RED_MEMORY_ASSERT( ( GetAddress() & ( GetAlignment() - 1 ) ) == 0, "Misaligned used region" );
			}
		}

	private:
		static const u32 c_packedFree = ( 1UL << 31 );
		static const u32 c_packedLock = ( 1 << 30 );
		static const u32 c_packMask = c_packedFree | c_packedLock;
		union
		{
			struct  
			{
				bool isFree : 1;
				bool isLocked : 1;
				u32 alignment : 30;
			} helper;
			u32 m_packedAlignmentAndStatus;
		};
	};

	///////////////////////////////////////////////////////////////////////
	// CTor
	LegacyGpuAllocator::LegacyGpuAllocator()
		: m_gpuMemoryBlock( nullptr )
		, m_cpuMemoryBlock( nullptr )
		, m_freeRegionHead( nullptr )
		, m_freeRegionTail( nullptr )
		, m_bottomPhysicalHead( nullptr )
		, m_bottomPhysicalTail( nullptr )
		, m_topPhysicalHead( nullptr )
		, m_topPhysicalTail( nullptr )
		, m_fragmentationScope( 0 )
		, m_systemAllocator( nullptr )
	{
	}

	///////////////////////////////////////////////////////////////////////
	// DTor
	LegacyGpuAllocator::~LegacyGpuAllocator()
	{
	}

	///////////////////////////////////////////////////////////////////////
	// DefragmentTopRegions
	//	Defrag a set of regions. Region addresses are fixed up, but no data is moved (this is the job of the userCallback)
	//	Returns number of bytes moved
	u32 LegacyGpuAllocator::DefragmentTopRegions( u32 maxMemoryToMove, DefragMoveRequestCallback userCallback, void* userData, u32 smallestGapToFill )
	{
		RegionNode* currentNode = m_topPhysicalTail;
		u64 bytesToMoveAccumulated = 0;
		u64 bytesMoved = 0;
		while( currentNode != nullptr )
		{
			if( bytesMoved >= maxMemoryToMove )
			{
				if( bytesToMoveAccumulated > 0 )	// There is a hole to be filled with a new free block
				{
					RegionNode* newFreeNode = MakeFreeRegion( currentNode->GetEndAddress(), bytesToMoveAccumulated );
					Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
					newFreeNode->ValidateNode();
					CoalesceFreeNode( m_topPhysicalHead, m_topPhysicalTail, newFreeNode );	// Merge with other free nodes
				}
				return static_cast< u32 >( bytesMoved );
			}
			else if( currentNode->IsFree() && currentNode->GetSize() >= smallestGapToFill )
			{
				RED_MEMORY_ASSERT( !currentNode->IsLocked(), "Locked nodes cannot be free!" );

				// We remove all free nodes, and accumulate the memory footprint we regained
				bytesToMoveAccumulated += currentNode->GetSize();
				RegionNode* nextNode = currentNode->GetPrevious();
				Utils::RemoveFromList( m_topPhysicalHead, m_topPhysicalTail, currentNode );
				ReleaseFreeRegion( currentNode );
				currentNode = nextNode;
				continue;
			}
			else if( bytesToMoveAccumulated > 0 )
			{
				if( currentNode->IsLocked() )
				{
					RED_MEMORY_ASSERT( !currentNode->IsFree(), "Locked nodes cannot be free!" );

					// Insert a free block rather than moving anything
					u64 newBaseAddress = currentNode->GetEndAddress();
					RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, bytesToMoveAccumulated );
					Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
					newFreeNode->ValidateNode();
					bytesToMoveAccumulated = 0;
				}
				else
				{
					// New base address may not fit exactly where last free block was
					u64 requiredAlignment = currentNode->GetAlignment();
					u64 newBaseAddress = currentNode->GetAddress() + bytesToMoveAccumulated;
					u64 alignedAddress = requiredAlignment == 0 ? newBaseAddress : ( newBaseAddress & ~( requiredAlignment - 1 ) );		// Align down (i.e. may need end-padding)
					if( newBaseAddress != alignedAddress )
					{
						// We need to insert a free block to pick up the slack from the alignment
						u64 freeBlockSize = newBaseAddress - alignedAddress;
						RegionNode* newFreeNode = MakeFreeRegion( alignedAddress + currentNode->GetSize(), freeBlockSize );
						Utils::InsertAfter( m_topPhysicalHead, m_topPhysicalTail, currentNode, newFreeNode );
						bytesToMoveAccumulated -= freeBlockSize;	// Remove the 'slack' from the free block
					}
					if( alignedAddress != currentNode->GetAddress() )
					{
						userCallback( reinterpret_cast< void* >( currentNode->GetAddress() ), reinterpret_cast< void* >( alignedAddress ), currentNode->GetSize(), userData );
						currentNode->SetAlignedAddress( alignedAddress );
						bytesMoved += currentNode->GetSize();
					}
				}
			}

			currentNode = currentNode->GetPrevious();
		}

		return static_cast< u32 >( bytesMoved );
	}

	///////////////////////////////////////////////////////////////////////
	// DefragmentBottomRegions
	//	Defrag a set of regions. Regions are fixed up, but no data is moved (this is the job of the userCallback)
	//	Returns amount of memory moved
	u32 LegacyGpuAllocator::DefragmentBottomRegions( u32 maxMemoryToMove, DefragMoveRequestCallback userCallback, void* userData, u32 smallestGapToFill )
	{
		RegionNode* currentNode = m_bottomPhysicalHead;
		u64 bytesToMoveAccumulated = 0;
		u64 regionMemoryMoved = 0;
		while( currentNode != nullptr )
		{
			if( regionMemoryMoved >= maxMemoryToMove )
			{
				if( bytesToMoveAccumulated > 0 )	// There is a hole to be filled with a new free block
				{
					RegionNode* newFreeNode = MakeFreeRegion( currentNode->GetAddress() - bytesToMoveAccumulated, bytesToMoveAccumulated );
					Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
					newFreeNode->ValidateNode();
					CoalesceFreeNode( m_bottomPhysicalHead, m_bottomPhysicalTail, newFreeNode );	// Merge with other free nodes
				}
				return static_cast< u32 >( regionMemoryMoved );
			}
			else if( currentNode->IsFree() && currentNode->GetSize() >= smallestGapToFill )					
			{
				RED_MEMORY_ASSERT( !currentNode->IsLocked(), "Locked nodes cannot be free!" );

				// We remove all free nodes, and accumulate the memory footprint we regained
				bytesToMoveAccumulated += currentNode->GetSize();
				RegionNode* nextNode = currentNode->GetNext();
				Utils::RemoveFromList( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode );
				ReleaseFreeRegion( currentNode );
				currentNode = nextNode;
				continue;
			}
			else if( bytesToMoveAccumulated > 0 )
			{
				if( currentNode->IsLocked() )
				{
					RED_MEMORY_ASSERT( !currentNode->IsFree(), "Locked nodes cannot be free!" );
					// We can't move this node; instead we add a free block which fills the accumulated movement, then we continue
					u64 newBaseAddress = currentNode->GetAddress() - bytesToMoveAccumulated;
					RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, bytesToMoveAccumulated );
					Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
					newFreeNode->ValidateNode();
					bytesToMoveAccumulated = 0;
				}
				else
				{
					// New base address may not fit exactly where last free block was, 
					u64 requiredAlignment = currentNode->GetAlignment();
					u64 newBaseAddress = currentNode->GetAddress() - bytesToMoveAccumulated;
					u64 alignedAddress = requiredAlignment == 0 ? newBaseAddress : ( ( newBaseAddress + ( requiredAlignment - 1 ) ) & ~( requiredAlignment - 1 ) );

					// Insert free block to pick up the slack if there is an alignment hole
					if( alignedAddress != newBaseAddress )
					{
						// We need to insert a free block to pick up the slack from the alignment
						u64 freeBlockSize = alignedAddress - newBaseAddress;
						RegionNode* newFreeNode = MakeFreeRegion( newBaseAddress, freeBlockSize );
						Utils::InsertBefore( m_bottomPhysicalHead, m_bottomPhysicalTail, currentNode, newFreeNode );
						bytesToMoveAccumulated -= freeBlockSize;	// Remove the 'slack' from the free block
					}

					// Finally, the block can be updated + data move requested
					if( alignedAddress != currentNode->GetAddress() )
					{
						userCallback( reinterpret_cast< void* >( currentNode->GetAddress() ), reinterpret_cast< void* >( alignedAddress ), currentNode->GetSize(), userData );
						currentNode->SetAlignedAddress( alignedAddress );
						regionMemoryMoved += static_cast< u32 >( currentNode->GetSize() );
					}
				}
			}
			currentNode = currentNode->GetNext();
		}

		return static_cast< u32 >( regionMemoryMoved );
	}

	///////////////////////////////////////////////////////////////////////
	// RequestDefragmentation
	//	Internal regions are moved and patched, but the data is left to the user to deal with
	//	Once this completes, the user MUST do the data moving, and then call FinaliseDefragmentation
	Red::System::MemUint LegacyGpuAllocator::RequestDefragmentation( DefragMode defragMode, const DefragSettings& settings )
	{
		m_syncLock.Acquire();		// Lock so nobody else can try to use this pool during defrag
		Int32 defragScope = m_fragmentationScope++;

		RED_FATAL_ASSERT( defragScope == 0, "Recursive defrag, or attempting to run multiple defrags at once" );
		if( defragScope == 0 )
		{
			u32 bytesMoved = 0;
			if( defragMode == DefragShortLivedRegions )
			{
				bytesMoved = DefragmentBottomRegions( settings.maxMemoryToMove, settings.moveRequestCb, settings.userData, 4096 );
			}
			else if( defragMode == DefragLongLivedRegions )
			{
				bytesMoved = DefragmentTopRegions( settings.maxMemoryToMove, settings.moveRequestCb, settings.userData, 4096 );
			}

#ifdef ENABLE_VALIDATE_ON_DEFRAG
			// Validate list is still valid (contiguous, no gaps)
			RegionNode* currentNode = m_bottomPhysicalHead;
			while( currentNode != nullptr )
			{
				currentNode->ValidateNode();
				currentNode = currentNode->GetNext();
			}

			// Validate list is still valid (contiguous, no gaps)
			currentNode = m_topPhysicalHead;
			while( currentNode != nullptr )
			{
				currentNode->ValidateNode();
				currentNode = currentNode->GetNext();
			}
#endif
		}

		return 0;
	}

	///////////////////////////////////////////////////////////////////////
	// FinaliseDefragmentation
	//	The user must call this once all move requests have been processed. The allocator will then be available for use again
	void LegacyGpuAllocator::FinaliseDefragmentation()
	{
		// ... 

		--m_fragmentationScope;

		m_syncLock.Release();		// Allocator is ready for use again
	}

	///////////////////////////////////////////////////////////////////////
	// Initialise
	void LegacyGpuAllocator::Initialize( const LegacyGpuAllocatorParameter & param )
	{
		RED_MEMORY_ASSERT( param.systemAllocator, "Cannot initialize Allocator without a SystemAllocator." );
		RED_MEMORY_ASSERT( param.gpuSize != 0, "No gpu budget" );
		RED_MEMORY_ASSERT( param.cpuSize != 0, "No cpu budget" );

		m_systemAllocator = param.systemAllocator;

		m_gpuRange = m_systemAllocator->ReserveVirtualRange( param.gpuSize, param.gpuFlags );
		m_cpuRange = m_systemAllocator->ReserveVirtualRange( param.cpuSize, Flags_CPU_Read_Write );

		SystemBlock gpuBlock = { m_gpuRange.start, GetVirtualRangeSize( m_gpuRange ) };
		SystemBlock cpuBlock = { m_cpuRange.start, GetVirtualRangeSize( m_cpuRange ) };

		gpuBlock = m_systemAllocator->Commit( gpuBlock, param.gpuFlags );
		cpuBlock = m_systemAllocator->Commit( cpuBlock, Flags_CPU_Read_Write );
		
		m_gpuMemoryBlock = reinterpret_cast< void* >( gpuBlock.address );
		m_cpuMemoryBlock = reinterpret_cast< void* >( cpuBlock.address );

		RED_MEMORY_ASSERT( m_gpuMemoryBlock, "Failed to Commit gpu memory." );
		RED_MEMORY_ASSERT( m_cpuMemoryBlock, "Failed to Commit cpu memory." );

		InitialiseRegionPool();
	}

	///////////////////////////////////////////////////////////////////////
	// InitialiseRegionPool
	//	Just builds our initial free-list of region nodes (NOT assigned addresses)
	void LegacyGpuAllocator::InitialiseRegionPool()
	{
		u32 regionPoolSize = static_cast< u32 >( GetVirtualRangeSize( m_cpuRange ) / sizeof( RegionNode ) );
		RegionNode* theNode = (RegionNode*)m_cpuMemoryBlock;
		m_freeRegionHead = theNode;
		theNode->SetPrevious( nullptr );
		theNode->SetNext( theNode + 1 );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetAlignedAddress( (u64)-1 );
		theNode->SetAlignedSize( (u64)-1 );
		++theNode;
		for( u32 i=1; i<regionPoolSize - 1; ++i )
		{
			theNode->SetPrevious( theNode - 1 );
			theNode->SetNext( theNode + 1 );
			theNode->SetAlignmentAndFreeStatus( 0, true );
			theNode->SetAlignedAddress( (u64)-1 );
			theNode->SetAlignedSize( (u64)-1 );
			++theNode;
		}
		theNode->SetPrevious( theNode - 1 );
		theNode->SetNext( nullptr );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetAlignedAddress( (u64)-1 );
		theNode->SetAlignedSize( (u64)-1 );
		m_freeRegionTail = theNode;
	}

	void LegacyGpuAllocator::Uninitialize( )
	{
		CScopedLock< CMutex > lock( m_syncLock );
		m_freeRegionHead = nullptr;
		m_freeRegionTail = nullptr;
		m_bottomPhysicalHead = nullptr;
		m_bottomPhysicalTail = nullptr;
		m_topPhysicalHead = nullptr;
		m_topPhysicalTail = nullptr;

		if ( m_systemAllocator )
		{
			m_systemAllocator->ReleaseVirtualRange( m_gpuRange );
			m_systemAllocator->ReleaseVirtualRange( m_cpuRange );
		}
	}

	///////////////////////////////////////////////////////////////////////
	// NodeCanFitRequest
	//	Can the node be used to allocate this size / alignment?
	bool LegacyGpuAllocator::NodeCanFitRequest( RegionNode* node, u64 size, u64 alignment )
	{
		if( node->GetSize() >= size )	// Region is big enough?
		{
			// Is it big enough to handle the aligned buffer?
			u64 baseAddr = node->GetAddress();
			u64 alignedUpAddr = (baseAddr + (alignment-1)) & ~(alignment-1);
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
	LegacyGpuAllocator::RegionNode* LegacyGpuAllocator::FindEmptyNodeInList( u64 size, u64 alignment, RegionNode* head, RegionNode* /*tail*/ )
	{
		u64 smallFittingNodeSize = 0xffffffffffffffff;
		RegionNode* smallestFittingNode = nullptr;
		RegionNode* theNode = head;
		while( theNode )
		{
			if( theNode->IsFree() && NodeCanFitRequest( theNode, size, alignment ) )
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
	u64 LegacyGpuAllocator::GetUnusedAvailable() const
	{
		u64 baseAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : reinterpret_cast< u64 >( m_gpuMemoryBlock );
		u64 topAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : reinterpret_cast< u64 >( m_gpuMemoryBlock ) + GetVirtualRangeSize( m_gpuRange );
		return topAddr - baseAddr;
	}

	///////////////////////////////////////////////////////////////////////
	// MakeFreeRegion
	//	Get a new node header from the pool
	LegacyGpuAllocator::RegionNode* LegacyGpuAllocator::MakeFreeRegion( u64 address, u64 size )
	{
		RED_FATAL_ASSERT( m_freeRegionHead, "Free region pool is full! Give this pool more CPU budget" );
		RegionNode* theNode = Utils::PopFront( m_freeRegionHead, m_freeRegionTail );
		theNode->SetAlignedAddress( address );
		theNode->SetAlignedSize( size );
		theNode->SetAlignmentAndFreeStatus( 0, true );
		theNode->SetLockStatus( false );
		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// ReleaseFreeRegion
	//	Release a region header back to the pool
	void LegacyGpuAllocator::ReleaseFreeRegion( RegionNode* node )
	{
		// Put a node back on the free node pool
		node->SetAlignedAddress( (u64)-1 );
		node->SetAlignedSize( (u64)-1 );
		node->SetAlignment( (u32)0 );
		Utils::PushFront( m_freeRegionHead, m_freeRegionTail, node );
	}

	///////////////////////////////////////////////////////////////////////
	// SplitFreeNode
	//	Takes a free region, splits it at the offset, returns the new node at the end + fixes up the list (the original node is now the 'front' of the split)
	LegacyGpuAllocator::RegionNode* LegacyGpuAllocator::SplitNode( RegionNode*& listHead, RegionNode*& listTail, RegionNode* sourceNode, u64 offset )
	{
		RegionNode* newNode = MakeFreeRegion( sourceNode->GetAddress() + offset, sourceNode->GetSize() - offset );
		Utils::InsertAfter( listHead, listTail, sourceNode, newNode );
		sourceNode->SetAlignedSize( offset );
		newNode->ValidateNode();
		sourceNode->ValidateNode();
		return newNode;
	}

	///////////////////////////////////////////////////////////////////////
	// PrepareAllocatedNode
	//	This takes a node with a region used for an allocation. We chop off the alignment padding, and end space,
	//  and add new free regions for them
	LegacyGpuAllocator::RegionNode* LegacyGpuAllocator::PrepareAllocatedNode( RegionNode*& listHead, RegionNode*& listTail, RegionNode* theNode,  u64 size, u64 alignment )
	{
		u64 baseAddress = theNode->GetAddress();
		u64 alignedAddress = (baseAddress + (alignment-1)) & ~(alignment-1);
		if( alignedAddress != baseAddress )
		{
			RegionNode* splitNodeFront = SplitNode( listHead, listTail, theNode, alignedAddress - baseAddress );
			theNode->SetIsFree( true );			// foundNode ptr contains the padding bit, we free it then continue
			theNode = splitNodeFront;
		}

		// The node may have space at the end that we can reclaim (if we recycle a previous region for example)
		if( theNode->GetSize() > size + k_maximumWastedSpacePerRegion )
		{
			RegionNode* splitNodeFront = SplitNode( listHead, listTail, theNode, size );
			splitNodeFront->SetIsFree( true );
		}

#ifdef ENABLE_REGION_LOCK_PROTECTION
		theNode->SetLockStatus( true );		// Allocated nodes are locked by default
#endif

		theNode->ValidateNode();

		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegionBottom
	//	Allocate from low address space
	RegionHandle LegacyGpuAllocator::AllocateRegionBottom( u64 size, u64 alignment )
	{
		// FindEmptyNodeInList is the bottleneck here; if it gets too slow we can try first-fit (huge performance increase, but more fragmentation potentially)
		RegionNode* foundNode = FindEmptyNodeInList( size, alignment, m_bottomPhysicalHead, m_bottomPhysicalTail );
		if( !foundNode )
		{
			// Allocate a new region big enough to handle the alignment
			u64 availableMemory = GetUnusedAvailable();
			u64 baseAddress = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : reinterpret_cast< u64 >( m_gpuMemoryBlock );
			u64 alignedAddress = (baseAddress + (alignment-1)) & ~(alignment-1);
			u64 regionTotalSize = alignedAddress + size - baseAddress;
			if( availableMemory >= regionTotalSize )
			{
				foundNode = MakeFreeRegion( baseAddress, regionTotalSize );
				Utils::PushBack( m_bottomPhysicalHead, m_bottomPhysicalTail, foundNode );
			}
			else
			{
				return nullptr;
			}
		}
		foundNode->ValidateNode();

		// We have a region big enough, chop off the padding and free it back to the allocator
		foundNode = PrepareAllocatedNode( m_bottomPhysicalHead, m_bottomPhysicalTail, foundNode, size, alignment );
		foundNode->SetAlignmentAndFreeStatus( static_cast< u32 >( alignment ), false );
		RED_FATAL_ASSERT( ( foundNode->GetAddress() & (alignment-1) ) == 0, "Misaligned region" );

		return static_cast< RegionHandle >( foundNode );
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegionTop
	//	Allocate from the high address space
	RegionHandle LegacyGpuAllocator::AllocateRegionTop( u64 size, u64 alignment )
	{
		// FindEmptyNodeInList is the bottleneck here; if it gets too slow we can try first-fit (huge performance increase, but more fragmentation potentially)
		RegionNode* foundNode = FindEmptyNodeInList( size, alignment, m_topPhysicalHead, m_topPhysicalTail );
		if( !foundNode )
		{
			// Allocate a new region big enough to handle the alignment
			u64 availableMemory = GetUnusedAvailable();
			u64 baseAddress = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : reinterpret_cast< u64 >( m_gpuMemoryBlock ) + GetVirtualRangeSize( m_gpuRange );
			u64 lowestAddress = baseAddress - ( size + ( alignment-1 ) );
			u64 alignedAddress = (lowestAddress + (alignment-1)) & ~(alignment-1);
			u64 regionTotalSize = baseAddress - alignedAddress;
			if( availableMemory >= regionTotalSize )
			{
				foundNode = MakeFreeRegion( alignedAddress, regionTotalSize );
				Utils::PushFront( m_topPhysicalHead, m_topPhysicalTail, foundNode );
			}
			else
			{
				return RegionHandle();
			}
		}
		foundNode->ValidateNode();

		// We have a region big enough, chop off the padding and free it back to the allocator
		foundNode = PrepareAllocatedNode( m_topPhysicalHead, m_topPhysicalTail, foundNode, size, alignment );
		foundNode->SetAlignmentAndFreeStatus( static_cast< u32 >( alignment ), false );
		RED_FATAL_ASSERT( ( foundNode->GetAddress() & (alignment-1) ) == 0, "Misaligned region" );

		return static_cast< RegionHandle >( foundNode );
	}

	///////////////////////////////////////////////////////////////////////
	// AllocateRegion
	//
	RegionHandle LegacyGpuAllocator::AllocateRegion( u64 size, u64 alignment, RegionLifetimeHint lifetimeHint )
	{
		CScopedLock< CMutex > lock( m_syncLock );
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
	LegacyGpuAllocator::RegionNode* LegacyGpuAllocator::CoalesceFreeNode( RegionNode*& head, RegionNode*& tail, RegionNode* theNode )
	{
		// Coalesce previous node
		RegionNode* previousNode = theNode->GetPrevious();
		if( previousNode != nullptr && previousNode->IsFree() )
		{
			previousNode->SetAlignedSize( previousNode->GetSize() + theNode->GetSize() );
			Utils::RemoveFromList( head, tail, theNode );

			// the old node is now invalid, push it back to the free node pool
			ReleaseFreeRegion( theNode );

			// carry on, using the merged node
			theNode = previousNode;
		}
		theNode->ValidateNode();

		// Coalesce next node
		RegionNode* nextNode = theNode->GetNext();
		if( nextNode != nullptr && nextNode->IsFree() )
		{
			theNode->SetAlignedSize( theNode->GetSize() + nextNode->GetSize() );
			Utils::RemoveFromList( head, tail, nextNode );

			// The merged 'next' node is now gone, release it back to the pool
			ReleaseFreeRegion( nextNode );
		}
		theNode->ValidateNode();

		return theNode;
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegionBottom
	//
	void LegacyGpuAllocator::FreeRegionBottom( RegionNode* theNode )
	{
		CoalesceFreeNode( m_bottomPhysicalHead, m_bottomPhysicalTail, theNode );
		theNode->SetIsFree( true );
		theNode->SetLockStatus( false );

		// If the tail of the list is free, we remove the final block to free up the middle
		if( m_bottomPhysicalTail->IsFree() )
		{
			RegionNode* endNode = m_bottomPhysicalTail;
			Utils::RemoveFromList( m_bottomPhysicalHead, m_bottomPhysicalTail, endNode );
			ReleaseFreeRegion( endNode );
		}
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegionTop
	//

	void LegacyGpuAllocator::FreeRegionTop( RegionNode* theNode )
	{
		CoalesceFreeNode( m_topPhysicalHead, m_topPhysicalTail, theNode );
		theNode->SetIsFree( true );
		theNode->SetLockStatus( false );

		// If the tail of the list is free, we remove the final block to free up the middle
		if( m_topPhysicalHead->IsFree() )
		{
			RegionNode* endNode = m_topPhysicalHead;
			Utils::RemoveFromList( m_topPhysicalHead, m_topPhysicalTail, endNode );
			ReleaseFreeRegion( endNode );
		}
	}

	///////////////////////////////////////////////////////////////////////
	// FreeRegion
	//
	void LegacyGpuAllocator::FreeRegion( RegionHandle handle )
	{
		CScopedLock< CMutex > lock( m_syncLock );
		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Free during defrag" );

		// First of all, we need to get hold of the node that matches the user region
		RegionNode* theNode = const_cast< RegionNode* >( static_cast< const RegionNode* >( handle.GetRegionInternal() ) );
		RED_MEMORY_ASSERT( !theNode->IsFree(), "This memory region is already free!" );
		if( theNode->IsFree() )
		{
			return ;
		}

#ifdef ENABLE_REGION_LOCK_PROTECTION		
		RED_MEMORY_ASSERT( !theNode->IsLocked(), "Locked regions should not be freed!" );
#endif

		theNode->ValidateNode();

		// Check the region address to figure out if it is allocated in the top or bottom memory
		u64 bottomBaseAddr = m_bottomPhysicalHead != nullptr ? m_bottomPhysicalHead->GetAddress() : (u64)-1;
		u64 bottomTailAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : (u64)-1;
		u64 topBaseAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : (u64)-1;
		u64 topTailAddr = m_topPhysicalTail != nullptr ? m_topPhysicalTail->GetEndAddress() : (u64)-1;
		
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
		}
	}

	/////////////////////////////////////////////////////////////////
	// UnlockRegion
	void LegacyGpuAllocator::UnlockRegion( RegionHandle handle )
	{
#ifdef ENABLE_REGION_LOCK_PROTECTION	
		if( handle.IsValid() )
		{
			// Do we even own this?
			RED_MEMORY_ASSERT( handle.GetAddress() >= reinterpret_cast< u64 >( m_gpuMemoryBlock ) && 
				( handle.GetAddress() + handle.GetSize() ) <= ( reinterpret_cast< u64 >( m_gpuMemoryBlock ) + GetVirtualRangeSize( m_gpuRange ) ),
				"Handle does not belong to this pool!" );
		
			CScopedLock< CMutex > lock( m_syncLock );
			RegionNode* theNode = const_cast< RegionNode* >( static_cast< const RegionNode* >( handle.GetRegionInternal() ) );
			RED_MEMORY_ASSERT( theNode->IsLocked(), "Attempting to unlock a node multiple times?!" );
			theNode->SetLockStatus( false );
		}
#endif

		RED_UNUSED( handle );
	}

	/////////////////////////////////////////////////////////////////
	// LockRegion
	void LegacyGpuAllocator::LockRegion( RegionHandle handle )
	{
#ifdef ENABLE_REGION_LOCK_PROTECTION	
		if( handle.IsValid() )
		{
			// Do we even own this?
			RED_MEMORY_ASSERT( handle.GetAddress() >= reinterpret_cast< u64 >( m_gpuMemoryBlock ) && 
				( handle.GetAddress() + handle.GetSize() ) <= ( reinterpret_cast< u64 >( m_gpuMemoryBlock ) + GetVirtualRangeSize( m_gpuRange ) ),
				"Handle does not belong to this pool!" );

			CScopedLock< CMutex > lock( m_syncLock );
			RegionNode* theNode = const_cast< RegionNode* >( static_cast< const RegionNode* >( handle.GetRegionInternal() ) );
			RED_MEMORY_ASSERT( !theNode->IsLocked(), "Attempting to lock a node multiple times?!" );
			theNode->SetLockStatus( true );
		}
#endif

		RED_UNUSED( handle );
	}

	/////////////////////////////////////////////////////////////////
	// SplitRegion
	RegionHandle LegacyGpuAllocator::SplitRegion( RegionHandle baseRegion, u64 splitPosition, u64 splitBlockAlignment )
	{
		// invalid block
		if ( !baseRegion.IsValid() || (splitPosition >= baseRegion.GetSize()) || (0==splitPosition) )
			return RegionHandle();

		// we can split only allocated regions
		RegionNode* theNode = const_cast< RegionNode* >( static_cast< const RegionNode* >( baseRegion.GetRegionInternal() ) );
		if ( theNode->IsFree() )
			return RegionHandle();

#ifdef ENABLE_REGION_LOCK_PROTECTION	
		RED_FATAL_ASSERT( theNode->IsLocked(), "Only locked regions may be split" );
#endif

		// the split block alignment must be correct upon calling thing (its too dangerous to do manually)
		RED_FATAL_ASSERT( ( ( theNode->GetAddress() + splitPosition ) & ( splitBlockAlignment - 1 ) ) == 0, "Split offset would result in misaligned region" );

		CScopedLock< CMutex > lock( m_syncLock );
		RED_FATAL_ASSERT( m_fragmentationScope == 0, "Split during defrag" );

		// Check the region address to figure out if it is allocated in the top or bottom memory
		u64 bottomBaseAddr = m_bottomPhysicalHead != nullptr ? m_bottomPhysicalHead->GetAddress() : (u64)-1;
		u64 bottomTailAddr = m_bottomPhysicalTail != nullptr ? m_bottomPhysicalTail->GetEndAddress() : (u64)-1;
		u64 topBaseAddr = m_topPhysicalHead != nullptr ? m_topPhysicalHead->GetAddress() : (u64)-1;
		u64 topTailAddr = m_topPhysicalTail != nullptr ? m_topPhysicalTail->GetEndAddress() : (u64)-1;

		RegionNode* splitRegion;
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
			return RegionHandle();
		}

		// the split block is NOT FREE
		RED_FATAL_ASSERT( splitRegion != nullptr, "Splitting the memory block failed" );
		splitRegion->SetAlignmentAndFreeStatus( static_cast< u32 >( splitBlockAlignment ), false );

#ifdef ENABLE_REGION_LOCK_PROTECTION		
		splitRegion->SetLockStatus( true );		// Split regions are always locked
#endif
		splitRegion->ValidateNode();

		// Create memory region wrapper for the split block
		return static_cast< RegionHandle >( splitRegion );
	}
} 
}
