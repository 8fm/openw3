/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"

#include "AkChunkRing.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

static AkUInt32 kSparseChunkSize = 2048;

void AkChunkRing::Init( AkMemPoolId in_PoolId, AkUInt32 in_ulSize )
{
	m_pStart = (AkUInt8 *) AK::MemoryMgr::GetBlock( in_PoolId );
	AKASSERT( m_pStart );

	m_pRead = m_pStart;
	m_pWrite = m_pStart;

	m_pEnd = m_pStart + in_ulSize;
	m_pVirtualEnd = m_pEnd;
}

void AkChunkRing::Term( AkMemPoolId in_PoolId )
{
	if ( m_pStart )
	{
		AK::MemoryMgr::ReleaseBlock( in_PoolId, m_pStart );
		m_pStart = NULL;
	}
}

void * AkChunkRing::BeginWrite( AkInt32 in_lSize )
{
	in_lSize += 4;    // so that pRead == pWrite never means a full buffer.
	in_lSize &= ~0x03; 

	m_writeLock.Lock();

	AkUInt8 * pRead = m_pRead; // ensure consistency if m_pRead changes while we're doing stuff

	if ( pRead > m_pWrite ) // simple case : contiguous free space
	{
		// NOTE: in theory the empty space goes to m_pEnd if pRead == m_pVirtualEnd, but
		// this breaks our lock-free model.

		if ( in_lSize < ( pRead - m_pWrite ) )
			return m_pWrite;
	}
	else
	{
		if ( in_lSize < ( m_pEnd - m_pWrite ) ) // fits in the remaining space before the end ?
			return m_pWrite;

		if ( pRead != m_pWrite )
		{
			if ( pRead == m_pVirtualEnd )
			{
				m_writeLock.Unlock();
				return NULL;
			}
		}

		if ( in_lSize < ( pRead - m_pStart ) ) // fits in the space before the read ptr ?
			return m_pStart;
	}

	m_writeLock.Unlock();
	return NULL;
}

void AkChunkRing::EndWrite( void * in_pWritePtr, AkInt32 in_lSize )
{
	AKASSERT( in_pWritePtr );
	AKASSERT( in_lSize != 0 ); // algorithm depends on this

	in_lSize += 3;
	in_lSize &= ~0x03; // Data is always aligned to 4 bytes.

	AkUInt8 * pOldWrite = m_pWrite;

	{
		AkAutoLock<CAkLock> lock( m_readLock );

		m_pWrite = (AkUInt8 *) in_pWritePtr + in_lSize;

		if ( in_pWritePtr == m_pStart ) // wrapped around ? set the new virtual end to last position
		{
			if ( pOldWrite != in_pWritePtr ) // exclude starting condition
				m_pVirtualEnd = pOldWrite;
		}
		else if ( m_pWrite > m_pVirtualEnd ) // went past virtual end ? move it
		{
			m_pVirtualEnd = m_pWrite + 4;
		}
	}

	m_writeLock.Unlock();
}

//------------------------------------------------------
// Sparse ring buffer class
//
AKRESULT AkSparseChunkRing::Init(AkUInt32 in_uNumChunks)
{
	// minimum of two blocks is required
	if(in_uNumChunks < 2)
	{
		in_uNumChunks = 2;
	}

	m_uMaxChunks = in_uNumChunks;

	for ( unsigned int i = 0; i < 2; ++i )
	{
		void * pData = AkAlloc( g_DefaultPoolId, kSparseChunkSize );
		if ( pData == NULL )
		{
			return AK_InsufficientMemory;
		}

		Block * pBlock = blocks.AddLast();
		if ( !pBlock )
		{
			AkFree( g_DefaultPoolId, pData );
			return AK_InsufficientMemory;
		}

		pBlock->pData = pData;
		pBlock->uUsedSize = 0;
	}

	return AK_Success;
}

void AkSparseChunkRing::Term()
{
	for ( BlockArray::Iterator it = blocks.Begin(); it != blocks.End(); ++it )
	{
		AkFree( g_DefaultPoolId, (*it).pData );
	}

	blocks.Term();
}

void * AkSparseChunkRing::BeginRead()
{
	AKASSERT( !IsEmpty() );

	AkUInt32 uReadBlock  = m_uReadBlock;
	AkUInt32 uReadOffset = m_uReadOffset;

	Block * pBlock = &( blocks[ uReadBlock ] );

	if ( uReadOffset < pBlock->uUsedSize )
	{
		return (AkUInt8 *) pBlock->pData + uReadOffset;
	}
	else
	{
        m_uReadOffset = 0;

		if ( ++uReadBlock >= blocks.Length() )
			uReadBlock = 0;

		m_uReadBlock = uReadBlock;

		pBlock = &( blocks[ uReadBlock ] );

		return (AkUInt8 *) pBlock->pData;
	}
}

void * AkSparseChunkRing::BeginReadEx( AkUInt32 & out_uSizeAvail )
{
	AKASSERT( !IsEmpty() );

	AkUInt32 uReadBlock = m_uReadBlock;
	AkUInt32 uReadOffset = m_uReadOffset;

	Block * pBlock = &( blocks[ uReadBlock ] );

	if ( uReadOffset < pBlock->uUsedSize )
	{
		out_uSizeAvail = pBlock->uUsedSize - uReadOffset;

		return (AkUInt8 *) pBlock->pData + uReadOffset;
	}
	else
	{
        m_uReadOffset = 0;

		if ( ++uReadBlock >= blocks.Length() )
			uReadBlock = 0;

		m_uReadBlock = uReadBlock;

		pBlock = &( blocks[ uReadBlock ] );

		out_uSizeAvail = pBlock->uUsedSize;

		return (AkUInt8 *) pBlock->pData;
	}
}

void AkSparseChunkRing::EndRead( AkUInt32 in_uSize )
{
	m_uReadOffset += in_uSize;
	
	// We have been read from, so now we can update for writing
	m_bBufferFull = false;

	// should never push past the marked end of block
	AKASSERT( m_uReadOffset <= blocks[ m_uReadBlock ].uUsedSize ); 

	// remove from the total count
#ifndef AK_OPTIMIZED
	m_TotalUsedSize -= in_uSize;
#endif
}

AKRESULT AkSparseChunkRing::Write( void * in_pData, AkUInt32 in_uSize )
{
	AKASSERT( !( in_uSize % 4 ) );
	AKASSERT( in_uSize <= kSparseChunkSize );

	if ( kSparseChunkSize - m_uWriteOffset >= in_uSize )
	{
		// Enough space in current block.

		AkUInt8 * pucData = (AkUInt8 *) blocks[ m_uWriteBlock ].pData + m_uWriteOffset;

		AKPLATFORM::AkMemCpy( pucData, in_pData, in_uSize );

		m_uWriteOffset += in_uSize;
	}
	else
	{
		// Don't do anything until reads have been made
		if(m_bBufferFull)
		{
			return AK_InsufficientMemory;
		}

		// move to next block
		AkUInt32 uWriteBlock = m_uWriteBlock + 1; // use temp variable to keep members intact in case of out-of-memory
		if ( uWriteBlock >= blocks.Length() )
			uWriteBlock = 0;

		void * pData;

		// write pointer is not allowed to follow read pointer in the same block, as it might cause
		// a situation where it is impossible to grow.
        if ( m_uReadBlock == uWriteBlock )
		{
			// see if we should add another block to the ring buffer
			if(blocks.Length() >= m_uMaxChunks)
			{
				m_bBufferFull = true;

				// Return and let the caller handle this
				return AK_InsufficientMemory;
			}

			// need to push read block ahead to make space for new block.
			pData = AkAlloc( g_DefaultPoolId, kSparseChunkSize );
			if ( !pData )
			{
				m_bBufferFull = true;

				return AK_InsufficientMemory;
			}

			Block * pBlock = blocks.Insert( uWriteBlock );
			if ( !pBlock )
			{
				m_bBufferFull = true;

				AkFree( g_DefaultPoolId, pData );
				return AK_InsufficientMemory; 
			}

			pBlock->pData = pData;

			++m_uReadBlock;
		}
		else // next block is free
		{
			pData = blocks[ uWriteBlock ].pData;
		}

		// Write at start of new block.

		m_uWriteBlock = uWriteBlock;
		m_uWriteOffset = in_uSize;

		AKPLATFORM::AkMemCpy( pData, in_pData, in_uSize );
	}

	blocks[ m_uWriteBlock ].uUsedSize = m_uWriteOffset;

	// Update total count here
#ifndef AK_OPTIMIZED
	m_TotalUsedSize += in_uSize;
#endif

	return AK_Success;
}

#ifndef AK_OPTIMIZED
AkUInt32 AkSparseChunkRing::GetTotalUsed()
{
	AkUInt32 result;
	
	if(m_TotalUsedSize < 0)
	{
		result = 0;
	}
	else if ((AkUInt32)m_TotalUsedSize > GetActualSize())
	{
		result = GetActualSize();
	}
	else
	{
		result = (AkUInt32)m_TotalUsedSize;
	}

	return result;
}

AkUInt32 AkSparseChunkRing::GetActualSize()
{
	return blocks.Length() * kSparseChunkSize;
}

AkReal32 AkSparseChunkRing::GetPercentageUsed()
{
	return (AkReal32)m_TotalUsedSize / (AkReal32)GetActualSize();
}

#endif

AkUInt32 AkSparseChunkRing::GetChunkSize()
{
	return kSparseChunkSize;
}

