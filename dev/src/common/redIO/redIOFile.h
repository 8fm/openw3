/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"
#include "redIOCommon.h"

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
# include "redIOSystemFileWinAPI.h"
#elif defined( RED_PLATFORM_ORBIS )
# include "redIOFiosFileOrbisAPI.h"
#else
# error Platform unsupported
#endif

REDIO_NAMESPACE_BEGIN

/**
 * This is unbuffered, synchronous I/O. If you can use IFile, use IFile.
 * E.g., could become a drop-in replacement internal to systemWin32.cpp (and PS4).
 */

//////////////////////////////////////////////////////////////////////////
// CNativeFileHandle
//////////////////////////////////////////////////////////////////////////
class CNativeFileHandle
{
private:
#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	OSAPI::CSystemFile						m_file;
#elif defined( RED_PLATFORM_ORBIS )
	OSAPI::CFiosFile						m_file;
#else
# error Unsupported platform!
#endif
	Bool									m_async;

public:
											CNativeFileHandle();
											~CNativeFileHandle();

public:
	//! Open file for blocking I/O. Returns true upon success.
	Bool									Open( const Char* path, Uint32 openFlags );

	//! Close file. Returns true upon success.
	Bool									Close();

public:
	//! Blocking read. Returns true upon success.
	Bool									Read( void* dest, Uint32 length, Uint32& /*[out]*/ outNumberOfBytesRead );

	//! Blocking write. Returns true upon success.
	Bool									Write( const void* src, Uint32 length, Uint32& /*[out]*/ outNumberOfBytesWritten );

	//! Seek to file position. Returns true upon success.
	Bool									Seek( Int64 offset, ESeekOrigin seekOrigin );

	//! Get file offset. Returns -1 upon error.
	Int64									Tell() const;

public:
	//! Is the file valid
	Bool									IsValid() const;

	//! Flush to disk. Returns true upon success.
	Bool									Flush();

	//! Get file size. Returns zero upon error.
	Uint64									GetFileSize() const;

	//! Get file handle. Returns zero upon error.
	Uint32									GetFileHandle() const;
};

REDIO_NAMESPACE_END
