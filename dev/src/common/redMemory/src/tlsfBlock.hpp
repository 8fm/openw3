/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_BLOCK_HPP_
#define _RED_MEMORY_TLSF_BLOCK_HPP_

#include "tlsfConstant.h"
#include "block.h"

namespace red
{
namespace memory
{
	RED_MEMORY_INLINE u32 TLSFBlockHeader::GetDataSize() const
	{
		return ( m_sizeAndStatus & ~c_tlsfBlockStatusMask ) - c_tlsfUsedBlockHeaderSize;
	}

	RED_MEMORY_INLINE TLSFBlockHeader* TLSFBlockHeader::GetNextPhysicalBlock()
	{
		u64 nextBlockAddress = reinterpret_cast< u64 >( this ) + GetTotalSize();
		RED_MEMORY_ASSERT( IsAligned( nextBlockAddress, 8 ), "Misaligned block pointer" );

		return reinterpret_cast< TLSFBlockHeader* >( nextBlockAddress );
	}

	RED_MEMORY_INLINE u32 TLSFBlockHeader::GetTotalSize() const
	{
		return ( m_sizeAndStatus & ~c_tlsfBlockStatusMask );
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetPreviousPhysicalState( u32 previousState )
	{
		m_sizeAndStatus = ( m_sizeAndStatus & ~c_tlsfBlockPrevStateMask ) | previousState;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetFreeState( u32 blockState )
	{
		m_sizeAndStatus = ( m_sizeAndStatus & ~c_tlsfBlockFreeStatusMask ) | blockState;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetPreviousPhysical( TLSFBlockHeader* header )
	{
		m_previousPhysical = header;
	}

	RED_MEMORY_INLINE u64 TLSFBlockHeader::GetDataStartAddress() const
	{
		return reinterpret_cast< u64 >( m_data );
	}

	/////////////////////////////////////////////////////////////////////
	// SetBlockSize
	//	Sets a blocks size without touching its state mask
	//	Since size is always a multiple of word_size, we don't worry about
	//	shifting it in the status mask, we just chop off the bottom 2 bits
	RED_MEMORY_INLINE void TLSFBlockHeader::SetSize( u32 size )
	{
		RED_MEMORY_ASSERT( ( size & ( c_tlsfMinimumAlignment - 1 ) ) == 0, "Attempting to set a block to a badly aligned size" );
		m_sizeAndStatus = ( size & ~c_tlsfBlockStatusMask ) | ( m_sizeAndStatus & c_tlsfBlockStatusMask );
	}

	/////////////////////////////////////////////////////////////////////
	// SetBlockState
	//	Set both parts of a block state mask at once
	RED_MEMORY_INLINE void TLSFBlockHeader::SetState( u32 previousPhysicalState, u32 blockState )
	{
		m_sizeAndStatus = ( m_sizeAndStatus & ~c_tlsfBlockStatusMask ) | previousPhysicalState | blockState;
	}

	/////////////////////////////////////////////////////////////////////
	// SetBlockState
	//	Set both parts of a block state mask at once
	RED_MEMORY_INLINE void TLSFBlockHeader::SetSizeAndState( u32 size, u32 previousPhysicalState, u32 blockState )
	{
		RED_MEMORY_ASSERT(( size & ( c_tlsfMinimumAlignment - 1 ) ) == 0, "Attempting to set a block to a badly aligned size" );
		m_sizeAndStatus = ( size & ~c_tlsfBlockStatusMask ) | previousPhysicalState | blockState;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetFreeListPointers( TLSFBlockHeader* previous, TLSFBlockHeader* next )
	{
		m_freeBlockData.prevSegregated = previous;
		m_freeBlockData.nextSegregated = next;
	}

	RED_MEMORY_INLINE bool TLSFBlockHeader::IsFreeBlock() const
	{
		return ( m_sizeAndStatus & c_tlsfBlockFreeStatusMask ) == c_tlsfBlockIsFree;
	}

	RED_MEMORY_INLINE bool TLSFBlockHeader::IsPreviousBlockFree() const
	{
		return (m_sizeAndStatus & c_tlsfBlockPrevStateMask) == c_tlsfPreviousBlockIsFree;
	}

	RED_MEMORY_INLINE TLSFBlockHeader* TLSFBlockHeader::GetNextFree() const
	{
		return m_freeBlockData.nextSegregated;
	}

	RED_MEMORY_INLINE TLSFBlockHeader* TLSFBlockHeader::GetPreviousFree() const
	{
		return m_freeBlockData.prevSegregated;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetNextFree( TLSFBlockHeader* next )
	{
		RED_MEMORY_ASSERT( next == nullptr || (next != nullptr && IsFreeBlock()), "Cannot set next pointer when this block is not free" );
		m_freeBlockData.nextSegregated = next;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::SetPreviousFree( TLSFBlockHeader* prev )
	{
		RED_MEMORY_ASSERT( prev == nullptr || (prev != nullptr && IsFreeBlock()), "Cannot set previous pointer when this block is not free" );
		m_freeBlockData.prevSegregated = prev;
	}

	RED_MEMORY_INLINE TLSFBlockHeader* TLSFBlockHeader::GetPreviousPhysical() const
	{
		return m_previousPhysical;
	}

	RED_MEMORY_INLINE void TLSFBlockHeader::VerifyBlockState()
	{
#ifdef _DEBUG
		// TODO
		/*red::MemUint myAddress = reinterpret_cast< red::MemUint >( this );
		RED_UNUSED( myAddress );

		// Test block header alignment
		RED_MEMORY_ASSERT( (myAddress & (TLSF::c_MinimumPoolAlignment-1)) == 0, "Misaligned block pointer" );

		// Test previous block address + size = my address
		if( m_previousPhysical != nullptr )
		{
			red::MemUint prevPhysAddress = reinterpret_cast< red::MemUint >( m_previousPhysical );
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
		}*/
#endif
	}

	RED_MEMORY_INLINE TLSFBlockHeader * GetTLSFBlockHeader( u64 block )
	{
		TLSFBlockHeader * blockHeader = reinterpret_cast< TLSFBlockHeader* >( block - c_tlsfUsedBlockHeaderSize ); 
		return blockHeader;
	}

	RED_MEMORY_INLINE TLSFBlockHeader * GetTLSFBlockHeader( const Block & block )
	{
		return GetTLSFBlockHeader( block.address );
	}

	RED_MEMORY_INLINE u32 GetTLSFBlockSize( u64 block )
	{
		return GetTLSFBlockHeader( block )->GetDataSize();
	}
}
}

#endif
