/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "file.h"
#include "fileSys.h"
#include "feedback.h"
#include "bufferedTempFile.h"

//----

CBufferedTempFile::CBufferedTempFile( const String& targetFile )
	: IFile( FF_Buffered | FF_FileBased | FF_Writer | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
	, m_targetFile( targetFile )
	, m_tempWriter( NULL )
{
#ifndef RED_PLATFORM_CONSOLE

	// Create temp file
	m_tempFileName = GFileManager->GenerateTemporaryFilePath();
	m_tempWriter = GFileManager->CreateFileWriter( m_tempFileName, FOF_AbsolutePath | FOF_Buffered | FOF_DoNotIOManage );
	if ( !m_tempWriter )
	{
		GFeedback->ShowError( TXT("Cannot create temporary output file '%ls'. Unable to save '%ls'."), m_tempFileName.AsChar(), m_targetFile.AsChar() );
	}

#else

	// On platforms other than Win32 we do not support temporary files
	ERR_CORE( TXT("This platform does not support writting to temporary files." ) );

#endif
}

CBufferedTempFile::~CBufferedTempFile()
{
	// Save to final file
	if ( m_tempWriter )
	{
		// Close temp writer
		delete m_tempWriter;
		m_tempWriter = NULL;

		// Reopen temp data
		IFile* reader = GFileManager->CreateFileReader( m_tempFileName, FOF_AbsolutePath | FOF_DoNotIOManage );
		if ( !reader )
		{
			GFeedback->ShowError( TXT("Unable to open temp file '%ls'. File '%ls' will not be saved."), m_tempFileName.AsChar(), m_targetFile.AsChar() );
			return;
		}

		// Dump to final file
		if ( !m_targetFile.Empty() )
		{
			// Create path
			GFileManager->CreatePath( m_targetFile );

			// Allocate temp data buffer
			void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary, 1 << 20 );
			if ( !buffer )
			{
				delete reader;
				GFeedback->ShowError( TXT("Out of memory for file copy buffer. File '%ls' will not be saved."), m_targetFile.AsChar() );
				return;
			}

			// See if destination is read only and ask user if he wants to remove this attribute
			if ( GFileManager->IsFileReadOnly( m_targetFile ) )
			{
				if ( GFeedback->AskYesNo( TXT("File '%ls' has read-only attribute. Overwrite?"), m_targetFile.AsChar() ) )
				{
					GFileManager->SetFileReadOnly( m_targetFile, false );
				}
			}

			// Write file
			IFile* target = GFileManager->CreateFileWriter( m_targetFile, FOF_AbsolutePath );
			if ( !target )
			{
				delete reader;
				RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, buffer );
				GFeedback->ShowError( TXT("Unable to open output file '%ls'. File will not be saved."), m_targetFile.AsChar() );
				return;
			}

			// Copy data
			Uint32 dataLeft = static_cast< Uint32 >( reader->GetSize() );
			while ( dataLeft )
			{
				// Calculate amount to copy
				Uint32 copyAmount = Min< Uint32 >( 1 << 20, dataLeft );

				// Copy data
				reader->Serialize( buffer, copyAmount );
				target->Serialize( buffer, copyAmount );

				// Advance
				dataLeft -= copyAmount;
			}

			// Cleanup
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, buffer );

			// Done
			delete reader;
			delete target;

			// Delete temporary file
			if ( ! GSystemIO.DeleteFile( m_tempFileName.AsChar() ) )
			{
				GFeedback->ShowError( TXT( "Unable to delete temporary file '%ls'"), m_tempFileName.AsChar() );
			}
		}
	}
}

void CBufferedTempFile::Serialize( void* buffer, size_t size )
{
	if ( size && m_tempWriter )
	{
		m_tempWriter->Serialize( buffer, size );
	}
}

Uint64 CBufferedTempFile::GetOffset() const
{
	if ( m_tempWriter )
	{
		return m_tempWriter->GetOffset();
	}
	else
	{
		return 0;
	}
}

Uint64 CBufferedTempFile::GetSize() const
{
	if ( m_tempWriter )
	{
		return m_tempWriter->GetSize();
	}
	else
	{
		return 0;
	}
}

void CBufferedTempFile::Seek( Int64 offset )
{
	if ( m_tempWriter )
	{
		m_tempWriter->Seek( offset );
	}
}
