/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "tlsfAllocator.h"
#include "tlsfConstant.h"
#include "tlsfBlock.h"
#include "assert.h"
#include "utils.h"

#include "../../redSystem/bitUtils.h"
#include "../../redSystem/unitTestMode.h"

RED_DISABLE_WARNING_CLANG( "-Wunused-private-field" ) // Ignore warning due to padding not being used.

namespace red
{
namespace memory
{
namespace
{
	void MarkAllocatedBlock( const Block & block )
	{
#ifdef RED_MEMORY_UNIT_TEST
		if( UnitTestMode() )
		{
			MemsetBlock( block, c_tlsfUnitTestAllocFiller );
		}
#endif

		RED_UNUSED( block );
	}

	void MarkFreeBlock( const Block & block )
	{
#ifdef RED_MEMORY_UNIT_TEST
		if( UnitTestMode() )
		{
			MemsetBlock( block, c_tlsfUnitTestFreeFiller );
		}
#endif

		RED_UNUSED( block );
	}
}

	TLSFAllocator::TLSFAllocator()
		:	m_firstLevelIndexWidth( 0 ),
			m_firstLevelBitmap( 0 ),
			m_secondLevelIndexWidth( 0 ),
			m_secondLevelIndexLog2( 0 ),
			m_secondLevelBitmap( nullptr ),
			m_freeListMatrix( nullptr ),
			m_areaHead( nullptr )
	{}

	TLSFAllocator::~TLSFAllocator()
	{}
	
	void TLSFAllocator::Initialize( const TLSFAllocatorParameter& parameter )
	{
		m_reservedMemoryRange = parameter.range;
		
		// We actually need an extra level in case we have a single huge free block that is almost the size of the pool
		m_firstLevelIndexWidth = static_cast< u32 >( BitUtils::Log2( GetVirtualRangeSize( parameter.range ) ) )  - c_tlsfSmallestSegregatedSizeLog2 + 2;
		m_secondLevelIndexWidth = c_tlsfMaximumSecondLevelDivisions;
		m_secondLevelIndexLog2 = BitUtils::Log2( c_tlsfMaximumSecondLevelDivisions );

		const u64 bookKeepingAddress = InitializeBookKeeping( parameter.block );

		u64 areaStartAddress = bookKeepingAddress;
		u64 addrdiff = areaStartAddress - parameter.block.address;
		const u32 areaSize = static_cast< u32 >( parameter.block.size - addrdiff );

		m_areaHead = InitializeArea( areaStartAddress, areaSize );
	}

	u64 TLSFAllocator::InitializeBookKeeping( const SystemBlock & block )
	{
		RED_MEMORY_ASSERT( IsAligned( block.address, c_tlsfBlockHeaderAlignment ), "Bad bookkeeping alignment" );

		u64 bookKeepingAddress = block.address;
		void * bookKeepingPtr = reinterpret_cast< void* >( bookKeepingAddress );

		// Create the second level bitmap pointers
		m_secondLevelBitmap = new( bookKeepingPtr ) FreeListBitmap[ m_firstLevelIndexWidth ];
		bookKeepingAddress += sizeof( FreeListBitmap ) * m_firstLevelIndexWidth;
		std::memset( m_secondLevelBitmap, 0, sizeof( FreeListBitmap ) * m_firstLevelIndexWidth );

		// Align up 
		bookKeepingAddress = AlignAddress( bookKeepingAddress, c_tlsfBlockHeaderAlignment );
		bookKeepingPtr = reinterpret_cast< void* >( bookKeepingAddress );

		// Create the free-list matrix
		m_freeListMatrix = new( bookKeepingPtr ) TLSFBlockHeader*[ m_firstLevelIndexWidth * m_secondLevelIndexWidth ];
		bookKeepingAddress += sizeof( TLSFBlockHeader* ) * m_firstLevelIndexWidth * m_secondLevelIndexWidth;
		std::memset( m_freeListMatrix, 0, sizeof( TLSFBlockHeader* ) * m_firstLevelIndexWidth * m_secondLevelIndexWidth );

		// Align up 
		bookKeepingAddress = AlignAddress( bookKeepingAddress, c_tlsfBlockHeaderAlignment );
		return bookKeepingAddress;
	}

	TLSFAllocator::TLSFAreaHeader * TLSFAllocator::InitializeArea( u64 areaStartAddress, u64 areaSize )
	{
		RED_MEMORY_ASSERT( IsAligned( areaStartAddress, c_tlsfBlockHeaderAlignment ), "Area start address is not aligned correctly" );

		// The area needs to be big enough for 2 sentinel blocks + the actual data
		RED_MEMORY_ASSERT( areaSize > ( c_tlsfFreeBlockHeaderSize * 3 ), "Trying to initialise a area but its too small to contain anything" );

		// The first block will be set to used and contain the area header
		u32 firstBlockSize = std::max( static_cast< u32 >( sizeof( TLSFAreaHeader ) ), c_tlsfFreeBlockHeaderSize );
		TLSFBlockHeader* firstBlock = reinterpret_cast< TLSFBlockHeader* >( areaStartAddress );
		firstBlock->SetSizeAndState( firstBlockSize, c_tlsfPreviousBlockIsUsed, c_tlsfBlockIsUsed );
		firstBlock->SetFreeListPointers( nullptr, nullptr );
		firstBlock->SetPreviousPhysical( nullptr );
		firstBlock->VerifyBlockState();

		// The second block will be the actual free data
		// We create it as a used block, then use FreeBlock() to add it to the bookkeeping properly
		u32 freeBlockSize = static_cast< u32 >( areaSize - firstBlockSize - c_tlsfFreeBlockHeaderSize );
		TLSFBlockHeader* blockToFree = firstBlock->GetNextPhysicalBlock();
		blockToFree->SetSizeAndState( freeBlockSize, c_tlsfPreviousBlockIsUsed, c_tlsfBlockIsUsed );
		blockToFree->SetPreviousPhysical( firstBlock );
		blockToFree->SetFreeListPointers( nullptr, nullptr );
		blockToFree->VerifyBlockState();

		// The final block acts as a sentinel and stops merges from running off the end of the area
		TLSFBlockHeader* finalBlock = blockToFree->GetNextPhysicalBlock();
		finalBlock->SetSizeAndState( 0, c_tlsfPreviousBlockIsUsed, c_tlsfBlockIsUsed );
		finalBlock->SetFreeListPointers( nullptr, nullptr );
		finalBlock->SetPreviousPhysical( blockToFree );
		finalBlock->VerifyBlockState();

		// Ensure the final block hasn't stomped over the end of the buffer
		RED_MEMORY_ASSERT(
			reinterpret_cast< u64 >( finalBlock ) + c_tlsfFreeBlockHeaderSize <= areaStartAddress + areaSize, 
			"Final sentinel block pushed over the end of the area buffer!" 
		);

		// Free the middle block. This sets up the bookkeeping so that block can be used for allocations
		FreeBlock( blockToFree );

		// Finally, initialize the area info
		TLSFAreaHeader* areaHeader = reinterpret_cast< TLSFAreaHeader* >( firstBlock->GetDataStartAddress() );
		areaHeader->lastBlock = finalBlock;
		areaHeader->nextArea = nullptr;
		return areaHeader;
	}

	void TLSFAllocator::Unitialize()
	{}

	void TLSFAllocator::Free( Block & block )
	{
		RED_MEMORY_ASSERT( IsInitialized(), "TLSFAllocator is not initialized." );

		if( block.address )
		{
			RED_MEMORY_ASSERT( OwnBlock( block.address ), "TLSF do own provided memory block." );

			TLSFBlockHeader* header = GetTLSFBlockHeader( block.address );
			RED_MEMORY_ASSERT( !header->IsFreeBlock(), "Double-free detected! This pointer has already been freed" );
			header->VerifyBlockState();

			block.size = header->GetDataSize();

			MarkFreeBlock( block );

			FreeBlock( header );
		}
	}

	Block TLSFAllocator::Allocate( u32 size )
	{
		return AllocateAligned( size, c_tlsfDefaultAlignment );
	}

	Block TLSFAllocator::AllocateAligned( u32 size, u32 alignment )
	{
		RED_MEMORY_ASSERT( IsInitialized(), "TLSFAllocator is not initialized." );
		RED_MEMORY_ASSERT( IsPowerOf2( alignment ), "Alignment must be power of two." );

		alignment = std::max( c_tlsfDefaultAlignment, alignment );

		// Round size up to smallest possible allocation and nearest alignment boundary
		size = std::max( c_tlsfSmallestAllocSize, size );
		size = ( size + c_tlsfBlockHeaderAlignment - 1 ) & ~( c_tlsfBlockHeaderAlignment - 1 );

		TLSFBlockHeader * foundBlock = TakeFreeBlock( size, alignment );

		if( foundBlock )
		{
			Block result = { foundBlock->GetDataStartAddress(), foundBlock->GetDataSize() };
			MarkAllocatedBlock( result );

			return result;
		}

		return NullBlock();
	}

	Block TLSFAllocator::Reallocate( Block & block, u32 size )
	{
		if( block.address == 0 )
		{
			return size != 0 ? Allocate( size ) : NullBlock();
		}

		if( size == 0 )
		{
			Free( block );
			return NullBlock();
		}

		// Round size up to smallest possible allocation and nearest alignment boundary
		size = std::max( c_tlsfSmallestAllocSize, size );
		size = ( size + c_tlsfBlockHeaderAlignment - 1 ) & ~( c_tlsfBlockHeaderAlignment - 1 );

		TLSFBlockHeader * blockHeader = GetTLSFBlockHeader( block );
		block.size = blockHeader->GetDataSize();
		if( size == block.size )
		{
			return block;
		}
		else if( size < block.size )
		{
			ShrinkBlock( blockHeader, size, c_tlsfDefaultAlignment );
			Block result = { block.address, blockHeader->GetDataSize() };
			return result;
		}
		else
		{
			// ctremblay, TODO Figure out why GrowBlock do not work in all cases.
			/*TLSFBlockHeader * newBlock = GrowBlock( blockHeader, size, c_tlsfDefaultAlignment );
			if( newBlock )
			{
				Block result = { newBlock->GetDataStartAddress(), newBlock->GetDataSize() };
				return result;
			}*/
			
			// Can't grow, fallback to alloc/memcpy/free
			Block allocBlock = Allocate( size );
			if( allocBlock.address )
			{
				MemcpyBlock( allocBlock.address, block.address, block.size );
				Free( block );
				return allocBlock;
			}
		}

		return NullBlock(); // Everything failed. No more memory most likely.
	}

	TLSFBlockHeader * TLSFAllocator::TakeFreeBlock( u32 size, u32 alignment )
	{
		// We need enough space to handle the worst-case alignment (required alignment - 3 words : 4 word free block would push us over alignment boundary)
		u32 sizeIncludingAlignmentPad = size;
		if( alignment > c_tlsfBlockHeaderAlignment )
		{
			sizeIncludingAlignmentPad += alignment + ( c_tlsfBlockHeaderAlignment * 3 );
		}

		TLSFBlockHeader * foundBlock = FindFreeBlock( sizeIncludingAlignmentPad );

		if( foundBlock )
		{
			foundBlock->VerifyBlockState();

			RED_MEMORY_ASSERT( foundBlock->IsFreeBlock(), "Found block is not free!?" );
			RED_MEMORY_ASSERT( foundBlock->GetDataSize() >= sizeIncludingAlignmentPad, "The found block is too small! Free-lists are probably broken!" );

			// Remove the block from its free list
			RemoveBlockFromFreeList( foundBlock );

			// The block is now used
			foundBlock->SetFreeState( c_tlsfBlockIsUsed );
			foundBlock->GetNextPhysicalBlock( )->SetPreviousPhysicalState( c_tlsfPreviousBlockIsUsed );
			foundBlock->VerifyBlockState();
		
			// Block might be too big. Split and give back to allocator new resulting block
			return ShrinkBlock( foundBlock, size, alignment );
		}

		return nullptr;
	}

	u32 TLSFAllocator::ComputeMinimumAreaSize( u32 size, u32 alignment )
	{
		u32 minimumSize = size;

		// Need to fit area header + block header + block footer + bookeeping metadata -> +- 4 times block header size.
		minimumSize += ( sizeof(TLSFBlockHeader) * 4 );  

		if( alignment > c_tlsfBlockHeaderAlignment )
		{
			minimumSize += alignment + ( c_tlsfBlockHeaderAlignment * 3 );
		}

		if( minimumSize > c_tlsfSmallestSegregatedSize )
		{
			u32 roundUp = ( 1 << ( BitUtils::BitScanReverse( minimumSize ) - m_secondLevelIndexLog2 ) ) - 1;
			minimumSize += roundUp;
		}

		return minimumSize;
	}

	TLSFBlockHeader * TLSFAllocator::FindFreeBlock( u32 size )
	{
		u32 roundedUpSize = size;
		if( size > c_tlsfSmallestSegregatedSize )
		{
			u32 roundUp = ( 1 << ( BitUtils::BitScanReverse( size ) - m_secondLevelIndexLog2 ) ) - 1;
			roundedUpSize += roundUp;
		}

		// Find the free list indices for the requested size
		u32 firstIndex = 0, secondIndex = 0;
		GetListIndices( roundedUpSize, firstIndex, secondIndex );

		RED_MEMORY_ASSERT( firstIndex < m_firstLevelIndexWidth, "FirstIndex is bad" );
		RED_MEMORY_ASSERT( secondIndex < m_secondLevelIndexWidth, "SecondIndex is bad" );

		// Search the second-level bitmap for the first-level index
		// Done by chopping off any bits before firstIndex, then testing if the result is != 0
		u32 secondLevelBitmap = m_secondLevelBitmap[ firstIndex ] & ( ~0u << secondIndex );
		if( secondLevelBitmap == 0 )	// No free blocks big enough in this list, search for the next largest free list
		{
			u32 firstLevelTopBits = m_firstLevelBitmap & ( ~0u <<  ( firstIndex + 1 ) );
			if( firstLevelTopBits == 0 )
			{
				// There are no free blocks big enough to hold the requested size. Try searching for one.
				return SearchFreeBlock( size );
			}  
			firstIndex = BitUtils::BitScanForward( firstLevelTopBits );
			secondLevelBitmap = m_secondLevelBitmap[ firstIndex ];
		}

		// By this point the first index should definitely be valid, so check its secondary free lists
		secondIndex = red::BitUtils::BitScanForward( secondLevelBitmap );

		// Return the head of the free list found. This block is guaranteed to be >= size
		return GetFreeListHead( firstIndex, secondIndex );
	}

	TLSFBlockHeader * TLSFAllocator::SearchFreeBlock( u32 size )
	{
		// At this stage, we could not find using fast segregation list look up. 
		// However ... There is slight chance block of correct size exist!
		// brute force Search previous segregation list. First fit! 

		u32 firstIndex = 0, secondIndex = 0;
		GetListIndices( size, firstIndex, secondIndex );
		TLSFBlockHeader * foundBlock = GetFreeListHead( firstIndex, secondIndex );
		while( foundBlock )
		{
			const u64 blockSize = foundBlock->GetDataSize();
			if( blockSize >= size )
			{
				return foundBlock;
			}
				
			foundBlock = foundBlock->GetNextFree();
		}

		return nullptr;
	}

	void TLSFAllocator::FreeBlock( TLSFBlockHeader * block )
	{
		RED_MEMORY_ASSERT( !block->IsFreeBlock(), "Attempting to free a block that is already free!" );
		block->SetFreeState( c_tlsfBlockIsFree );
		block->GetNextPhysicalBlock()->SetPreviousPhysicalState( c_tlsfPreviousBlockIsFree );
		block->SetFreeListPointers( nullptr, nullptr );

		// Merge the block with any free neighbors
		MergeFreeBlock( block );
	}

	void TLSFAllocator::MergeFreeBlock( TLSFBlockHeader * block )
	{
		RED_MEMORY_ASSERT( block->IsFreeBlock(), "Merging a used block!" );

		TLSFBlockHeader* nextBlock = block->GetNextPhysicalBlock();
		nextBlock->VerifyBlockState();
		if( nextBlock->IsFreeBlock() )
		{
			RemoveBlockFromFreeList( nextBlock );	// Remove it from its free list
			block->SetSize( block->GetTotalSize() + nextBlock->GetTotalSize() );	// grow the previous block by the next block size
		}

		if( block->IsPreviousBlockFree() )
		{
			TLSFBlockHeader* previousBlock = block->GetPreviousPhysical();
			RED_MEMORY_ASSERT( previousBlock->IsFreeBlock(), "Bookkeeping is broken somewhere!" );
			RemoveBlockFromFreeList( previousBlock );	// Remove it from free list

			// Since we are merging with a previous block, we don't actually need the currently freed block any more
			// It will become part of the new larger block's data. Instead, we take the previous block and add it back to the free list
			previousBlock->SetSize( previousBlock->GetTotalSize() + block->GetTotalSize() );
			block = previousBlock;	// we don't need the current block any more
		}
		block->VerifyBlockState();
		AddBlockToFreeList( block );

		// Update the next physical block and set its previous_free flag
		TLSFBlockHeader* nextPhysical = block->GetNextPhysicalBlock();
		nextPhysical->SetPreviousPhysical( block );
		nextPhysical->SetPreviousPhysicalState( c_tlsfPreviousBlockIsFree );
		nextPhysical->VerifyBlockState();
	}


	TLSFBlockHeader * TLSFAllocator::ShrinkBlock( TLSFBlockHeader * foundBlock, u32 size, u32 alignment )
	{
		// calculate the padding required so we can decide if/how to split the block most efficiently
		u64 dataStartAddress = foundBlock->GetDataStartAddress();
		u32 addressAlignment = dataStartAddress & ( alignment - 1 );
		u32 blockPadding = 0;
		if( addressAlignment != 0 )
		{
			blockPadding = alignment - addressAlignment;

			// if the padding required < block size, we need to move the start address on by max(free block, alignment) + padding
			if( blockPadding > 0 && blockPadding < c_tlsfFreeBlockHeaderSize )
			{
				blockPadding = std::max( alignment, c_tlsfFreeBlockHeaderSize ) + blockPadding;
			}
		}

		u32 blockStartOffset = blockPadding;
		RED_MEMORY_ASSERT( blockStartOffset + size <= foundBlock->GetDataSize(), "Alignment padding has gone wrong! The block is too small" );

		// split the found block if required
		TLSFBlockHeader* newBlock;
		if( blockStartOffset > 0 )
		{
			newBlock = SplitBlock( foundBlock, blockStartOffset - c_tlsfUsedBlockHeaderSize );
			RED_MEMORY_ASSERT( newBlock, "Failed to split the free block" );

			// Free the start padding block
			FreeBlock( foundBlock );
			foundBlock->VerifyBlockState();
			newBlock->VerifyBlockState();
		}
		else
		{
			newBlock = foundBlock;
		}

		// if there is room, chop off the end and add another free block
		u32 blockFreeSpace = newBlock->GetDataSize() - size;
		if( blockFreeSpace > c_tlsfFreeBlockHeaderSize )
		{
			TLSFBlockHeader* endPadBlock = SplitBlock( newBlock, size );

			// Free the end padding block
			FreeBlock( endPadBlock );
			endPadBlock->VerifyBlockState();
		}

		RED_MEMORY_ASSERT( !newBlock->IsFreeBlock(), "The new block is not set to used" );
		RED_MEMORY_ASSERT( newBlock->GetDataSize() >= size, "Newly allocated block is not big enough for the size requested! Something has gone wrong with bookkeeping" );
		RED_MEMORY_ASSERT( IsAligned( newBlock->GetDataStartAddress(), alignment ), "Newly allocated block is not aligned properly" );

		newBlock->VerifyBlockState();

		return newBlock;
	}

	TLSFBlockHeader * TLSFAllocator::GrowBlock( TLSFBlockHeader * blockHeader, u32 requiredSize, u32 /*alignment*/ )
	{
		// TODO this function is buggy. Also alignment is not even considered.
		TLSFBlockHeader * nextPhysicalBlock = blockHeader->GetNextPhysicalBlock();
		const u32 blockDataSize = blockHeader->GetDataSize();
		if( nextPhysicalBlock->IsFreeBlock() )
		{
			u32 availableFreeMemory = nextPhysicalBlock->GetDataSize() + blockDataSize; 
			if( availableFreeMemory >= requiredSize )
			{
				RemoveBlockFromFreeList( nextPhysicalBlock );
				blockHeader->SetSize( blockHeader->GetTotalSize() + nextPhysicalBlock->GetTotalSize() );
				nextPhysicalBlock = blockHeader->GetNextPhysicalBlock();
				nextPhysicalBlock->SetPreviousPhysical( blockHeader );
				nextPhysicalBlock->SetPreviousPhysicalState( c_tlsfPreviousBlockIsUsed );
				TLSFBlockHeader * endBlock = SplitBlock( blockHeader, requiredSize );
				FreeBlock( endBlock );
				return blockHeader;
			}
		}

		return nullptr;
	}

	TLSFBlockHeader * TLSFAllocator::SplitBlock( TLSFBlockHeader * block, u32 splitDataOffset )
	{
		// Split the block to create 2 new free blocks
		// The new block takes the remainder of the space in the block
		u64 newBlockAddress = block->GetDataStartAddress() + splitDataOffset;
		u32 newBlockSize = block->GetDataSize() - splitDataOffset;
		RED_MEMORY_ASSERT( newBlockSize <= block->GetDataSize(), "The block split offset is too large!" );

		// Cache all interesting info from theBlock into registers asap
		u32 isPreviousFree = block->IsFreeBlock() ? c_tlsfPreviousBlockIsFree : c_tlsfPreviousBlockIsUsed;
		u32 isBlockFree = block->IsFreeBlock() ? c_tlsfBlockIsFree : c_tlsfBlockIsUsed;
		TLSFBlockHeader* nextPhysical = block->GetNextPhysicalBlock();

		// Write this now; we won't touch it again
		block->SetSize( block->GetTotalSize() - newBlockSize );

		// Create a new block at the offset address. Inherit the used state from theBlock
		TLSFBlockHeader* newBlock = reinterpret_cast< TLSFBlockHeader* >( newBlockAddress );
		newBlock->SetPreviousPhysical( block );
		newBlock->SetSizeAndState( newBlockSize, isPreviousFree, isBlockFree );
		newBlock->SetFreeListPointers( nullptr, nullptr );

		// Fix up the next blocks previous physical pointer
		nextPhysical->SetPreviousPhysical( newBlock );
		nextPhysical->SetPreviousPhysicalState( ( isBlockFree == c_tlsfBlockIsFree ) ? c_tlsfPreviousBlockIsFree : c_tlsfPreviousBlockIsUsed );

		nextPhysical->VerifyBlockState();
		block->VerifyBlockState();
		newBlock->VerifyBlockState();

		return newBlock;
	}

	void TLSFAllocator::RemoveBlockFromFreeList( TLSFBlockHeader * block )
	{
		// Extract the block from its free list
		if( block->GetPreviousFree() != nullptr )
		{
			block->GetPreviousFree()->SetNextFree( block->GetNextFree() );
		}
		if( block->GetNextFree() != nullptr )
		{
			block->GetNextFree()->SetPreviousFree( block->GetPreviousFree() );
		}	

		// If the block is the first one in its free list, move the free list head along
		u32 firstIndex = 0, secondIndex = 0;
		GetListIndices( block->GetDataSize(), firstIndex, secondIndex );
		TLSFBlockHeader* freeListHead = GetFreeListHead( firstIndex, secondIndex );
		if( freeListHead == block )
		{
			freeListHead = block->GetNextFree();
			SetFreeListHead( freeListHead, firstIndex, secondIndex );

			// If the free list is now empty, clear the bitmap 
			if( freeListHead == nullptr )
			{
				ClearFreelistBitmap( firstIndex, secondIndex );
			}
		}

		// Finally clear the free list pointers for the block
		block->SetFreeListPointers( nullptr, nullptr );
	}


	void TLSFAllocator::AddBlockToFreeList( TLSFBlockHeader * block )
	{
		RED_MEMORY_ASSERT( block->IsFreeBlock(), "Cannot add a used block to the free list!" );

		// Calculate which list the block belongs to
		u32 firstIndex = 0, secondIndex = 0;
		GetListIndices( block->GetDataSize(), firstIndex, secondIndex );

		// Add the block to the head of its free list
		TLSFBlockHeader* freeListHead = GetFreeListHead( firstIndex, secondIndex );
		block->SetPreviousFree( nullptr );
		block->SetNextFree( freeListHead );
		if( freeListHead )
		{
			freeListHead->SetPreviousFree( block );
		}
		SetFreeListHead( block, firstIndex, secondIndex );

		// Update the bitmaps
		SetFreeListBitmap( firstIndex, secondIndex );
		block->VerifyBlockState();
	}

	void TLSFAllocator::GetListIndices( u32 size, u32 & firstLevel, u32 & secondLevel )
	{
		//	This is the heart of the TLSF allocator
		//	For a given size, 2 indices will be returned that can be used to 
		//  find the matching free list and bitmap indices

		u32 linearSegregationLevelLog2 = m_secondLevelIndexLog2;
		u32 linearSegregationLevel = m_secondLevelIndexWidth;

		// Special case for small allocations
		if( size < c_tlsfSmallestSegregatedSize )
		{
			firstLevel = 0;
			secondLevel = size / ( c_tlsfSmallestSegregatedSize / linearSegregationLevel );
		}
		else
		{
			firstLevel = BitUtils::Log2( size );
			secondLevel = ( size >> ( firstLevel - linearSegregationLevelLog2 ) ) - linearSegregationLevel;
			firstLevel = firstLevel - c_tlsfSmallestSegregatedSizeLog2 + 1;	// Take into account the special case small alloc lists
		}
	}

	TLSFBlockHeader * TLSFAllocator::GetFreeListHead( u32 firstLevel, u32 secondLevel )
	{
		RED_MEMORY_ASSERT( firstLevel < m_firstLevelIndexWidth && secondLevel < m_secondLevelIndexWidth, "Bad free list indices" );
		return m_freeListMatrix[ firstLevel + ( secondLevel * m_firstLevelIndexWidth ) ];
	}

	void TLSFAllocator::SetFreeListHead( TLSFBlockHeader* newHead, u32 firstLevel, u32 secondLevel )
	{
		RED_MEMORY_ASSERT( firstLevel < m_firstLevelIndexWidth && secondLevel < m_secondLevelIndexWidth, "Bad free list indices" );
		m_freeListMatrix[ firstLevel + ( secondLevel * m_firstLevelIndexWidth ) ] = newHead;
	}

	void TLSFAllocator::ClearFreelistBitmap( u32 firstLevel, u32 secondLevel )
	{
		//	Clear a bit in the corresponding second-level free list bitmap
		//	If the bitmap is empty after clearing the bit, the first level bit
		//  is also cleared

		RED_MEMORY_ASSERT( firstLevel < m_firstLevelIndexWidth && secondLevel < m_secondLevelIndexWidth,  "Bad free list indices"  );
		FreeListBitmap& secondLevelBitmap = m_secondLevelBitmap[ firstLevel ];
		secondLevelBitmap &= ~( 1 << secondLevel );

		if( secondLevelBitmap == 0 )
		{
			// There are no more free lists for the first level index, clear the first-level bitmap
			m_firstLevelBitmap &= ~( 1 << firstLevel );
		}
	}

	void TLSFAllocator::SetFreeListBitmap( u32 firstLevel, u32 secondLevel )
	{
		//	Update the first and second level bitmaps
		RED_MEMORY_ASSERT( firstLevel < m_firstLevelIndexWidth && secondLevel < m_secondLevelIndexWidth,  "Bad free list indices" );
		FreeListBitmap& secondLevelBitmap = m_secondLevelBitmap[ firstLevel ];
		secondLevelBitmap |= ( 1 << secondLevel );
		m_firstLevelBitmap |= ( 1 << firstLevel );
	}

	void TLSFAllocator::AddArea( SystemBlock areaBlock )
	{
		RED_MEMORY_ASSERT( areaBlock.address - m_reservedMemoryRange.start < GetVirtualRangeSize( m_reservedMemoryRange ),
			"Area need to be within reserved Virtual Range." );
	
		TLSFAreaHeader * newArea = InitializeArea( areaBlock.address, areaBlock.size );
		RED_MEMORY_ASSERT( newArea->lastBlock != nullptr, "Failed to create a new area. Most likely out of physical memory or virtual address space" );
		MergeArea( newArea );	
	}

	// Walk the area list, merging any areas that need it. Any merged areas are removed from the area list
	// Note, this assumes the new area has NOT already been added to the areas list!
	void TLSFAllocator::MergeArea( TLSFAreaHeader * area )
	{
		TLSFAreaHeader * currentArea = m_areaHead;
		TLSFAreaHeader * previousArea = nullptr;
		while( currentArea != nullptr )
		{
			u64 areaBlockHeaderPosition = reinterpret_cast< u64 >( area ) - c_tlsfUsedBlockHeaderSize;
			u64 currentAreaEndAddress = reinterpret_cast< u64 >( currentArea->lastBlock ) + c_tlsfFreeBlockHeaderSize;
			if( currentAreaEndAddress == areaBlockHeaderPosition )
			{
				// Provided area is contiguous to current area, merge them together.
				currentArea = MergeContiguousAreas( currentArea, area );
				if( m_areaHead == currentArea )
				{
					m_areaHead = currentArea->nextArea;
				}
				else if( previousArea && currentArea )
				{
					previousArea->nextArea = currentArea->nextArea;
				}
				  
				area = currentArea;
			}

			previousArea = currentArea;
			currentArea = currentArea->nextArea;
		}

		area->nextArea = m_areaHead;
		m_areaHead = area;

		RED_MEMORY_ASSERT( m_areaHead->nextArea == nullptr, "INTERNAL ERROR. TLSF Added Area is not contiguous to previous area." );
	}

	TLSFAllocator::TLSFAreaHeader* TLSFAllocator::MergeContiguousAreas( TLSFAreaHeader * firstArea, TLSFAreaHeader * secondArea )
	{
		u64 secondAreaBlockPosition = reinterpret_cast< u64 >( secondArea ) - c_tlsfUsedBlockHeaderSize;
		TLSFBlockHeader * secondAreaBlockHeader = reinterpret_cast< TLSFBlockHeader* >( secondAreaBlockPosition );
		TLSFBlockHeader * firstLastBlock = firstArea->lastBlock;
		TLSFBlockHeader * secondLastBlock = secondArea->lastBlock;

		RED_MEMORY_ASSERT( firstLastBlock->GetDataStartAddress() + c_tlsfUsedBlockHeaderSize == secondAreaBlockPosition, 
			"Attempted to merge non-contiguous areas! This will most likely crash" );
		RED_MEMORY_ASSERT( firstLastBlock->GetTotalSize() == 0, "Previous area final block is invalid or already merged" );
		RED_MEMORY_ASSERT( !firstLastBlock->IsFreeBlock(), "Previous area final block should be used. Bookkeeping has gone bang" );
		RED_MEMORY_ASSERT( secondLastBlock->GetTotalSize() == 0, "Next area final block is invalid or already merged" );
		RED_MEMORY_ASSERT( !secondLastBlock->IsFreeBlock(), "Next area final block should be used. Bookkeeping has gone bang" );

		u64 newFreeBlockSize = reinterpret_cast< u64 >( secondAreaBlockHeader->GetNextPhysicalBlock() ) - reinterpret_cast< u64 >( firstLastBlock );
		firstLastBlock->SetFreeState( c_tlsfBlockIsFree );
		firstLastBlock->SetSize( static_cast< u32 >( newFreeBlockSize ) );
		firstLastBlock->GetNextPhysicalBlock()->SetPreviousPhysical( firstLastBlock );
		firstLastBlock->GetNextPhysicalBlock()->SetPreviousPhysicalState( c_tlsfPreviousBlockIsFree );

		MergeFreeBlock( firstLastBlock );
		firstArea->lastBlock = secondLastBlock;

		return firstArea;
	}

	bool TLSFAllocator::OwnBlock( u64 block ) const
	{
		RED_MEMORY_ASSERT( IsInitialized(), "TLSFAllocator was not intialized." );

		// Ok, this is "correct" but also incorrect. If virtual memory is not committed it will return true.
		// However if the memory was not committed then any access to this address will make cpu fire exception.
		return block - m_reservedMemoryRange.start < GetVirtualRangeSize( m_reservedMemoryRange ); 
	}

	bool TLSFAllocator::IsInitialized() const
	{
		return m_freeListMatrix != nullptr;
	}

	u32 ComputeTLSFBookKeepingSize( u32 maxAllocatedSize )
	{
		const u32 firstLevelIndexWidth = BitUtils::Log2( maxAllocatedSize ) - c_tlsfSmallestSegregatedSizeLog2 + 2;
		const u32 firstLevelFreeListSize = sizeof( u32 ) * firstLevelIndexWidth;
		const u32 secondLevelIndexWidth = c_tlsfMaximumSecondLevelDivisions;
		const u32 freeListMatrixSize = sizeof( void* ) * firstLevelIndexWidth * secondLevelIndexWidth;
		const u32 result = firstLevelFreeListSize + freeListMatrixSize;

		return result;
	}

	void TLSFAllocator::BuildMetrics( TLSFAllocatorMetrics & metrics )
	{
		auto & freeBlocks = metrics.freeBlocksMetrics;

		u64 largestBlockSize = 0;
		u64 smallestBlockSize = ~0ull;
		u64 totalAllocCount = 0;

		for( u32 firstLevelIndex = 0; firstLevelIndex != m_firstLevelIndexWidth; ++firstLevelIndex )
		{
			auto & freeBlock = freeBlocks[ firstLevelIndex ];
			freeBlock.blockSize = 1ull << ( firstLevelIndex + c_tlsfSmallestSegregatedSizeLog2 );

			for( u32 secondLevelIndex = 0; secondLevelIndex != m_secondLevelIndexWidth; ++ secondLevelIndex )
			{
				TLSFBlockHeader * listHead = GetFreeListHead( firstLevelIndex, secondLevelIndex );
				while( listHead )
				{
					++totalAllocCount;
					++freeBlock.totalCount;

					const u64 blockSize = listHead->GetDataSize();
					freeBlock.totalSize += blockSize;

					largestBlockSize = std::max( largestBlockSize, blockSize );
					smallestBlockSize = std::min( smallestBlockSize, blockSize );
					
					listHead = listHead->GetNextFree();
				}
			}
		}

		metrics.metrics.largestBlockSize = largestBlockSize;
		metrics.metrics.smallestBlockSize = smallestBlockSize;
	}
}
}
