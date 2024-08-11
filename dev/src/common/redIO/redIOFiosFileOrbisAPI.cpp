/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redIOFiosFileOrbisAPI.h"
#include "redIOAsyncReadToken.h"

#if defined( RED_PLATFORM_ORBIS )

#include <fios2.h>
#include <locale>

REDIO_ORBISAPI_NAMESPACE_BEGIN
	
//////////////////////////////////////////////////////////////////////////
// CFiosFile
//////////////////////////////////////////////////////////////////////////
CFiosFile::CFiosFile()
	: m_sceFileHandle( SCE_FIOS_HANDLE_INVALID )
{
}

CFiosFile::~CFiosFile()
{
	if ( m_sceFileHandle != SCE_FIOS_HANDLE_INVALID )
	{
		Close();
	}
}

Bool CFiosFile::Open( const Char* path, Uint32 openFlags )
{
	// Ignores eOpenFlag_Async

	REDIO_ASSERT( m_sceFileHandle == SCE_FIOS_HANDLE_INVALID );

	SceFiosOpenParams sceFiosOpenParams = SCE_FIOS_OPENPARAMS_INITIALIZER;
	SceInt32 sceFiosOpenFlags = 0;

	if ( openFlags & eOpenFlag_Read )
	{
		sceFiosOpenFlags |= SCE_FIOS_O_READ;
	}
	if ( openFlags & eOpenFlag_Write )
	{
		sceFiosOpenFlags |= SCE_FIOS_O_WRITE;
	}
	if ( openFlags & eOpenFlag_Append )
	{
		sceFiosOpenFlags |= SCE_FIOS_O_APPEND;
	}
	if ( openFlags & eOpenFlag_Create )
	{
		sceFiosOpenFlags |= SCE_FIOS_O_CREAT;
	}
	if ( openFlags & eOpenFlag_Truncate )
	{
		sceFiosOpenFlags = SCE_FIOS_O_TRUNC;
	}

	sceFiosOpenParams.openFlags = sceFiosOpenFlags;

	AnsiChar sysPath[ REDIO_MAX_PATH_LENGTH + 1 ];
	Red::System::WideCharToStdChar( sysPath, path, sizeof(sysPath) );
	for ( AnsiChar* ch = sysPath; *ch; ++ch )
	{
		if ( *ch == '\\') *ch = '/';
	}

	SceFiosOpAttr attr = SCE_FIOS_OPATTR_INITIALIZER;
	attr.opflags = SCE_FIOS_OPFLAG_IMMED; // Don't use up an async op for this
	const Int32 sceErr = ::sceFiosFHOpenSync( &attr, &m_sceFileHandle, sysPath, &sceFiosOpenParams );
	
	// File not found is not necessarily assert worthy, but other errors are
	REDIO_FIOS_CHECK( sceErr || sceErr == SCE_FIOS_ERROR_BAD_PATH );
	
	if ( sceErr != SCE_FIOS_OK )
	{
#ifdef RED_LOGGING_ENABLED
		const Uint32 BUFSZ = 64;
		AnsiChar errTxt[BUFSZ] = {'\0'};
		::sceFiosDebugDumpError( sceErr, errTxt, BUFSZ );
		REDIO_ERR(TXT("sceFiosFHOpenSync failed to open '%hs'. Error code 0x%08X (%hs)"), sysPath, sceErr, errTxt);
#endif

		m_sceFileHandle = SCE_FIOS_FH_INVALID;
		return false;
	}

	return true;
}

RED_TLS Bool GIsFiosCallbackThread;

static SceInt32 CloseFileCallback( void* pContext, SceFiosOp op, SceFiosOpEvent event, SceInt32 err )
{
	REDIO_ASSERT( op != SCE_FIOS_OP_INVALID );

	switch ( event )
	{
		// SDK docs on SceFiosOpCallback: "A callback can be called from the caller's thread or from a FIOS2 thread."
	case SCE_FIOS_OPEVENT_COMPLETE:
		{
			::sceFiosOpDelete( op );
		}
		break;
	case SCE_FIOS_OPEVENT_DELETE:
		break;
	default:
		break;
	}

	return SCE_FIOS_OK;
}

Bool CFiosFile::Close()
{
	Int32 err = SCE_FIOS_ERROR_BAD_FH;
	Bool ok = false;
	if ( m_sceFileHandle != SCE_FIOS_HANDLE_INVALID )
	{
		SceFiosOpAttr attr = SCE_FIOS_OPATTR_INITIALIZER;
		if ( !GIsFiosCallbackThread )
		{
			attr.opflags = SCE_FIOS_OPFLAG_IMMED; // Don't use up an async op for this if possible (ignored by FIOS if ops in flight)
			err = ::sceFiosFHCloseSync( &attr, m_sceFileHandle );
			REDIO_FIOS_CHECK( err );
			ok = (err == SCE_FIOS_OK);
		}
		else
		{
			// avoid deadlock by waiting for an op on the thread that needs to process it
			// Could potentially just close the file sync on yet another thread
			attr.pCallback = &CloseFileCallback;
			attr.pCallbackContext = nullptr;
			SceFiosOp op = ::sceFiosFHClose( &attr, m_sceFileHandle );
			ok = (op != SCE_FIOS_OP_INVALID); // can't really know for sure
		}
		m_sceFileHandle = SCE_FIOS_HANDLE_INVALID;
	}

	return ok;
}

Bool CFiosFile::Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead )
{
	outNumberOfBytesRead = 0;
	
	const SceFiosSize sceReadSize = ::sceFiosFHReadSync( nullptr, m_sceFileHandle, dest, length );
	if ( sceReadSize < 0 )
	{	
		return false;
	}

	REDIO_ASSERT( sceReadSize >= 0 && sceReadSize <= Uint32(-1) );
	outNumberOfBytesRead = static_cast< Uint32 >( sceReadSize );
	
	return true;
}

Bool CFiosFile::Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten )
{
	outNumberOfBytesWritten = 0;

	const SceFiosSize sceWriteSize = ::sceFiosFHWriteSync( nullptr, m_sceFileHandle, src, length );
	if ( sceWriteSize < 0 )
	{
		return false;
	}

	REDIO_ASSERT( sceWriteSize >=0 && sceWriteSize <= Uint32(-1) );
	outNumberOfBytesWritten = static_cast< Uint32 >( sceWriteSize );
	
	return true;
}

Bool CFiosFile::Seek( Int64 offset, ESeekOrigin seekOrigin )
{
	static_assert( (SceFiosWhence)eSeekOrigin_Set == SCE_FIOS_SEEK_SET, "Enum mismatch" );
	static_assert( (SceFiosWhence)eSeekOrigin_Current == SCE_FIOS_SEEK_CUR, "Enum mismatch" );
	static_assert( (SceFiosWhence)eSeekOrigin_End == SCE_FIOS_SEEK_END, "Enum mismatch" );

	const SceFiosWhence whence =  static_cast< SceFiosWhence >( seekOrigin );
	const Int64 newPos = ::sceFiosFHSeek( m_sceFileHandle, offset, whence );

	return newPos >= 0;
}

Int64 CFiosFile::Tell() const
{
	return static_cast< Int64 >( ::sceFiosFHTell( m_sceFileHandle ) ); // returns -1 if invalid FH
}

Bool CFiosFile::IsValid() const
{
	return ::sceFiosIsValidHandle( m_sceFileHandle );
}

Bool CFiosFile::Flush()
{
	const Int32 err = ::sceFiosFHSyncSync( nullptr, m_sceFileHandle );
	REDIO_FIOS_CHECK( err );

	return err == SCE_FIOS_OK;
}

Uint64 CFiosFile::GetFileSize() const
{
	const SceFiosSize sceFileSize = ::sceFiosFHGetSize( m_sceFileHandle );
	if ( sceFileSize < 0 ) // file handle is invalid
	{
		return 0;
	}

	return static_cast< Uint64 >( sceFileSize );
}

Uint32 CFiosFile::GetFileHandle() const
{
	return (Uint32)m_sceFileHandle;
}

REDIO_ORBISAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_ORBIS )