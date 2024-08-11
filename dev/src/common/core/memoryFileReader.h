/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "file.h"
#include "dynarray.h"

/////////////////////////////////////////////////////////////////////////////////////////////

/// Array based file reader
class CMemoryFileReader : public IFile, public IFileDirectMemoryAccess, Red::System::NonCopyable
{
private:
	const Uint8*					m_data;
	size_t							m_dataSize;
	uintptr_t						m_offset;		// Write offset

protected:
	void SetData( const Uint8 * data ) { m_data = data; }
	void SetSize( size_t size ) { m_dataSize = size; }

public:
	template< EMemoryClass memClass, RED_CONTAINER_POOL_TYPE memPool >
	CMemoryFileReader( const TDynArray< Uint8, memClass, memPool >& data, uintptr_t offset );
	CMemoryFileReader( const Uint8* data, size_t dataSize, uintptr_t offset );

	virtual ~CMemoryFileReader();

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;
};

template< EMemoryClass memClass, RED_CONTAINER_POOL_TYPE memPool >
CMemoryFileReader::CMemoryFileReader( const TDynArray< Uint8, memClass, memPool >& data, uintptr_t offset )
	: IFile( FF_Buffered | FF_MemoryBased | FF_Reader | FF_NoBuffering )
	, m_data( data.TypedData() )
	, m_dataSize( data.Size() )
	, m_offset( offset )
{
	RED_FATAL_ASSERT( offset <= m_dataSize, "offset overflow buffer" );
}

/////////////////////////////////////////////////////////////////////////////////////////////

/// Pointer to array based reader
class CMemoryFileReaderWithBuffer : public CMemoryFileReader
{
private:
	TDynArray< Uint8 >			m_dataPtr;		// Data buffer

public:
	CMemoryFileReaderWithBuffer( Uint32 size );
	virtual ~CMemoryFileReaderWithBuffer();

	//! Get data
	void* GetData(){ return m_dataPtr.Data(); }
};

/////////////////////////////////////////////////////////////////////////////////////////////

/// External buffer based reader
class CMemoryFileReaderExternalBuffer : public IFile, public IFileDirectMemoryAccess
{
private:
	const void*			m_buffer;		//!< Data buffer
	Uint32				m_offset;		//!< Current offset
	Uint32				m_size;			//!< Data size

private:
	CMemoryFileReaderExternalBuffer& operator=( const CMemoryFileReaderExternalBuffer& ) { return *this; }

public:
	CMemoryFileReaderExternalBuffer( const void* buffer, Uint32 size );
	virtual ~CMemoryFileReaderExternalBuffer() {};

	// IFile interface
	virtual void Serialize( void* buffer, size_t size ) override;
	virtual Uint64 GetOffset() const override;
	virtual Uint64 GetSize() const override;
	virtual void Seek( Int64 offset ) override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast<IFileDirectMemoryAccess*>( this ); }

	// IFileDirectMemoryAccess interface
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////
