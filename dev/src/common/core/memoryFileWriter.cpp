/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "memoryFileWriter.h"

// Serialize data buffer of given size
void CMemoryFileWriter::Serialize( void* buffer, size_t size )
{
	if ( size )
	{
		if ( m_offset + size > m_data.Size() )
		{
			m_data.GrowBuffer< Int8 >( ( m_offset + size ) - m_data.Size(), m_memoryClass );
		}

		void* writeBuffer = reinterpret_cast< void* >( reinterpret_cast< MemUint >( m_data.Data() ) + m_offset );
		Red::System::MemoryCopy( writeBuffer, buffer, size );
		m_offset += static_cast< Uint32 >( size );
	}
}

// Get position in file stream
Uint64 CMemoryFileWriter::GetOffset() const
{
	return m_offset;
}

// Get size of the file stream
Uint64 CMemoryFileWriter::GetSize() const
{
	return m_data.Size();
}

// Seek to file position
void CMemoryFileWriter::Seek( Int64 offset )
{
	//ASSERT( offset <= (Int32)m_data.Size() )
	m_offset = static_cast< Uint32 >( offset );
}

// Save file to HDD
void CMemoryFileWriter::Close()
{
	m_data.Shrink< Int8 >( m_memoryClass );
}

// Get data buffer
const void* CMemoryFileWriter::GetBuffer() const
{
	return m_data.Data();
}

// Get the buffer allocation size
size_t CMemoryFileWriter::GetBufferCapacity() const
{
	return m_data.Capacity(1);
}

Uint8* CMemoryFileWriter::GetBufferBase() const
{
	return (Uint8*) m_data.Data();
}

Uint32 CMemoryFileWriter::GetBufferSize() const
{
	return m_data.Size();
}

CMemoryFileWriterExternalBuffer::CMemoryFileWriterExternalBuffer( void* buffer, Uint32 size )
	: IFileEx( FF_Buffered | FF_MemoryBased | FF_Writer | FF_NoBuffering | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) ) 
	, m_buffer( buffer )
	, m_size( size )
	, m_offset( 0 )
	, m_realSize( 0 )
{

}

// Serialize data buffer of given size
void CMemoryFileWriterExternalBuffer::Serialize( void* buffer, size_t size )
{
	if ( size )
	{
		size_t freeSpace = m_size - m_offset;
		size = Clamp< size_t >( size, 0, freeSpace );

		void* writeBuffer = reinterpret_cast< void* >( reinterpret_cast< MemUint >( m_buffer ) + m_offset );
		Red::System::MemoryCopy( writeBuffer, buffer, size );
		m_offset += static_cast< Uint32 >( size );
		m_realSize = Max( m_offset, m_realSize );
	}
}

Uint64 CMemoryFileWriterExternalBuffer::GetOffset() const
{
	return m_offset;
}

Uint64 CMemoryFileWriterExternalBuffer::GetSize() const
{
	// Pretty sure GetSize() should return the size of the file we are writing, all the other IFiles work like this
	return m_realSize;		
}

void CMemoryFileWriterExternalBuffer::Seek( Int64 offset )
{
	m_offset = static_cast< Uint32 >( offset );
}

void CMemoryFileWriterExternalBuffer::Close()
{
	// do nothing
}

const void* CMemoryFileWriterExternalBuffer::GetBuffer() const
{
	return m_buffer;
}

// Get the buffer allocation size
size_t CMemoryFileWriterExternalBuffer::GetBufferCapacity() const
{
	return m_size;
}

Uint8* CMemoryFileWriterExternalBuffer::GetBufferBase() const
{
	return (Uint8*) m_buffer;
}

Uint32 CMemoryFileWriterExternalBuffer::GetBufferSize() const
{
	return m_size;
}