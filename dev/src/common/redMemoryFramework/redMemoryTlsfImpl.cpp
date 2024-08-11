/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryTlsfImpl.h"
#include "redMemoryAllocatorInfo.h"
#include "redMemoryLog.h"
#include "../redSystem/error.h"
#include "../redSystem/bitUtils.h"
#include "../redMath/numericalutils.h"
#include <new>		// In order to use placement-new, this has to be included

//#define RED_MEMORY_ENABLE_TLSF_HEAP_DEBUGGING

namespace Red { namespace MemoryFramework { 

/////////////////////////////////////////////////////////////////////
// Pool Header CTor
//
TLSF::PoolHeader::PoolHeader( Red::System::Uint32 firstLevelBitmapWidth, Red::System::Uint32 secondLevelBitmapWidth, Red::System::Uint32 maximumSystemBlocks )
	: m_firstLevelIndexWidth( firstLevelBitmapWidth )
	, m_firstLevelBitmap( 0 )
	, m_secondLevelIndexWidth( secondLevelBitmapWidth )
	, m_secondLevelBitmap( nullptr )
	, m_freeListMatrix( nullptr )
	, m_areaHead( nullptr )
	, m_maximumSystemBlocks( maximumSystemBlocks )
	, m_systemBlockCount( 0 )
	, m_systemBlocks( nullptr )
{
	m_secondLevelIndexLog2 = Red::System::BitUtils::Log2( secondLevelBitmapWidth );
}

/////////////////////////////////////////////////////////////////////
// CTor
//
TLSFAllocatorImpl::TLSFAllocatorImpl( )
	: m_tlsfPoolHead( nullptr )
	, m_maximumSize( 0 )
	, m_initialPoolSize( 0 )
	, m_systemBlockSize( 0 )
{
}

/////////////////////////////////////////////////////////////////////
// DTor
//
TLSFAllocatorImpl::~TLSFAllocatorImpl()
{
}

/////////////////////////////////////////////////////////////////////
// WalkAllocator
// Walk each large area of the allocator
void TLSFAllocatorImpl::WalkAllocator( AllocatorWalker* theWalker )
{
	TLSF::AreaHeader* theArea = m_tlsfPoolHead->m_areaHead;
	while( theArea != nullptr )
	{
		Red::System::MemUint areaAddress = reinterpret_cast< Red::System::MemUint >( theArea ) - TLSF::c_UsedBlockHeaderSize;
		Red::System::MemSize areaSize = reinterpret_cast< Red::System::MemUint >( theArea->m_lastBlock ) - areaAddress;
		theWalker->OnMemoryArea( areaAddress, areaSize );
		theArea = theArea->m_nextArea;
	}
}

/////////////////////////////////////////////////////////////////////
// WalkPoolArea
// Walk all allocations in a particular area
void TLSFAllocatorImpl::WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker )
{
	// First, find the corresponding area header
	TLSF::AreaHeader* theArea = m_tlsfPoolHead->m_areaHead;
	TLSF::AreaHeader* matchedHeader = nullptr;
	while( theArea != nullptr )
	{
		Red::System::MemUint areaAddress = reinterpret_cast< Red::System::MemUint >( theArea ) - TLSF::c_UsedBlockHeaderSize;
		if( areaAddress == startAddress )
		{
			matchedHeader = theArea;
			break;
		}
		theArea = theArea->m_nextArea;
	}

	if( matchedHeader == nullptr )
	{
		return;
	}

	// Walk each block in the area. Merge any blocks with matching status
	TLSF::BlockHeader* blckHeader = reinterpret_cast< TLSF::BlockHeader* >( reinterpret_cast< Red::System::MemUint >( theArea ) - TLSF::c_UsedBlockHeaderSize );
	TLSF::BlockHeader* lastBlock = matchedHeader->m_lastBlock;

	Red::System::MemSize bytesProcessed = 0;
	Red::System::MemSize processingBlockSize = 0;
	Red::System::MemUint processingBlockAddress = reinterpret_cast< Red::System::MemUint >( blckHeader );
	Red::System::Bool processingBlockFreeState = blckHeader->IsFreeBlock();
	Red::System::Uint16 processingBlockMemoryClass = blckHeader->GetMemoryClass();

	while( blckHeader != lastBlock && bytesProcessed <= size )
	{
		Red::System::MemSize currentBlockSize = blckHeader->GetTotalSize();
		Red::System::Bool currentBlockIsFree = blckHeader->IsFreeBlock();
		bytesProcessed += currentBlockSize;

		Red::System::Uint16 currentBlockMemoryClass = blckHeader->GetMemoryClass();

		// If state changed, output the current size
		if( processingBlockFreeState != currentBlockIsFree || processingBlockMemoryClass != currentBlockMemoryClass )
		{
			if( processingBlockFreeState )
			{
				theWalker->OnFreeArea( processingBlockAddress, processingBlockSize );
			}
			else
			{
				theWalker->OnUsedArea( processingBlockAddress, processingBlockSize, processingBlockMemoryClass );
			}

			processingBlockSize = 0;
			processingBlockFreeState = currentBlockIsFree;
			processingBlockAddress = reinterpret_cast< Red::System::MemUint >( blckHeader );
			processingBlockMemoryClass = currentBlockMemoryClass;
		}
		else
		{
			RED_FATAL_ASSERT( processingBlockFreeState || processingBlockMemoryClass == currentBlockMemoryClass, "Memory class mismatch, %hu != %hu", processingBlockMemoryClass, currentBlockMemoryClass );
		}

		processingBlockSize += currentBlockSize;
		blckHeader = blckHeader->GetNextPhysicalBlock();
	}

	// Output the final block if there is anything there
	if( processingBlockSize > 0 )
	{
		if( processingBlockFreeState )
		{
			theWalker->OnFreeArea( processingBlockAddress, processingBlockSize );
		}
		else
		{
			theWalker->OnUsedArea( processingBlockAddress, processingBlockSize, blckHeader->GetMemoryClass() );
		}
	}
}

/////////////////////////////////////////////////////////////////////
// CalculateMemoryBookkeeping
//	Calculate how much memory is required for bookkeeping
Red::System::MemSize TLSFAllocatorImpl::CalculateMemoryBookkeeping( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity )
{
	Red::System::Uint32 firstLevelWidth = GetFirstLevelIndexWidthFromPoolSize( poolSize );
	return GetBookkeepingOverhead( poolSize, systemBlockSize, firstLevelWidth, segregationGranularity );
}

/////////////////////////////////////////////////////////////////////
// CalculateMemoryRequiredForPool
//	For a given pool size, calculate how much extra memory is needed for bookkeeping
Red::System::MemSize TLSFAllocatorImpl::CalculateMemoryRequiredForPool( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity )
{
	Red::System::Uint32 firstLevelWidth = GetFirstLevelIndexWidthFromPoolSize( poolSize );
	return poolSize + GetBookkeepingOverhead( poolSize, systemBlockSize, firstLevelWidth, segregationGranularity );
}

/////////////////////////////////////////////////////////////////////
// InitialisePool
//	Create a new TLSF pool at the specified address
TLSFAllocatorImpl::EPoolCreationResults TLSFAllocatorImpl::TestInitialParameters( Red::System::MemSize initialPoolSize, Red::System::MemSize maximumPoolSize, Red::System::MemSize systemSize, Red::System::Uint32 segregationGranularity )
{
	// Test that the pool parameters are valid (big enough for the allocator, aligned correctly, etc)
	Red::System::Bool isBigEnoughForHeader = initialPoolSize > sizeof( TLSF::PoolHeader );
	RED_MEMORY_ASSERT( isBigEnoughForHeader, "Pool size is small to contain TLSF bookkeeping data" );

	// calculate the maximum first level index width required for this pool size 
	Red::System::Uint32 firstLevelIndexWidth = GetFirstLevelIndexWidthFromPoolSize( maximumPoolSize );
	Red::System::Bool firstLevelWidthValid = firstLevelIndexWidth < TLSF::c_MaximumFirstLevelBits;
	RED_MEMORY_ASSERT( firstLevelWidthValid, "This pool is too big to be supported properly by TLSF" );
	
	// Test the segregation granularity 
	Red::System::Bool secondLevelWidthValid = segregationGranularity <= TLSF::c_MaximumSecondLevelDivisions;
	RED_MEMORY_ASSERT( secondLevelWidthValid, "Second level segregation granularity is too high! This would require >32 bit bitmaps!" );
	
	// From the FLI and SLI, we can calculate how much space is needed for everything
	Red::System::MemSize totalRequired = CalculateMemoryBookkeeping( maximumPoolSize, systemSize, segregationGranularity );
	Red::System::Bool isPoolBigEnough = initialPoolSize >= totalRequired && systemSize >= totalRequired;

	// Test that we have enough space for all the bookkeeping + the pool itself
	RED_MEMORY_ASSERT( isPoolBigEnough, "The pool is not big enough to store bookkeeping data (initial or system block size too small)" );

	if( !firstLevelWidthValid || !secondLevelWidthValid )
	{
		return Pool_BadParameters;
	}

	if( !isBigEnoughForHeader || !isPoolBigEnough )
	{
		return Pool_MemoryTooSmall;
	}

	return Pool_OK;
}

/////////////////////////////////////////////////////////////////////
// InitialisePoolHeader
//	Initialise the internal data for the pool header
//	Returns the address of the pool memory after the bookkeeping stuff
Red::System::MemUint TLSFAllocatorImpl::InitialisePoolHeader()
{
	// Calculate the first usable address after the header
	void* poolStartAddress = static_cast< void* >( m_tlsfPoolHead );
	Red::System::MemUint bookKeepingAddress = reinterpret_cast< Red::System::MemUint >( poolStartAddress ) + sizeof( *m_tlsfPoolHead );
	void* bookKeepingPtr = reinterpret_cast< void* >( bookKeepingAddress );
	RED_MEMORY_ASSERT( IsValueAligned( bookKeepingAddress, TLSF_BLOCK_HEADER_ALIGNMENT ), "Bad bookkeeping alignment" );

	// Create the second level bitmap pointers
	m_tlsfPoolHead->m_secondLevelBitmap = new( bookKeepingPtr ) TLSF::FreelistBitmap[ m_tlsfPoolHead->m_firstLevelIndexWidth ];
	bookKeepingAddress += sizeof( TLSF::FreelistBitmap ) * m_tlsfPoolHead->m_firstLevelIndexWidth;

	// Set them all to 0
	Red::System::MemorySet( m_tlsfPoolHead->m_secondLevelBitmap, 0, sizeof( TLSF::FreelistBitmap )* m_tlsfPoolHead->m_firstLevelIndexWidth );

	// Align up 
	bookKeepingAddress = ( bookKeepingAddress + TLSF_BLOCK_HEADER_ALIGNMENT - 1 ) & ~( TLSF_BLOCK_HEADER_ALIGNMENT - 1 );
	bookKeepingPtr = reinterpret_cast< void* >( bookKeepingAddress );

	// Create the free-list matrix
	m_tlsfPoolHead->m_freeListMatrix = new( bookKeepingPtr ) TLSF::BlockHeader*[ m_tlsfPoolHead->m_firstLevelIndexWidth * m_tlsfPoolHead->m_secondLevelIndexWidth ];
	bookKeepingAddress += sizeof( TLSF::BlockHeader* ) * m_tlsfPoolHead->m_firstLevelIndexWidth * m_tlsfPoolHead->m_secondLevelIndexWidth;
	Red::System::MemorySet( m_tlsfPoolHead->m_freeListMatrix, 0, sizeof( TLSF::BlockHeader* ) * m_tlsfPoolHead->m_firstLevelIndexWidth * m_tlsfPoolHead->m_secondLevelIndexWidth );

	bookKeepingAddress = ( bookKeepingAddress + TLSF_BLOCK_HEADER_ALIGNMENT - 1 ) & ~( TLSF_BLOCK_HEADER_ALIGNMENT - 1 );
	bookKeepingPtr = reinterpret_cast< void* >( bookKeepingAddress );

	// Create the system block list
	m_tlsfPoolHead->m_systemBlocks = new( bookKeepingPtr ) TLSF::SystemMemoryBlock[ m_tlsfPoolHead->m_maximumSystemBlocks ];
	bookKeepingAddress += sizeof( TLSF::SystemMemoryBlock ) * m_tlsfPoolHead->m_maximumSystemBlocks;
	Red::System::MemorySet( m_tlsfPoolHead->m_systemBlocks, 0, sizeof( TLSF::SystemMemoryBlock ) * m_tlsfPoolHead->m_maximumSystemBlocks );
	RED_MEMORY_ASSERT( IsValueAligned( bookKeepingAddress, TLSF_BLOCK_HEADER_ALIGNMENT ), "Bad bookkeeping alignment" );

	return bookKeepingAddress;
}

/////////////////////////////////////////////////////////////////////
// MergeAreas
//	Merge 2 areas that are contiguous and returns the new merged area
//  Does NOT remove the areas from the area bookkeeping
TLSF::AreaHeader* TLSFAllocatorImpl::MergeContiguousAreas( TLSF::AreaHeader* header0, TLSF::AreaHeader* header1 )
{
	TLSF::BlockHeader* areaBlockHeader1 = reinterpret_cast< TLSF::BlockHeader* >( reinterpret_cast< Red::System::MemUint >( header1 ) - TLSF::c_UsedBlockHeaderSize );
	TLSF::BlockHeader* lastBlock0 = header0->m_lastBlock;	
	TLSF::BlockHeader* lastBlock1 = header1->m_lastBlock;

	RED_MEMORY_ASSERT( reinterpret_cast< Red::System::MemUint >( lastBlock0->GetDataStartAddress() ) + TLSF::c_UsedBlockHeaderSize == reinterpret_cast< Red::System::MemUint >( areaBlockHeader1 ), 
				"Attempted to merge non-contiguous areas! This will most likely crash" );
	RED_MEMORY_ASSERT( lastBlock0->GetTotalSize() == 0, "Previous area final block is invalid or already merged" );
	RED_MEMORY_ASSERT( !lastBlock0->IsFreeBlock(), "Previous area final block should be used. Bookkeeping has gone bang" );
	RED_MEMORY_ASSERT( lastBlock1->GetTotalSize() == 0, "Next area final block is invalid or already merged" );
	RED_MEMORY_ASSERT( !lastBlock1->IsFreeBlock(), "Next area final block should be used. Bookkeeping has gone bang" );

	// Expand over the previous sentinel block and the next area header block
	Red::System::MemSize newFreeBlockSize = reinterpret_cast< Red::System::MemUint >( areaBlockHeader1->GetNextPhysicalBlock() ) - reinterpret_cast< Red::System::MemUint >( lastBlock0 );
	lastBlock0->SetFreeState( TLSF::c_BlockIsFree );
	lastBlock0->SetSize( newFreeBlockSize );
	lastBlock0->GetNextPhysicalBlock()->SetPreviousPhysical( lastBlock0 );
	lastBlock0->GetNextPhysicalBlock()->SetPreviousPhysicalState( TLSF::c_PreviousBlockIsFree );
	lastBlock0->VerifyBlockState();

	// Merge the new block with neighbours
	MergeFreeBlock( lastBlock0 );

	// Fix up the new area sentinel pointer
	header0->m_lastBlock = lastBlock1;

	return header0;
}

/////////////////////////////////////////////////////////////////////
// MergeAreas
//	Take a new area and merge it with any existing areas
//  Both the physical blocks and free lists will be merged
//	Note, this assumes the new area has NOT already been added to the areas list!
void TLSFAllocatorImpl::MergeNewArea( TLSF::AreaHeader* newHeader )
{
	// Walk the area list, merging any areas that need it. Any merged areas are removed from the area list
	TLSF::AreaHeader* currentHeader = m_tlsfPoolHead->m_areaHead;
	TLSF::AreaHeader* previousHeader = nullptr;
	while( currentHeader != nullptr )
	{
		// Find the area block header info
		TLSF::BlockHeader* areaBlockHeader = reinterpret_cast< TLSF::BlockHeader* >( reinterpret_cast< Red::System::MemUint >( newHeader ) - TLSF::c_UsedBlockHeaderSize );
		Red::System::MemUint newAreaStartAddress = reinterpret_cast< Red::System::MemUint >( areaBlockHeader );

		// Merge with previous area
		Red::System::MemUint areaEndAddress = reinterpret_cast< Red::System::MemUint >( currentHeader->m_lastBlock ) + TLSF::c_FreeBlockHeaderSize;
		if( areaEndAddress == newAreaStartAddress )		
		{
			currentHeader = MergeContiguousAreas( currentHeader, newHeader );

			// Remove the merged area from the areas list
			if( m_tlsfPoolHead->m_areaHead == currentHeader )
			{
				m_tlsfPoolHead->m_areaHead = currentHeader->m_nextArea;
			}
			else if( previousHeader != nullptr && currentHeader != nullptr )
			{
				previousHeader->m_nextArea = currentHeader->m_nextArea;
			}

			newHeader = currentHeader;
		}

		previousHeader = currentHeader;
		currentHeader = currentHeader->m_nextArea;
	}

	// Insert the new area at the head of the list
	newHeader->m_nextArea = m_tlsfPoolHead->m_areaHead;
	m_tlsfPoolHead->m_areaHead = newHeader;
}

/////////////////////////////////////////////////////////////////////
// InitialiseArea
//	Prepare this area for use by adding a area header block, 
//  a large free block (put into the bookkeeping),
//  and a area-end sentinel block
TLSF::AreaHeader* TLSFAllocatorImpl::InitialiseArea( void* areaStartAddress, Red::System::MemSize areaSize )
{
	RED_MEMORY_ASSERT( areaStartAddress != nullptr, "Cannot initialise a null area" );
	RED_MEMORY_ASSERT( IsValueAligned( reinterpret_cast< Red::System::MemUint >( areaStartAddress ), TLSF_BLOCK_HEADER_ALIGNMENT ), "Area start address is not aligned correctly" );

	// The area needs to be big enough for 2 sentinel blocks + the actual data
	RED_MEMORY_ASSERT( areaSize > ( TLSF::c_FreeBlockHeaderSize * 3 ), "Trying to initialise a area but its too small to contain anything" );

	// The first block will be set to used and contain the area header
	Red::System::MemSize firstBlockSize = Red::Math::NumericalUtils::Max( sizeof( TLSF::AreaHeader ), TLSF::c_FreeBlockHeaderSize );
	TLSF::BlockHeader* firstBlock = reinterpret_cast< TLSF::BlockHeader* >( areaStartAddress );
	firstBlock->SetSizeAndState( firstBlockSize, TLSF::c_PreviousBlockIsUsed, TLSF::c_BlockIsUsed );
	firstBlock->SetFreeListPointers( nullptr, nullptr );
	firstBlock->SetPreviousPhysical( nullptr );
	firstBlock->VerifyBlockState();

	// The second block will be the actual free data
	// We create it as a used block, then use FreeBlock() to add it to the bookkeeping properly
	Red::System::MemSize freeBlockSize = areaSize - firstBlockSize - TLSF::c_FreeBlockHeaderSize;
	TLSF::BlockHeader* blockToFree = firstBlock->GetNextPhysicalBlock();
	blockToFree->SetSizeAndState( freeBlockSize, TLSF::c_PreviousBlockIsUsed, TLSF::c_BlockIsUsed );
	blockToFree->SetPreviousPhysical( firstBlock );
	blockToFree->SetFreeListPointers( nullptr, nullptr );
	blockToFree->VerifyBlockState();

	// The final block acts as a sentinel and stops merges from running off the end of the area
	TLSF::BlockHeader* finalBlock = blockToFree->GetNextPhysicalBlock();
	finalBlock->SetSizeAndState( 0, TLSF::c_PreviousBlockIsUsed, TLSF::c_BlockIsUsed );
	finalBlock->SetFreeListPointers( nullptr, nullptr );
	finalBlock->SetPreviousPhysical( blockToFree );
	finalBlock->VerifyBlockState();

	// Ensure the final block hasn't stomped over the end of the buffer
	RED_MEMORY_ASSERT( reinterpret_cast< Red::System::MemUint >( finalBlock ) + TLSF::c_FreeBlockHeaderSize <= 
				reinterpret_cast< Red::System::MemUint >( areaStartAddress ) + areaSize, 
				"Final sentinel block pushed over the end of the area buffer!" );

	// Free the middle block. This sets up the bookkeeping so that block can be used for allocations
	FreeBlock( blockToFree );

	// Finally, initialise the area info
	TLSF::AreaHeader* areaHeader = reinterpret_cast< TLSF::AreaHeader* >( firstBlock->GetDataStartAddress() );
	areaHeader->m_lastBlock = finalBlock;
	areaHeader->m_nextArea = nullptr;

	return areaHeader;
}

/////////////////////////////////////////////////////////////////////
// FreeSystemMemoryBlock
//	Return a block of memory to the OS, removes it from the bookkeeping
void TLSFAllocatorImpl::FreeSystemMemoryBlock( AllocatorAreaCallback& areaCallback, Red::System::Uint32 blockIndex )
{
	RED_MEMORY_ASSERT( blockIndex < m_tlsfPoolHead->m_systemBlockCount, "Invalid system block index" );
	RED_MEMORY_ASSERT( m_tlsfPoolHead->m_systemBlockCount > 0, "No blocks to remove!" );

	areaCallback.OnAreaRemoved( m_tlsfPoolHead->m_systemBlocks[ blockIndex ].m_address, m_tlsfPoolHead->m_systemBlocks[ blockIndex ].m_address + m_tlsfPoolHead->m_systemBlocks[ blockIndex ].m_allocatedSize );
	PageAllocator::GetInstance().FreePagedMemory( reinterpret_cast< void* >( m_tlsfPoolHead->m_systemBlocks[ blockIndex ].m_address ), m_tlsfPoolHead->m_systemBlocks[ blockIndex ].m_allocatedSize, m_flags );

	// Fast delete, swap the delete block with the tail
	m_tlsfPoolHead->m_systemBlocks[ blockIndex ] = m_tlsfPoolHead->m_systemBlocks[ m_tlsfPoolHead->m_systemBlockCount - 1 ];
	m_tlsfPoolHead->m_systemBlockCount--;
}

/////////////////////////////////////////////////////////////////////
// AllocateSystemMemory
//	Returns a block of system memory and adds it to the internal list
TLSF::SystemMemoryBlock TLSFAllocatorImpl::AllocateSystemMemory()
{
	TLSF::SystemMemoryBlock newMemoryBlock;
	if( m_tlsfPoolHead->m_systemBlockCount < m_tlsfPoolHead->m_maximumSystemBlocks )
	{
		Red::System::MemSize sizeAllocated = 0u;
		void* memoryAddress = PageAllocator::GetInstance().GetPagedMemory( m_systemBlockSize, sizeAllocated, m_flags );
		RED_MEMORY_ASSERT( memoryAddress, "Failed to allocate a block of system memory. Out of memory?" );

		if( memoryAddress )
		{
			newMemoryBlock.m_address = reinterpret_cast< Red::System::MemUint >( memoryAddress );
			newMemoryBlock.m_allocatedSize = sizeAllocated;

			m_tlsfPoolHead->m_systemBlocks[ m_tlsfPoolHead->m_systemBlockCount++ ] = newMemoryBlock;
		}
	}
	else
	{
		RED_HALT( "Trying to allocate a block of system memory when the bookkeeping list isn't big enough");
	}

	return newMemoryBlock;
}

/////////////////////////////////////////////////////////////////////
// Initialise
//	Allocate initial memory and set up the allocator header
TLSFAllocatorImpl::EPoolCreationResults TLSFAllocatorImpl::Initialise( Red::System::MemSize initialPoolSize, Red::System::MemSize maximumPoolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity, Red::System::Uint32 flags )
{
	m_systemBlockSize = systemBlockSize;
	m_maximumSize = maximumPoolSize;
	m_flags = flags;

	if( m_flags & Allocator_StaticSize )		// Static pools allocate the maximum size on startup
	{
		initialPoolSize = maximumPoolSize;
	}

	// First, make sure the parameters are all valid
	EPoolCreationResults parameterTestResult = TestInitialParameters( initialPoolSize, maximumPoolSize, systemBlockSize, segregationGranularity );
	if( parameterTestResult != Pool_OK )
	{
		return parameterTestResult;
	}

	// Make sure we don't make the pool too big if system block > initial size - system block should be big enough for the bookkeeping at this point
	Red::System::MemSize firstBlockSize = Red::Math::NumericalUtils::Min( systemBlockSize, initialPoolSize );
	Red::System::Uint32 firstLevelIndexWidth = GetFirstLevelIndexWidthFromPoolSize( maximumPoolSize );
	
	// Allocate the first block and set up the tlsf header
	Red::System::MemSize initialBlockActualSize = 0u;
	void* firstBlock = PageAllocator::GetInstance().GetPagedMemory( firstBlockSize, initialBlockActualSize, m_flags );
	m_initialPoolSize = initialBlockActualSize;
	RED_MEMORY_ASSERT( firstBlock, "Failed to allocate initial tlsf header block" );
	if( firstBlock == nullptr )
	{
		return Pool_OutOfMemory;
	}

	// Initialise the pool header
	Red::System::Uint32 systemBlocksRequired = Red::Math::NumericalUtils::Max( 1u, static_cast< Red::System::Uint32 >( maximumPoolSize / systemBlockSize ) );
	RED_MEMORY_ASSERT( systemBlocksRequired * systemBlockSize >= maximumPoolSize, "Bad system block count" );

	m_tlsfPoolHead = new ( firstBlock ) TLSF::PoolHeader( firstLevelIndexWidth, segregationGranularity, systemBlocksRequired );
	Red::System::MemUint areaStartAddress = InitialisePoolHeader();
	Red::System::MemSize firstAreaSize = initialBlockActualSize - ( areaStartAddress - reinterpret_cast< Red::System::MemUint >( firstBlock ) );
	m_tlsfPoolHead->m_areaHead = InitialiseArea( reinterpret_cast< void* >( areaStartAddress ), firstAreaSize );

	// Now allocate any more system blocks we need to meet the initial size
	while( initialBlockActualSize < initialPoolSize )
	{
		TLSF::SystemMemoryBlock newBlock = AllocateSystemMemory();
		RED_MEMORY_ASSERT( newBlock.m_address != 0u, "Failed to allocate memory for memory pool initial size" );
		if( newBlock.m_address == 0u )
		{
			return Pool_OutOfMemory;
		}

		AddArea( reinterpret_cast< void* >( newBlock.m_address ), newBlock.m_allocatedSize );
		initialBlockActualSize += newBlock.m_allocatedSize;
	}

	return Pool_OK;
}

/////////////////////////////////////////////////////////////////////
// AddArea
//	Add a new area, push it to the head of the areas list
void TLSFAllocatorImpl::AddArea( void* areaPtr, Red::System::MemSize areaSize )
{
	TLSF::AreaHeader* newHeader = InitialiseArea( areaPtr, areaSize );
	RED_MEMORY_ASSERT( newHeader->m_lastBlock != nullptr, "Failed to create a new area. Most likely out of physical memory or virtual address space" );
	MergeNewArea( newHeader );
}

/////////////////////////////////////////////////////////////////////
// ReleasePool
//	Cleanup a TLSF allocator for a specific pool
void TLSFAllocatorImpl::Release()
{
	// Free the memory used for each system block, then free the memory containing the pool bookkeeping block
	for( Red::System::Uint32 sysBlockIndex = 0; sysBlockIndex < m_tlsfPoolHead->m_systemBlockCount; ++sysBlockIndex )
	{
		PageAllocator::GetInstance().FreePagedMemory( reinterpret_cast< void* >( m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_address ), m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_allocatedSize, m_flags );
	}
	
	PageAllocator::GetInstance().FreePagedMemory( reinterpret_cast< void* >( m_tlsfPoolHead ), m_initialPoolSize, m_flags );
	m_tlsfPoolHead = nullptr;
}

/////////////////////////////////////////////////////////////////////
// FindContiguousSystemMemoryInBlock
//	Find a range of memory spanning system blocks inside the data
//	segment of theBlock
void TLSFAllocatorImpl::FindContiguousSystemMemoryInBlock( TLSF::BlockHeader* theBlock, Red::System::MemUint& lowAddress, Red::System::MemUint& highAddress )
{
	Red::System::MemUint lowFreeAddress = (Red::System::MemUint)-1;
	Red::System::MemUint highFreeAddress = 0u;
	Red::System::MemUint blockStartAddress = reinterpret_cast< Red::System::MemUint >( theBlock->GetDataStartAddress() );
	Red::System::MemUint blockEndAddress = blockStartAddress + theBlock->GetDataSize();

	for( Red::System::Uint32 sysBlockIndex = 0; sysBlockIndex < m_tlsfPoolHead->m_systemBlockCount; ++sysBlockIndex )
	{
		Red::System::MemUint sysBlockAddress = m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_address;
		Red::System::MemUint sysBlockHighAddress = sysBlockAddress + m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_allocatedSize;

		if( sysBlockAddress > blockStartAddress && sysBlockHighAddress < blockEndAddress )
		{
			lowFreeAddress = Red::Math::NumericalUtils::Min( lowFreeAddress, sysBlockAddress );
			highFreeAddress = Red::Math::NumericalUtils::Max( highFreeAddress, sysBlockHighAddress );
		}
	}

	lowAddress = lowFreeAddress;
	highAddress = highFreeAddress;
}

/////////////////////////////////////////////////////////////////////
// FreeSystemMemoryInRange
//	Release system blocks in the specified range back to the OS
Red::System::MemSize TLSFAllocatorImpl::FreeSystemMemoryInRange( AllocatorAreaCallback& areaCallback, Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
	Red::System::Uint32 sysBlockIndex = 0;
	Red::System::MemSize memoryFreed = 0;
	while( sysBlockIndex < m_tlsfPoolHead->m_systemBlockCount )
	{
		Red::System::MemUint sysBlockAddress = m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_address;
		Red::System::MemUint sysBlockHighAddress = sysBlockAddress + m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_allocatedSize;

		if( sysBlockAddress >= lowAddress && sysBlockHighAddress <= highAddress )
		{
			memoryFreed += m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_allocatedSize;

			FreeSystemMemoryBlock( areaCallback, sysBlockIndex );

			sysBlockIndex = 0;	// Reset the loop as the list has been modified
		}
		else
		{
			++sysBlockIndex;
		}
	}

	return memoryFreed;
}

/////////////////////////////////////////////////////////////////////
// SplitFreeBlockAndArea
//	Take a free block and remove a chunk defined by lowAddress and highAddress
//  Also fix up the containing area by adding a area-end sentinel before the freed area
//  and a new area block at the end
Red::System::Bool TLSFAllocatorImpl::SplitFreeBlockAndArea( TLSF::BlockHeader* freeBlock, Red::System::MemUint lowAddress, Red::System::MemUint highAddress )
{
	RED_MEMORY_ASSERT( freeBlock->IsFreeBlock(), "Cannot split block that is not free!" );
	freeBlock->VerifyBlockState();

	// If there is space before lowFreeAddress and after highFreeAddress for area end / area start blocks, then we can split the free block and the area it is contained in
	// we also make sure the free blocks at either side are a minimum of 64 bytes (to ensure something else can use the remaining blocks)
	const Red::System::MemSize smallestFreeBlock = Red::Math::NumericalUtils::Max( sizeof( TLSF::AreaHeader ), TLSF::c_FreeBlockHeaderSize + 64u );
	Red::System::MemUint freeBlockStartAddress = reinterpret_cast< Red::System::MemUint >( freeBlock->GetDataStartAddress() );
	Red::System::MemUint freeBlockEndAddress = freeBlockStartAddress + freeBlock->GetDataSize();
	if( ( lowAddress - freeBlockStartAddress ) <= TLSF::c_FreeBlockHeaderSize || ( freeBlockEndAddress - highAddress ) <= smallestFreeBlock )
	{
		return false;		// Gap is too small for area sentinel blocks
	}

	// Find the area containing this free block
	TLSF::AreaHeader* containingArea = FindAreaContainingBlock( freeBlock );
	RED_MEMORY_ASSERT( containingArea, "Could not find a matching area for a free block to be release. Bookeeping data corrupt?" );

	// Remove the block from the free-lists
	RemoveFreeBlock( freeBlock );

	// Add a new area-end block behind the startAddress
	TLSF::BlockHeader* oldAreaEnd = reinterpret_cast< TLSF::BlockHeader* >( lowAddress - TLSF::c_FreeBlockHeaderSize );
	oldAreaEnd->SetPreviousPhysical( freeBlock );
	oldAreaEnd->SetSizeAndState( 0, TLSF::c_PreviousBlockIsFree, TLSF::c_BlockIsUsed );
	oldAreaEnd->SetFreeListPointers( nullptr, nullptr );

	// Resize the containing area and free block- We need to keep track of the old area end block (to fix up the new area)
	TLSF::BlockHeader* oldFreeBlockNextPhysical = freeBlock->GetNextPhysicalBlock();
	TLSF::BlockHeader* oldAreaFinalBlock = containingArea->m_lastBlock;
	containingArea->m_lastBlock = oldAreaEnd;			
	freeBlock->SetSize( reinterpret_cast< Red::System::MemUint >( oldAreaEnd ) - reinterpret_cast< Red::System::MemUint >( freeBlock ) );
	oldAreaEnd->VerifyBlockState();
	freeBlock->VerifyBlockState();
	AddBlockToFreeList( freeBlock );	// The free block is ready for use

	// Add a new area-start block at endAddress
	TLSF::BlockHeader* newAreaStart = reinterpret_cast< TLSF::BlockHeader* >( highAddress );
	newAreaStart->SetPreviousPhysical( nullptr );
	newAreaStart->SetFreeListPointers( nullptr, nullptr );
	newAreaStart->SetSizeAndState( Red::Math::NumericalUtils::Max( sizeof( TLSF::AreaHeader ), TLSF::c_FreeBlockHeaderSize ), TLSF::c_PreviousBlockIsUsed, TLSF::c_BlockIsUsed );
	newAreaStart->VerifyBlockState();
	TLSF::AreaHeader* newAreaHeader = reinterpret_cast< TLSF::AreaHeader* >( newAreaStart->GetDataStartAddress() );
	newAreaHeader->m_lastBlock = oldAreaFinalBlock;

	// Add the area to the areas list
	newAreaHeader->m_nextArea = m_tlsfPoolHead->m_areaHead;
	m_tlsfPoolHead->m_areaHead = newAreaHeader;

	// Finally, add the remaining space from the free block as a new free block
	TLSF::BlockHeader* newFreeBlock = reinterpret_cast< TLSF::BlockHeader* >( newAreaStart->GetNextPhysicalBlock() );
	Red::System::MemSize newFreeSize = reinterpret_cast< Red::System::MemUint >( oldFreeBlockNextPhysical ) - reinterpret_cast< Red::System::MemUint >( newFreeBlock );
	newFreeBlock->SetSizeAndState( newFreeSize, TLSF::c_PreviousBlockIsUsed, TLSF::c_BlockIsFree );
	newFreeBlock->SetPreviousPhysical( newAreaStart );
	newFreeBlock->SetFreeListPointers( nullptr, nullptr );
	oldFreeBlockNextPhysical->SetPreviousPhysical( newFreeBlock );
	newFreeBlock->VerifyBlockState();
	oldFreeBlockNextPhysical->VerifyBlockState();
	AddBlockToFreeList( newFreeBlock );

	return true;
}

/////////////////////////////////////////////////////////////////////
// ReleaseFreeMemoryToSystem
//	Attempt to free chunks of free memory back to the os
//	This is pretty slow, but it should only get hit in emergency (low-memory) situations
//
//	Before:
//		|---------|---------|---------|---------|---------|---------|---------|---------|---------| (Virtual blocks)
//		A----------------FFFFFFFFFFFFFFFFFFFFFFFFFFFFF----------------------FFFFFFFFFFFFFFFFFFFFFFE (Allocator blocks)
//
//	After:
//		|---------|---------|                   |---------|---------|---------|---------|---------| (Virtual blocks)
//		A----------------FFFE                   AFFFFF----------------------FFFFFFFFFFFFFFFFFFFFFFE (Allocator blocks)
Red::System::MemSize TLSFAllocatorImpl::ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback )
{
	WalkHeapDebug();
	Red::System::MemSize memoryFreed = 0u;

	// Smallest block to free = system block size. Any free blocks bigger than this are candidates for releasing
	Red::System::MemSize smallestBlockSize = m_systemBlockSize;
	const Red::System::MemSize smallestBlockFirstLevelIndex = Red::System::BitUtils::Log2( smallestBlockSize ) - TLSF::c_SmallestSegregatedSizeLog2 + 1;

	// Iterate through the free lists, finding any blocks that are big enough to be candidates for freeing
	for( Red::System::MemSize freeListFirstIndex = smallestBlockFirstLevelIndex; freeListFirstIndex < m_tlsfPoolHead->m_firstLevelIndexWidth; ++freeListFirstIndex )
	{
		for( Red::System::MemSize freeListSecondIndex = 0; freeListSecondIndex < m_tlsfPoolHead->m_secondLevelIndexWidth; ++freeListSecondIndex )
		{
			TLSF::BlockHeader* freeBlockHead = GetFreeListHead( freeListFirstIndex, freeListSecondIndex );
			while( freeBlockHead != nullptr )
			{
				if( freeBlockHead->GetTotalSize() > smallestBlockSize )		// Only test blocks that are big enough (there will be smaller ones in this bucket too)
				{
					// Find the largest contiguous block that lines up with system memory blocks in this free block data segment
					Red::System::MemUint lowFreeAddress = (Red::System::MemUint)-1;
					Red::System::MemUint highFreeAddress = 0u;
					FindContiguousSystemMemoryInBlock( freeBlockHead, lowFreeAddress, highFreeAddress );
					if( lowFreeAddress == (Red::System::MemUint)-1 || highFreeAddress == 0u )
					{
						freeBlockHead = freeBlockHead->GetNextFree();
						continue;	// No system blocks fully inside this free block
					}

					// Split the free block, leaving the memory between low and high address unused
					if( SplitFreeBlockAndArea( freeBlockHead, lowFreeAddress, highFreeAddress ) )
					{
						// If we successfully split the free block, then free any system memory blocks in between lowAddress and highAddress
						memoryFreed += FreeSystemMemoryInRange( areaCallback, lowFreeAddress, highFreeAddress );
						freeBlockHead = GetFreeListHead( freeListFirstIndex, freeListSecondIndex );		// This free block is now invalid, continue with the free list head
					}
					else
					{
						freeBlockHead = freeBlockHead->GetNextFree();	// Carry on searching this free list
					}
				}
				else
				{
					freeBlockHead = freeBlockHead->GetNextFree();	// Carry on searching this free list
				}
			}
		}
	}

	WalkHeapDebug();
	return memoryFreed;
}

/////////////////////////////////////////////////////////////////////
// IncreaseMemoryFootprint
//	If we can, try to allocate a new system block and create an area for it
Red::System::Bool TLSFAllocatorImpl::IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired )
{
	if( m_flags & Allocator_StaticSize )
	{
		return false;
	}

	// Calculate how many system blocks are required. The size is rounded up to the next block size to ensure we allocate enough memory
	if( sizeRequired > TLSF::c_SmallestSegregatedSize )
	{
		Red::System::MemSize roundUp = ( 1 << ( Red::System::BitUtils::BitScanReverse( sizeRequired ) - m_tlsfPoolHead->m_secondLevelIndexLog2 ) ) - 1;
		sizeRequired += roundUp;
	}

	Red::System::MemUint sysBlockCount = Red::Math::NumericalUtils::Max( (MemSize)1u, sizeRequired / m_systemBlockSize );	// Always need at least 1 new block
	if( ( m_tlsfPoolHead->m_systemBlockCount + sysBlockCount ) * m_systemBlockSize < m_maximumSize )
	{
		while( sysBlockCount > 0 )
		{
			TLSF::SystemMemoryBlock newBlock = AllocateSystemMemory();
			RED_MEMORY_ASSERT( newBlock.m_address != 0u, "Failed to allocate memory to grow memory pool" );
			if( newBlock.m_address == 0u )
			{
				RED_MEMORY_LOG( TXT( "Failed to grow memory pool. Out of system memory!" ) );
				return false;
			}

			areaCallback.OnAreaAdded( newBlock.m_address, newBlock.m_address + newBlock.m_allocatedSize );
			AddArea( reinterpret_cast< void* >( newBlock.m_address ), newBlock.m_allocatedSize );

			sysBlockCount--;
		}

		return true;
	}
	else
	{
		RED_MEMORY_LOG_ONCE( TXT( "Attempting to increase pool footprint by %" ) RED_PRIWsize_t TXT( " bytes; but this would make the pool over budget (%" ) RED_PRIWsize_t TXT (")"), m_systemBlockSize * sysBlockCount, m_maximumSize );
	}

	return false;
}

/////////////////////////////////////////////////////////////////////
// FreeBlock
//	Takes a used block, merges it with free neighbours, and adds it to the free list matrix
void TLSFAllocatorImpl::FreeBlock( TLSF::BlockHeader* theBlock )
{
	RED_MEMORY_ASSERT( !theBlock->IsFreeBlock(), "Attempting to free a block that is already free!" );
	theBlock->SetFreeState( TLSF::c_BlockIsFree );
	theBlock->GetNextPhysicalBlock()->SetPreviousPhysicalState( TLSF::c_PreviousBlockIsFree );
	theBlock->SetFreeListPointers( nullptr, nullptr );

	// Merge the block with any free neighbours
	MergeFreeBlock( theBlock );
}

/////////////////////////////////////////////////////////////////////
// Free
//	Free some memory 
EAllocatorFreeResults TLSFAllocatorImpl::Free( const void* ptr )
{
	WalkHeapDebug();

	RED_MEMORY_ASSERT( m_tlsfPoolHead != nullptr, "Trying to free from an uninitialised pool" );

	// Only need that check if overflow pool might be use.
#ifdef ENABLE_MEMORY_MANAGER_OVERFLOW_POOL 
	// First of all, test if this allocator actually owns the pointer
	if( !PoolOwnsPointer( ptr ) )
	{
		return Free_NotOwned;
	}
#endif

	RED_MEMORY_ASSERT( PoolOwnsPointer( ptr ), "Allocator do not own pointer." );

	// Get the block header for this pointer
	Red::System::MemUint ptrAddress = reinterpret_cast< Red::System::MemUint >( ptr );
	TLSF::BlockHeader* header = reinterpret_cast< TLSF::BlockHeader* >( ptrAddress - TLSF::c_UsedBlockHeaderSize );
	RED_MEMORY_ASSERT( !header->IsFreeBlock(), "Double-free detected! This pointer has already been freed" );
	header->VerifyBlockState();

	if( header->IsFreeBlock() )
	{
		RED_MEMORY_LOG( TXT( "A pointer was freed multiple times! Possible dangling pointer. This is very bad! (Address: %p, Size: %" ) RED_PRIWsize_t TXT( " bytes" ), header->GetDataSize() );
		RED_HALT( "A pointer was freed multiple times! Possible dangling pointer. This is very bad! (Address: %p, Size: %zd bytes", header->GetDataSize() );
		return Free_AlreadyFree;
	}

	FreeBlock( header );

	WalkHeapDebug();

	return Free_OK;
}

/////////////////////////////////////////////////////////////////////
// HandleGpuProtectionChange_Lock
//	On platforms where we support it, change the protection flag on the memory to CPU R/W for pools with the correct flag
void TLSFAllocatorImpl::HandleGpuProtectionChange_Lock() const
{
	PageAllocator::GetInstance().ProtectMemoryRegion( reinterpret_cast< Red::System::MemUint >( m_tlsfPoolHead ), sizeof( TLSF::PoolHeader ), m_flags, true );

	TLSF::AreaHeader* header = m_tlsfPoolHead->m_areaHead;
	while( header )
	{
		TLSF::AreaHeader* nextHeader = header->m_nextArea;
		Red::System::MemUint areaStart = reinterpret_cast< Red::System::MemUint >( header );
		Red::System::MemUint areaEnd = reinterpret_cast< Red::System::MemUint >( header->m_lastBlock->GetDataStartAddress() );
		PageAllocator::GetInstance().ProtectMemoryRegion( areaStart, areaEnd - areaStart, m_flags, true );
		header = nextHeader;
	}
}

/////////////////////////////////////////////////////////////////////
// HandleGpuProtectionChange_Unlock
//	On platforms where we support it, change the protection flag on the memory to *no* CPU R/W for pools with the correct flag
void TLSFAllocatorImpl::HandleGpuProtectionChange_Unlock() const
{
	TLSF::AreaHeader* header = m_tlsfPoolHead->m_areaHead;
	while( header )
	{
		TLSF::AreaHeader* nextHeader = header->m_nextArea;
		Red::System::MemUint areaStart = reinterpret_cast< Red::System::MemUint >( header );
		Red::System::MemUint areaEnd = reinterpret_cast< Red::System::MemUint >( header->m_lastBlock->GetDataStartAddress() );
		PageAllocator::GetInstance().ProtectMemoryRegion( areaStart, areaEnd - areaStart, m_flags, false );
		header = nextHeader;
	}

	PageAllocator::GetInstance().ProtectMemoryRegion( reinterpret_cast< Red::System::MemUint >( m_tlsfPoolHead ), sizeof( TLSF::PoolHeader ), m_flags, false );
}

/////////////////////////////////////////////////////////////////////
// FallbackAllocate
//	Fallback allocation for when internal fragmentation causes false negatives (slow)
//	This is pretty simple, and will only return blocks that already meet the alignment requirements
void* TLSFAllocatorImpl::FallbackAllocate( Red::System::MemSize size, Red::System::MemSize alignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
{
	// If the pool can grow, prefer to do that over the slowpath
	if( !( m_flags & Allocator_StaticSize ) && ( m_tlsfPoolHead->m_systemBlockCount * m_systemBlockSize < m_maximumSize ) )
	{
		return nullptr;		// Fail and let the pool grow
	}

	TLSF::BlockHeader* blockToUse = nullptr;

	// For each bucket with free blocks, search for one that can fit our request (SLOW!)
	Red::System::MemSize firstIndex = 0, secondIndex = 0;
	GetListIndices( size, firstIndex, secondIndex );	
	for( ; ( firstIndex < m_tlsfPoolHead->m_firstLevelIndexWidth ) && ( blockToUse==nullptr ); ++ firstIndex )
	{
		for( ; ( secondIndex < m_tlsfPoolHead->m_secondLevelIndexWidth ) && ( blockToUse==nullptr ); ++ secondIndex )
		{
			TLSF::BlockHeader* freeBlock = GetFreeListHead( firstIndex, secondIndex );
			while( freeBlock )
			{
				if( freeBlock->GetDataSize() >= size )
				{
					// Make sure it can deal with the alignment
					Red::System::MemSize blockAddress = reinterpret_cast< Red::System::MemSize >( freeBlock->GetDataStartAddress() );
					if( ( blockAddress & ( alignment-1 ) ) == 0 )
					{
						blockToUse = freeBlock;
						break;
					}
				}
				freeBlock = freeBlock->GetNextFree();
			}
		}
		secondIndex = 0;
	}
	if( !blockToUse )
	{
		return nullptr;
	}
	blockToUse->VerifyBlockState();
	RED_MEMORY_ASSERT( blockToUse->IsFreeBlock(), "Found block is not free!?" );

	// The block is now used
	RemoveFreeBlock( blockToUse );
	blockToUse->SetFreeState( TLSF::c_BlockIsUsed );
	blockToUse->GetNextPhysicalBlock( )->SetPreviousPhysicalState( TLSF::c_PreviousBlockIsUsed );
	blockToUse->VerifyBlockState();
	
	// if there is room, chop off the end and add another free block
	Red::System::MemSize blockFreeSpace = blockToUse->GetDataSize() - size;
	if( blockFreeSpace > TLSF::c_FreeBlockHeaderSize )
	{
		TLSF::BlockHeader* endPadBlock = SplitBlock( blockToUse, size );
		FreeBlock( endPadBlock );
		endPadBlock->VerifyBlockState();
	}

	blockToUse->SetMemoryClass( memoryClass );

	RED_MEMORY_ASSERT( !blockToUse->IsFreeBlock(), "The new block is not set to used" );
	RED_MEMORY_ASSERT( blockToUse->GetDataSize() >= size, "Newly allocated block is not big enough for the size requested! Something has gone wrong with bookkeeping" );
	RED_MEMORY_ASSERT( IsValueAligned( reinterpret_cast< Red::System::MemUint >( blockToUse->GetDataStartAddress() ), alignment ), "Newly allocated block is not aligned properly" );
	blockToUse->VerifyBlockState();
	allocatedSize = blockToUse->GetDataSize();
	WalkHeapDebug();
	return blockToUse->GetDataStartAddress();
}

/////////////////////////////////////////////////////////////////////
// Allocate
//	Allocate some memory 
void* TLSFAllocatorImpl::Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass )
{
	// Allocsize 0 should return a null ptr (although operator new(0) should return at least some memory!)
	if( allocSize == 0 )
	{
		return nullptr;
	}

	WalkHeapDebug();

	// Alignment must be power-of-two and >= word size
	allocAlignment = allocAlignment > TLSF_BLOCK_HEADER_ALIGNMENT ? allocAlignment : TLSF_BLOCK_HEADER_ALIGNMENT;
	RED_MEMORY_ASSERT( (allocAlignment & (allocAlignment-1)) == 0, "Alignment must be a power-of-two." );

	// Round size up to smallest possible allocation and nearest alignment boundary
	allocSize = Red::Math::NumericalUtils::Max( allocSize, TLSF::c_SmallestAllocation );
	allocSize = ( allocSize + TLSF_BLOCK_HEADER_ALIGNMENT - 1 ) & ~( TLSF_BLOCK_HEADER_ALIGNMENT-1 );
	
	// We need enough space to handle the worst-case alignment (required alignment - 3 words : 4 word free block would push us over alignment boundary)
	Red::System::MemSize sizeIncludingAlignmentPad = allocSize;
	if( allocAlignment > TLSF_BLOCK_HEADER_ALIGNMENT )
	{
		sizeIncludingAlignmentPad += allocAlignment + ( TLSF_BLOCK_HEADER_ALIGNMENT * 3 );
	}

	TLSF::BlockHeader* foundBlock = FindFreeblockForSize( sizeIncludingAlignmentPad );
	if( foundBlock == nullptr )
	{
		// Emergency fallback. Internal fragmentation can cause allocation to fail when asking for the largest single block
		// (due to the way we find good-fit by rounding up bucket indices)
		return FallbackAllocate( allocSize, allocAlignment, allocatedSize, memoryClass );
	}

	foundBlock->VerifyBlockState();
	RED_MEMORY_ASSERT( foundBlock->IsFreeBlock(), "Found block is not free!?" );
	RED_MEMORY_ASSERT( foundBlock->GetDataSize() >= sizeIncludingAlignmentPad, "The found block is too small! Free-lists are probably broken!" );

	// calculate the padding required so we can decide if/how to split the block most efficiently
	Red::System::MemUint dataStartAddress = reinterpret_cast< Red::System::MemUint >( foundBlock->GetDataStartAddress() );
	Red::System::MemUint addressAlignment = dataStartAddress & ( allocAlignment - 1 );
	Red::System::MemUint blockPadding = 0;
	if( addressAlignment != 0 )
	{
		blockPadding = allocAlignment - addressAlignment;

		// if the padding required < block size, we need to move the start address on by max(free block, alignment) + padding
		if( blockPadding > 0 && blockPadding < TLSF::c_FreeBlockHeaderSize )
		{
			blockPadding = Red::Math::NumericalUtils::Max( allocAlignment, TLSF::c_FreeBlockHeaderSize ) + blockPadding;
		}
	}
	
	Red::System::MemUint blockStartOffset = blockPadding;
	RED_MEMORY_ASSERT( blockStartOffset + allocSize <= foundBlock->GetDataSize(), "Alignment padding has gone wrong! The block is too small" );

	// Remove the block from its free list
	RemoveFreeBlock( foundBlock );

	// The block is now used
	foundBlock->SetFreeState( TLSF::c_BlockIsUsed );
	foundBlock->GetNextPhysicalBlock( )->SetPreviousPhysicalState( TLSF::c_PreviousBlockIsUsed );
	foundBlock->VerifyBlockState();

	// split the found block if required
	TLSF::BlockHeader* newBlock;
	if( blockStartOffset > 0 )
	{
		newBlock = SplitBlock( foundBlock, blockStartOffset - TLSF::c_UsedBlockHeaderSize );
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
	Red::System::MemSize blockFreeSpace = newBlock->GetDataSize() - allocSize;
	if( blockFreeSpace > TLSF::c_FreeBlockHeaderSize )
	{
		TLSF::BlockHeader* endPadBlock = SplitBlock( newBlock, allocSize );

		// Free the end padding block
		FreeBlock( endPadBlock );
		endPadBlock->VerifyBlockState();
	}

	RED_MEMORY_ASSERT( !newBlock->IsFreeBlock(), "The new block is not set to used" );
	RED_MEMORY_ASSERT( newBlock->GetDataSize() >= allocSize, "Newly allocated block is not big enough for the size requested! Something has gone wrong with bookkeeping" );
	RED_MEMORY_ASSERT( IsValueAligned( reinterpret_cast< Red::System::MemUint >( newBlock->GetDataStartAddress() ), allocAlignment ), "Newly allocated block is not aligned properly" );

	newBlock->SetMemoryClass( memoryClass );
	newBlock->VerifyBlockState();
	allocatedSize = newBlock->GetDataSize();
	void* thePtr = newBlock->GetDataStartAddress();

	WalkHeapDebug();

	return thePtr;
}

/////////////////////////////////////////////////////////////////////
// Reallocate
//	Reallocate memory. Tries to handle all corner cases defined in C standard
void* TLSFAllocatorImpl::Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass )
{
	WalkHeapDebug();

	// First of all, test if this allocator actually owns the pointer
	if( ptr != nullptr && !PoolOwnsPointer( ptr ) )
	{
		allocatedSize = 0;
		freedSize = 0;
		return nullptr;
	}

	// If ptr is null, this acts like allocate
	if( ptr == nullptr )
	{
		void* newPtr = nullptr;
		freedSize = 0;
		if( allocSize > 0 )
		{
			newPtr = Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
		}		
		return newPtr;
	}

	// Get the block header for this pointer
	Red::System::MemUint ptrAddress = reinterpret_cast< Red::System::MemUint >( ptr );
	TLSF::BlockHeader* blockHeader = reinterpret_cast< TLSF::BlockHeader* >( ptrAddress - TLSF::c_UsedBlockHeaderSize );
	RED_MEMORY_ASSERT( !blockHeader->IsFreeBlock(), "This pointer has already been freed" );
	blockHeader->VerifyBlockState();

	if( allocSize == 0 )	// Handle size=0
	{
		allocatedSize = 0;
		freedSize = blockHeader->GetDataSize();
		FreeBlock( blockHeader );

		WalkHeapDebug();
		return nullptr;
	}

	// Ensure the allocation size is at least the size of the smallest allocation possible, and is a multiple of the minimum alignment
	Red::System::MemSize adjustedAllocSize = Red::Math::NumericalUtils::Max( allocSize, TLSF::c_SmallestAllocation );
	adjustedAllocSize = ( adjustedAllocSize + TLSF_BLOCK_HEADER_ALIGNMENT - 1 ) & ~( TLSF_BLOCK_HEADER_ALIGNMENT-1 );

	// In order to keep the metrics in check, we always report that the block was freed
	freedSize = blockHeader->GetDataSize();

	if( adjustedAllocSize == blockHeader->GetDataSize() )		// Handle size unchanged
	{
		allocatedSize = adjustedAllocSize;
		return ptr;
	}
	else if( adjustedAllocSize > blockHeader->GetDataSize() )	// growing the buffer
	{
		// First allocate a new block of the required size (use the original alloc size as Allocate will adjust it)
		void* newPtr = Allocate( allocSize, allocAlignment, allocatedSize, memoryClass );
		if( newPtr != nullptr )
		{
			// Grow successful, copy the data and free the old block
			Red::System::MemoryCopy( newPtr, ptr, blockHeader->GetDataSize() );
			FreeBlock( blockHeader );
		}

		WalkHeapDebug();
		return newPtr;
	}
	else	// shrinking the buffer
	{
		Red::System::MemSize shrinkAmount = blockHeader->GetDataSize() - adjustedAllocSize;
		if( shrinkAmount >= TLSF::c_FreeBlockHeaderSize )
		{
			// We can create a new free block from the shrink space
			TLSF::BlockHeader* endBlock = SplitBlock( blockHeader, adjustedAllocSize );

			// Free the end block
			FreeBlock( endBlock );
			endBlock->VerifyBlockState();
		}

		allocatedSize = blockHeader->GetDataSize();

		WalkHeapDebug();

		// Since the old buffer is still valid (its just smaller), we just return the original pointer
		return ptr;
	}
}

/////////////////////////////////////////////////////////////////////
// WalkHeapDebug
//	Walk the entire heap, if this crashes or asserts, the internal state is corrupted 
//  Note that this is INCREDIBLY slow!
void TLSFAllocatorImpl::WalkHeapDebug( )
{
#ifdef RED_MEMORY_ENABLE_TLSF_HEAP_DEBUGGING
	// Use these statics to enable the various debug bits
	static Red::System::Bool doHeapWalk = true;
	static Red::System::Bool walkFreeLists = true;

	if( walkFreeLists )
	{
		// First of all, walk all the free lists and verify the blocks
		for(Red::System::Int32 flb = (Red::System::Int32)m_tlsfPoolHead->m_firstLevelIndexWidth - 1;
			flb >= 0;
			flb--)
		{
			if( m_tlsfPoolHead->m_firstLevelBitmap & ( 1 << flb ) )
			{
				RED_MEMORY_ASSERT( m_tlsfPoolHead->m_secondLevelBitmap != 0, "First level free list bitmap is invalid" );
			}

			for( Red::System::Int32 slb = 0;
				slb < (Red::System::Int32)m_tlsfPoolHead->m_secondLevelIndexWidth;
				slb++ )
			{
				TLSF::BlockHeader* listHead = GetFreeListHead( flb, slb );
				if( m_tlsfPoolHead->m_secondLevelBitmap[flb] & ( 1 << slb ) )
				{
					RED_MEMORY_ASSERT( listHead != nullptr, "Free list bitmap set but the list is empty!" );
					RED_MEMORY_ASSERT( listHead->GetPreviousFree() == nullptr, "Free list head has a previous free block!?" );
					RED_MEMORY_ASSERT( listHead->IsFreeBlock(), "Free list head is not a free block!?" );
					listHead->VerifyBlockState();

					// Walk through the free list
					TLSF::BlockHeader* previousFreeBlock = nullptr;
					while( listHead != nullptr )
					{
						RED_MEMORY_ASSERT( previousFreeBlock == listHead->GetPreviousFree(), "Free block linked list is invalid!" );
						RED_MEMORY_ASSERT( listHead->IsFreeBlock(), "Free list block is not a free block!?" );
						listHead->VerifyBlockState();

						// Test the block is in the correct list
						Red::System::MemSize firstLevelCheck = 0, secondLevelCheck = 0;
						GetListIndices( listHead->GetDataSize(), firstLevelCheck, secondLevelCheck );
						RED_MEMORY_ASSERT( flb == (Red::System::Int32)firstLevelCheck, "Block is in the wrong first-level free list!" );
						RED_MEMORY_ASSERT( slb == (Red::System::Int32)secondLevelCheck, "Block is in the wrong second-level free list!" );

						previousFreeBlock = listHead;
						listHead = listHead->GetNextFree();
					}
				}
				else
				{
					RED_MEMORY_ASSERT( listHead == nullptr, "Second level bitmap is invalid" );
				}
			}
		}
	}

	if( doHeapWalk )
	{
		// Finally, walk the entire block list for each area
		TLSF::AreaHeader* areaHead = m_tlsfPoolHead->m_areaHead;
		while( areaHead != nullptr )
		{
			Red::System::MemSize blocksCounted = 0;
			TLSF::BlockHeader* areaBlock = (TLSF::BlockHeader*)( ( Red::System::MemUint )( areaHead ) - TLSF::c_UsedBlockHeaderSize );
			while( areaBlock != nullptr )
			{
				blocksCounted++;
				areaBlock->VerifyBlockState();

				if( areaBlock->GetTotalSize() == 0 )
				{
					// This is the final block, just make sure it is used
					RED_MEMORY_ASSERT( !areaBlock->IsFreeBlock(), "Final area block should not be freed!" );
					RED_MEMORY_ASSERT( areaBlock == areaHead->m_lastBlock, "Block has size 0 but it is not the final block!" );
					break;
				}

				// Make sure free blocks are on the free lists, and used ones are not!
				Red::System::MemSize firstLevelCheck = 0, secondLevelCheck = 0;
				GetListIndices( areaBlock->GetDataSize(), firstLevelCheck, secondLevelCheck );
				TLSF::BlockHeader* freeList = GetFreeListHead( firstLevelCheck, secondLevelCheck );
				Red::System::Bool isOnFreeLists = false;
				while( freeList != nullptr )
				{
					if( freeList == areaBlock )
					{
						isOnFreeLists = true;
						break;
					}

					freeList = freeList->GetNextFree();
				}

				if( areaBlock->IsFreeBlock() )
				{
					RED_MEMORY_ASSERT( isOnFreeLists, "Block is set as free but not on a free list!" );
				}
				else
				{
					RED_MEMORY_ASSERT( !isOnFreeLists, "Block is set as used, but its on a free list!" );
				}

				areaBlock = areaBlock->GetNextPhysicalBlock();
			}

			areaHead = areaHead->m_nextArea;
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////
// OutputBinary64
//	Output a 64 bit number as binary. Helper for the OOM output
//  Buffer MUST be at least 65 chars
void OutputBinary64( Red::System::Uint64 value, Char* output )
{
	const Char* c_one = TXT( "1" );
	const Char* c_zero = TXT( "0" );
	Char* outPtr = output;
	for( Int64 i = 63; i >= 0; --i )
	{
		Int64 b = ( 1 << i );
		*(outPtr++) = value & b ? *c_one : *c_zero;
	}
	*(outPtr++) = 0;
}

/////////////////////////////////////////////////////////////////////
// OutputAllocationHistogram
//	Display a simple histogram of the data provided
void OutputAllocationHistogram( Red::System::Uint64* srcData, Red::System::Uint64 maxSrcDataValue, Red::System::Uint64 maxIterations,  Red::System::Uint64 maxGraphWidth )
{
	// First, calculate how many allocs = 1 'block'
	Red::System::Float allocStepValue = (Red::System::Float)maxSrcDataValue / (Red::System::Float)maxGraphWidth;
	
	// Draw the graph
	for( Int32 i=0; i<maxIterations; ++i )
	{
		Char graphBlocks[ 128 ] = { 0 };
		Char* blockBuffer = graphBlocks;
		Red::System::Int32 blockCount = (Red::System::Int32)( (Red::System::Float)srcData[i] / allocStepValue );
		for( Int32 b=0; b < blockCount; ++b )
		{
			*(blockBuffer++) = *TXT( "X" );
		}
		Red::System::SNPrintF( blockBuffer, ( graphBlocks + 128 ) - blockBuffer, TXT( " %" ) RED_PRIWsize_t, srcData[ i ] );
		RED_MEMORY_LOG( TXT( "<%15" ) RED_PRIWsize_t TXT( " |%s" ), 1ull << ( i + TLSF::c_SmallestSegregatedSizeLog2 ), graphBlocks );
	}
}

/////////////////////////////////////////////////////////////////////
// LogOutOfMemoryInfo
//	Display everything we can that may help in debugging (usage, fragmentation, etc)
void TLSFAllocatorImpl::LogOutOfMemoryInfo()
{
#ifdef RED_LOGGING_ENABLED
	if( ! m_tlsfPoolHead )
	{
		RED_MEMORY_LOG( TXT( "Pool is not initialised!!!" ) );
		return ;
	}

	// Walk the areas so we get a precise breakdown of memory allocated via the os
	Red::System::MemSize osMemoryAllocated = 0u;
	TLSF::AreaHeader* areaHead = m_tlsfPoolHead->m_areaHead;
	while( areaHead )
	{
		osMemoryAllocated += reinterpret_cast< Red::System::MemSize >( areaHead->m_lastBlock ) - reinterpret_cast< Red::System::MemSize >( areaHead ) + sizeof( TLSF::BlockHeader );
		areaHead = areaHead->m_nextArea;
	}
	osMemoryAllocated += sizeof( TLSF::PoolHeader );

	RED_MEMORY_LOG( TXT( "TLSF Allocator Info:" ) );
	RED_MEMORY_LOG( TXT( "\tMaximum Size: %" ) RED_PRIWsize_t, m_maximumSize );
	RED_MEMORY_LOG( TXT( "\tSystem Block Size: %" ) RED_PRIWsize_t, m_systemBlockSize );
	RED_MEMORY_LOG( TXT( "\tSystem Blocks Allocated: %" ) RED_PRIWsize_t, m_tlsfPoolHead->m_systemBlockCount );
	RED_MEMORY_LOG( TXT( "\tOS Memory Allocated: %" ) RED_PRIWsize_t, osMemoryAllocated );

	Char freeListMatrixBuffer[ 65 ];
	OutputBinary64( m_tlsfPoolHead->m_firstLevelBitmap, freeListMatrixBuffer );
	RED_MEMORY_LOG( TXT( "\tFirst level free list bitmap matrix: %s" ), freeListMatrixBuffer );

	// Walk the free lists, and for each power of two size, output how many free blocks are available
	// Also keep track of exact free block sizes
	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" ) );
	RED_MEMORY_LOG( TXT( "Free Block Info:" ) );
	Red::System::Uint64 freeBlockCountTotal = 0;
	Red::System::Uint64 freeBlockSizeTotal = 0;
	Red::System::Uint64 largestFreeBlock = 0u;
	Red::System::Uint64 smallestFreeBlock = (Red::System::MemSize)-1;
	Red::System::Uint64 freeBlockCount[64] = { 0 };
	Red::System::Uint64 freeBlockSize[64] = { 0 };
	for(Red::System::Int32 flb = 0;
		flb < (Red::System::Int32)m_tlsfPoolHead->m_firstLevelIndexWidth;
		flb++)
	{
		Red::System::MemSize firstLevelPowerTwoSegregatedSize = 1ull << ( flb + TLSF::c_SmallestSegregatedSizeLog2 );
		RED_UNUSED( firstLevelPowerTwoSegregatedSize );
		for( Red::System::Int32 slb = 0;
			slb < (Red::System::Int32)m_tlsfPoolHead->m_secondLevelIndexWidth;
			slb++ )
		{
			TLSF::BlockHeader* listHead = GetFreeListHead( flb, slb );
			while( listHead )
			{
				++freeBlockCount[flb];
				++freeBlockCountTotal;
				Red::System::Uint64 blockSize = (Red::System::Uint64)listHead->GetDataSize();
				freeBlockSize[flb] += blockSize;
				freeBlockSizeTotal += blockSize;
				largestFreeBlock = Red::Math::NumericalUtils::Max( largestFreeBlock, blockSize );
				smallestFreeBlock = Red::Math::NumericalUtils::Min( smallestFreeBlock, blockSize );
				listHead = listHead->GetNextFree();
			}
		}
		if( freeBlockCount[flb]>0 )
		{
			RED_MEMORY_LOG( TXT( "\t%" ) RED_PRIWsize_t TXT( " bytes free in %" ) RED_PRIWsize_t TXT( " free blocks of size < ~%" ) RED_PRIWsize_t TXT( " bytes" ), freeBlockSize[flb], freeBlockCount[flb], firstLevelPowerTwoSegregatedSize );
		}
	}
	RED_MEMORY_LOG( TXT( "\t%" ) RED_PRIWsize_t TXT( " total bytes free in %" ) RED_PRIWsize_t TXT( " free blocks" ), freeBlockSizeTotal, freeBlockCountTotal );

	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" ) );
	RED_MEMORY_LOG( TXT( "Free block count distribution: " ) );
	OutputAllocationHistogram( freeBlockCount, freeBlockCountTotal, m_tlsfPoolHead->m_firstLevelIndexWidth, 80 );

	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" ) );
	RED_MEMORY_LOG( TXT( "Free block size distribution: " ) );
	OutputAllocationHistogram( freeBlockSize, freeBlockSizeTotal, m_tlsfPoolHead->m_firstLevelIndexWidth, 80 );

	RED_MEMORY_LOG( TXT( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" ) );
	RED_MEMORY_LOG( TXT( "Fragmentation: " ) );
	RED_MEMORY_LOG( TXT( "\tSmallest free block = %" ) RED_PRIWsize_t TXT( " bytes; Largest free block = %" ) RED_PRIWsize_t TXT( " bytes" ), smallestFreeBlock, largestFreeBlock );
	Red::System::Float approxFragmentation = static_cast< Red::System::Float >( freeBlockSizeTotal - largestFreeBlock ) / static_cast< Red::System::Float >( freeBlockSizeTotal ) * 100.0f;
	RED_MEMORY_LOG( TXT( "\tApprox %3.1f%% fragmented" ), approxFragmentation );
	RED_UNUSED( approxFragmentation );
#endif
}

/////////////////////////////////////////////////////////////////////
// DumpDebugOutput
//	Spits out some debug logging stuff
void TLSFAllocatorImpl::DumpDebugOutput()
{
	// First level bitmap / size mapping
	const Red::System::MemSize c_maxDbgChars = 1024 * 16;

	Red::System::AnsiChar debugOutputBuffer[ c_maxDbgChars ] = {'\0'};
	Red::System::AnsiChar debugBuffer[512] = {'\0'};
	
	// Table titles = first level sizes
	Red::System::SNPrintF( debugBuffer, 512, "First Level Bitmap (%d bits wide)", m_tlsfPoolHead->m_firstLevelIndexWidth );
	Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );

	RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
	debugOutputBuffer[0] = '\0';

	// Output the first-level segregation boundaries
	Red::System::SNPrintF( debugBuffer, 512, "| SLI\t" );
	Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
	for(Red::System::Int32 flb = (Red::System::Int32)m_tlsfPoolHead->m_firstLevelIndexWidth - 1;
		flb >= 0;
		flb--)
	{
		Red::System::SNPrintF( debugBuffer, 512, "| %lu(%d)\t\t\t\t", (1 << ( flb + TLSF::c_SmallestSegregatedSizeLog2 ) ), m_tlsfPoolHead->m_firstLevelBitmap & (1 << flb) ? 1 : 0 );
		Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
	}
	RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
	debugOutputBuffer[0] = '\0';

	// For each first level boundary, output the second-level free list heads
	for( Red::System::Int32 slb = 0;
		slb < (Red::System::Int32)m_tlsfPoolHead->m_secondLevelIndexWidth;
		slb++ )
	{
		Red::System::SNPrintF( debugBuffer, 512, "| %d \t", slb );
		Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
		for(Red::System::Int32 flb = (Red::System::Int32)m_tlsfPoolHead->m_firstLevelIndexWidth - 1;
			flb >= 0;
			flb--)
		{
			Red::System::SNPrintF( debugBuffer, 512, "|\t%p(%d)\t", GetFreeListHead( flb, slb ), m_tlsfPoolHead->m_secondLevelBitmap[flb] & (1 << slb) ? 1 : 0 );
			Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
		}
		RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
		debugOutputBuffer[0] = '\0';
	}

	// Do a walk of the heap backwards through the blocks to give a rough block breakdown
	Red::System::SNPrintF( debugBuffer, 512, "Heap walk (reversed)" );
	Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
	RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
	debugOutputBuffer[0] = '\0';

	TLSF::AreaHeader* areaHead = m_tlsfPoolHead->m_areaHead;
	Red::System::MemSize blockCount = 0;
	while( areaHead != nullptr )
	{
		Red::System::SNPrintF( debugBuffer, 512, "AS|" );
		Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );

		TLSF::BlockHeader* finalBlock = reinterpret_cast< TLSF::BlockHeader* >( areaHead->m_lastBlock );
		do
		{
			finalBlock->VerifyBlockState();
			Red::System::SNPrintF( debugBuffer, 512, "%c(%lu)|", finalBlock->IsFreeBlock() ? 'f' : 'u', finalBlock->GetTotalSize() );
			Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );

			// If there are two many blocks for one line, start a new line
			if( (blockCount++) > 2000 )
			{
				RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
				debugOutputBuffer[0] = '\0';
				blockCount = 0;
			}

			finalBlock = finalBlock->GetPreviousPhysical();
		}
		while( finalBlock );
		areaHead = areaHead->m_nextArea;

		Red::System::SNPrintF( debugBuffer, 512, "AE|" );
		Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );

		RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
		debugOutputBuffer[0] = '\0';
	}

	Red::System::SNPrintF( debugBuffer, 512, "Free lists:" );
	Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
	RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
	debugOutputBuffer[0] = '\0';

	for( Red::System::Int32 slb = 0;
		slb < (Red::System::Int32)m_tlsfPoolHead->m_secondLevelIndexWidth;
		slb++ )
	{
		for(Red::System::Int32 flb = (Red::System::Int32)m_tlsfPoolHead->m_firstLevelIndexWidth - 1;
			flb >= 0;
			flb--)
		{
			TLSF::BlockHeader* fl = GetFreeListHead( flb, slb );
			if( fl != nullptr )
			{
				Red::System::SNPrintF( debugBuffer, 512, "FL %d, SL %d: ", flb, slb );
				Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
				while( fl != nullptr )
				{
					Red::System::SNPrintF( debugBuffer, 512, "%p (%lu), ", (Red::System::MemUint)fl, fl->GetDataSize() );
					Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
					fl = fl->GetNextFree();
				}
				RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
				debugOutputBuffer[0] = '\0';
			}
		}
	}

	// System memory blocks
	Red::System::SNPrintF( debugBuffer, 512, "System Memory Blocks" );
	Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
	RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
	debugOutputBuffer[0] = '\0';
	for( Red::System::Uint32 sysBlockIndex = 0; sysBlockIndex < m_tlsfPoolHead->m_systemBlockCount; ++sysBlockIndex )
	{
		Red::System::SNPrintF( debugBuffer, 512, "%p (%lu)", m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_address, m_tlsfPoolHead->m_systemBlocks[ sysBlockIndex ].m_allocatedSize );
		Red::System::StringConcatenate( debugOutputBuffer, debugBuffer, c_maxDbgChars );
		RED_MEMORY_LOG( TXT( "%" ) RED_PRIWas, debugOutputBuffer );
		debugOutputBuffer[0] = '\0';
	}
}

} }
