/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FILE_WRITER_DURANGO_H
#define _RED_MEMORY_FILE_WRITER_DURANGO_H
#pragma once

#include "redMemoryFrameworkTypes.h"

namespace Red { namespace MemoryFramework { namespace DurangoAPI {

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

//////////////////////////////////////////////////////////////////
// FileWriter is used to wrap fast i/o for the metrics
class FileWriter
{
public:
	FileWriter();
	~FileWriter();

	Red::System::Bool IsFileOpen() const;
	EFileWriterResult OpenFile( const Red::System::Char* filePath );
	EFileWriterResult CloseFile( );
	EFileWriterResult Write( const void* buffer, Red::System::MemSize size );
	EFileWriterResult Flush();

private:
	static const Red::System::MemSize c_bufferSize = 1024 * 32;
	Red::System::Uint8 m_buffer[ c_bufferSize ];
	Red::System::MemSize m_dataBuffered;
	FILE* m_fileHandle;
};

} } }

#endif