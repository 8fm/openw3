/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryFileWriterOrbis.h"
#include "redMemoryLog.h"

#pragma comment( lib, "libSceFios2_stub_weak.a" )

namespace Red { namespace MemoryFramework { namespace OrbisAPI {

// Helper function to wait on a operation; deletes the op when complete
#define WAIT_ON_FIOS_OP(op)		if( op != SCE_FIOS_OP_INVALID )			\
								{										\
									while (!sceFiosOpIsDone(op)) {}		\
									sceFiosOpDelete(op);				\
									op = SCE_FIOS_OP_INVALID;			\
								}

///////////////////////////////////////////////////////////////////
// CTor
//
FileWriter::FileWriter()
	: m_fileHandle( SCE_FIOS_FH_INVALID )
	, m_inactiveWriteOperations( nullptr )
	, m_activeWriteOperations( nullptr )
	, m_writingOperation( nullptr )
{
	for( Red::System::Uint32 opIndex = 0; opIndex < c_maximumFileWriteOperations - 1; ++opIndex )
	{
		m_writeOperations[ opIndex ].m_nextOperation = &m_writeOperations[ opIndex + 1 ];
	}
	m_writeOperations[ c_maximumFileWriteOperations - 1 ].m_nextOperation = nullptr;
	m_inactiveWriteOperations = m_writeOperations;
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
//	Open a file (blocking)
EFileWriterResult FileWriter::OpenFile( const Red::System::Char* filePath )
{
	Red::System::AnsiChar filenameOneByte[ 512 ] = {'\0'};
	Red::System::StringConvert( filenameOneByte, filePath, 512 );

	if( m_fileHandle != SCE_FIOS_FH_INVALID )
	{
		return FW_OpenFailed;
	}

	SceFiosOpenParams fopenParams = SCE_FIOS_OPENPARAMS_INITIALIZER;
	fopenParams.openFlags = SCE_FIOS_O_WRITE | SCE_FIOS_O_CREAT | SCE_FIOS_O_TRUNC;
	fopenParams.opFlags = 0;

	// Open the file (this is an async op, but we will block on it)
	SceFiosOp openFileOp = sceFiosFHOpen( nullptr, &m_fileHandle, filenameOneByte, &fopenParams );
	RED_MEMORY_ASSERT( m_fileHandle != SCE_FIOS_FH_INVALID,  "Failed to open file for writing" );
	if( m_fileHandle == SCE_FIOS_FH_INVALID )
	{
		return FW_OpenFailed;
	}

	WAIT_ON_FIOS_OP( openFileOp );

	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// Flush
//	Blocks until all operations are complete (full flush)
EFileWriterResult FileWriter::Flush()
{
	if( m_fileHandle == SCE_FIOS_FH_INVALID )
	{
		return FW_FileNotOpen;
	}

	FileWriteOp* activeOperation = m_activeWriteOperations;
	while( activeOperation != nullptr )
	{
		FileWriteOp* nextOperation = activeOperation->m_nextOperation;
		WAIT_ON_FIOS_OP( activeOperation->m_fiosOperation );
		
		DeactivateOperation( activeOperation );

		activeOperation = nextOperation;
	}

	RED_MEMORY_ASSERT( m_activeWriteOperations == nullptr,  "Failed to flush all write operations" );

	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// CloseFile
//	Flush the write buffers, then close the file (blocking)
EFileWriterResult FileWriter::CloseFile( )
{
	if( m_fileHandle != SCE_FIOS_FH_INVALID )
	{
		// Kick out the final write if required and block before closing the file
		DispatchAsync();
		Flush();	

		SceFiosOp closeOp = sceFiosFHClose( nullptr, m_fileHandle );
		WAIT_ON_FIOS_OP( closeOp );
		m_fileHandle = SCE_FIOS_FH_INVALID;
	}
	else
	{
		return FW_FileNotOpen;
	}

	return FW_OK;
}

///////////////////////////////////////////////////////////////////
// DispatchAsync
//	Kick off the current write
void FileWriter::DispatchAsync()
{
	if( m_writingOperation != nullptr && m_writingOperation->m_bytesWritten > 0 )
	{
		// Kick out the write
		m_writingOperation->m_fiosOperation = sceFiosFHWrite( nullptr, m_fileHandle, m_writingOperation->m_writeBuffer, m_writingOperation->m_bytesWritten );
		
		// Push the write op to the head of the active ops list
		m_writingOperation->m_nextOperation = m_activeWriteOperations;
		m_activeWriteOperations = m_writingOperation;
		m_writingOperation = nullptr;
	}
}

///////////////////////////////////////////////////////////////////
// DeactivateOperation
//	move an active op to the inactive list
void FileWriter::DeactivateOperation( FileWriteOp* activeOp )
{
	// remove from the active list
	if( m_activeWriteOperations == activeOp )
	{
		m_activeWriteOperations = activeOp->m_nextOperation;
	}
	else
	{
		FileWriteOp* previousOp = m_activeWriteOperations;
		while( previousOp != nullptr )
		{
			if( previousOp->m_nextOperation == activeOp )
			{
				previousOp->m_nextOperation = activeOp->m_nextOperation;
				break;
			}
			previousOp = previousOp->m_nextOperation;
		}
	}

	// push to the head of the inactive list
	activeOp->m_nextOperation = m_inactiveWriteOperations;
	m_inactiveWriteOperations = activeOp;
	activeOp->Reset();
}

///////////////////////////////////////////////////////////////////
// Write
//	Write to the active write op (or grab an inactive one if if doesn't exist)
EFileWriterResult FileWriter::Write( const void* buffer, Red::System::MemSize size )
{
	// If the write buffer isn't big enough to accomodate the new data, dispatch it then get a new write operation
	if( m_writingOperation != nullptr && m_writingOperation->m_bytesWritten + size >= FileWriteOp::c_writeBufferMaxSize )
	{
		DispatchAsync();
	}

	// Grab a new write op if required
	if( m_writingOperation == nullptr )
	{
		// no writing op, get the last inactive operation as our new write handle (or flush if there are none available)
		if( m_inactiveWriteOperations == nullptr )
		{
			Flush();
		}		

		// pop the latest inactive op
		m_writingOperation = m_inactiveWriteOperations;
		m_inactiveWriteOperations = m_inactiveWriteOperations->m_nextOperation;

		m_writingOperation->Reset();
		m_writingOperation->m_nextOperation = nullptr;
	}

	if( m_writingOperation == nullptr || m_writingOperation->m_bytesWritten + size >= FileWriteOp::c_writeBufferMaxSize )
	{
		RED_MEMORY_HALT( "No available write ops left, or the write buffer is not big enough (flush failed?!" );
		return FW_WriteFailed;		// No write buffer, or its not big enough for the data
	}

	// Write the data to the write op and continue
	Red::System::MemoryCopy( &( m_writingOperation->m_writeBuffer[ m_writingOperation->m_bytesWritten ] ), buffer, size );
	m_writingOperation->m_bytesWritten += size;

	// Finally, poll any active writes; make them inactive if the ops are complete
	FileWriteOp* activeOperation = m_activeWriteOperations;
	while( activeOperation != nullptr )
	{
		FileWriteOp* nextActive = activeOperation->m_nextOperation;
		if( sceFiosOpIsDone( activeOperation->m_fiosOperation ) )
		{
			sceFiosOpDelete( activeOperation->m_fiosOperation );
			DeactivateOperation( activeOperation );
		}
		activeOperation = nextActive;
	}

	return FW_OK;
}

} } }