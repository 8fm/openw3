/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "diskFile.h"

/// Entry in the cache
class CDiskBundleCacheEntry;
class CDiskBundleCacheEntryMemoryBlock;

/// Cache for raw bundle data - allows some bundles to be kept fully or partially in the memory without loading the content
class CDiskBundleCache
{
public:
	CDiskBundleCache();
	~CDiskBundleCache();

	// Add file data to cache, NOTE: the data gets copied
	void AddCachedData( const Red::Core::Bundle::FileID id, Uint8 compressionType, const Uint32 uncompressedSize, CDiskBundleCacheEntryMemoryBlock* compressedData );

	// Remove cached data for given file
	void RemoveCachedData( const Red::Core::Bundle::FileID id );

	// Remove all cached data
	void RemoveAllCachedData();

	// Create asynchronous reader for given file ID
	// Fails if the file is not cached (it returns null)
	// NOTE: the cached memory will be referenced by the reader and will not be release for the duration of the read
	IAsyncFile* CreateAsyncReader( const Red::Core::Bundle::FileID fileID ) const;

	// Create asynchronous reader for given file ID
	// Files if the file is not cached (it returns NULL to indicate that)
	// NOTE: the cached memory will be referenced by the reader and will not be release for the duration of the read
	IFile* CreateReader( const Red::Core::Bundle::FileID fileID ) const;

private:
	typedef THashMap< Red::Core::Bundle::FileID, CDiskBundleCacheEntry* >			TEntries;
	typedef Red::Threads::CMutex													TListLock;

	// cache entries
	TEntries				m_entries;
	mutable TListLock		m_entriesLock;
};
