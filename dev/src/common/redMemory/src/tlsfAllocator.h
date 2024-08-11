/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_ALLOCATOR_H_
#define _RED_MEMORY_TLSF_ALLOCATOR_H_

#include "../include/utils.h"

#include "systemBlock.h"
#include "virtualRange.h"
#include "block.h"
#include "allocatorMetrics.h"

namespace red
{
namespace memory
{
	class TLSFBlockHeader;

	struct TLSFAllocatorMetrics
	{
		AllocatorMetrics metrics;

		struct FreeBlockMetric
		{
			u64 blockSize;
			u64 totalCount;
			u64 totalSize;
		};

		std::array< FreeBlockMetric, 32 > freeBlocksMetrics;
	};
	
	///////////////////////////////////////////////////////////////////////
	// This is an implementation of the Two-level segregated fit allocator, as described in 
	// the following papers 
	// 'Description of the TLSF Memory Allocator Version 2.0 (Masmano, Ripoll, Crespo)'
	// 'Implementation of a Constant-time dynamic storage allocator' (Masmano, Ripoll, Real, Crespo, Wellings)'
	// 'Dynamic storage allocation for real-time embedded systems (Masmano, Ripoll, Crespo)'
	//
	struct TLSFAllocatorParameter
	{
		VirtualRange range;
		SystemBlock block;
	};

	class RED_MEMORY_API TLSFAllocator
	{
	public:

		TLSFAllocator();
		~TLSFAllocator();

		void Initialize( const TLSFAllocatorParameter & param );
		void Unitialize();

		Block Allocate( u32 size );
		Block AllocateAligned( u32 size, u32 alignment );

		Block Reallocate( Block & block, u32 size );

		void Free( Block & block );

		void AddArea( SystemBlock areaBlock );

		bool OwnBlock( u64 block ) const;

		void BuildMetrics( TLSFAllocatorMetrics & metrics );

		u32 ComputeMinimumAreaSize( u32 size, u32 alignment );

	private:

		typedef u32 FreeListBitmap;

		//	This struct represents a single area of memory used by the allocator
		//	Areas can be merged and simply represent arbitrary blocks of data that the allocator can dish out
		struct RED_ALIGN( 16 ) TLSFAreaHeader 
		{
			TLSFAreaHeader* nextArea; // Linked list of areas
			TLSFBlockHeader* lastBlock; // The final sentinel block in this area
		};

		TLSFAllocator( const TLSFAllocator& );
		TLSFAllocator & operator=( const TLSFAllocator& );

		bool IsInitialized() const;
		u64 InitializeBookKeeping( const SystemBlock & block );

		TLSFAreaHeader * InitializeArea( u64 address, u64 size );
		void MergeArea( TLSFAreaHeader * area );
		TLSFAreaHeader* MergeContiguousAreas( TLSFAreaHeader * firstArea, TLSFAreaHeader * secondArea );

		TLSFBlockHeader * TakeFreeBlock( u32 size, u32 alignment );
		TLSFBlockHeader * FindFreeBlock( u32 size );
		TLSFBlockHeader * SearchFreeBlock( u32 size );
		void FreeBlock( TLSFBlockHeader * block );
		void MergeFreeBlock( TLSFBlockHeader * block );
		TLSFBlockHeader * ShrinkBlock( TLSFBlockHeader * foundBlock, u32 size, u32 alignment );
		TLSFBlockHeader * GrowBlock( TLSFBlockHeader * foundBlock, u32 requiredSize, u32 alignment );
		TLSFBlockHeader * SplitBlock( TLSFBlockHeader * block, u32 splitDataOffset );
		
		void AddBlockToFreeList( TLSFBlockHeader * block );
		void RemoveBlockFromFreeList( TLSFBlockHeader * block );

		void GetListIndices( u32 size, u32 & firstLevel, u32 & secondLevel );
		TLSFBlockHeader * GetFreeListHead( u32 firstLevel, u32 secondLevel );
		void SetFreeListHead( TLSFBlockHeader* newHead, u32 firstLevel, u32 secondLevel );
		
		void ClearFreelistBitmap( u32 firstLevel, u32 secondLevel );
		void SetFreeListBitmap( u32 firstLevel, u32 secondLevel );

		// 0 byte
		// The first level bitmap represents free lists split on power-of-two boundaries
		// This width is the number of bits valid in the first level segregated list
		u32 m_firstLevelIndexWidth;
		FreeListBitmap m_firstLevelBitmap;

		// The second level bitmap represents a bitmap per first-level line, 
		// with each allocation size split linearly. 
		u32	m_secondLevelIndexWidth;
		u32 m_secondLevelIndexLog2;
		FreeListBitmap* m_secondLevelBitmap;	// 1 bitmap per FLI
	
		// The free-list headers are stored as a FLIxSLI matrix of pointers
		// The bitmaps above correspond to a single free list in this matrix
		TLSFBlockHeader** m_freeListMatrix;

		// The first area of memory managed by this pool
		TLSFAreaHeader* m_areaHead;

		VirtualRange m_reservedMemoryRange;
	
		// 56 byte
		u8 m_padding[ 8 ];
	};

	u32 ComputeTLSFBookKeepingSize( u32 maxAllocatedSize );
}
}

#endif
