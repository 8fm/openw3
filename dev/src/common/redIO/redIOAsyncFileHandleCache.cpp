/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "../redSystem/crt.h"
#include "../redSystem/error.h"
#include "../redSystem/nameHash.h"

#include "redIOAsyncFileHandleCache.h"
#include "redIOFile.h"

#include "../core/fileSystemProfilerWrapper.h"

#include <new>

REDIO_NAMESPACE_BEGIN

#define REDIO_DEBUG_FH_CACHE 0

//////////////////////////////////////////////////////////////////////////
// FIsFilePathEqual
//////////////////////////////////////////////////////////////////////////
RED_FORCE_INLINE Bool FIsFilePathEqual( const Char* const a, const Char* const b )
{
	RED_FATAL_ASSERT( a, "filePath cannot be null" );
	RED_FATAL_ASSERT( b, "filePath cannot be null" );

	const Char* pa = a;
	const Char* pb = b;

	for(;;)
	{
		Char ach = *pa++;
		Char bch = *pb++;
		if ( ach == TXT('/') )
			ach = TXT('\\');
		if ( bch == TXT('/') )
			bch = TXT('\\');
		if ( ach >= TXT('A') && ach <= TXT('Z') )
			ach += TXT('a')-TXT('A');
		if ( bch >= TXT('A') && bch <= TXT('Z') )
			bch += TXT('a')-TXT('A');
		if ( ach != bch )
			return false;
		if ( !ach )
			break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// FHashFilePath
//////////////////////////////////////////////////////////////////////////
RED_FORCE_INLINE Uint32 FHashFilePath( const Char* filePath )
{
	RED_FATAL_ASSERT( filePath, "filePath cannot be null" );

	static const Uint32 HASH_OFFSET = 2166136261u;
	static const Uint32 HASH_PRIME = 16777619u;

	Uint32 hash = HASH_OFFSET;
	for ( const Char* pch = filePath; *pch; ++pch )
	{
		Char ch = *pch;
		if ( ch == TXT('/') )
			ch = TXT('\\');
		if ( ch >= TXT('A') && ch <= TXT('Z') )
			ch += TXT('a')-TXT('A');

		hash ^= Uint32(ch);
		hash *= HASH_PRIME;
	}

	return hash;
}

//////////////////////////////////////////////////////////////////////////
// FGetFileCacheEntryIndex
//////////////////////////////////////////////////////////////////////////
RED_FORCE_INLINE Int32 FGetFileCacheEntryIndex( CAsyncFileHandleCache::TFileHandle handle )
{
	struct { Int32 m_cacheEntryIndex:11; } s;
	s.m_cacheEntryIndex = handle;
	return s.m_cacheEntryIndex;
}

//////////////////////////////////////////////////////////////////////////
// FGetFileTableIndex
//////////////////////////////////////////////////////////////////////////
RED_FORCE_INLINE Int32 FGetFileTableIndex( CAsyncFileHandleCache::TFileHandle handle )
{
	struct { Int32 m_fileTableIndex:10; } s;
	s.m_fileTableIndex = handle;
	return s.m_fileTableIndex;
}

//////////////////////////////////////////////////////////////////////////
// FGetFileHandleID
//////////////////////////////////////////////////////////////////////////
RED_FORCE_INLINE Uint32 FGetFileHandleID( CAsyncFileHandleCache::TFileHandle handle )
{
	return ( handle & 0xFFFFF800 ) >> 11;
}

//////////////////////////////////////////////////////////////////////////
// CAsyncFileHandleCache
//////////////////////////////////////////////////////////////////////////
CAsyncFileHandleCache::CAsyncFileHandleCache( Uint32 softLimitNumCacheEntries, Uint32 hardLimitNumCacheEntries )
	: m_softLimitNumCacheEntries( softLimitNumCacheEntries )
	, m_hardLimitNumCacheEntries( hardLimitNumCacheEntries )
	, m_numCacheEntries( 0 )
	, m_statPeakNumCacheEntries( 0 )
	, m_statNumEvictedCacheEntries( 0 )
	, m_lruListHeadCacheEntryIndex( -1 )
	, m_lruListTailCacheEntryIndex( -1 )
	, m_idPool( 0 )
{
	RED_FATAL_ASSERT( softLimitNumCacheEntries > 0, "Invalid soft limit");
	RED_FATAL_ASSERT( hardLimitNumCacheEntries <= MaxAsyncFileHandles, "Invalid hard limit" );
	RED_FATAL_ASSERT( softLimitNumCacheEntries <= hardLimitNumCacheEntries, "Soft limit cannot exceed hard limit" );

	const uintptr_t bufAddr = reinterpret_cast< uintptr_t >( m_fileEntriesBuf );
	const uintptr_t adjust = (bufAddr & AlignOffset);
	m_fileEntries = reinterpret_cast< CAsyncFile* >( m_fileEntriesBuf + adjust );

	Red::System::MemoryZero( m_cacheEntryHashTable, sizeof(m_cacheEntryHashTable) );
	
	m_fileTableFreeMask.SetAll();

#if REDIO_DEBUG_FH_CACHE
	Red::System::MemoryZero( m_fileEntriesBuf, sizeof(m_fileEntriesBuf) );
	Red::System::MemoryZero( m_pathEntries, sizeof(m_pathEntries) );
	Red::System::MemoryZero( m_entryData, sizeof(m_entryData) );
#endif
}

CAsyncFileHandleCache::~CAsyncFileHandleCache()
{
	// close any dangling handles
	ForceClearCache();

#ifdef RED_ASSERTS_ENABLED
	CScopedLock lock( m_mutex );

	for ( Uint32 i = 0; i < HashTableSize; ++i )
	{
		SCacheEntry& cacheEntry = m_cacheEntryHashTable[ i ];
		RED_FATAL_ASSERT( ! cacheEntry.m_isValid, "Cache entry still in use at shutdown" );
	}
#endif
}

void CAsyncFileHandleCache::ForceClearCache()
{
	CScopedLock lock( m_mutex );

	for ( Uint32 i = 0; i < HashTableSize; ++i )
	{
		SCacheEntry& cacheEntry = m_cacheEntryHashTable[ i ];
		if ( cacheEntry.m_isValid )
		{
			if ( cacheEntry.m_useCount > 0 )
			{
				RED_FATAL_ASSERT( cacheEntry.m_fileTableIndex >= 0, "Invalid file index %d", cacheEntry.m_fileTableIndex );
#ifdef RED_LOGGING_ENABLED
				const SPathEntry& pathEntry = m_pathEntries[ cacheEntry.m_fileTableIndex ];
				REDIO_ERR(TXT("CAsyncFileHandleCache::ForceClearCache file handle %u ('%ls') closed while still in use (use count %d)"), cacheEntry.m_id, pathEntry.m_buf, cacheEntry.m_useCount);
#endif
				cacheEntry.m_useCount = 0; // Avoid fatal asserts or other further warnings when closing
			}
			CloseCacheEntry_NoSync( &cacheEntry );
		}
	}
}

void CAsyncFileHandleCache::AddRef_NoSync( SCacheEntry& cacheEntry )
{
	RED_FATAL_ASSERT( cacheEntry.m_useCount < MaxUseCount, "Usecount overflow!");
	++cacheEntry.m_useCount;
}

Int32 CAsyncFileHandleCache::Release_NoSync( SCacheEntry& cacheEntry )
{
	RED_FATAL_ASSERT( cacheEntry.m_useCount > 0, "Usecount underflow!");
	--cacheEntry.m_useCount;
	return cacheEntry.m_useCount;
}

CAsyncFileHandleCache::TFileHandle CAsyncFileHandleCache::Open( const Char* absoluteFilePath, Uint32 asyncFlags )
{
	if ( ! absoluteFilePath || !absoluteFilePath[0] )
	{
		return INVALID_FILE_HANDLE;
	}

	// TBD: Verify path normalized.

	CScopedLock lock( m_mutex );

	SCacheEntry* cacheEntry = FindFileHandleByPath_NoSync( absoluteFilePath );
	if ( ! cacheEntry && ( m_numCacheEntries < m_hardLimitNumCacheEntries || TryEvictHandlesLRU_NoSync(m_hardLimitNumCacheEntries-1) ) )
	{
		// FIXME: If file not found. Should return error codes/log.
		cacheEntry = OpenCacheEntry_NoSync( absoluteFilePath );
	}

	if ( ! cacheEntry )
	{
		return INVALID_FILE_HANDLE;
	}

	AddRef_NoSync( *cacheEntry );
	MoveToFrontLRU_NoSync( *cacheEntry );

	// Check if eAsyncFlag_CloseWhenNotUsed *not* set, so keep it open when use count reaches zero
	// Can't undo this, since can't know for sure what's releasing the FH
	if ( (asyncFlags & eAsyncFlag_TryCloseFileWhenNotUsed) == 0 )
	{
		SCacheEntryData& data = m_entryData[ cacheEntry->m_fileTableIndex ];
		data.m_closeWhenNotUsed = false;
	}

	return MakeFileHandle_NoSync( *cacheEntry );
}

void CAsyncFileHandleCache::Release( TFileHandle handle )
{
	CScopedLock lock( m_mutex );

	SCacheEntry* cacheEntry = FindCacheEntryByHandle_NoSync( handle );
	if ( ! cacheEntry )
	{
		return;
	}

	const Int32 newUseCount = Release_NoSync( *cacheEntry );
	if ( newUseCount == 0 )
	{
		const SCacheEntryData& data = m_entryData[ cacheEntry->m_fileTableIndex ];
		if ( data.CloseWhenNotUsed() )
		{
			CloseCacheEntry_NoSync( cacheEntry );
		}
		else if ( m_numCacheEntries > m_softLimitNumCacheEntries )
		{
			// Try to bring the number of cache entries back to the soft limit
			// Consider all entries to avoid cache spills. Worst case is we remove the current cacheEntry.
			TryEvictHandlesLRU_NoSync( m_softLimitNumCacheEntries );
		}
	}
}

const Char* CAsyncFileHandleCache::GetFileName( TFileHandle handle ) const
{
	CScopedLock lock( m_mutex );

	SCacheEntry* cacheEntry = const_cast< CAsyncFileHandleCache* >( this )->FindCacheEntryByHandle_NoSync( handle );
	if ( ! cacheEntry )
	{
		return nullptr;
	}

	RED_FATAL_ASSERT( cacheEntry->m_fileTableIndex >= 0, "Invalid file index %d", cacheEntry->m_fileTableIndex );
	const SPathEntry& pathEntry = m_pathEntries[ cacheEntry->m_fileTableIndex ];

	return pathEntry.m_buf;
}

void CAsyncFileHandleCache::GetStatsForDebug( SDebugStats& outStats ) const
{
	CScopedLock lock( m_mutex );

	outStats.m_numEntries = m_numCacheEntries;
	outStats.m_peakEntries = m_statPeakNumCacheEntries;
	outStats.m_softLimit = m_softLimitNumCacheEntries;
	outStats.m_hardLimit = m_hardLimitNumCacheEntries;
	outStats.m_numEvictions = m_statNumEvictedCacheEntries;
}

void CAsyncFileHandleCache::GetCacheEntriesForDebug( SDebugCacheEntry outEntries[], Uint32 capacity, Uint32* outNumEntries ) const
{
	CScopedLock lock( m_mutex );

	Red::System::MemoryZero( outEntries, sizeof(SDebugCacheEntry)*capacity );

	Uint32 entryCount = 0;

	for ( Uint32 i = 0; i < HashTableSize; ++i )
	{
		const SCacheEntry& cacheEntry = m_cacheEntryHashTable[ i ];
		if ( cacheEntry.m_isValid )
		{
			SDebugCacheEntry& debugEntry = outEntries[ entryCount++ ];
			debugEntry.m_filePath = m_pathEntries[ cacheEntry.m_fileTableIndex ].m_buf;
			debugEntry.m_fh = MakeFileHandle_NoSync( cacheEntry );
			debugEntry.m_hashTableIndex = i;
			debugEntry.m_calcHashTableIndex = FHashFilePath( debugEntry.m_filePath ) % HashTableSize;
			debugEntry.m_hash = cacheEntry.m_absolutePathHash;
			debugEntry.m_useCount = cacheEntry.m_useCount;
			debugEntry.m_data = m_entryData[ cacheEntry.m_fileTableIndex ];

			Int32 lru = 0;
			Int32 lruIndex = m_lruListHeadCacheEntryIndex;
			while ( lruIndex != i && lruIndex >= 0 )
			{
				++lru;
				const SCacheEntry& next = m_cacheEntryHashTable[ lruIndex ];
				lruIndex = next.m_lruNextCacheEntryIndex;
			}

			debugEntry.m_lru = lru;

			if ( entryCount >= capacity )
				break;
		}
	}

	*outNumEntries = entryCount;
}

CAsyncFile* CAsyncFileHandleCache::GetAsyncFile_AddRef( TFileHandle handle )
{
	CScopedLock lock( m_mutex );

	SCacheEntry* cacheEntry = FindCacheEntryByHandle_NoSync( handle );
	if ( ! cacheEntry )
	{
		return nullptr;
	}

	CAsyncFile* asyncFile = GetCacheEntryFile_NoSync( *cacheEntry );
	if ( ! asyncFile )
	{
		return nullptr;
	}

	// TBD: affect LRU?
	++cacheEntry->m_useCount;

	return asyncFile;
}

void CAsyncFileHandleCache::ScrubCache()
{
	CScopedLock lock( m_mutex );

	for ( Uint32 i = 0; i < HashTableSize; ++i )
	{
		SCacheEntry& cacheEntry = m_cacheEntryHashTable[ i ];
		if ( cacheEntry.m_isValid && 0 == cacheEntry.m_useCount )
		{
			CloseCacheEntry_NoSync( &cacheEntry );
		}
	}
}

void CAsyncFileHandleCache::MoveToFrontLRU_NoSync( SCacheEntry& cacheEntry )
{
	const Int32 cacheEntryIndex = GetCacheEntryIndex_NoSync( cacheEntry );

	if ( cacheEntryIndex == m_lruListHeadCacheEntryIndex )
	{
		return; // Already at the front
	}
	else if ( cacheEntryIndex == m_lruListTailCacheEntryIndex )
	{
		m_lruListTailCacheEntryIndex = cacheEntry.m_lruPrevCacheEntryIndex;
	}

	if ( cacheEntry.m_lruPrevCacheEntryIndex >= 0 )
	{
		SCacheEntry& lruPrev = m_cacheEntryHashTable[ cacheEntry.m_lruPrevCacheEntryIndex ];
		lruPrev.m_lruNextCacheEntryIndex = cacheEntry.m_lruNextCacheEntryIndex;
	}

	if ( cacheEntry.m_lruNextCacheEntryIndex >= 0 )
	{
		SCacheEntry& lruNext = m_cacheEntryHashTable[ cacheEntry.m_lruNextCacheEntryIndex ];
		lruNext.m_lruPrevCacheEntryIndex = cacheEntry.m_lruPrevCacheEntryIndex;
	}

	cacheEntry.m_lruNextCacheEntryIndex = m_lruListHeadCacheEntryIndex;
	cacheEntry.m_lruPrevCacheEntryIndex = -1;

	if ( m_lruListHeadCacheEntryIndex >= 0 )
	{
		SCacheEntry& lruListHead = m_cacheEntryHashTable[ m_lruListHeadCacheEntryIndex ];
		lruListHead.m_lruPrevCacheEntryIndex = cacheEntryIndex;
	}

	m_lruListHeadCacheEntryIndex = cacheEntryIndex;

	if ( m_lruListTailCacheEntryIndex < 0 )
	{
		m_lruListTailCacheEntryIndex = m_lruListHeadCacheEntryIndex;
	}
}

void CAsyncFileHandleCache::UnlinkLRU_NoSync( SCacheEntry& cacheEntry )
{
	const Int32 cacheEntryIndex = GetCacheEntryIndex_NoSync( cacheEntry );

	if ( cacheEntryIndex == m_lruListHeadCacheEntryIndex )
	{
		m_lruListHeadCacheEntryIndex = cacheEntry.m_lruNextCacheEntryIndex;
	}

	if ( cacheEntryIndex == m_lruListTailCacheEntryIndex )
	{
		m_lruListTailCacheEntryIndex = cacheEntry.m_lruPrevCacheEntryIndex;
	}

	if ( cacheEntry.m_lruPrevCacheEntryIndex >= 0 )
	{
		SCacheEntry& lruPrev = m_cacheEntryHashTable[ cacheEntry.m_lruPrevCacheEntryIndex ];
		lruPrev.m_lruNextCacheEntryIndex = cacheEntry.m_lruNextCacheEntryIndex;
	}

	if ( cacheEntry.m_lruNextCacheEntryIndex >= 0 )
	{
		SCacheEntry& lruNext = m_cacheEntryHashTable[ cacheEntry.m_lruNextCacheEntryIndex ];
		lruNext.m_lruPrevCacheEntryIndex = cacheEntry.m_lruPrevCacheEntryIndex;
	}

	cacheEntry.m_lruNextCacheEntryIndex = -1;
	cacheEntry.m_lruPrevCacheEntryIndex = -1;
}

Bool CAsyncFileHandleCache::TryEvictHandlesLRU_NoSync( Uint32 limitNumCacheEntries )
{
	Uint32 numEvicted = 0;

	Int32 victimCacheEntryIndex = m_lruListTailCacheEntryIndex;
	while ( victimCacheEntryIndex >= 0 && ( m_numCacheEntries > limitNumCacheEntries ) )
	{
		SCacheEntry& victim = m_cacheEntryHashTable[ victimCacheEntryIndex ];
		Int32 lruPrevCacheEntryIndex = victim.m_lruPrevCacheEntryIndex;
		if ( 0 == victim.m_useCount )
		{
			CloseCacheEntry_NoSync( &victim );
			++numEvicted;
		}
		victimCacheEntryIndex = lruPrevCacheEntryIndex;
	}

	m_statNumEvictedCacheEntries += numEvicted;

	return numEvicted > 0;
}

CAsyncFileHandleCache::SCacheEntry* CAsyncFileHandleCache::FindFileHandleByPath_NoSync( const Char* absolutePath )
{
	const Uint32 hash = FHashFilePath( absolutePath );
	const Uint32 startCacheEntryIndex = hash % HashTableSize;
	Uint32 cacheEntryIndex = startCacheEntryIndex;
	SCacheEntry* cacheEntryToReturn = nullptr;

	do 
	{
		SCacheEntry* cacheEntry = &m_cacheEntryHashTable[ cacheEntryIndex ];
		if ( cacheEntry->m_isValid && cacheEntry->m_absolutePathHash == hash )
		{
			RED_FATAL_ASSERT( cacheEntry->m_fileTableIndex >= 0, "Invalid file index %d", cacheEntry->m_fileTableIndex );
			const SPathEntry& pathEntry = m_pathEntries[ cacheEntry->m_fileTableIndex ];
			if ( FIsFilePathEqual( pathEntry.m_buf, absolutePath ) )
			{
				cacheEntryToReturn = cacheEntry;
				break;
			}
		}
		cacheEntryIndex = ( cacheEntryIndex + 1 ) % HashTableSize;
	}
	while ( cacheEntryIndex != startCacheEntryIndex );

	return cacheEntryToReturn;
}

CAsyncFileHandleCache::SCacheEntry* CAsyncFileHandleCache::FindCacheEntryByHandle_NoSync( TFileHandle handle )
{
	const Int32 cacheEntryIndex = FGetFileCacheEntryIndex( handle );
	if ( cacheEntryIndex < 0 )
	{
		REDIO_ERR(TXT("CAsyncFileHandleCache: Invalid file handle %u"), handle );
		return nullptr;
	}

	SCacheEntry* cacheEntry = &m_cacheEntryHashTable[ cacheEntryIndex ];
	if ( ! cacheEntry->m_isValid || cacheEntry->m_id != FGetFileHandleID( handle ) )
	{
		REDIO_ERR(TXT("CAsyncFileHandleCache::FindCacheEntryByHandle_NoSync: Stale or invalid file handle '%u' used"), handle );
		return nullptr;
	}

	return cacheEntry;
}

CAsyncFileHandleCache::SCacheEntry* CAsyncFileHandleCache::OpenCacheEntry_NoSync( const Char* absoluteFilePath )
{
	const Int32 fileTableIndex = m_fileTableFreeMask.FindNextSet( 0 );
	RED_FATAL_ASSERT( fileTableIndex != MaxAsyncFileHandles, "Can't alloc another file table slot!");
	void* buf = &m_fileEntries[ fileTableIndex ];
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileAsyncIOOpenFileStart( absoluteFilePath );
#endif
	CAsyncFile* file = ::new( buf ) CAsyncFile;
	if ( ! file->Open( absoluteFilePath, eOpenFlag_Read | eOpenFlag_Async ) )
	{
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileAsyncIOOpenFileEnd( 0 );
#endif
		REDIO_WARN(TXT("CAsyncFileHandleCache::Open: Failed to open file handle for '%ls'. Consider lowering the hardlimit to avoid hits to the filesystem"), absoluteFilePath );
		file->~CAsyncFile();
		return nullptr;
	}
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileAsyncIOOpenFileEnd( file->GetFileHandle() );
#endif

	m_fileTableFreeMask.Clear( fileTableIndex );

	SPathEntry& pathEntry = m_pathEntries[ fileTableIndex ];
	Red::System::StringCopy( pathEntry.m_buf, absoluteFilePath, MaxPathLength );
	
	SCacheEntryData& data = m_entryData[ fileTableIndex ];
	Red::System::MemoryZero( &data, sizeof(SCacheEntryData) );
	data.m_closeWhenNotUsed = true;
	
	const Char* absolutePath = m_pathEntries[ fileTableIndex ].m_buf;
	const Uint32 hash = FHashFilePath( absolutePath );
	const Uint32 startCacheEntryIndex = hash % HashTableSize;
	Uint32 cacheEntryIndex = startCacheEntryIndex;
	SCacheEntry* cacheEntryToAlloc = nullptr;

	do 
	{
		SCacheEntry& cacheEntry = m_cacheEntryHashTable[ cacheEntryIndex ];
		if ( ! cacheEntry.m_isValid )
		{
			cacheEntryToAlloc = &cacheEntry;
			break;
		}
		cacheEntryIndex = ( cacheEntryIndex + 1 ) % HashTableSize;
	}
	while ( cacheEntryIndex != startCacheEntryIndex );

	RED_FATAL_ASSERT( cacheEntryToAlloc, "Can't find a free cacheEntry slot!");

	Uint32 id = m_idPool++;
	if ( id > MaxHandleId )
	{
		m_idPool = 0;
		id = m_idPool++;
	}

	cacheEntryToAlloc->m_isValid = 1;
	cacheEntryToAlloc->m_fileTableIndex = fileTableIndex;
	cacheEntryToAlloc->m_id = id;
	cacheEntryToAlloc->m_absolutePathHash = hash;
	cacheEntryToAlloc->m_useCount = 0;
	cacheEntryToAlloc->m_lruNextCacheEntryIndex = -1;
	cacheEntryToAlloc->m_lruPrevCacheEntryIndex = -1;

	++m_numCacheEntries;
	if ( m_numCacheEntries > m_statPeakNumCacheEntries )
		m_statPeakNumCacheEntries = m_numCacheEntries;

	RED_FATAL_ASSERT( m_numCacheEntries <= m_hardLimitNumCacheEntries, "Number of cache entries %u exceeded hardlimit %u!", m_numCacheEntries, m_hardLimitNumCacheEntries );

	return cacheEntryToAlloc;
}

void CAsyncFileHandleCache::CloseCacheEntry_NoSync( SCacheEntry* cacheEntry )
{
	RED_FATAL_ASSERT( cacheEntry, "Cache entry is null" );
	RED_FATAL_ASSERT( cacheEntry->m_useCount == 0, "FreeFile_NoSync: Closing file with refcount %d!", (Int32)cacheEntry->m_useCount );

	CAsyncFile* file = GetCacheEntryFile_NoSync( *cacheEntry );
	RED_FATAL_ASSERT( file, "No file entry!" );
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileAsyncIOCloseFileStart( file->GetFileHandle() );
#endif
	file->~CAsyncFile();
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileAsyncIOCloseFileEnd();
#endif

	RED_FATAL_ASSERT( cacheEntry->m_fileTableIndex >= 0, "Invalid file table index %d", cacheEntry->m_fileTableIndex );
	RED_FATAL_ASSERT( ! m_fileTableFreeMask.Get( cacheEntry->m_fileTableIndex ), "Double free of file index %d", cacheEntry->m_fileTableIndex );
	m_fileTableFreeMask.Set( cacheEntry->m_fileTableIndex );

#if REDIO_DEBUG_FH_CACHE
//	SPathEntry* pathEntry = GetCacheEntryPath( *cacheEntry );
	Red::System::MemorySet( pathEntry->m_buf, 0xDEADBEEF, MaxPathLength );
	Red::System::MemorySet( file, 0xDEADBEEF, sizeof(CAsyncFile) );
#endif

	UnlinkLRU_NoSync( *cacheEntry );
	
	cacheEntry->m_isValid = 0;

#if REDIO_DEBUG_FH_CACHE
	cacheEntry->m_fileTableIndex = -1;
	cacheEntry->m_id = MaxHandleId;
	cacheEntryToFree->m_absolutePathHash = 0;
	cacheEntryToFree->m_useCount = 0;
	cacheEntryToFree->m_lruNextCacheEntryIndex = -1;
	cacheEntryToFree->m_lruPrevCacheEntryIndex = -1;
#endif

	RED_FATAL_ASSERT( m_numCacheEntries > 0, "Number of cache entries underflow!" );
	--m_numCacheEntries;
}

CAsyncFileHandleCache::TFileHandle CAsyncFileHandleCache::MakeFileHandle_NoSync( const SCacheEntry& cacheEntry ) const
{
	const Int32 cacheEntryIndex = GetCacheEntryIndex_NoSync( cacheEntry );
	return (cacheEntry.m_id << (32-HandleIdBits)) | cacheEntryIndex;
}

CAsyncFile* CAsyncFileHandleCache::GetCacheEntryFile_NoSync( const SCacheEntry& entry ) const
{
	if ( ! entry.m_isValid )
	{
		return nullptr;
	}

	RED_FATAL_ASSERT( entry.m_fileTableIndex >= 0, "Invalid file table index %d", entry.m_fileTableIndex );
	CAsyncFile* file = &m_fileEntries[ entry.m_fileTableIndex ];
	return file;
}

REDIO_NAMESPACE_END
