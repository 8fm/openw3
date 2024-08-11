/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "memoryFileReader.h"

/////////////////////////////////////////////////////////////////////////////////////////////

CMemoryFileReader::CMemoryFileReader( const Uint8* data, size_t dataSize, uintptr_t offset )
	: IFile( FF_Buffered | FF_MemoryBased | FF_Reader | FF_NoBuffering )
	, m_data( data )
	, m_dataSize( dataSize )
	, m_offset( offset )
{
	RED_FATAL_ASSERT( offset <= m_dataSize, "offset overflow buffer" );
}

CMemoryFileReader::~CMemoryFileReader(){};

// Serialize data buffer of given size
void CMemoryFileReader::Serialize( void* buffer, size_t size )
{
	ASSERT( m_offset + size <= m_dataSize );
	if(  m_offset + size > m_dataSize )
	{
		size = m_dataSize - m_offset;
	}

	Red::System::MemoryCopy( buffer, &m_data[ m_offset ], size );
	m_offset += size;

}

// Get position in file stream
Uint64 CMemoryFileReader::GetOffset() const
{
	return m_offset;
}

// Get size of the file stream
Uint64 CMemoryFileReader::GetSize() const
{
	return m_dataSize;
}

// Seek to file position
void CMemoryFileReader::Seek( Int64 offset )
{
	ASSERT( offset <= (Int64)m_dataSize );
	if( offset <= (Int64)m_dataSize )
	{
		m_offset = (uintptr_t)offset;
	}
}

Uint8* CMemoryFileReader::GetBufferBase() const
{
	return (Uint8*)m_data;
}

Uint32 CMemoryFileReader::GetBufferSize() const
{
	RED_FATAL_ASSERT( m_dataSize <= 0xFFFFFFFF, "Memory buffer is to big" );
	return (Uint32) m_dataSize;
}

CMemoryFileReaderWithBuffer::CMemoryFileReaderWithBuffer( Uint32 size )
	: CMemoryFileReader( NULL, 0, 0 )
	, m_dataPtr( size )
{
	SetData( m_dataPtr.TypedData() );
	SetSize( size );
}

CMemoryFileReaderWithBuffer::~CMemoryFileReaderWithBuffer()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////

CMemoryFileReaderExternalBuffer::CMemoryFileReaderExternalBuffer( const void* buffer, Uint32 size )
	: IFile( FF_Buffered | FF_MemoryBased | FF_Reader | FF_NoBuffering )
	, m_buffer( buffer )
	, m_size( size )
	, m_offset( 0 )
{
}

void CMemoryFileReaderExternalBuffer::Serialize( void* buffer, size_t size )
{
	ASSERT( m_offset + size <= m_size );

	// We need to protect against reading / writing off the end of the buffer, otherwise we will get buffer overruns
	if( m_offset + size <= m_size )
	{
		Red::System::MemoryCopy( buffer, OffsetPtr( m_buffer, m_offset ), size );
		m_offset += static_cast< Uint32 >( size );
	}
	else
	{
		// Worst case, reset the buffer to all zeros in case something attempts to serialise from it
		Red::System::MemorySet( buffer, 0, size );
	}
}

Uint64 CMemoryFileReaderExternalBuffer::GetOffset() const
{
	return m_offset;
}

Uint64 CMemoryFileReaderExternalBuffer::GetSize() const
{
	return m_size;
}

void CMemoryFileReaderExternalBuffer::Seek( Int64 offset )
{
	ASSERT( offset <= (Int64)m_size );
	m_offset = static_cast< Uint32 >( offset );
}

Uint8* CMemoryFileReaderExternalBuffer::GetBufferBase() const
{
	return (Uint8*)m_buffer;
}

Uint32 CMemoryFileReaderExternalBuffer::GetBufferSize() const
{
	return m_size;
}

/////////////////////////////////////////////////////////////////////////////////////////////
