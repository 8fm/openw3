/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_IMPL_INL
#define _RED_MEMORY_TLSF_IMPL_INL

#include "../redSystem/bitUtils.h"
#include "redMemoryAssert.h"

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////////////
// ScopedGpuProtection
//	Handle gpu memory protection in scope
class TLSFAllocatorImpl::ScopedGpuProtection
{
public:
	RED_INLINE ScopedGpuProtection( const TLSFAllocatorImpl* pool )
		: m_pool( const_cast< TLSFAllocatorImpl* >( pool ) )
	{
		m_pool->HandleGpuProtectionChange_Lock();
	}
	RED_INLINE ~ScopedGpuProtection()
	{
		m_pool->HandleGpuProtectionChange_Unlock();
	}
	TLSFAllocatorImpl* m_pool;
};

/////////////////////////////////////////////////////////////////////
// SystemMemory constructor
//
RED_INLINE TLSF::SystemMemoryBlock::SystemMemoryBlock()
	: m_address( 0u )
	, m_allocatedSize( 0u )
{
}

/////////////////////////////////////////////////////////////////////
// FindAreaContainingBlock
//	Find the area containing this block header
RED_INLINE TLSF::AreaHeader* TLSFAllocatorImpl::FindAreaContainingBlock( TLSF::BlockHeader* blockHeader )
{
	TLSF::AreaHeader* area = m_tlsfPoolHead->m_areaHead;
	Red::System::MemUint blockStart = reinterpret_cast< Red::System::MemUint >( blockHeader );
	Red::System::MemUint blockEnd = reinterpret_cast< Red::System::MemUint >( blockHeader->GetNextPhysicalBlock() );

	while( area != nullptr )
	{
		Red::System::MemUint areaStart = reinterpret_cast< Red::System::MemUint >( area );
		Red::System::MemUint areaEnd = reinterpret_cast< Red::System::MemUint >( area->m_lastBlock );

		if( blockStart > areaStart && blockStart < areaEnd && blockEnd <= areaEnd )
		{
			return area;
		}

		area = area->m_nextArea;
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////
// IsValueAligned
//	Returns true if value % alignment == 0
RED_INLINE Red::System::Bool TLSFAllocatorImpl::IsValueAligned( Red::System::MemUint value, Red::System::MemSize alignment )
{
	return ( value & ( alignment - 1 ) ) == 0;
}

/////////////////////////////////////////////////////////////////////
// GetMemoryBudget
//	Returns total budget (does not include bookkeeping, actual total will be slightly less)
RED_INLINE Red::System::MemSize TLSFAllocatorImpl::GetMemoryBudget()
{
	return m_tlsfPoolHead->m_maximumSystemBlocks * m_systemBlockSize;
}

/////////////////////////////////////////////////////////////////////
// GetFirstLevelIndexWidthFromPoolSize
//	First level index full width (bits) = log2(size)
RED_INLINE Red::System::Uint32 TLSFAllocatorImpl::GetFirstLevelIndexWidthFromPoolSize( Red::System::MemSize poolSize )
{
	Red::System::MemSize firstLevelWidth = Red::System::BitUtils::Log2( poolSize ) - TLSF::c_SmallestSegregatedSizeLog2 + 2;

	// We actually need an extra level in case we have a single huge free block that is almost the size of the pool
	return static_cast< Red::System::Uint32 >( firstLevelWidth );
}

/////////////////////////////////////////////////////////////////////
// GetBookkeepingOverhead
//	Calculate the exact bookkeeping overhead for an entire pool
RED_INLINE Red::System::MemSize TLSFAllocatorImpl::GetBookkeepingOverhead( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 firstLevelBitmapWidth, Red::System::Uint32 segregationGranularity )
{
	Red::System::MemSize secondLevelBitmapSize = sizeof( TLSF::FreelistBitmap ) * firstLevelBitmapWidth;
	secondLevelBitmapSize += (firstLevelBitmapWidth % 2) == 0 ? 0 : sizeof( TLSF::FreelistBitmap );		// If the bitmap width is odd, add 4 bytes of padding
	Red::System::MemSize freeListMatrixSize = sizeof( TLSF::BlockHeader* ) * firstLevelBitmapWidth * segregationGranularity;

	Red::System::MemSize maxSystemBlocks = ( poolSize / systemBlockSize ) + 1;
	Red::System::MemSize systemBlockListSize = sizeof( TLSF::SystemMemoryBlock ) * maxSystemBlocks;

	return sizeof(TLSF::PoolHeader) + secondLevelBitmapSize + freeListMatrixSize + systemBlockListSize;
}

/////////////////////////////////////////////////////////////////////
// PoolOwnsPointer
//	Returns true if the pointer lies in the range of the pools
//  owned memory. Note this does not test if the block is valid!
RED_INLINE Red::System::Bool TLSFAllocatorImpl::PoolOwnsPointer( const void* thePtr ) const
{
	Red::System::MemUint ptrAddress = reinterpret_cast< Red::System::MemUint >( thePtr );
	TLSF::AreaHeader* header = m_tlsfPoolHead->m_areaHead;
	while( header != nullptr )
	{
		Red::System::MemUint areaAddress = reinterpret_cast< Red::System::MemUint >( header );
		Red::System::MemUint areaEnd = reinterpret_cast< Red::System::MemUint >( header->m_lastBlock );

		if( ptrAddress > areaAddress && ptrAddress < areaEnd )
		{
			return true;
		}

		header = header->m_nextArea;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////
// SizeOfOwnedBlock
//	Returns the size of the block at ptr
RED_INLINE Red::System::MemSize TLSFAllocatorImpl::SizeOfOwnedBlock( const void* thePtr ) const
{
	if( thePtr == nullptr || !PoolOwnsPointer( thePtr ) )
	{
		return 0;
	}

	Red::System::MemUint ptrAddress = reinterpret_cast< Red::System::MemUint >( thePtr );
	TLSF::BlockHeader* header = reinterpret_cast< TLSF::BlockHeader* >( ptrAddress - TLSF::c_UsedBlockHeaderSize );
	RED_MEMORY_ASSERT( !header->IsFreeBlock(),  "Block is free or this pointer is invalid" );
	header->VerifyBlockState();

	// Track the amount freed and return the block to the free lists
	return header->GetDataSize();
}

/////////////////////////////////////////////////////////////////////
// GetListIndices
//	This is the heart of the TLSF allocator
//	For a given size, 2 indices will be returned that can be used to 
//  find the matching free list and bitmap indices
RED_INLINE void TLSFAllocatorImpl::GetListIndices( Red::System::MemSize size, Red::System::MemSize& firstLevel, Red::System::MemSize& secondLevel )
{
	Red::System::Uint32 linearSegregationLevelLog2 = m_tlsfPoolHead->m_secondLevelIndexLog2;
	Red::System::Uint32 linearSegregationLevel = m_tlsfPoolHead->m_secondLevelIndexWidth;
	
	// Special case for small allocations
	if( size < TLSF::c_SmallestSegregatedSize )
	{
		firstLevel = 0;
		secondLevel = size / ( TLSF::c_SmallestSegregatedSize / linearSegregationLevel );
	}
	else
	{
		firstLevel = Red::System::BitUtils::Log2( size );
		secondLevel = ( size >> ( firstLevel - linearSegregationLevelLog2 ) ) - linearSegregationLevel;
		firstLevel = firstLevel - TLSF::c_SmallestSegregatedSizeLog2 + 1;	// Take into account the special case small alloc lists
	}
}

/////////////////////////////////////////////////////////////////////
// RemoveFreeBlock
//	Removes a block from its segregated list and updates the bitmaps and free list matrix
RED_INLINE void TLSFAllocatorImpl::RemoveFreeBlock( TLSF::BlockHeader* theBlock )
{
	// Extract the block from its free list
	if( theBlock->GetPreviousFree() != nullptr )
	{
		theBlock->GetPreviousFree()->SetNextFree( theBlock->GetNextFree() );
	}
	if( theBlock->GetNextFree() != nullptr )
	{
		theBlock->GetNextFree()->SetPreviousFree( theBlock->GetPreviousFree() );
	}	

	// If the block is the first one in its free list, move the free list head along
	Red::System::MemSize firstIndex = 0, secondIndex = 0;
	GetListIndices( theBlock->GetDataSize(), firstIndex, secondIndex );
	TLSF::BlockHeader* freeListHead = GetFreeListHead( firstIndex, secondIndex );
	if( freeListHead == theBlock )
	{
		freeListHead = theBlock->GetNextFree();
		SetFreeListHead( freeListHead, firstIndex, secondIndex );

		// If the free list is now empty, clear the bitmap 
		if( freeListHead == nullptr )
		{
			ClearFreelistBitmap( firstIndex, secondIndex );
		}
	}

	// Finally clear the free list pointers for the block
	theBlock->SetFreeListPointers( nullptr, nullptr );
}

/////////////////////////////////////////////////////////////////////
// ClearFreelistBitmap
//	Clear a bit in the corresponding second-level free list bitmap
//	If the bitmap is empty after clearing the bit, the first level bit
//  is also cleared
RED_INLINE void TLSFAllocatorImpl::ClearFreelistBitmap( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel )
{
	RED_MEMORY_ASSERT( firstLevel < m_tlsfPoolHead->m_firstLevelIndexWidth && secondLevel < m_tlsfPoolHead->m_secondLevelIndexWidth,  "Bad free list indices"  );
	TLSF::FreelistBitmap& secondLevelBitmap = m_tlsfPoolHead->m_secondLevelBitmap[ firstLevel ];
	secondLevelBitmap &= ~( 1 << secondLevel );

	if( secondLevelBitmap == 0 )
	{
		// There are no more free lists for the first level index, clear the first-level bitmap
		m_tlsfPoolHead->m_firstLevelBitmap &= ~( 1 << firstLevel );
	}
}

/////////////////////////////////////////////////////////////////////
// SetFreelistBitmap
//	Update the first and second level bitmaps
RED_INLINE void TLSFAllocatorImpl::SetFreelistBitmap( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel )
{
	RED_MEMORY_ASSERT( firstLevel < m_tlsfPoolHead->m_firstLevelIndexWidth && secondLevel < m_tlsfPoolHead->m_secondLevelIndexWidth,  "Bad free list indices" );
	TLSF::FreelistBitmap& secondLevelBitmap = m_tlsfPoolHead->m_secondLevelBitmap[ firstLevel ];
	secondLevelBitmap |= ( 1 << secondLevel );
	m_tlsfPoolHead->m_firstLevelBitmap |= ( 1 << firstLevel );
}

/////////////////////////////////////////////////////////////////////
// GetFreeListHead
//	Returns the head of a free list (or null on bad indices)
RED_INLINE TLSF::BlockHeader* TLSFAllocatorImpl::GetFreeListHead( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel )
{
	RED_MEMORY_ASSERT( firstLevel < m_tlsfPoolHead->m_firstLevelIndexWidth && secondLevel < m_tlsfPoolHead->m_secondLevelIndexWidth, "Bad free list indices" );
	return m_tlsfPoolHead->m_freeListMatrix[ firstLevel + ( secondLevel * m_tlsfPoolHead->m_firstLevelIndexWidth ) ];
}

/////////////////////////////////////////////////////////////////////
// SetFreeListHead
//	Set the free list head pointer. Does not do any list stitching
RED_INLINE void TLSFAllocatorImpl::SetFreeListHead( TLSF::BlockHeader* newHead, Red::System::MemSize firstLevel, Red::System::MemSize secondLevel )
{
	RED_MEMORY_ASSERT( firstLevel < m_tlsfPoolHead->m_firstLevelIndexWidth && secondLevel < m_tlsfPoolHead->m_secondLevelIndexWidth, "Bad free list indices" );
	m_tlsfPoolHead->m_freeListMatrix[ firstLevel + ( secondLevel * m_tlsfPoolHead->m_firstLevelIndexWidth ) ] = newHead;
}

/////////////////////////////////////////////////////////////////////
// AddBlockToFreeList
//	Add a block to its corresponding free list and update the bitmaps
RED_INLINE void TLSFAllocatorImpl::AddBlockToFreeList( TLSF::BlockHeader* theBlock )
{
	RED_MEMORY_ASSERT( theBlock->IsFreeBlock(), "Cannot add a used block to the free list!" );

	// Calculate which list the block belongs to
	Red::System::MemSize firstIndex = 0, secondIndex = 0;
	GetListIndices( theBlock->GetDataSize(), firstIndex, secondIndex );

	// Add the block to the head of its free list
	TLSF::BlockHeader* freeListHead = GetFreeListHead( firstIndex, secondIndex );
	theBlock->SetPreviousFree( nullptr );
	theBlock->SetNextFree( freeListHead );
	if( freeListHead )
	{
		freeListHead->SetPreviousFree( theBlock );
	}
	SetFreeListHead( theBlock, firstIndex, secondIndex );

	// Update the bitmaps
	SetFreelistBitmap( firstIndex, secondIndex );
	theBlock->VerifyBlockState();
}

/////////////////////////////////////////////////////////////////////
// FindFreeblockForSize
//	Search for a block that can contain the size specified
RED_INLINE TLSF::BlockHeader* TLSFAllocatorImpl::FindFreeblockForSize( Red::System::MemSize size )
{
	// Round the size up to the next block size if the small block list 
	if( size > TLSF::c_SmallestSegregatedSize )
	{
		Red::System::MemSize roundUp = ( 1 << ( Red::System::BitUtils::BitScanReverse( size ) - m_tlsfPoolHead->m_secondLevelIndexLog2 ) ) - 1;
		size += roundUp;
	}

	// Find the free list indices for the requested size
	Red::System::MemSize firstIndex = 0, secondIndex = 0;
	GetListIndices( size, firstIndex, secondIndex );

	RED_MEMORY_ASSERT( firstIndex < m_tlsfPoolHead->m_firstLevelIndexWidth, "FirstIndex is bad" );
	RED_MEMORY_ASSERT( secondIndex < m_tlsfPoolHead->m_secondLevelIndexWidth, "SecondIndex is bad" );

	// Search the second-level bitmap for the first-level index
	// Done by chopping off any bits before firstIndex, then testing if the result is != 0
	Red::System::Uint32 secondLevelBitmap = m_tlsfPoolHead->m_secondLevelBitmap[ firstIndex ] & ( ~Red::System::Uint32(0) << secondIndex );
	if( secondLevelBitmap == 0 )	// No free blocks big enough in this list, search for the next largest free list
	{
		Red::System::Uint32 firstLevelTopBits = m_tlsfPoolHead->m_firstLevelBitmap & ( ~Red::System::Uint32(0) <<  ( firstIndex + 1 ) );
		if( firstLevelTopBits == 0 )
		{
			// There are no free blocks big enough to hold the requested size
			return nullptr;
		}
		firstIndex = Red::System::BitUtils::BitScanForward( firstLevelTopBits );
		secondLevelBitmap = m_tlsfPoolHead->m_secondLevelBitmap[ firstIndex ];
	}

	// By this point the first index should definitely be valid, so check its secondary free lists
	secondIndex = Red::System::BitUtils::BitScanForward( secondLevelBitmap );
	
	// Return the head of the free list found. This block is guaranteed to be >= size
	return GetFreeListHead( firstIndex, secondIndex );
}

/////////////////////////////////////////////////////////////////////
// MergeFreeBlock
//	Merge a free block with its neighbouring free blocks
RED_INLINE void TLSFAllocatorImpl::MergeFreeBlock( TLSF::BlockHeader* theBlock )
{
	RED_MEMORY_ASSERT( theBlock->IsFreeBlock(), "Merging a used block!" );

	TLSF::BlockHeader* nextBlock = theBlock->GetNextPhysicalBlock();
	nextBlock->VerifyBlockState();
	if( nextBlock->IsFreeBlock() )
	{
		RemoveFreeBlock( nextBlock );	// Remove it from its free list
		theBlock->SetSize( theBlock->GetTotalSize() + nextBlock->GetTotalSize() );	// grow the previous block by the next block size
	}

	if( theBlock->IsPreviousBlockFree() )
	{
		TLSF::BlockHeader* previousBlock = theBlock->GetPreviousPhysical();
		RED_MEMORY_ASSERT( previousBlock->IsFreeBlock(), "Bookkeeping is broken somewhere!" );
		RemoveFreeBlock( previousBlock );	// Remove it from free list

		// Since we are merging with a previous block, we don't actually need the currently freed block any more
		// It will become part of the new larger block's data. Instead, we take the previous block and add it back to the free list
		previousBlock->SetSize( previousBlock->GetTotalSize() + theBlock->GetTotalSize() );
		theBlock = previousBlock;	// we don't need the current block any more
	}
	theBlock->VerifyBlockState();
	AddBlockToFreeList( theBlock );

	// Update the next physical block and set its previous_free flag
	TLSF::BlockHeader* nextPhysical = theBlock->GetNextPhysicalBlock();
	nextPhysical->SetPreviousPhysical( theBlock );
	nextPhysical->SetPreviousPhysicalState( TLSF::c_PreviousBlockIsFree );
	nextPhysical->VerifyBlockState();
}

/////////////////////////////////////////////////////////////////////
// SplitBlock
//	Chop a free block in half at dataPointer + splitDataOffset
//	Note you must handle the new block bookkeeping!
RED_INLINE TLSF::BlockHeader* TLSFAllocatorImpl::SplitBlock( TLSF::BlockHeader* theBlock, Red::System::MemSize splitDataOffset )
{
	// Split the block to create 2 new free blocks
	// The new block takes the remainder of the space in the block
	Red::System::MemUint newBlockAddress = reinterpret_cast< Red::System::MemUint >( theBlock->GetDataStartAddress() ) + splitDataOffset;
	Red::System::MemSize newBlockSize = theBlock->GetDataSize() - splitDataOffset;
	RED_MEMORY_ASSERT( newBlockSize <= theBlock->GetDataSize(), "The block split offset is too large!" );

	// Cache all interesting info from theBlock into registers asap
	Red::System::MemSize isPreviousFree = theBlock->IsFreeBlock() ? TLSF::c_PreviousBlockIsFree : TLSF::c_PreviousBlockIsUsed;
	Red::System::MemSize isBlockFree = theBlock->IsFreeBlock() ? TLSF::c_BlockIsFree : TLSF::c_BlockIsUsed;
	TLSF::BlockHeader* nextPhysical = theBlock->GetNextPhysicalBlock();

	// Write this now; we won't touch it again
	theBlock->SetSize( theBlock->GetTotalSize() - newBlockSize );

	// Create a new block at the offset address. Inherit the used state from theBlock
	TLSF::BlockHeader* newBlock = reinterpret_cast< TLSF::BlockHeader* >( newBlockAddress );
	newBlock->SetPreviousPhysical( theBlock );
	newBlock->SetSizeAndState( newBlockSize, isPreviousFree, isBlockFree );
	newBlock->SetFreeListPointers( nullptr, nullptr );

	// Fix up the next blocks previous physical pointer
	nextPhysical->SetPreviousPhysical( newBlock );
	nextPhysical->SetPreviousPhysicalState( ( isBlockFree == TLSF::c_BlockIsFree ) ? TLSF::c_PreviousBlockIsFree : TLSF::c_PreviousBlockIsUsed );

	nextPhysical->VerifyBlockState();
	theBlock->VerifyBlockState();
	newBlock->VerifyBlockState();

	return newBlock;
}

/////////////////////////////////////////////////////////////////////
// SetPreviousPhysical
//	Set the previous physical block pointer
//	In debug, test that everything matches up
RED_INLINE void TLSF::BlockHeader::SetPreviousPhysical( BlockHeader* header )
{
	m_previousPhysical = header;
}

/////////////////////////////////////////////////////////////////////
// GetNextPhysicalBlock
//	Get the address of the next block in memory 
RED_INLINE TLSF::BlockHeader* TLSF::BlockHeader::GetNextPhysicalBlock()
{
	Red::System::MemUint nextBlockAddress = reinterpret_cast< Red::System::MemUint >( this ) + GetTotalSize();
	RED_MEMORY_ASSERT( (nextBlockAddress & (TLSF::c_MinimumPoolAlignment-1)) == 0, "Misaligned block pointer" );

	return reinterpret_cast< TLSF::BlockHeader* >( nextBlockAddress );
}

/////////////////////////////////////////////////////////////////////
// VerifyBlockState
//	Call this to test everything we can in debug
RED_INLINE void TLSF::BlockHeader::VerifyBlockState()
{
#ifdef _DEBUG
	Red::System::MemUint myAddress = reinterpret_cast< Red::System::MemUint >( this );
	RED_UNUSED( myAddress );

	// Test block header alignment
	RED_MEMORY_ASSERT( (myAddress & (TLSF::c_MinimumPoolAlignment-1)) == 0, "Misaligned block pointer" );

	// Test previous block address + size = my address
	if( m_previousPhysical != nullptr )
	{
		Red::System::MemUint prevPhysAddress = reinterpret_cast< Red::System::MemUint >( m_previousPhysical );
		RED_UNUSED( prevPhysAddress );

		RED_MEMORY_ASSERT( prevPhysAddress + m_previousPhysical->GetTotalSize() == myAddress, "A block has a bad size! Previous physical block does not point to this block" );

		if( m_previousPhysical->IsFreeBlock() )
		{
			RED_MEMORY_ASSERT( IsPreviousBlockFree(), "The previous physical block is free but my previous physical flag is set to used!" );
		}
		else
		{
			RED_MEMORY_ASSERT( !IsPreviousBlockFree(), "The previous physical block is used but my previous physical flag is set to free!" );
		}
	}

	if( IsFreeBlock() )
	{
		if( m_freeBlockData.m_nextSegregated != nullptr )
		{
			RED_MEMORY_ASSERT( m_freeBlockData.m_nextSegregated->IsFreeBlock(), "Free block linked list is broken (next free block is NOT free)" );
			RED_MEMORY_ASSERT( m_freeBlockData.m_nextSegregated->GetPreviousFree() == this, "Free block linked list is corrupt (next free's previous is not this)" );
		}

		if( m_freeBlockData.m_prevSegregated != nullptr )
		{
			RED_MEMORY_ASSERT( m_freeBlockData.m_prevSegregated->IsFreeBlock(), "Free block linked list is broken (previous free block is NOT free)" );
			RED_MEMORY_ASSERT( m_freeBlockData.m_prevSegregated->GetNextFree() == this, "Free block linked list is corrupt (prev free's next is not this)" );
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////
// SetMemoryClass
//	Record the associated memory class of each allocation (Debug only)
RED_INLINE void TLSF::BlockHeader::SetMemoryClass( Uint16 memoryClass )
{
	m_memoryClass = memoryClass;
}

/////////////////////////////////////////////////////////////////////
// GetMemoryClass
RED_INLINE Uint16 TLSF::BlockHeader::GetMemoryClass() const
{
	return m_memoryClass;
}

/////////////////////////////////////////////////////////////////////
// GetDataStartAddress
//	Returns the start address of the block data
RED_INLINE void* TLSF::BlockHeader::GetDataStartAddress() const
{
	return (void*)&m_data[0];
}

/////////////////////////////////////////////////////////////////////
// SetBlockSize
//	Sets a blocks size without touching its state mask
//	Since size is always a multiple of word_size, we don't worry about
//	shifting it in the status mask, we just chop off the bottom 2 bits
RED_INLINE void TLSF::BlockHeader::SetSize( Red::System::MemSize size )
{
	RED_MEMORY_ASSERT( (size & (TLSF::c_MinimumPoolAlignment-1)) == 0, "Attempting to set a block to a badly aligned size" );
	m_sizeAndStatus = ( size & ~TLSF::c_BlockStatusMask ) | ( m_sizeAndStatus & TLSF::c_BlockStatusMask );
}

/////////////////////////////////////////////////////////////////////
// SetBlockState
//	Set both parts of a block state mask at once
RED_INLINE void TLSF::BlockHeader::SetState( Red::System::MemSize previousPhysicalState, Red::System::MemSize blockState )
{
	m_sizeAndStatus = ( m_sizeAndStatus & ~TLSF::c_BlockStatusMask ) | previousPhysicalState | blockState;
}

/////////////////////////////////////////////////////////////////////
// SetBlockState
//	Set both parts of a block state mask at once
RED_INLINE void TLSF::BlockHeader::SetSizeAndState( Red::System::MemSize size, Red::System::MemSize previousPhysicalState, Red::System::MemSize blockState )
{
	RED_MEMORY_ASSERT( (size & (TLSF::c_MinimumPoolAlignment-1)) == 0, "Attempting to set a block to a badly aligned size" );
	m_sizeAndStatus = ( size & ~TLSF::c_BlockStatusMask ) | previousPhysicalState | blockState;
}

/////////////////////////////////////////////////////////////////////
// SetFreeState
//
RED_INLINE void TLSF::BlockHeader::SetFreeState( Red::System::MemSize blockState )
{
	m_sizeAndStatus = ( m_sizeAndStatus & ~TLSF::c_BlockFreeStatusMask ) | blockState;
}

/////////////////////////////////////////////////////////////////////
// SetPreviousPhysicalState
//
RED_INLINE void TLSF::BlockHeader::SetPreviousPhysicalState( Red::System::MemSize previousState )
{
	m_sizeAndStatus = ( m_sizeAndStatus & ~TLSF::c_BlockPrevStateMask ) | previousState;
}

/////////////////////////////////////////////////////////////////////
// GetTotalSize
//	Returns the total size of a block (header + data)
RED_INLINE Red::System::MemSize TLSF::BlockHeader::GetTotalSize() const
{
	return ( m_sizeAndStatus & ~TLSF::c_BlockStatusMask );
}

/////////////////////////////////////////////////////////////////////
// GetDataSize
//	Returns the data size of a block
RED_INLINE Red::System::MemSize TLSF::BlockHeader::GetDataSize() const
{
	return ( m_sizeAndStatus & ~TLSF::c_BlockStatusMask ) - c_UsedBlockHeaderSize;
}

/////////////////////////////////////////////////////////////////////
// SetFreeListPointers
//	
RED_INLINE void TLSF::BlockHeader::SetFreeListPointers( BlockHeader* previous, BlockHeader* next )
{
	m_freeBlockData.m_prevSegregated = previous;
	m_freeBlockData.m_nextSegregated = next;
}

/////////////////////////////////////////////////////////////////////
// IsFreeBlock
//	
RED_INLINE Red::System::Bool TLSF::BlockHeader::IsFreeBlock() const
{
	return (m_sizeAndStatus & TLSF::c_BlockFreeStatusMask) == TLSF::c_BlockIsFree;
}

/////////////////////////////////////////////////////////////////////
// IsPreviousBlockFree
//	
RED_INLINE Red::System::Bool TLSF::BlockHeader::IsPreviousBlockFree() const
{
	return (m_sizeAndStatus & TLSF::c_BlockPrevStateMask) == TLSF::c_PreviousBlockIsFree;
}

/////////////////////////////////////////////////////////////////////
// GetNextFree
//
RED_INLINE TLSF::BlockHeader* TLSF::BlockHeader::GetNextFree() const
{
	return m_freeBlockData.m_nextSegregated;
}

/////////////////////////////////////////////////////////////////////
// GetPreviousFree
//
RED_INLINE TLSF::BlockHeader* TLSF::BlockHeader::GetPreviousFree() const
{
	return m_freeBlockData.m_prevSegregated;
}

/////////////////////////////////////////////////////////////////////
// SetNextFree
//
RED_INLINE void TLSF::BlockHeader::SetNextFree( BlockHeader* next )
{
	RED_MEMORY_ASSERT( next == nullptr || (next != nullptr && IsFreeBlock()), "Cannot set next pointer when this block is not free" );
	m_freeBlockData.m_nextSegregated = next;
}

/////////////////////////////////////////////////////////////////////
// SetPreviousFree
//
RED_INLINE void TLSF::BlockHeader::SetPreviousFree( BlockHeader* prev )
{
	RED_MEMORY_ASSERT( prev == nullptr || (prev != nullptr && IsFreeBlock()), "Cannot set previous pointer when this block is not free" );
	m_freeBlockData.m_prevSegregated = prev;
}

/////////////////////////////////////////////////////////////////////
// GetPreviousPhysical
//
RED_INLINE TLSF::BlockHeader* TLSF::BlockHeader::GetPreviousPhysical() const
{
	return m_previousPhysical;
}


} }

#endif