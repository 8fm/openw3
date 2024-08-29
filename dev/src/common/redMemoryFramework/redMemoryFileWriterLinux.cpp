/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryFileWriterLinux.h"

namespace Red { namespace MemoryFramework { namespace LinuxAPI {

///////////////////////////////////////////////////////////////////
// CTor
//
FileWriter::FileWriter()
	: m_fileDescriptor( c_invalidDescriptor )
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
	Red::System::AnsiChar filenameOneByte[ 512 ] = {'\0'};
	Red::System::StringConvert( filenameOneByte, filePath, 512 );

	Int32 descriptorFlags = O_WRONLY | O_CREAT | O_TRUNC;
	m_fileDescriptor = ::open( filenameOneByte,
							   O_WRONLY | O_CREAT | O_TRUNC, // similar to GENERIC_WRITE, CREATE_ALWAYS in Windows
							   S_IRUSR | S_IWUSR );			 // user has read/write permissions
	m_dataBuffered = 0;

	return m_fileDescriptor == c_invalidDescriptor ? FW_OpenFailed : FW_OK;
}

///////////////////////////////////////////////////////////////////
// Flush
//	Write pending data
EFileWriterResult FileWriter::Flush()
{
	if( m_fileDescriptor != c_invalidDescriptor )
	{
		const ssize_t bytesWritten = ::write( m_fileDescriptor, m_buffer, m_dataBuffered );
		if( bytesWritten == m_dataBuffered )
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

///////////////////////////////////////////////////////////////////
// CloseFile
//	Flush and close
EFileWriterResult FileWriter::CloseFile( )
{
	if( m_fileDescriptor != c_invalidDescriptor )
	{
		::fsync( m_fileDescriptor );
		EFileWriterResult result = ::close( m_fileDescriptor ) == 0 ? FW_OK : FW_CloseFailed;
		m_fileDescriptor = c_invalidDescriptor;	// Always assume close was successful
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
