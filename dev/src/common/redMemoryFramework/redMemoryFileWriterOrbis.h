/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_FILE_WRITER_ORBIS_H
#define _RED_MEMORY_FILE_WRITER_ORBIS_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include <fios2.h>

namespace Red { namespace MemoryFramework { namespace OrbisAPI {

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
//	Uses FIOS asynchronous writing
class FileWriter
{
public:
	FileWriter();
	~FileWriter();

	RED_INLINE Red::System::Bool IsFileOpen() const { return m_fileHandle != SCE_FIOS_FH_INVALID; }
	EFileWriterResult OpenFile( const Red::System::Char* filePath );
	EFileWriterResult CloseFile( );
	EFileWriterResult Write( const void* buffer, Red::System::MemSize size );
	EFileWriterResult Flush();

private:
	static const Red::System::Uint32 c_maximumFileWriteOperations = 4;	// max async writes to manage at once

	// FileWriteOp is used to track write operations
	struct FileWriteOp
	{
		FileWriteOp()
			: m_fiosOperation( SCE_FIOS_OP_INVALID )
			, m_bytesWritten( 0 )
			, m_nextOperation( nullptr )
		{
		}

		RED_INLINE void Reset()
		{
			m_fiosOperation = SCE_FIOS_OP_INVALID;
			m_bytesWritten = 0;
		}

		static const Red::System::Uint32 c_writeBufferMaxSize = 1024 * 16;
		SceFiosOp				m_fiosOperation;	// Fios async operation handle
		Red::System::AnsiChar	m_writeBuffer[ c_writeBufferMaxSize ];
		Red::System::MemSize	m_bytesWritten;
		FileWriteOp*			m_nextOperation;	// Intrusive linked list of operations
	};

	void DeactivateOperation( FileWriteOp* activeOp );	// move an active op to the inactive list
	void DispatchAsync();	// kick out the current write buffer and add it to the active list

	SceFiosFH m_fileHandle;
	FileWriteOp m_writeOperations[ c_maximumFileWriteOperations ];
	FileWriteOp* m_inactiveWriteOperations;			
	FileWriteOp* m_activeWriteOperations;		
	FileWriteOp* m_writingOperation;		// Keep an operation as the active writing op
};

} } }

#endif