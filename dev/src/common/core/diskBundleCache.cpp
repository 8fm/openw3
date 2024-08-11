/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "diskBundleCache.h"
#include "diskBundleCacheEntry.h"

//-------------------------------------------------------------------

CDiskBundleCache::CDiskBundleCache()
{
}

CDiskBundleCache::~CDiskBundleCache()
{
	RemoveAllCachedData();
}

void CDiskBundleCache::AddCachedData( const Red::Core::Bundle::FileID id, Uint8 compressionType, const Uint32 uncompressedSize, CDiskBundleCacheEntryMemoryBlock* compressedData )
{
	Red::Threads::CScopedLock< TListLock >  lock( m_entriesLock );

	// already cached ?
	CDiskBundleCacheEntry* entry = nullptr;
	if ( m_entries.Find( id, entry ) )
		return;

	// create cache entry
	entry = new CDiskBundleCacheEntry( id, compressionType, uncompressedSize, compressedData );
	m_entries.Insert( id, entry );
}

void CDiskBundleCache::RemoveCachedData( const Red::Core::Bundle::FileID id )
{
	Red::Threads::CScopedLock< TListLock >  lock( m_entriesLock );

	// already cached ?
	CDiskBundleCacheEntry* entry = nullptr;
	if ( m_entries.Find( id, entry ) )
	{
		entry->Release();
		m_entries.Erase( id );
	}
}

void CDiskBundleCache::RemoveAllCachedData()
{
	Red::Threads::CScopedLock< TListLock > lock( m_entriesLock );

	// release data
	for ( const auto it : m_entries )
	{
		it.m_second->Release();
	}

	// cleanup array
	m_entries.ClearFast();
}

IAsyncFile* CDiskBundleCache::CreateAsyncReader( const Red::Core::Bundle::FileID fileID ) const
{
	Red::Threads::CScopedLock< TListLock >  lock( m_entriesLock );

	// already cached ?
	CDiskBundleCacheEntry* entry = nullptr;
	if ( !m_entries.Find( fileID, entry ) )
		return nullptr;

	// create reader
	RED_FATAL_ASSERT( entry != nullptr, "NULL entry in cache" );
	return entry->CreateAsyncReader();
}

IFile* CDiskBundleCache::CreateReader( const Red::Core::Bundle::FileID fileID ) const
{
	Red::Threads::CScopedLock< TListLock >  lock( m_entriesLock );

	// already cached ?
	CDiskBundleCacheEntry* entry = nullptr;
	if ( !m_entries.Find( fileID, entry ) )
		return nullptr;

	// create reader
	RED_FATAL_ASSERT( entry != nullptr, "NULL entry in cache" );
	return entry->CreateReader();
}

//-------------------------------------------------------------------
