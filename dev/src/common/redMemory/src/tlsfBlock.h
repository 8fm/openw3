/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_BLOCK_H_
#define _RED_MEMORY_TLSF_BLOCK_H_

#include "../include/utils.h"

namespace red
{
namespace memory
{
	class TLSFBlockHeader;

	///////////////////////////////////////////////////////////////////////
	// Free blocks contain pointers to the next and previous free block in the segregated list
	struct RED_ALIGN( 16 ) TLSFFreeBlockData
	{
		// Double-linked list of blocks in this segregated list (2nd level)
		TLSFBlockHeader* nextSegregated;
		TLSFBlockHeader* prevSegregated;
	};

	class RED_ALIGN( 16 ) TLSFBlockHeader
	{
	public:

		void SetSize( u32 size );
		void SetState( u32 previousPhysicalState, u32 blockState );
		void SetFreeState( u32 blockState );
		void SetPreviousPhysicalState( u32 previousState );
		void SetSizeAndState( u32 size, u32 previousPhysicalState, u32 blockState );
		void SetPreviousPhysical( TLSFBlockHeader* header );
		void SetFreeListPointers( TLSFBlockHeader* previous, TLSFBlockHeader* next );
		void SetNextFree( TLSFBlockHeader* next );
		void SetPreviousFree( TLSFBlockHeader* next );

		u32 GetTotalSize() const;
		u32 GetDataSize() const;
		bool IsFreeBlock() const;
		bool IsPreviousBlockFree() const;
		TLSFBlockHeader* GetNextFree() const;
		TLSFBlockHeader* GetPreviousFree() const;
		TLSFBlockHeader* GetPreviousPhysical() const;
		u64 GetDataStartAddress() const;
		TLSFBlockHeader* GetNextPhysicalBlock();

		void VerifyBlockState();

	private:
		// Linked-list of blocks by physical address
		TLSFBlockHeader * m_previousPhysical;

		// 2 least significant bits are status bits, the rest is size (since block sizes are always multiple of four)
		// Note that the size is the size of the data + the block header!
		// bit 0 = free / used
		// bit 1 = last physical block marker
		u64	m_sizeAndStatus;

		// A union is used so we can get either a pointer to the used block memory (m_data), or the free list,
		// depending on what kind of block this is
		union
		{
			TLSFFreeBlockData m_freeBlockData;
			u8 m_data[1];
		};
	};

	TLSFBlockHeader * GetTLSFBlockHeader( u64 block );
	TLSFBlockHeader * GetTLSFBlockHeader( const Block & block );
	u32 GetTLSFBlockSize( u64 block );
}
}

#include "tlsfBlock.hpp"

#endif
