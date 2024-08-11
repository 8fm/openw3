/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//-----------------------------------------------------------------------------

#include "file.h"

//-----------------------------------------------------------------------------

/// Buffer that supports writing data to memory but not in a contiguous block
/// therefore allowing more data to be saved without OOM
/// NOTE: it allocates from DEFAULT Pool in the MC_Temporary
class CPagedMemoryBuffer
{
public:
	static const Uint32 DEFAULT_PAGE_SIZE = 65536;

	CPagedMemoryBuffer( const Uint32 pageSize = DEFAULT_PAGE_SIZE );
	~CPagedMemoryBuffer();

	// clear buffer, free all memory
	void Clear();

	// get total size of data in the buffer
	const Uint32 GetTotalSize() const;

	// copy out the data to given memory
	void CopyData( void* outData, const Uint32 bufferSize ) const;

	// save data to a file, saves in pages
	Bool SaveToFile( IFile* file ) const;

	// load data from file, clears current data
	Bool LoadFromFile( IFile* file );

	// append data at the end of the buffer
	void Append( const void* data, const Uint32 dataSize );

	// write data at given position of the buffer
	void Write( const Uint32 offset, const void* data, const Uint32 dataSize );

	// read data at given position in the buffer, number of bytes actually read is returned
	void Read( const Uint32 offset, void* data, const Uint32 dataSize, Uint32& outSizeRead ) const;

private:
	struct Page
	{
		Uint8*	m_base;
		Uint8*	m_end;
		Uint8*	m_pos;

		RED_INLINE const Uint32 GetDataSize() const
		{
			return (Uint32)( m_pos - m_base );
		}
	};

	Uint32				m_totalSize;
	Uint32				m_pageSize;
	TDynArray< Page >	m_pages;
	Page*				m_curPage;

	static Uint8* AllocPage( const Uint32 pageSize );
	static void FreePage( void* ptr );
};

//-----------------------------------------------------------------------------

/// IFile based wrapper for paged buffer
/// NOTE: it DOES NOT support DMA (direct memory access so it cannot be used for optimized serialization stream)
class CPagedMemoryWriter : public IFile
{
public:
	CPagedMemoryWriter( CPagedMemoryBuffer& buffer );
	~CPagedMemoryWriter();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;

private:
	CPagedMemoryBuffer*		m_buffer;
	Uint32					m_offset;
};

//-----------------------------------------------------------------------------

/// IFile based wrapper for paged buffer
/// NOTE: it DOES NOT support DMA (direct memory access so it cannot be used for optimized serialization stream)
class CPagedMemoryReader : public IFile
{
public:
	CPagedMemoryReader( const CPagedMemoryBuffer& buffer );
	~CPagedMemoryReader();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;

private:
	const CPagedMemoryBuffer*		m_buffer;
	Uint32							m_offset;
};

//-----------------------------------------------------------------------------
