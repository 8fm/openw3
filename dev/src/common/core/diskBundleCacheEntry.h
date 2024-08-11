/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "diskFile.h"

// Owned memory block
class CDiskBundleCacheEntryMemoryBlock
{
public:
	CDiskBundleCacheEntryMemoryBlock( const Bool ioPool, void* data, const Uint32 size );

	// refcounting
	void AddRef();
	void Release();

	// data access
	RED_INLINE void* GetData() const { return m_data; }
	RED_INLINE const Uint32 GetSize() const { return m_size; }

	// allocate block of given size - can fail for ioPool
	static CDiskBundleCacheEntryMemoryBlock* Create( const Bool ioPool, const Uint32 size );

private:
	~CDiskBundleCacheEntryMemoryBlock();

	Red::Threads::CAtomic< Int32 >		m_refCount;

	Bool		m_ioPool;
	void*		m_data;
	Uint32		m_size;
};

// Entry in the bundle data cache
class CDiskBundleCacheEntry
{
public:
	CDiskBundleCacheEntry( Red::Core::Bundle::FileID fileID, Uint8 compressionType, const Uint32 uncompressedSize, CDiskBundleCacheEntryMemoryBlock* compressedData );

	// Get file ID
	RED_INLINE const Red::Core::Bundle::FileID GetFileID() const { return m_file; }

	// Reference counting
	void AddRef();
	void Release();

	// Create normal access
	// NOTE: this addrefs this entry
	IFile* CreateReader();

	// Create access via async file interface
	// NOTE: this addrefs this entry
	IAsyncFile* CreateAsyncReader();

private:
	~CDiskBundleCacheEntry();

	// thread safe ref counting
	Red::Threads::CAtomic< Int32 >	m_refCount;

	// data
	Red::Core::Bundle::FileID			m_file;
	CDiskBundleCacheEntryMemoryBlock*	m_compressedData;
	const Uint8							m_compressionType;
	const Uint32						m_uncompressedSize;

	// decompress data
	CDiskBundleCacheEntryMemoryBlock* DecompressData();

	// do we have compressed data ?
	const Bool IsCompressed() const;
};

