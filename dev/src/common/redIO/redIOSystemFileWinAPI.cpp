/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redIOCommon.h"

#include "../redSystem/error.h"

#include "redIOAsyncReadToken.h"
#include "redIOSystemFileWinAPI.h"

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )

REDIO_WINAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// CSystemFile
//////////////////////////////////////////////////////////////////////////
CSystemFile::CSystemFile()
	: m_hFile( INVALID_HANDLE_VALUE )
{
}

CSystemFile::~CSystemFile()
{
	if ( m_hFile!= INVALID_HANDLE_VALUE )
	{
		REDIO_WIN_CHECK( ::CloseHandle( m_hFile ) );
	}
}

Bool CSystemFile::Open( const Char* path, Uint32 openFlags )
{
	REDIO_ASSERT( m_hFile == INVALID_HANDLE_VALUE );
	if ( m_hFile != INVALID_HANDLE_VALUE )
	{
		return false;
	}

	DWORD sysDesiredAccess = 0;
	DWORD sysShareMode = 0;
	DWORD sysCreationDisposition = OPEN_EXISTING;
	DWORD sysFlagsAndAttributes = FILE_FLAG_SEQUENTIAL_SCAN;

	if ( openFlags & eOpenFlag_Read )
	{
		sysDesiredAccess |= GENERIC_READ;
		sysShareMode |= FILE_SHARE_READ;
	}
	if ( openFlags & eOpenFlag_Write )
	{
		sysDesiredAccess |= GENERIC_WRITE;
		sysShareMode |= FILE_SHARE_WRITE;
	}
	if ( openFlags & eOpenFlag_Append )
	{
		sysDesiredAccess = FILE_APPEND_DATA; // no other attributes for atomic appends
	}

	// CREATE_ALWAYS truncates, OPEN_ALWAYS creates but does not truncate
	if ( openFlags & eOpenFlag_Create )
	{
		sysCreationDisposition = ( openFlags & eOpenFlag_Truncate ) ? CREATE_ALWAYS : OPEN_ALWAYS;
	}
	else if ( openFlags & eOpenFlag_Truncate )
	{
		sysCreationDisposition = TRUNCATE_EXISTING;
	}

// 	if ( openFlags & eOpenFlag_Async )
// 	{
// 		sysFlagsAndAttributes &= ~FILE_FLAG_SEQUENTIAL_SCAN;
// 
// 		// If you want true async IO on Windows, you must remove buffering,
// 		// and ensue the consequences of properly aligned reads and sector read sizes
// 		sysFlagsAndAttributes |= (FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING);
// 	}

	m_hFile = ::CreateFile( path, sysDesiredAccess, sysShareMode, nullptr, sysCreationDisposition, sysFlagsAndAttributes, nullptr );
	//REDIO_WIN_CHECK( m_hFile != INVALID_HANDLE_VALUE );

	if ( m_hFile != INVALID_HANDLE_VALUE )
	{
		return true;
	}

	return false;
}

Bool CSystemFile::Close()
{
	if ( m_hFile!= INVALID_HANDLE_VALUE )
	{
		REDIO_WIN_CHECK( ::CloseHandle( m_hFile ) );

		m_hFile = INVALID_HANDLE_VALUE;
		return true;
	}

	return false;
}

Bool CSystemFile::Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead )
{
	DWORD dwNumberOfBytesRead = 0;
	Bool bOK = ::ReadFile( m_hFile, dest, length, &dwNumberOfBytesRead, nullptr ) != FALSE;
	REDIO_WIN_CHECK( bOK );

	if ( bOK )
	{
		outNumberOfBytesRead = static_cast< Uint32 >( dwNumberOfBytesRead );
	}

	return bOK;
}

Bool CSystemFile::Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten )
{
	outNumberOfBytesWritten = 0;
	DWORD dwNumberOfBytesWritten = 0;
	Bool bOK = ::WriteFile( m_hFile, src, length, &dwNumberOfBytesWritten, nullptr ) != FALSE;
	REDIO_WIN_CHECK( bOK );

	if ( bOK )
	{
		outNumberOfBytesWritten = dwNumberOfBytesWritten;
	}

	return bOK;
}

Bool CSystemFile::Seek( Int64 offset, ESeekOrigin seekOrigin )
{
	LARGE_INTEGER distanceToMove;
	distanceToMove.QuadPart = offset;

	DWORD moveMethod = FILE_BEGIN;
	switch( seekOrigin )
	{
		case eSeekOrigin_Set:
			moveMethod = FILE_BEGIN;
			break;
		case eSeekOrigin_Current:
			moveMethod = FILE_CURRENT;
			break;
		case eSeekOrigin_End:
			moveMethod = FILE_END;
			break;
		default:
			REDIO_HALT( "Unexpected seekOrigin %u", (Uint32)seekOrigin );
			break;
	}
	
	const Bool bOK = ::SetFilePointerEx( m_hFile, distanceToMove, nullptr, moveMethod ) != FALSE;
	REDIO_WIN_CHECK( bOK );

	return bOK;
}

Int64 CSystemFile::Tell() const
{
	const LARGE_INTEGER distanceToMove = { 0 };
	LARGE_INTEGER offset = { 0 };
	const Bool bOK = ::SetFilePointerEx( m_hFile, distanceToMove, &offset, FILE_CURRENT ) != FALSE;
	REDIO_WIN_CHECK( bOK );

	return bOK ? offset.QuadPart : -1;
}

Bool CSystemFile::Flush()
{
	const Bool bOK = ::FlushFileBuffers( m_hFile ) != FALSE;
	REDIO_WIN_CHECK( bOK );

	return bOK;
}

Uint64 CSystemFile::GetFileSize() const
{
	LARGE_INTEGER fileSize;
	const Bool bOK = ::GetFileSizeEx( m_hFile, &fileSize ) != FALSE;
	if ( bOK )
	{
		return static_cast< Uint64 >( fileSize.QuadPart );
	}

	return 0;
}

Uint32 CSystemFile::GetFileHandle() const
{
	return (Uint32)m_hFile;
}

 
// TODO: Actual async cancelation instead of canceling scheduling chunks. Besides not being part of the supported Durango API,
// the comment in the CancelIOEx function heavily suggests the possibility of the IOCP *not* receiving a completion packet, in which case
// we have no way of safely knowing when to free the overlapped structure. You can't just check with ::GetOverlappedResult because if
// the I/O *wasn't* actually canceled before it completed, then we will get a completion port notification, at which point we'll corrupt memory having
// already freed the overlapped structure.
// Perhaps the way to go is to use unbuffered I/O, which should not produce any synchronous results. Or just ditch IOCP for I/O over this one apparent issue...
/* CancelIoEx function:
		If the file handle is associated with a completion port, an I/O completion packet is not queued to the port if a synchronous operation is
		successfully canceled. For asynchronous operations still pending, the cancel operation will queue an I/O completion packet.
*/

// Win32 I/O Cancellation Support in Windows Vista
// http://msdn.microsoft.com/en-us/library/aa480216.aspx	 

// void CSystemFile::CancelAsync( SSysOverlappedEx& /*overlappexEx*/ )
// {
// #if 0//ndef RED_PLATFORM_DURANGO
// 	Bool bOK = ::CancelIoEx( m_hFile, &overlappexEx ) != FALSE;
// 	DWORD dwError = ::GetLastError();
// 
// 	REDIO_WIN_CHECK( bOK || dwError == ERROR_NOT_FOUND );
// #endif
// }
// 
// void CSystemFile::CancelAsync()
// {
// #if 0//ndef RED_PLATFORM_DURANGO
// 	Bool bOK = ::CancelIoEx( m_hFile, nullptr ) != FALSE; // Use instead of CancelIo because this cancels all in the process not just the calling thread
// 	DWORD dwError = ::GetLastError();
// 	REDIO_WIN_CHECK( bOK || dwError == ERROR_NOT_FOUND );
// #endif
// }

REDIO_WINAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
