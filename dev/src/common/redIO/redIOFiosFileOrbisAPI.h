/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "redIOPlatform.h"
#include "redIOCommon.h"
#include "redIOFiosFwd.h"

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
#if defined( RED_PLATFORM_ORBIS )

REDIO_ORBISAPI_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
class CIOProactor;

//////////////////////////////////////////////////////////////////////////
// CFiosFile
//////////////////////////////////////////////////////////////////////////
class CFiosFile
{
private:
	SceFiosFH								m_sceFileHandle;

public:
											CFiosFile();
											CFiosFile( CFiosFile&& other );
											CFiosFile& operator=( CFiosFile&& rhs );
											~CFiosFile();

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
	Bool									IsValid() const;

public:
	Uint64									GetFileSize() const;
	Uint32									GetFileHandle() const;

public:
	SceFiosFH								GetPlatformHandle() const { return m_sceFileHandle; }
};

typedef CFiosFile CAsyncFile;

REDIO_ORBISAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_ORBIS )
