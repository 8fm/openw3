/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redThreads/redThreadsThread.h"

#include "redIOCommon.h"
#include "redIOFile.h"
#include "redIOTempBitset.h"

REDIO_NAMESPACE_BEGIN

typedef OSAPI::CAsyncFile CAsyncFile;

//////////////////////////////////////////////////////////////////////////
// CAsyncFileHandleCache
//////////////////////////////////////////////////////////////////////////
class CAsyncFileHandleCache
{
	REDIO_NOCOPY_CLASS( CAsyncFileHandleCache );

public:
	typedef Uint32 TFileHandle;

public:
	static const TFileHandle INVALID_FILE_HANDLE = 0xFFFFFFFF;

private:
	typedef ::Red::Threads::CMutex					CMutex;
	typedef ::Red::Threads::CScopedLock< CMutex >	CScopedLock;

private:
	static const Uint32 AlignOffset = 0xF;
	static const Uint32 MaxHandleId = 0x1FFFFF; // 21 bits unsigned.
	static const Uint32 MaxUseCount = 0x1FF; // m_useCount max

	static const Uint32 MaxAsyncFileHandles = 512;
	static const Uint32 HashTableSize = 521;  // Prime number >= MaxAsyncFileHandles
	static const Uint32 MaxPathLength = 256;

	static const Uint32 HandleIdBits = 21;

private:
	struct SPathEntry
	{
		Char m_buf[ MaxPathLength ];
	};

	struct SCacheEntryData
	{
		Uint8 m_closeWhenNotUsed	:1; // For files that are accessed one-time or infrequently
		Uint8						:0;

		RED_FORCE_INLINE Bool CloseWhenNotUsed() const { return m_closeWhenNotUsed != 0; }
	};

	// 12 bytes to fit more into the cacheline
	struct SCacheEntry
	{
		// 4 bytes
		Uint32	m_isValid:1;				 // Makes it easier to check for a stale handle than having an invalid ID instead.
		Int32	m_fileTableIndex:10;		 // [ ... -1, 0, ... MaxAsyncFileHandles)
		Uint32	m_id:21;					 // [ 0, 0x1FFFFF ]. 21 bits since matching ID used in TFileHandle for cacheEntryIndices, which are 11 bits
		
		// 4 bytes
		Uint32	m_absolutePathHash;
		

		Int32	m_useCount:10;
		Int32	m_lruNextCacheEntryIndex:11; // [ ... -1, 0, ... HashTableSize )
		Int32	m_lruPrevCacheEntryIndex:11; // [ ... -1, 0, ... HashTableSize )
	};

	static_assert( sizeof(SCacheEntry) == 12, "Nothing magic about 12, but why is it now bigger?" );

private:
	// Cache entry hash table
	SCacheEntry							m_cacheEntryHashTable[ HashTableSize ];

	// File table
	SPathEntry							m_pathEntries[ MaxAsyncFileHandles ];
	SCacheEntryData						m_entryData[ MaxAsyncFileHandles ];
	CAsyncFile*							m_fileEntries;
	Uint8								m_fileEntriesBuf[ MaxAsyncFileHandles * sizeof(CAsyncFile) + AlignOffset];

private:
	RED_FORCE_INLINE Int32				GetCacheEntryIndex_NoSync( const SCacheEntry& entry ) const { return Int32(&entry - &m_cacheEntryHashTable[0]); }
	CAsyncFile*							GetCacheEntryFile_NoSync( const SCacheEntry& entry ) const;

private:
	TBitSet< MaxAsyncFileHandles >		m_fileTableFreeMask;

private:
	const Uint32						m_softLimitNumCacheEntries;
	const Uint32						m_hardLimitNumCacheEntries;
	Uint32								m_numCacheEntries;
	
	Uint32								m_statPeakNumCacheEntries;
	Uint32								m_statNumEvictedCacheEntries;

private:
	Int32								m_lruListHeadCacheEntryIndex;	// Index to the most recently used cache entry
	Int32								m_lruListTailCacheEntryIndex;	// Index to the least recently used cache entry

private:
	Uint32								m_idPool;

private:
	mutable CMutex						m_mutex;

public:
	CAsyncFileHandleCache( Uint32 softLimitNumCacheEntries, Uint32 hardLimitNumCacheEntries );
	~CAsyncFileHandleCache();

public:
	TFileHandle							Open( const Char* absoluteFilePath, Uint32 asyncFlags );
	void								Release( TFileHandle handle );
	void								ScrubCache();
	CAsyncFile*							GetAsyncFile_AddRef( TFileHandle handle );
	const Char*							GetFileName( TFileHandle handle ) const;

	struct SDebugCacheEntry
	{
		const Char*		m_filePath;
		TFileHandle		m_fh;
		Uint32			m_hash;
		Uint16			m_hashTableIndex;
		Uint16			m_calcHashTableIndex;
		Uint16			m_useCount;
		Uint16			m_lru;
		SCacheEntryData	m_data;
	};

	struct SDebugStats
	{
		Uint32	m_numEntries;
		Uint32	m_peakEntries;
		Uint32	m_softLimit;
		Uint32	m_hardLimit;
		Uint32	m_numEvictions;
	};

	void								GetStatsForDebug( SDebugStats& outStats ) const;
	void								GetCacheEntriesForDebug( SDebugCacheEntry outEntries[], Uint32 capacity, Uint32* outNumEntries ) const;

private:
	TFileHandle							MakeFileHandle_NoSync( const SCacheEntry& cacheEntry ) const;

private:
	void								AddRef_NoSync( SCacheEntry& cacheEntry );
	Int32								Release_NoSync( SCacheEntry& cacheEntry );

private:
	void								MoveToFrontLRU_NoSync( SCacheEntry& cacheEntry );
	void								UnlinkLRU_NoSync( SCacheEntry& cacheEntry );
	Bool								TryEvictHandlesLRU_NoSync( Uint32 limitNumCacheEntries );

private:
	SCacheEntry*						FindFileHandleByPath_NoSync( const Char* absolutePath );
	SCacheEntry*						FindCacheEntryByHandle_NoSync( TFileHandle handle );

private:
	SCacheEntry*						OpenCacheEntry_NoSync( const Char* absoluteFilePath );
	void								CloseCacheEntry_NoSync( SCacheEntry* cacheEntry );

private:
	void								ForceClearCache();
};

REDIO_NAMESPACE_END
