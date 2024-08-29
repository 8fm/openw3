/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FILE_WRITER_LINUX_H
#define _RED_MEMORY_FILE_WRITER_LINUX_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "../redSystem/os.h"

namespace Red { namespace MemoryFramework { namespace LinuxAPI {

/////////////////////////////////////////////////////////////////
// EFileWriterResult
//	FileWriter functions may return one of these
enum EFileWriterResult
{
	FW_OK,
	FW_OpenFailed,
	FW_CloseFailed,
	FW_FileNotOpen,
	FW_WriteFailed,
};

const int c_invalidDescriptor = -1;

//////////////////////////////////////////////////////////////////
// FileWriter is used to wrap fast i/o for the metrics
class FileWriter
{
public:
	FileWriter();
	~FileWriter();

	RED_INLINE Red::System::Bool IsFileOpen() const { return m_fileDescriptor != c_invalidDescriptor; }
	EFileWriterResult OpenFile( const Red::System::Char* filePath );
	EFileWriterResult CloseFile( );
	EFileWriterResult Write( const void* buffer, Red::System::MemSize size );
	EFileWriterResult Flush();

private:
	static const Red::System::MemSize c_bufferSize = 1024 * 32;
	Red::System::Uint8 m_buffer[ c_bufferSize ];
	Red::System::MemSize m_dataBuffered;
	int m_fileDescriptor;
};

} } }

#endif
