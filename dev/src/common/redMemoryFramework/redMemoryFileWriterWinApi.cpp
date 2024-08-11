/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryFileWriterWinAPI.h"

namespace Red { namespace MemoryFramework { namespace WinAPI {

///////////////////////////////////////////////////////////////////
// CTor
//
FileWriter::FileWriter()
	: m_fileHandle( INVALID_HANDLE_VALUE )
	, m_dataBuffered( 0 )
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
// OpenFile
//
EFileWriterResult FileWriter::OpenFile( const Red::System::Char* filePath )
{
	m_fileHandle = CreateFile( filePath, 
							   GENERIC_WRITE,			// We are only writing to the file
							   0,						// Do not let other processes access it while we have it open
							   NULL,					// Do not let inherited processes access the file
							   CREATE_ALWAYS,			// Never append
							   FILE_ATTRIBUTE_NORMAL,	// Normal file attribs in the OS
							   nullptr );

	m_dataBuffered = 0;

	return m_fileHandle == INVALID_HANDLE_VALUE ? FW_OpenFailed : FW_OK;
}

///////////////////////////////////////////////////////////////////
// Flush
//	Write pending data
EFileWriterResult FileWriter::Flush()
{
	if( m_fileHandle != INVALID_HANDLE_VALUE )
	{
		DWORD bytesWritten = 0;
		DWORD bytesToWrite = static_cast< DWORD >( m_dataBuffered );
		if( WriteFile( m_fileHandle, m_buffer, bytesToWrite, &bytesWritten, nullptr ) )
		{
			if( bytesWritten == m_dataBuffered )
			{
				m_dataBuffered = 0;
				return FW_OK;
			}
		}
	}
	else
	{
		return FW_FileNotOpen;
	}

	return FW_WriteFailed;
}

///////////////////////////////////////////////////////////////////
// CloseFile
//	Flush and close
EFileWriterResult FileWriter::CloseFile( )
{
	if( m_fileHandle != INVALID_HANDLE_VALUE )
	{
		FlushFileBuffers( m_fileHandle );
		EFileWriterResult result = CloseHandle( m_fileHandle ) == TRUE ? FW_OK : FW_CloseFailed;
		m_fileHandle = INVALID_HANDLE_VALUE;	// Always assume close was successful
		m_dataBuffered = 0;

		return result;
	}

	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// Write
//	Write data to the internal buffer; flush if its too big
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

} } }