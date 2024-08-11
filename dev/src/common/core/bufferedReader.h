#pragma once

/************************************************************************/
/* IFile implementation that implements read buffering					*/
/************************************************************************/

#include "file.h"

class CBufferedReader : public IFile
{
protected:
	IFile*			m_reader;			// Low level reader
	Uint8*			m_buffer;			// Data buffer
	size_t			m_bufferSize;		// Size of the data buffer
	Uint64			m_bufferBase;		// From where in the source file we have the data cached
	size_t			m_bufferCount;		// Number of bytes in the read buffer
	Uint64			m_offset;			// File offset
	Uint64			m_size;				// File size

public:
	CBufferedReader( IFile* reader, Uint32 bufferSize )
		: IFile( reader->GetFlags() | FF_Buffered )
		, m_reader( reader )
		, m_bufferSize( bufferSize )
		, m_bufferBase( 0 )
		, m_bufferCount( 0 )
		, m_offset( reader->GetOffset() )
		, m_size( reader->GetSize() )
	{
		// Set IFile version as writer
		m_version = reader->GetVersion();

		// Allocate buffer
		m_buffer = reinterpret_cast< Uint8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, sizeof( Uint8 ) * bufferSize) );
	}

	virtual ~CBufferedReader()
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, m_buffer );
		delete m_reader;
	}

	// Serialize data buffer of given size
	virtual void Serialize( void* buffer, size_t size )
	{
#ifndef RED_FINAL_BUILD
		// Error
		if ( HasErrors() )
		{
			ERR_CORE( TXT("Reading from file '%ls' that in in error state. Offset: %d, Size: %d, SizeToRead: %d"),
				m_reader->GetFileNameForDebug(), m_offset, m_size, size );
			Red::MemoryZero( buffer, size );
			return;
		}

		// Validate read
		if ( m_offset + size > m_size )
		{
			ERR_CORE( TXT("Out of bounds access for file '%ls'. Offset: %d, Size: %d, SizeToRead: %d"),
				m_reader->GetFileNameForDebug(), m_offset, m_size, size );

			SetError();
			Red::MemoryZero( buffer, size );
			return;
		}
#endif
		
		// While there is something to read
		while ( size > 0 )
		{
			// Calculate how much data we can copy from internal buffer
			size_t bytesInBuffer = Min( size, static_cast< size_t >( m_bufferBase + m_bufferCount - m_offset ) );

			// There's nothing left in the buffer, fill the buffer
			if ( bytesInBuffer == 0 )
			{
				// If remaining data is larger than buffer size read it directly
				if ( size >= m_bufferSize )
				{
					// Read file
					m_reader->Serialize( buffer, size );

					// Move position by the number of bytes read
					m_offset += size;

					// Reset buffer
					m_bufferBase = m_offset;
					m_bufferCount = 0;  

					// Nothing more to do
					break;
				}
				else
				{
					// Precache data in buffer
					m_bufferBase = m_offset;
					m_bufferCount = Min( m_bufferSize, static_cast< size_t >( m_size - m_offset ) );
					if ( m_bufferCount )
					{
						m_reader->Serialize( m_buffer, m_bufferCount );
						continue;
					}
					else
					{
						// Out of file bounds
						break;
					}
				}
			}

			// Copy data to buffer
			Red::System::MemoryCopy( buffer, m_buffer + ( m_offset - m_bufferBase ), bytesInBuffer );

			// Move file offset
			m_offset += bytesInBuffer;

			// Decrement amount of work
			size -= bytesInBuffer;
			buffer = ( Uint8* ) buffer + bytesInBuffer;
		}
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
		// Out of bounds seek ?
#ifndef RED_FINAL_BUILD
		if ( offset > (Int64) m_size )
		{
			ERR_CORE( TXT("Out of bounds seek for file '%ls'. Offset: %lld, Size: %llu"),
				m_reader->GetFileNameForDebug(), offset, m_size );
		}
		else
		{
			// valid seek - try the reading again
			ClearError();
		}
#endif

		// If we are inside the buffered region just move the pointer
		const Uint64 bufferEnd = m_bufferBase + m_bufferCount;
		if ( (Uint64)offset >= m_bufferBase && (Uint64)offset < bufferEnd )
		{
			m_offset = offset;
		}
		else
		{
			// Seek to new file position
			// TODO: implement sector align here
			const Uint64 seekOffset = offset;
			m_reader->Seek( seekOffset );
		
			// Reset buffer
			m_bufferCount = 0;
			m_bufferBase = seekOffset;
			m_offset = offset;
		}
	}

	// For debug purposes only
	virtual const Char *GetFileNameForDebug() const { return m_reader->GetFileNameForDebug(); }

	// Create latent loading token for current file position
	virtual IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 currentOffset ) { return m_reader->CreateLatentLoadingToken( currentOffset ); }
};
