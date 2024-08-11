/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_IMPL_H
#define _RED_MEMORY_TLSF_IMPL_H

#include "redMemoryFrameworkTypes.h"
#include "redMemoryPageAllocator.h"
#include "../redSystem/utility.h"

///////////////////////////////////////////////////////////////////////
// This is an implementation of the Two-level segregated fit allocator, as described in 
// the following papers 
// 'Description of the TLSF Memory Allocator Version 2.0 (Masmano, Ripoll, Crespo)'
// 'Implementation of a Constant-time dynamic storage allocator' (Masmano, Ripoll, Real, Crespo, Wellings)'
// 'Dynamic storage allocation for real-time embedded systems (Masmano, Ripoll, Crespo)'
// 
namespace Red { namespace MemoryFramework { 

class AllocatorWalker;
class PoolAreaWalker;
class AllocatorAreaCallback;

namespace TLSF 
{

///////////////////////////////////////////////////////////////////////
// TLSF Allocator Types / Macros

// Bitmap corresponding to free lists
typedef Red::System::Uint32 FreelistBitmap;		

// Block and pool headers should be word aligned
#define TLSF_BLOCK_HEADER_ALIGNMENT	16


///////////////////////////////////////////////////////////////////////
// Block Header Structures
//  These are used as boundary tags
//	In 32 bit machines - BlockHeader is 8 bytes when used, 16 bytes when free. Smallest allocation supported = 8 bytes
//	In 64 bit machines - BlockHeader is 16 bytes when used, 32 bytes when free. Smallest allocation supported = 16 bytes

class BlockHeader;

///////////////////////////////////////////////////////////////////////
// Free blocks contain pointers to the next and previous free block in the segregated list
RED_ALIGNED_CLASS( FreeBlockData, TLSF_BLOCK_HEADER_ALIGNMENT )
{
public:
	// Double-linked list of blocks in this segregated list (2nd level)
	BlockHeader*					m_nextSegregated;
	BlockHeader*					m_prevSegregated;
};

///////////////////////////////////////////////////////////////////////
// Blocks contain an always valid previous block pointer and size
// Free blocks also contain a next and previous pointer
RED_ALIGNED_CLASS( BlockHeader : Red::System::NonCopyable, TLSF_BLOCK_HEADER_ALIGNMENT )
{
public:
	// Block header Manipulation
	RED_INLINE void SetSize( Red::System::MemSize size );
	RED_INLINE void SetState( Red::System::MemSize previousPhysicalState, Red::System::MemSize blockState );
	RED_INLINE void SetFreeState( Red::System::MemSize blockState );
	RED_INLINE void SetPreviousPhysicalState( Red::System::MemSize previousState );
	RED_INLINE void SetSizeAndState( Red::System::MemSize size, Red::System::MemSize previousPhysicalState, Red::System::MemSize blockState );
	RED_INLINE void SetPreviousPhysical( BlockHeader* header );
	RED_INLINE void SetFreeListPointers( BlockHeader* previous, BlockHeader* next );
	RED_INLINE void SetNextFree( BlockHeader* next );
	RED_INLINE void SetPreviousFree( BlockHeader* next );

	// Accessors
	RED_INLINE Red::System::MemSize GetTotalSize() const;
	RED_INLINE Red::System::MemSize GetDataSize() const;
	RED_INLINE Red::System::Bool IsFreeBlock() const;
	RED_INLINE Red::System::Bool IsPreviousBlockFree() const;
	RED_INLINE BlockHeader* GetNextFree() const;
	RED_INLINE BlockHeader* GetPreviousFree() const;
	RED_INLINE BlockHeader* GetPreviousPhysical() const;
	RED_INLINE void* GetDataStartAddress() const;
	RED_INLINE TLSF::BlockHeader* GetNextPhysicalBlock();

	// Debug stuff
	RED_INLINE void VerifyBlockState();
	RED_INLINE void SetMemoryClass( Uint16 memoryClass );
	RED_INLINE Uint16 GetMemoryClass() const;

private:
	// Linked-list of blocks by physical address
	BlockHeader*					m_previousPhysical;

	// 2 least significant bits are status bits, the rest is size (since block sizes are always multiple of four)
	// Note that the size is the size of the data + the block header!
	// bit 0 = free / used
	// bit 1 = last physical block marker
	Red::System::MemSize			m_sizeAndStatus : 48;

	// Debug
	Red::System::MemSize			m_memoryClass : 16;

	// A union is used so we can get either a pointer to the used block memory (m_data), or the free list,
	// depending on what kind of block this is
	union
	{
		FreeBlockData m_freeBlockData;
		Red::System::Uint8 m_data[1];
	};
};

///////////////////////////////////////////////////////////////////////
// Pool Area Struct
//	This struct represents a single area of memory used by the allocator
//	Areas can be merged and simply represent arbitrary blocks of data that the allocator can dish out
RED_ALIGNED_CLASS( AreaHeader : Red::System::NonCopyable, TLSF_BLOCK_HEADER_ALIGNMENT )
{
public:	
	AreaHeader* m_nextArea;		// Linked list of areas
	BlockHeader* m_lastBlock;	// The final sentinel block in this area
};

///////////////////////////////////////////////////////////////////////
// SystemMemoryBlock tracking struct
//	Used so we can free blocks of memory back to the OS on demand (pc only)
RED_ALIGNED_CLASS( SystemMemoryBlock , TLSF_BLOCK_HEADER_ALIGNMENT )
{
public:
	SystemMemoryBlock();

	Red::System::MemUint	m_address;
	Red::System::MemSize	m_allocatedSize;
};

///////////////////////////////////////////////////////////////////////
// Pool Header Struct
//	This struct contains the first and second level bitmaps, as well as
//	the matrix of free-list pointers, and anything else needed to process
//	this pool
RED_ALIGNED_CLASS( PoolHeader : Red::System::NonCopyable, TLSF_BLOCK_HEADER_ALIGNMENT )
{
public:
	PoolHeader( Red::System::Uint32 firstLevelBitmapWidth,
				Red::System::Uint32 secondLevelBitmapWidth,
				Red::System::Uint32 maximumSystemBlocks );

	// The first level bitmap represents free lists split on power-of-two boundaries
	// This width is the number of bits valid in the first level segregated list
	Red::System::Uint32			m_firstLevelIndexWidth;
	FreelistBitmap				m_firstLevelBitmap;

	// The second level bitmap represents a bitmap per first-level line, 
	// with each allocation size split linearly. 
	Red::System::Uint32			m_secondLevelIndexWidth;
	Red::System::Uint32			m_secondLevelIndexLog2;
	FreelistBitmap*				m_secondLevelBitmap;	// 1 bitmap per FLI

	// The free-list headers are stored as a FLIxSLI matrix of pointers
	// The bitmaps above correspond to a single free list in this matrix
	BlockHeader**				m_freeListMatrix;

	// The first area of memory managed by this pool
	AreaHeader*					m_areaHead;

	// System memory block tracking
	Red::System::Uint32			m_maximumSystemBlocks;
	Red::System::Uint32			m_systemBlockCount;
	TLSF::SystemMemoryBlock*	m_systemBlocks;
};


///////////////////////////////////////////////////////////////////////
// TLSF Constants. 
//	These will not need to be modified unless something goes horribly wrong.

// There is no point splitting small allocation sizes into multiple 
// first-level lists as the size granularity is too low
// This constant controls the minimum size to start segregating
// (Any allocations small than this go into the FLI 0)
// Note this must be a power of 2 (as this is only for the first level)
const Red::System::MemSize c_SmallestSegregatedSizeLog2 = 7;		// Log(128) = 7
const Red::System::MemSize c_SmallestSegregatedSize = (1 << c_SmallestSegregatedSizeLog2);

// Maximum FLI (First level index) width (bits).
const Red::System::Uint32 c_MaximumFirstLevelBits = 32;

// Maximum SLI (Second level index) width (log2). 5 = max of 32 splits
const Red::System::Uint32 c_MaximumSecondLevelLog2 = 5;
const Red::System::Uint32 c_MaximumSecondLevelDivisions = (1 << c_MaximumSecondLevelLog2);

// Smallest allocation allowed (i.e. size of used block + memory for free block segregated list)
const Red::System::MemSize c_SmallestAllocation = sizeof(BlockHeader) - sizeof(FreeBlockData);

// Minimum pool alignment supported
const Red::System::MemSize c_MinimumPoolAlignment = sizeof(Red::System::MemSize);

// Size of a used block header
const Red::System::MemSize c_UsedBlockHeaderSize = sizeof(BlockHeader) - sizeof(FreeBlockData);

// Size of a free block header
const Red::System::MemSize c_FreeBlockHeaderSize = sizeof(BlockHeader);

// Block status masks (first 2 bits)
const Red::System::MemSize c_BlockFreeStatusMask = 0x1;					// Used / free
const Red::System::MemSize c_BlockPrevStateMask = 0x2;					// Previous physical block used / free
const Red::System::MemSize c_BlockStatusMask = c_BlockFreeStatusMask | c_BlockPrevStateMask;		// Full mask

// Block free status
const Red::System::MemSize c_BlockIsFree = 0x1;
const Red::System::MemSize c_BlockIsUsed = 0x0;

// Previous physical block status
const Red::System::MemSize c_PreviousBlockIsFree = 0x2;
const Red::System::MemSize c_PreviousBlockIsUsed = 0x0;

}

///////////////////////////////////////////////////////////////////////
// TLSFImpl class
//	This class wraps the functionality of the TLSF allocator
class TLSFAllocatorImpl : public Red::System::NonCopyable
{
public:
	TLSFAllocatorImpl();
	~TLSFAllocatorImpl();

	enum EPoolCreationResults
	{
		Pool_OK,
		Pool_MemoryTooSmall,
		Pool_OutOfMemory,
		Pool_BadParameters
	};

	// Pool creation / destruction
	EPoolCreationResults Initialise( Red::System::MemSize initialPoolSize, Red::System::MemSize maximumPoolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity, Red::System::Uint32 flags );
	void Release( );

	// Allocator interface
	void*	Allocate( Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );
	void*	Reallocate( void* ptr, Red::System::MemSize allocSize, Red::System::MemSize allocAlignment, Red::System::MemSize& allocatedSize, Red::System::MemSize& freedSize, Red::System::Uint16 memoryClass );
	EAllocatorFreeResults Free( const void* ptr );
	Red::System::Bool IncreaseMemoryFootprint( AllocatorAreaCallback& areaCallback, Red::System::MemSize sizeRequired );
	Red::System::MemSize ReleaseFreeMemoryToSystem( AllocatorAreaCallback& areaCallback );

	// Debug output
	void LogOutOfMemoryInfo();
	void DumpDebugOutput();
	void WalkHeapDebug( );

	// Pointer ownership
	RED_INLINE Red::System::Bool PoolOwnsPointer( const void* thePtr ) const;
	RED_INLINE Red::System::MemSize SizeOfOwnedBlock( const void* thePtr ) const;

	// Metrics
	RED_INLINE Red::System::MemSize GetMemoryBudget();

	// Walk each large area of the allocator
	void WalkAllocator( AllocatorWalker* theWalker );

	// Walk all allocations in a particular area
	void WalkPoolArea( Red::System::MemUint startAddress, Red::System::MemSize size, PoolAreaWalker* theWalker );

	// Instantiate this object to allow CPU read/write access to a pool set as GPU only. Very slow, but useful for debugging
	class ScopedGpuProtection;

private:

	// Handle GPU-only memory protection change (debug)
	void HandleGpuProtectionChange_Lock() const;
	void HandleGpuProtectionChange_Unlock() const;

	// Pool bookkeeping 
	RED_INLINE Red::System::Uint32 GetFirstLevelIndexWidthFromPoolSize( Red::System::MemSize poolSize );
	RED_INLINE Red::System::MemSize GetBookkeepingOverhead( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 firstLevelBitmapWidth, Red::System::Uint32 segregationGranularity );

	// Block manipulation
	RED_INLINE void RemoveFreeBlock( TLSF::BlockHeader* theBlock );
	RED_INLINE TLSF::BlockHeader* SplitBlock( TLSF::BlockHeader* theBlock, Red::System::MemSize splitDataOffset );
	RED_INLINE void MergeFreeBlock( TLSF::BlockHeader* theBlock );
	void FreeBlock( TLSF::BlockHeader* theBlock );
	
	// Free-list stuff
	RED_INLINE void GetListIndices( Red::System::MemSize size, Red::System::MemSize& firstLevel, Red::System::MemSize& secondLevel );
	RED_INLINE TLSF::BlockHeader* GetFreeListHead( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel );
	RED_INLINE void SetFreeListHead( TLSF::BlockHeader* newHead, Red::System::MemSize firstLevel, Red::System::MemSize secondLevel );
	RED_INLINE void AddBlockToFreeList( TLSF::BlockHeader* theBlock );
	RED_INLINE TLSF::BlockHeader* FindFreeblockForSize( Red::System::MemSize size );

	// Free-list bitmap manipulation
	RED_INLINE void ClearFreelistBitmap( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel );
	RED_INLINE void SetFreelistBitmap( Red::System::MemSize firstLevel, Red::System::MemSize secondLevel );

	// Fallback allocation for when internal fragmentation causes false negatives (slow)
	void* FallbackAllocate( Red::System::MemSize size, Red::System::MemSize alignment, Red::System::MemSize& allocatedSize, Red::System::Uint16 memoryClass );

	// Area manipulation
	TLSF::AreaHeader* InitialiseArea( void* areaStartAddress, Red::System::MemSize areaSize );
	void MergeNewArea( TLSF::AreaHeader* newHeader );
	TLSF::AreaHeader* MergeContiguousAreas( TLSF::AreaHeader* header0, TLSF::AreaHeader* header1 );
	void AddArea( void* areaPtr, Red::System::MemSize areaSize );
	TLSF::AreaHeader* FindAreaContainingBlock( TLSF::BlockHeader* blockHeader );

	// Pool helper functions
	Red::System::MemUint InitialisePoolHeader();
	EPoolCreationResults TestInitialParameters( Red::System::MemSize initialPoolSize, Red::System::MemSize maximumPoolSize, Red::System::MemSize systemSize, Red::System::Uint32 segregationGranularity );
	RED_INLINE Red::System::Bool IsValueAligned( Red::System::MemSize value, Red::System::MemSize alignment );
	Red::System::MemSize CalculateMemoryRequiredForPool( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity );
	Red::System::MemSize CalculateMemoryBookkeeping( Red::System::MemSize poolSize, Red::System::MemSize systemBlockSize, Red::System::Uint32 segregationGranularity );

	// System memory block functions
	TLSF::SystemMemoryBlock	AllocateSystemMemory();
	void FreeSystemMemoryBlock( AllocatorAreaCallback& areaCallback, Red::System::Uint32 blockIndex );
	void FindContiguousSystemMemoryInBlock(  TLSF::BlockHeader* theBlock, Red::System::MemUint& lowAddress, Red::System::MemUint& highAddress );
	Red::System::Bool SplitFreeBlockAndArea( TLSF::BlockHeader* freeBlock, Red::System::MemUint lowAddress, Red::System::MemUint highAddress );
	Red::System::MemSize FreeSystemMemoryInRange( AllocatorAreaCallback& areaCallback, Red::System::MemUint lowAddress, Red::System::MemUint highAddress );

	TLSF::PoolHeader*		m_tlsfPoolHead;		// The tlsf bookkeeping data
	Red::System::MemSize	m_maximumSize;		// Maximum size we will allow for
	Red::System::MemSize	m_initialPoolSize;	// Size of the first block (includes bookkeeping)
	Red::System::MemSize	m_systemBlockSize;	// How much system / virtual memory to allocate for an area
	Red::System::Uint32		m_flags;
};

} }

#include "redMemoryTlsfImpl.inl"

#endif
