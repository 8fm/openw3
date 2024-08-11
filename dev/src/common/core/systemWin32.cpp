/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "string.h"
#include "events.h"
#include "namesRegistry.h"
#include "fileSystemProfilerWrapper.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
#	include "Shlobj.h"
#endif

/************************************************************************/
/* Globals																*/
/************************************************************************/
CSystemIO		GSystemIO;

/************************************************************************/
/* System I/O implementation                                            */
/************************************************************************/
Bool CSystemIO::CopyFile(const Char* existingFileName, const Char* newFileName, Bool failIfExists) const
{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	return 0 != ::CopyFile(existingFileName, newFileName, failIfExists);
#elif defined( RED_PLATFORM_DURANGO )
// 	BOOL cancel = FALSE;
// 	COPYFILE2_EXTENDED_PARAMETERS copyParams;
// 	copyParams.dwSize = static_cast< DWORD >( sizeof( COPYFILE2_EXTENDED_PARAMETERS ) );
// 	copyParams.dwCopyFlags = failIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0;
// 	copyParams.pfCancel = &cancel; // Stack variable... but not an opt param?
// 	copyParams.pProgressRoutine = nullptr;
// 	copyParams.pvCallbackContext = nullptr;
// 
// #pragma message( "CopyFile2: async copy")
// 	HRESULT hr = ::CopyFile2(existingFileName, newFileName, &copyParams );
// 	if ( hr == S_OK )
// 	{
// 		return true;
// 	}
#endif

	return false;
}

Bool CSystemIO::CreateDirectory(const Char* pathName) const
{
#ifdef RED_PLATFORM_WINPC
	DWORD code = ::SHCreateDirectoryEx( NULL, pathName, NULL );

	// SHCreateDirectoryEx() creates all the intermediate directories as well as the final one
	if ( code == ERROR_SUCCESS || code == ERROR_CANCELLED || code == ERROR_ALREADY_EXISTS )
	{
		return true;
	}

#elif defined( RED_PLATFORM_DURANGO )
	//FIXME: Doesn't create intermediate directories. This is just using the "old" code before CL# 219192 changed it.
	if ( 0 != ::CreateDirectory(TO_PLATFORMCODE(pathName), NULL) )
	{
		return true;
	}
	else if ( ::GetLastError() == ERROR_ALREADY_EXISTS )
	{
		return true;
	}
#endif

	return false;
}

Bool CSystemIO::DeleteFile(const Char* fileName) const
{
	return 0 != ::DeleteFile(fileName);
}

Bool CSystemIO::IsFileReadOnly(const Char* fileName) const
{
#ifdef RED_PLATFORM_WINPC
	const DWORD fileAttributes = ::GetFileAttributes(fileName);

	// file doesn't exist
	if ( fileAttributes == INVALID_FILE_ATTRIBUTES ) 
	{
		return false;
	}

	// check if file is read only
	return fileAttributes & FILE_ATTRIBUTE_READONLY;
#else
	// files on cooked platforms are always read only
	return true;
#endif
}

Bool CSystemIO::FileExist(const Char* fileName) const
{
#ifdef RED_PLATFORM_WINPC
	const DWORD fileAttributes = ::GetFileAttributes(fileName);

	return ( fileAttributes != INVALID_FILE_ATTRIBUTES );
#else
	WARN_CORE(TXT("Someone tried to check if file exists, probably this is causing a bug now: %s"), fileName);
	return false;
#endif
}

Bool CSystemIO::SetFileReadOnly(const Char* fileName, Bool readOnlyFlag )
{
	DWORD fileAttributes = ::GetFileAttributes(fileName);

	if ( readOnlyFlag )
	{
		fileAttributes |= FILE_ATTRIBUTE_READONLY;
	}
	else
	{
		fileAttributes &= ~FILE_ATTRIBUTE_READONLY;
	}

	return ::SetFileAttributes(fileName, fileAttributes) != 0;
}

Bool CSystemIO::MoveFile(const Char* existingFileName, const Char* newFileName) const
{
	const Uint32 flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED;
	if ( 0 == ::MoveFileEx( existingFileName, newFileName, flags ) )
	{
		LOG_CORE( TXT("[IO]: Low level move file failed: 0x%X"), GetLastError() );
		return false;
	}

	return true;
}

Bool CSystemIO::RemoveDirectory(const Char* pathName) const
{
	return 0 != ::RemoveDirectory(pathName);
}

Bool CSystemIO::CreatePath( const Char* pathName ) const
{
	Char buffer[ 4096 ];
	Red::System::StringCopy( buffer, pathName, ARRAY_COUNT( buffer ) );

	// Create path
	Char *path = buffer;	 
	for ( Char *pos=path; *pos; pos++ )
	{
		if ( *pos == '\\' || *pos == '/' )
		{
			Char was = *pos;
			*pos = 0;
			if ( !CreateDirectory( path ))
			{
				if ( !Red::System::StringSearch( path, ':' ) )
				{
					return false;
				}
			}
			*pos = was;
		}
	}

	// Path created
	return true;
}

Red::System::DateTime CSystemIO::GetFileTime( const Char* pathName )
{
	Red::System::DateTime fileTime;

	// Open the file handle without opening the file
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSyncIOOpenFileStart( pathName );
#endif
	HANDLE handle = ::CreateFile( pathName, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSyncIOOpenFileEnd( (Uint32)handle );
#endif

	if ( handle != INVALID_HANDLE_VALUE )
	{
		// Initialize the time
		FILETIME systemFormat;
		Red::System::MemoryZero( &systemFormat, sizeof( FILETIME ) );

		// Get the time of last write access to the file ( modification time )
		::GetFileTime( handle, NULL, NULL, &systemFormat );

		// Convert the value into a format compatible with our DateTime
		SYSTEMTIME dateFormat;
		Red::System::MemoryZero( &dateFormat, sizeof( FILETIME ) );

		::FileTimeToSystemTime( &systemFormat, &dateFormat );

		// Fill in the return value
		fileTime.SetYear			( static_cast< Uint32 >( dateFormat.wYear ) );
		fileTime.SetMonth			( static_cast< Uint32 >( dateFormat.wMonth ) - 1 );
		fileTime.SetDay				( static_cast< Uint32 >( dateFormat.wDay ) - 1 );
		fileTime.SetHour			( static_cast< Uint32 >( dateFormat.wHour ) );
		fileTime.SetMinute			( static_cast< Uint32 >( dateFormat.wMinute ) );
		fileTime.SetSecond			( static_cast< Uint32 >( dateFormat.wSecond ) );
		fileTime.SetMilliSeconds	( static_cast< Uint32 >( dateFormat.wMilliseconds ) );

		// Close the handle as we no longer need it
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSyncIOCloseFileStart( (Uint32)handle );
#endif
		::CloseHandle( handle );
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSyncIOCloseFileEnd();
#endif
	}

	return fileTime;
}

Uint64 CSystemIO::GetFileSize( const Char* pathName ) const
{
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSyncIOOpenFileStart( pathName );
#endif
	HANDLE file = ::CreateFile( pathName, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileSyncIOOpenFileEnd( (Uint32)file );
#endif

	if ( INVALID_HANDLE_VALUE == file )
	{
		return 0;
	}
	else
	{
		LARGE_INTEGER size;
		Red::System::MemorySet( &size, 0, sizeof(size) );
		::GetFileSizeEx( file, &size );
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSyncIOCloseFileStart( (Uint32)file );
#endif
		CloseHandle( file );
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileSyncIOCloseFileEnd();
#endif
		return size.QuadPart;
	}

	return 0;
}

/************************************************************************/
/* System File implementation                                           */
/************************************************************************/

CSystemFile* CSystemFile::m_first = 0;

CSystemFile::CTime::CTime()
{
	m_fileTime.dwHighDateTime = 0;
	m_fileTime.dwLowDateTime = 0;
}

CSystemFile::CTime::CTime(Uint64 time)
{
	m_fileTime.dwHighDateTime = (DWORD)(time >> 32);
	m_fileTime.dwLowDateTime = (DWORD)(time & 0xffffffff);
}

String CSystemFile::CTime::ToString()
{
	SYSTEMTIME st;

	FILETIME localFileTime;
	FileTimeToLocalFileTime( &m_fileTime, &localFileTime );
	FileTimeToSystemTime( &localFileTime, &st );
	
	// Build a string showing the date and time.
	return String::Printf(TXT("%02d/%02d/%d  %02d:%02d"),
        st.wMonth, st.wDay, st.wYear,
		st.wHour, st.wMinute);
}

void CSystemFile::CTime::Extract( Int32& year, Int32& month, Int32& day, Int32& hour, Int32& minute, Int32& second )
{
	SYSTEMTIME st;

	FILETIME localFileTime;
	FileTimeToLocalFileTime( &m_fileTime, &localFileTime );
	FileTimeToSystemTime( &localFileTime, &st );

	day = st.wDay;
	month = st.wMonth;
	year = st.wYear;
	hour = st.wHour;
	minute = st.wMinute;
	second = st.wSecond;
}

Uint64 CSystemFile::CTime::ToUint64() const
{
	return (( Uint64 ) m_fileTime.dwHighDateTime << 32) | ( m_fileTime.dwLowDateTime );
}

Bool CSystemFile::CTime::operator >(const CSystemFile::CTime& time) const
{
	return 1 == ::CompareFileTime(&m_fileTime, &time.m_fileTime);
}

CSystemFile::CSystemFile()
: m_file(INVALID_HANDLE_VALUE)
, m_next( 0 )
, m_lastPointer( 0 )
, m_currentPointer( 0 )
{
}

CSystemFile::CSystemFile( const CSystemFile& other )
: m_file( other.m_file )
, m_next( other.m_next )
, m_lastPointer( 0 )
, m_currentPointer( 0 )
{
	Red::System::StringCopy( m_fileName, other.m_fileName, MAX_FILE_NAME);
	if( m_file == INVALID_HANDLE_VALUE )
	{
		return;
	}
}

CSystemFile::~CSystemFile()
{
}

CSystemFile::operator Bool() const
{
	return m_file != INVALID_HANDLE_VALUE;
}

Bool CSystemFile::CreateWriter( const Char* fileName, Bool append )
{
	DWORD dwCreationDisposition;

	//CSystem::SystemFileAccessLock();

	if( append )
	{
		dwCreationDisposition = OPEN_ALWAYS;
		m_currentPointer = GetSize();
	}
	else
	{
		dwCreationDisposition = CREATE_ALWAYS;
	}
	
	EDITOR_DISPATCH_EVENT( CNAME( FileOperationStarted ), CreateEventData( String( fileName ) ) );

	m_file = ::CreateFile( fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_FLAG_SEQUENTIAL_SCAN, NULL );

#ifdef RED_PLATFORM_WINPC

	// If not created then create path and try again
	if ( INVALID_HANDLE_VALUE == m_file )
	{
		// Copy to local storage
		Char filePath[ MAX_PATH ];
		Red::System::StringCopy( filePath, fileName, MAX_PATH );

		// Create directories
		Char* pos = filePath;
		while ( *pos )
		{
			// Create path
			if ( *pos == '\\' || *pos == '/' )
			{
				Char org = *pos;
				*pos = 0;
				if ( ERROR_PATH_NOT_FOUND == CreateDirectory( filePath, NULL ) )
				{
					// Unable to create final path
					return false;
				}
				*pos = org;
			}

			// Next char
			pos++;
		}

		m_file = ::CreateFile( fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	}
#endif
	Red::System::StringCopy(m_fileName, fileName, MAX_FILE_NAME);

	//CSystem::SystemFileAccessUnlock();

	return operator Bool();
}

Bool CSystemFile::CreateReader(const Char* fileName)
{
	//CSystem::SystemFileAccessLock();
	m_file = ::CreateFile( fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );

	Red::System::StringCopy( m_fileName, fileName, MAX_FILE_NAME );
	//CSystem::SystemFileAccessUnlock();

	return operator Bool();
}

Bool CSystemFile::Close()
{
	if( m_file == INVALID_HANDLE_VALUE ) 
	{
		return false;
	}

	//CSystem::SystemFileAccessLock();

	BOOL ret = ::CloseHandle(m_file);

	//CSystem::SystemFileAccessUnlock();

	m_file = INVALID_HANDLE_VALUE;

	return 0 != ret;
}

CSystemFile* CSystemFile::FindAlreadyOpened( const Char* filePath )
{
	CSystemFile* current = m_first;
	while( current )
	{
		if( Red::System::StringCompare( current->m_fileName, filePath ) == 0 )
		{
			return current;
		}
		current = current->m_next;
	}
	return 0;
}

Bool CSystemFile::FlushBuffers() const
{
	return 0 != ::FlushFileBuffers(m_file);
}

Uint64 CSystemFile::GetSize() const
{
	LARGE_INTEGER fileSize = { 0 };
	if ( ::GetFileSizeEx(m_file, &fileSize ) != FALSE )
	{
		RED_ASSERT( fileSize.QuadPart >= 0 );
		return fileSize.QuadPart;
	}
	return 0;
}

CSystemFile::CTime CSystemFile::GetCreationTime() const
{
	CTime out;

	::GetFileTime(m_file, &out.m_fileTime, NULL, NULL);

	return out;
}

CSystemFile::CTime CSystemFile::GetLastAccessTime() const
{
	CTime out;

	::GetFileTime(m_file, NULL, &out.m_fileTime, NULL);

	return out;
}

CSystemFile::CTime CSystemFile::GetLastWriteTime() const
{
	CTime out;

	::GetFileTime(m_file, NULL, NULL, &out.m_fileTime);

	return out;
}

size_t CSystemFile::Read( void* buf, size_t bytesToRead )
{
	DWORD nr;

	//CSystem::SystemFileAccessLock();

	if( m_lastPointer != (uintptr_t)m_currentPointer )
	{
		LARGE_INTEGER result;
		result.QuadPart = m_currentPointer;

		if ( ::SetFilePointerEx( m_file, result, NULL, FILE_BEGIN ) == FALSE )
		{
			LOG_CORE( TXT("[IO]: Low level file pointer change to %I64u failed, error code: 0x%X"), m_currentPointer, GetLastError() );
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	if( !::ReadFile( m_file, buf, static_cast< DWORD >( bytesToRead ), &nr, NULL ) )
	{
		return 0;
	}

	m_currentPointer += nr;
	m_lastPointer = m_currentPointer;
	
	//CSystem::SystemFileAccessUnlock();

	return nr;
}

uintptr_t CSystemFile::Write( const void* buf, size_t bytesToWrite )
{
	DWORD nw;
	//CSystem::SystemFileAccessLock();

	if( m_lastPointer != (uintptr_t)m_currentPointer )
	{
		LARGE_INTEGER result;
		result.QuadPart = m_currentPointer;

		if ( ::SetFilePointerEx( m_file, result, NULL, FILE_BEGIN ) == FALSE )
		{
			LOG_CORE( TXT("[IO]: Low level file pointer change to %I64u failed, error code: 0x%X"), m_currentPointer, GetLastError() );
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	if( !::WriteFile( m_file, buf, static_cast< DWORD >( bytesToWrite ), &nw, NULL ) )
	{
		nw = 0;
	}

	m_currentPointer += nw;
	m_lastPointer = m_currentPointer;

	//CSystem::SystemFileAccessUnlock();
	return nw;
}

Bool CSystemFile::SetEndOfFile()
{
	//CSystem::SystemFileAccessLock();

	if( m_lastPointer != (uintptr_t)m_currentPointer )
	{
		LARGE_INTEGER result;
		result.QuadPart = m_currentPointer;

		if ( ::SetFilePointerEx( m_file, result, NULL, FILE_BEGIN ) == FALSE )
		{
			LOG_CORE( TXT("[IO]: Low level file pointer change to %I64u failed, error code: 0x%X"), m_currentPointer, GetLastError() );
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	BOOL result = 0 != ::SetEndOfFile(m_file);

	//CSystem::SystemFileAccessUnlock();

	return 0 != result;
}

Bool CSystemFile::GetPointerCurrent( Uint64& filePointer ) const
{
	filePointer = m_currentPointer;
	return true;
}

Bool CSystemFile::SetPointerBegin( Int64 distanceToMove )
{
	m_currentPointer = distanceToMove;
	return true;
}

/************************************************************************/
/* System FindFile                                                      */
/************************************************************************/
CSystemFindFile::CSystemFindFile(const Char* fileName)
{
	m_findFile.m_hasMore = true;
	m_findFile.m_handle = ::FindFirstFileEx( TO_PLATFORMCODE(fileName), FindExInfoStandard, &m_findFile.m_findData, FindExSearchNameMatch, NULL, 0 );

	while (Bool(*this))
	{
		const Char* fileName = GetFileName();
		if ( !( fileName[0] == '.' && ( fileName[1] == '\0' || ( fileName[1] == '.' && fileName[2] == '\0' ) ) ) )
			break;
		++(*this);
	}
}

CSystemFindFile::~CSystemFindFile()
{
	if ( m_findFile.m_handle != INVALID_HANDLE_VALUE )
	{
		::FindClose( m_findFile.m_handle );
		m_findFile.m_handle = INVALID_HANDLE_VALUE;
	}
}

CSystemFindFile::operator Bool() const
{
	return m_findFile.m_handle != INVALID_HANDLE_VALUE && m_findFile.m_hasMore;
}

const Char* CSystemFindFile::GetFileName()
{
	return m_findFile.m_findData.cFileName;
}

Bool CSystemFindFile::IsDirectory() const
{
	return 0 != (m_findFile.m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

Uint32 CSystemFindFile::GetSize()
{
	return m_findFile.m_findData.nFileSizeLow;
}

void CSystemFindFile::operator ++()
{
	m_findFile.m_hasMore = 0 != ::FindNextFile(m_findFile.m_handle, &m_findFile.m_findData);
}

#endif // RED_PLATFORM_WIN32 || RED_PLATFORM_WIN64 || RED_PLATFORM_DURANGO
