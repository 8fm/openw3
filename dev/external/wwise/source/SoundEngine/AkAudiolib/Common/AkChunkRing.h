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

#ifndef _AK_CHUNK_RING_H_
#define _AK_CHUNK_RING_H_

#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>

#include <AK/Tools/Common/AkArray.h>
#include "AkPoolSizes.h"


// Ring Buffer class. This is a fairly task-specific class, it is optimized for the case where:
// - Reading from a single thread.
// - Writing from multiple threads.
// - Data is made of chunks that need to be contiguous in memory (for example, structs).
// - Data chunks are of varying size (reader MUST be able to tell the size of a chunk by peeking in it).
// - Data chunks are multiple of align size.
// Due to the 'contiguous chunk' requirement, there is usually a bit of wasted space near the end of the
// buffer (equal to m_pEnd - m_pVirtualEnd) -- but this is nothing in comparison to the overhead of a
// linked list and fixed block memory allocator managing the same buffer in this case.

class AkChunkRing
{
public:
	AkChunkRing()
		: m_pStart( NULL )
	{
	}

	~AkChunkRing()
	{
		AKASSERT( m_pStart == NULL );
	}

	void Init( AkMemPoolId in_PoolId, AkUInt32 in_ulSize );
	void Term( AkMemPoolId in_PoolId );

	bool IsEmpty()
	{
		return m_pRead == m_pWrite;
	}

	void * BeginRead()
	{
		AkAutoLock<CAkLock> lock( m_readLock );
		AKASSERT( !IsEmpty() );

		return ( m_pRead == m_pVirtualEnd ) ? m_pStart : m_pRead;
	}

	void EndRead( void * in_pReadPtr, AkUInt32 in_ulSize )
	{
		in_ulSize += 3;
		in_ulSize &= ~0x03; // Data is always aligned to 4 bytes.

		m_pRead = (AkUInt8 *) in_pReadPtr + in_ulSize;
		AKASSERT( m_pRead <= m_pVirtualEnd );
	}

	// Start writing. Input size is for reservation purpose; final size (passed to EndWrite)
	// needs to be equal or less. If this method returns a buffer, you NEED to call EndWrite
	// as the write lock is being held.
	void *	BeginWrite( AkInt32 in_lSize );

	// Stop writing. Need to pass pointer returned by BeginWrite().
	void EndWrite( void * in_pWritePtr, AkInt32 in_lSize );

	AkForceInline void LockWrite() { m_writeLock.Lock(); }
	AkForceInline void UnlockWrite() { m_writeLock.Unlock(); }

private:
	AkUInt8 * volatile m_pRead;			// Read position in buffer
	AkUInt8 * volatile m_pWrite;		// Write position in buffer

	AkUInt8 * volatile m_pStart;		// Memory start of buffer
	AkUInt8 * volatile m_pVirtualEnd;	// Memory end of used buffer -- changes each time 'write' wraps around
	AkUInt8 * volatile m_pEnd;			// Memory end of buffer

	CAkLock m_readLock;					// Read lock; only necessary because m_pWrite and m_pVirtualEnd need to be 
										// changed 'together' atomically from the reader's point of view.
	CAkLock m_writeLock;
};

// Sparse ring buffer class. It divides itself into chunk of reasonable size,
// so that it can gracefully grow the area between the write pointer and the 
// read pointer as long as they are separated by at least one empty chunk.

// Has no internal locking.

class AkSparseChunkRing
{
public:
	AkSparseChunkRing()
		: m_uReadBlock( 0 )
		, m_uReadOffset( 0 )
		, m_uWriteBlock( 0 )
		, m_uWriteOffset( 0 )
#ifndef AK_OPTIMIZED
		, m_TotalUsedSize( 0 )
#endif
		, m_bBufferFull (false)
	{
	}

	~AkSparseChunkRing()
	{
	}

	AKRESULT Init(AkUInt32 in_uNumChunks);
	void Term();

	AkForceInline bool IsEmpty() 
	{
		bool result = (( m_uReadBlock == m_uWriteBlock ) && ( m_uReadOffset == m_uWriteOffset ) && !m_bBufferFull);

		return result;
	}

	// IMPORTANT: make sure !IsEmpty() before calling BeginRead/BeginReadEx
	void * BeginRead();
	void * BeginReadEx( AkUInt32 & out_uSizeAvail );

	void EndRead( AkUInt32 in_uSize );
	AKRESULT Write( void * in_pData, AkUInt32 in_uSize );

#ifndef AK_OPTIMIZED
	AkUInt32		GetTotalUsed();			// total amount used in bytes
	AkUInt32		GetActualSize();		// total size of ring buffer in bytes
	AkReal32		GetPercentageUsed();	// from 0.0 -> 1.0f
#endif

	static AkUInt32 GetChunkSize();		// The size of one chunk of data in the ring buffer

private:
	struct Block
	{
		void *   pData;
		AkUInt32 uUsedSize;
	};

	typedef AkArray<Block, const Block &, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof(Block)> BlockArray;

	BlockArray blocks;

	AkUInt32 m_uMaxChunks;		// The maximum size this ring buffer can grow

	AkUInt32 m_uReadBlock;
	AkUInt32 m_uReadOffset;

	AkUInt32 m_uWriteBlock;
	AkUInt32 m_uWriteOffset;

#ifndef AK_OPTIMIZED
	AkInt32	 m_TotalUsedSize;	// The total used size of the buffer
#endif
	
	bool	 m_bBufferFull;		// True if we've filled up the ring buffer
};

#endif
