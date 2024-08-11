/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redMemoryFileWriterDurango.h"

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

///////////////////////////////////////////////////////////////////
// CTOr
//
FileWriter::FileWriter()
	: m_fileHandle( nullptr )
{

}

///////////////////////////////////////////////////////////////////
// DTor
//
FileWriter::~FileWriter()
{
	Flush();
	CloseFile();
}

///////////////////////////////////////////////////////////////////
// IsFileOpen
//
Red::System::Bool FileWriter::IsFileOpen() const
{
	return m_fileHandle != nullptr;
}

///////////////////////////////////////////////////////////////////
// OpenFile
//
EFileWriterResult FileWriter::OpenFile( const Red::System::Char* filePath )
{
	RED_ASSERT( m_fileHandle == nullptr, TXT( "File already open" ) );

	Red::System::AnsiChar filePathBuffer[ MAX_PATH + 1 ] = {'\0'};
	Red::System::StringConvert( filePathBuffer, filePath, MAX_PATH + 1 );

	m_dataBuffered = 0;
	errno_t err = fopen_s( &m_fileHandle, filePathBuffer, "wb" );

	return err == 0 && m_fileHandle != nullptr ? FW_OK : FW_OpenFailed;
}

///////////////////////////////////////////////////////////////////
// CloseFile
//
EFileWriterResult FileWriter::CloseFile( )
{
	if( m_fileHandle )
	{
		fclose( m_fileHandle );
		m_fileHandle = nullptr;
	}
	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// Write
//
EFileWriterResult FileWriter::Write( const void* buffer, Red::System::MemSize size )
{
	if( m_dataBuffered + size >= c_bufferSize )
	{
		if( Flush() != FW_OK )	// If the flush fails, we cannot do anything
		{
			return FW_WriteFailed;
		}
	}

	Red::System::MemoryCopy( m_buffer + m_dataBuffered, buffer, size );
	m_dataBuffered += size;

	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// Flush
//
EFileWriterResult FileWriter::Flush()
{
	if( m_fileHandle != nullptr )
	{
		Red::System::MemSize byteswritten = fwrite( m_buffer, 1, m_dataBuffered, m_fileHandle );
		if( byteswritten == m_dataBuffered )
		{
			m_dataBuffered = 0;
			return FW_OK;
		}
	}
	else
	{
		return FW_FileNotOpen;
	}

	return FW_WriteFailed;
}

} } }