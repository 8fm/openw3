/**
* Copyright (c) 2018 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"
#include "redIOCommon.h"

#if defined( RED_PLATFORM_LINUX )

REDIO_LINUXAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// CSystemFile
//////////////////////////////////////////////////////////////////////////
class CSystemFile
{
public:
	CSystemFile();
	~CSystemFile();
	Bool Open( const char* path, Uint32 openFlags );
	Bool Close();

	Bool Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead );
	Bool Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten );
	Bool Seek( Int64 offset, ESeekOrigin seekOrigin );
	Int64 Tell() const;
	Bool Flush();

	Bool IsValid() const;

	Uint64 GetFileSize() const;
	Uint32 GetFileHandle() const;

private:
	int	m_fileDescriptor;
};

typedef CSystemFile CAsyncFile;

REDIO_LINUXAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_LINUX )
