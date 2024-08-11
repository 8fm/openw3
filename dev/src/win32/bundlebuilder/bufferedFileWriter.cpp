/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "bufferedFileWriter.h"

#include "bundleBuilderMemory.h"

namespace Bundler {

CBufferedFileWriter::CBufferedFileWriter()
:	m_fileHandle( nullptr )
,	m_buffer( nullptr )
,	m_size( 0 )
,	m_writePosition( 0 )
{

}

CBufferedFileWriter::~CBufferedFileWriter()
{
	if( m_fileHandle )
	{
		Close();
	}

	if( m_buffer )
	{
		BUNDLER_MEMORY_FREE( MC_BundlerOutputBuffer, m_buffer );
		m_buffer = nullptr;
	}
}

Bool CBufferedFileWriter::Initialise( Uint32 size )
{
	RED_FATAL_ASSERT( m_buffer == nullptr, "Buffer has already been initialised" );

	m_buffer = static_cast< Uint8* >( BUNDLER_MEMORY_ALLOCATE_ALIGNED( MC_BundlerOutputBuffer, size, 16 ) );

	if( m_buffer )
	{
		m_size = size;
		return true;
	}

	return false;
}

Bool CBufferedFileWriter::Open( const AnsiChar* path )
{
	RED_FATAL_ASSERT( m_buffer != nullptr, "Buffer has not been initialised" );

	m_fileHandle = fopen( path, "wb" );

	return m_fileHandle != nullptr;
}

void CBufferedFileWriter::Close()
{
	RED_FATAL_ASSERT( m_fileHandle != nullptr, "File has not been opened" );

	if( m_writePosition != 0 )
	{
		Flush();
	}

	fclose( m_fileHandle );
	m_fileHandle = nullptr;

	m_writePosition = 0;
}

Bool CBufferedFileWriter::Write( const void* data, Uint32 size )
{
	Uint32 spaceRemaining = m_size - m_writePosition;
	void* destination = m_buffer + m_writePosition;

	if( spaceRemaining > size )
	{
		// Copy the whole amount into the buffer
		Red::System::MemoryCopy( destination, data, size );

		m_writePosition += size;
	}
	else
	{
		// Copy as much as we can fit into the buffer
		Red::System::MemoryCopy( destination, data, spaceRemaining );
		m_writePosition += spaceRemaining;

		// Write the buffer to disk
		if( !Flush() )
		{
			return false;
		}

		// Now that the buffer is empty again, copy the rest of the data in
		const Uint8* source = static_cast< const Uint8* >( data ) + spaceRemaining;
		Uint32 amountToCopy = size - spaceRemaining;

		// Recurse here, as the remaining amount left to copy could still
		// potentially be larger than the space we have allocated for the buffer
		return Write( source, amountToCopy );
	}

	return true;
}

Bool CBufferedFileWriter::Flush()
{
	RED_FATAL_ASSERT( m_fileHandle != nullptr, "File has not been opened" );
	RED_FATAL_ASSERT( m_writePosition > 0, "No data has been written to the buffer" );

	Uint32 size = m_writePosition;
	m_writePosition = 0;

	return fwrite( m_buffer, sizeof( Uint8 ), size, m_fileHandle ) == size;
}

Bool CBufferedFileWriter::Seek( Uint32 position )
{
	if( m_writePosition == 0 || Flush() )
	{
		return fseek( m_fileHandle, position, SEEK_SET ) == 0;
	}

	return false;
}

} // namespace Bundler {
