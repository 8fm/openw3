/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redIOSystemFileLinuxAPI.h"

#if defined( RED_PLATFORM_LINUX )

#include <utility>
#include <string.h>

#define ERROR_MESSAGE( errorBuffer )			\
	Int32 errorCode = errno;					\
	char errorBuffer[256];						\
	strerror_r( errorCode, errorBuffer, 256 );

REDIO_LINUXAPI_NAMESPACE_BEGIN

const Int32 c_invalidDescriptor = -1;

//////////////////////////////////////////////////////////////////////////
// CSystemFile
//////////////////////////////////////////////////////////////////////////
CSystemFile::CSystemFile()
	: m_fileDescriptor( c_invalidDescriptor )
{
}

CSystemFile::~CSystemFile()
{
	Close();
}

Bool CSystemFile::Open( const char* path, Uint32 openFlags )
{
	REDIO_ASSERT( m_fileDescriptor == c_invalidDescriptor );

	Int32 descriptorFlags = 0;

	if ( ( openFlags & ( eOpenFlag_Read | eOpenFlag_Write ) ) == ( eOpenFlag_Read | eOpenFlag_Write ) )
	{
		descriptorFlags |= O_RDWR;
	}
	else
	{
		if ( openFlags & eOpenFlag_Read )
		{
			descriptorFlags |= O_RDONLY;
		}
		if ( openFlags & eOpenFlag_Write )
		{
			descriptorFlags |= O_WRONLY;
		}
	}

	if ( openFlags & eOpenFlag_Append )
	{
		descriptorFlags |= O_APPEND;
	}
	if ( openFlags & eOpenFlag_Create )
	{
		descriptorFlags |= O_CREAT;
	}

	// #tbd: check if O_RDONLY is set and fail?
	if ( openFlags & eOpenFlag_Truncate )
	{
		descriptorFlags |= O_TRUNC;
	}

	m_fileDescriptor = ::open( path, descriptorFlags, S_IRUSR | S_IWUSR );
	if ( m_fileDescriptor == c_invalidDescriptor )
	{
		ERROR_MESSAGE( errorBuffer );
		RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("open failed to open '%hs', openFlags=%d, error=%hs"), path, descriptorFlags, errorBuffer );
		return false;
	}

	return true;
}

Bool CSystemFile::Close()
{
	if ( m_fileDescriptor != c_invalidDescriptor )
	{
		const Int32 err = ::close( m_fileDescriptor );
		m_fileDescriptor = c_invalidDescriptor;
		if ( err != 0 )
		{
			return false;
		}
	}

	return true;
}

Bool CSystemFile::Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead )
{
	const size_t readSize = ::read( m_fileDescriptor, dest, length );
	if ( readSize < 0 )
	{
		ERROR_MESSAGE( errorBuffer );
		RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("fread failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
		outNumberOfBytesRead = 0;
		return false;
	}

	outNumberOfBytesRead = static_cast< Uint32 >( readSize );

	return true;
}

Bool CSystemFile::Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten )
{
	const ssize_t writeSize = ::write( m_fileDescriptor, src, length );
	if ( writeSize < 0 )
	{
		ERROR_MESSAGE( errorBuffer );
		RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("fwrite failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
		outNumberOfBytesWritten = 0;
		return false;
	}

	outNumberOfBytesWritten = static_cast< Uint32 >( writeSize );

	return true;
}

Bool CSystemFile::Seek( Int64 offset, ESeekOrigin seekOrigin )
{
	static_assert( static_cast< Int32 >( eSeekOrigin_Set ) == SEEK_SET, "Enum mismatch" );
	static_assert( static_cast< Int32 >( eSeekOrigin_Current ) == SEEK_CUR, "Enum mismatch" );
	static_assert( static_cast< Int32 >( eSeekOrigin_End ) == SEEK_END, "Enum mismatch" );

	const Int64 newPos = ::lseek( m_fileDescriptor, offset, static_cast< Int32 >( seekOrigin ) );

	if ( newPos >= 0 )
	{
		return true;
	}

	ERROR_MESSAGE( errorBuffer );
	RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("fseek failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
	return false;
}

Int64 CSystemFile::Tell() const
{
	const Int64 pos = ::lseek( m_fileDescriptor, 0, SEEK_CUR );
	if ( pos >= 0 )
	{
		return pos;
	}

	ERROR_MESSAGE( errorBuffer );
	RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("ftell failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
	return -1;
}

Bool CSystemFile::Flush()
{
	const Int32 err = ::fsync( m_fileDescriptor );
	if ( err == 0 )
	{
		return true;
	}

	ERROR_MESSAGE( errorBuffer );
	RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("fflush failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
	return false;
}

Bool CSystemFile::IsValid() const
{
	return m_fileDescriptor != c_invalidDescriptor;
}

Uint64 CSystemFile::GetFileSize() const
{
	struct stat stat;
	const Int32 err = ::fstat( m_fileDescriptor, &stat );
	if ( err != 0 )
	{
		ERROR_MESSAGE( errorBuffer );
		RED_LOG_ERROR( RED_LOG_CHANNEL( RedIO ), TXT("fstat failed for descriptor %d. Error=%hs"), m_fileDescriptor, errorBuffer );
		return 0;
	}

	return stat.st_size;
}

Uint32 CSystemFile::GetFileHandle() const
{
	return static_cast< Uint32 >( m_fileDescriptor );
}

REDIO_LINUXAPI_NAMESPACE_END

#endif // #if defined( RED_PLATFORM_LINUX )
