/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "redIOSystemFileLinuxAPI.h"

#if defined( RED_PLATFORM_LINUX )

#include "../../redSystem/include/threads.h"
#include <utility>
#include <string.h>

#define ERROR_MESSAGE( errorBuffer )			\
	Int32 errorCode = errno;					\
	char errorBuffer[256];						\
	strerror_r( errorCode, errorBuffer, 256 );

namespace io
{

	Uint32 LastError()
	{
		// TODO
		return 0;
	}

	namespace linux
	{
		const Int32 c_invalidDescriptor = -1;

		//////////////////////////////////////////////////////////////////////////
		// SystemFile
		//////////////////////////////////////////////////////////////////////////
		SystemFile::SystemFile()
			: m_fileDescriptor( c_invalidDescriptor )
		{
		}

		SystemFile::SystemFile( SystemFile&& other )
			: m_fileDescriptor( other.m_fileDescriptor )
		{
			other.m_fileDescriptor = c_invalidDescriptor;
		}

		SystemFile& SystemFile::operator=( SystemFile&& other )
		{
			if ( this != &other )
			{
				Close();
				std::swap( m_fileDescriptor, other.m_fileDescriptor );
			}
			return *this;
		}

		SystemFile::~SystemFile()
		{
			Close();
		}

		Bool SystemFile::Open( const char* path, Uint32 openFlags )
		{
			RED_ASSERT( m_fileDescriptor == c_invalidDescriptor );

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
				RED_LOG_ERROR( "RedIO: open failed to open '%hs', openFlags=%d, error=%hs", path, descriptorFlags, errorBuffer );
				return false;
			}

			return true;
		}

		Bool SystemFile::Close()
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

		Bool SystemFile::Read( void* dest, Uint32 length, Uint32& outNumberOfBytesRead )
		{
			const size_t readSize = ::read( m_fileDescriptor, dest, length );
			if ( readSize < 0 )
			{
				ERROR_MESSAGE( errorBuffer );
				RED_LOG_ERROR( "RedIO: fread failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
				outNumberOfBytesRead = 0;
				return false;
			}

			outNumberOfBytesRead = static_cast< Uint32 >( readSize );

			return true;
		}

		Bool SystemFile::Write( const void* src, Uint32 length, Uint32& outNumberOfBytesWritten )
		{
			const ssize_t writeSize = ::write( m_fileDescriptor, src, length );
			if ( writeSize < 0 )
			{
				ERROR_MESSAGE( errorBuffer );
				RED_LOG_ERROR( "RedIO: fwrite failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
				outNumberOfBytesWritten = 0;
				return false;
			}

			outNumberOfBytesWritten = static_cast< Uint32 >( writeSize );

			return true;
		}

		Bool SystemFile::Seek( Int64 offset, ESeekOrigin seekOrigin )
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
			RED_LOG_ERROR( "RedIO: fseek failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
			return false;
		}

		Int64 SystemFile::Tell() const
		{
			const Int64 pos = ::lseek( m_fileDescriptor, 0, SEEK_CUR );
			if ( pos >= 0 )
			{
				return pos;
			}

			ERROR_MESSAGE( errorBuffer );
			RED_LOG_ERROR( "RedIO: ftell failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
			return -1;
		}

		Bool SystemFile::Truncate( Uint64 totalFileSize )
		{
			const auto size = static_cast< off_t >( totalFileSize );
			if ( size < 0 )
			{
				return false;
			}
			const Int32 err = ::ftruncate( m_fileDescriptor, size );
			if ( err == 0 )
			{
				return true;
			}

			ERROR_MESSAGE( errorBuffer );
			RED_LOG_ERROR( "RedIO: ftruncate failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
			return false;
		}

		Bool SystemFile::IsValid() const
		{
			return m_fileDescriptor != c_invalidDescriptor;
		}

		Bool SystemFile::Flush()
		{
			const Int32 err = ::fsync( m_fileDescriptor );
			if ( err == 0 )
			{
				return true;
			}

			ERROR_MESSAGE( errorBuffer );
			RED_LOG_ERROR( "RedIO: fflush failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
			return false;
		}

		Uint64 SystemFile::GetFileSize() const
		{
			struct stat stat;
			const Int32 err = ::fstat( m_fileDescriptor, &stat );
			if ( err != 0 )
			{
				ERROR_MESSAGE( errorBuffer );
				RED_LOG_ERROR( "RedIO: fstat failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
				return 0;
			}

			return stat.st_size;
		}

		Uint64 SystemFile::GetFileTimestamp() const
		{
			struct stat stat;
			const Int32 err = ::fstat( m_fileDescriptor, &stat );
			if ( err != 0 )
			{
				ERROR_MESSAGE( errorBuffer );
				RED_LOG_ERROR( "RedIO: fstat failed for descriptor %d. Error=%hs", m_fileDescriptor, errorBuffer );
				return 0;
			}

			// Unix time epoch stores
			// nanoseconds since 00:00:00 1 Jan 1970 UTC
			// Convert it to Windows timestamp which is counted in
			// 100 nanosecond intervals since 00:00:00 1 Jan 1601 UTC
			const Uint64 epochDifference = 116444736000000000ull;
			return ( stat.st_mtime / 100 ) - epochDifference;
		}

		Uint32 SystemFile::GetFileHandle() const
		{
			return static_cast< Uint32 >( m_fileDescriptor );
		}

		Uint32 SystemFile::GetFileID() const
		{
			return GetFileHandle();
		}

	}
}

#else
	RED_NO_EMPTY_FILE();
#endif // #if defined( RED_PLATFORM_LINUX )
