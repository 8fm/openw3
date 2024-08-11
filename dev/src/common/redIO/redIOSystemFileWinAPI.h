/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"
#include "redIOCommon.h"

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )

REDIO_WINAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CIOProactor;

//////////////////////////////////////////////////////////////////////////
// CSystemFile
//////////////////////////////////////////////////////////////////////////
class CSystemFile
{
private:
	HANDLE									m_hFile;

public:


public:
											CSystemFile();
											~CSystemFile();

	Bool									Open( const Char* path, Uint32 openFlags );
	Bool									Close();


public:
	Bool									Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead );
	Bool									Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten );
	Bool									Seek( Int64 offset, ESeekOrigin seekOrigin );
	Int64									Tell() const;

public:
	Bool									Flush();

public:
	Bool									IsValid() const { return m_hFile != INVALID_HANDLE_VALUE; }

public:
	Uint64									GetFileSize() const;
	Uint32									GetFileHandle() const;
};

typedef CSystemFile CAsyncFile;

REDIO_WINAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
