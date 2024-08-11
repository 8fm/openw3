#pragma once

/************************************************************************/
/* IFile implementation that implements write buffering					*/
/************************************************************************/

#include "file.h"

class CBufferedWriter : public IFile
{
protected:
	IFile*			m_writer;			// Low level writer
	Uint8*			m_buffer;			// Data buffer
	size_t			m_bufferSize;		// Size of the data buffer
	size_t			m_bufferCount;		// Number of bytes in the read buffer
	Uint64			m_offset;			// File offset
	Uint64			m_size;				// File size

public:
	CBufferedWriter( IFile* writer, Uint32 bufferSize )
		: IFile( writer->GetFlags() | FF_Buffered )
		, m_writer( writer )
		, m_bufferSize( bufferSize )
		, m_bufferCount( 0 )
		, m_offset( writer->GetOffset() )
		, m_size( writer->GetSize() )
	{
		// Set IFile version as writer
		m_version = writer->GetVersion();

		// Allocate buffer
		m_buffer = static_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Engine, bufferSize ) );
	}

	virtual ~CBufferedWriter()
	{
		FlushBufferedData();
		RED_MEMORY_FREE( MemoryPool_Default, MC_Engine, m_buffer );
		delete m_writer;
	}

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
		// If we have more to write that will fit in current block
		if ( size > ( m_bufferSize - m_bufferCount) )
		{
			// Flush what's left to write
			FlushBufferedData();

			// Write block directly
			m_writer->Serialize( buffer, size );

			// Shift position
			m_offset += size;
		}
		else
		{
			// Write data to buffer
			Red::System::MemoryCopy( m_buffer + m_bufferCount, buffer, size );
			m_bufferCount += size;
			m_offset += size;   
		}

		// Track size
		m_size = Max< Uint64 >( m_size, m_offset );
	}

	// Get position in file stream
	virtual Uint64 GetOffset() const
	{
		return m_offset;
	}

	// Get size of the file stream
	virtual Uint64 GetSize() const
	{
		return m_size;
	}

	// Seek to file position
	virtual void Seek( Int64 offset )
	{
		// Always flush pending data on seek
		FlushBufferedData();

		// Seek to new file position
		m_writer->Seek( offset );

		// Reset buffer
		m_bufferCount = 0;
		m_offset = offset;
	}

public:
	// Flush buffered data
	void FlushBufferedData()
	{
		if ( m_bufferCount )
		{
			// Write data in buffer
			m_writer->Serialize( m_buffer, m_bufferCount );

			// Reset buffer
			m_bufferCount = 0;
		}
	}
};
