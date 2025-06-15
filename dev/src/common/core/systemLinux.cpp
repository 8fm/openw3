/**
* Copyright (c) 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifdef RED_PLATFORM_LINUX

#include <fstream>
#include <fnmatch.h>

/************************************************************************/
/* Globals																*/
/************************************************************************/
CSystemIO		GSystemIO;

void LogLastError( const Char* action, const Char* target )
{
	Int32 errorCode = errno;
	char errorBuffer[256];
	strerror_r( errorCode, errorBuffer, 256 );

	LOG_CORE( TXT("[IO]: %ls failed: target=%ls. error=%ls"), action, target, errorBuffer );
}

/************************************************************************/
/* System I/O implementation                                            */
/************************************************************************/
Bool CSystemIO::CopyFile(const Char* existingFileName, const Char* newFileName, Bool failIfExists) const
{
	// check if new file already exists
	Bool exists = FileExist( newFileName );
	if ( exists  && failIfExists )
	{
		LOG_CORE( TXT("[IO]: Low level copy file failed: origin=%ls target=%ls. Error=file already exists"), existingFileName, newFileName );
		return false;
	}

	// copy the file
	std::ifstream  sourceFile( TO_PLATFORMCODE(existingFileName), std::ios::binary );
	std::ofstream  destinationFile( TO_PLATFORMCODE(newFileName), std::ios::binary );

	if ( sourceFile.good() && destinationFile.good() )
	{
		destinationFile << sourceFile.rdbuf();
		return true;
	}

	LOG_CORE( TXT("[IO]: Low level copy file failed: origin=%ls target=%ls."), existingFileName, newFileName );
	return false;
}

Bool CSystemIO::CreateDirectory(const Char* pathName) const
{
	Int32 status = mkdir( TO_PLATFORMCODE(pathName), S_IRWXU );
	if ( status != 0 && errno != EEXIST )
	{
		LogLastError( TXT("Low level create directory"), pathName );
		return false;
	}
	return true;
}

Bool CSystemIO::DeleteFile(const Char* fileName) const
{
	Int32 status = ::unlink( TO_PLATFORMCODE(fileName) );
	if ( status != 0 )
	{
		LogLastError( TXT("Low level delete file"), fileName );
		return false;
	}

	return true;
}

Bool CSystemIO::IsFileReadOnly(const Char* fileName) const
{
	Bool readable = ( ::access( TO_PLATFORMCODE(fileName), R_OK ) == 0 );
	Bool writable = ( ::access( TO_PLATFORMCODE(fileName), W_OK ) == 0 );

	return readable && !writable;
}

Bool CSystemIO::FileExist(const Char* fileName) const
{
	Bool status = ::access( TO_PLATFORMCODE(fileName), F_OK );
	return status == 0;
}

Bool CSystemIO::SetFileReadOnly(const Char* fileName, Bool readOnlyFlag )
{
	Int32 status = ::chmod( TO_PLATFORMCODE(fileName), readOnlyFlag ? S_IRUSR : ( S_IRUSR | S_IWUSR ) );
	if ( status != 0 )
	{
		LogLastError( TXT("Low level set file read only"), fileName );
		return false;
	}
	return true;
}

Bool CSystemIO::MoveFile(const Char* existingFileName, const Char* newFileName) const
{
	// #tbd: need to preserve permissions?
	Int32 status = rename( TO_PLATFORMCODE(existingFileName), TO_PLATFORMCODE(newFileName) );
	if ( status != 0 )
	{
		LogLastError( TXT("Low level move file"), existingFileName );
		return false;
	}
	return true;
}

Bool CSystemIO::RemoveDirectory(const Char* pathName) const
{
	Int32 status = ::rmdir( TO_PLATFORMCODE(pathName) );
	if ( status != 0 )
	{
		LogLastError( TXT("Low level remove directory"), pathName );
		return false;
	}
	return true;
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

	struct stat stats;
	if ( ::stat( TO_PLATFORMCODE(pathName), &stats ) == 0 )
	{
		// Convert to format that can easily be passed through to
		struct tm linuxFileTime = { 0 };
		if ( gmtime_r( &stats.st_mtime, &linuxFileTime ) != NULL  )
		{
			fileTime.SetYear( static_cast< Uint32 >( linuxFileTime.tm_year ) + 1900 ); // 1900+n
			fileTime.SetMonth( static_cast< Uint32 >( linuxFileTime.tm_mon ) ); // 0-11
			fileTime.SetDay( static_cast< Uint32 >( linuxFileTime.tm_mday ) - 1 ); // 1-31
			fileTime.SetHour( static_cast< Uint32 >( linuxFileTime.tm_hour ) );
			fileTime.SetMinute( static_cast< Uint32 >( linuxFileTime.tm_min ) );
			fileTime.SetSecond( static_cast< Uint32 >( linuxFileTime.tm_sec ) );
		}
	}

	return fileTime;
}

Uint64 CSystemIO::GetFileSize( const Char* pathName ) const
{
	struct stat stats;
	if ( ::stat( TO_PLATFORMCODE(pathName), &stats ) != 0 )
	{
		return 0;
	}

	return stats.st_size >= 0 ? static_cast< Uint64 >( stats.st_size ) : 0;
}

/************************************************************************/
/* System File implementation                                           */
/************************************************************************/

CSystemFile* CSystemFile::m_first = 0;
const int c_invalidDescriptor = -1;

CSystemFile::CSystemFile()
: m_file(c_invalidDescriptor)
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
	if( m_file == c_invalidDescriptor )
	{
		return;
	}
}

CSystemFile::~CSystemFile()
{
}

CSystemFile::operator Bool() const
{
	return m_file != c_invalidDescriptor;
}

Bool CSystemFile::CreateWriter( const Char* fileName, Bool append )
{
	int dwCreationDisposition = O_CREAT | O_WRONLY; // or O_RDWR ?

	if( append )
	{
		dwCreationDisposition |= O_APPEND;
		m_currentPointer = GetSize();
	}
	else
	{
		dwCreationDisposition |= O_TRUNC;
	}

#ifndef NO_EDITOR_EVENT_SYSTEM
	EDITOR_DISPATCH_EVENT( CNAME( FileOperationStarted ), CreateEventData( String( fileName ) ) );
#endif

	//m_file = ::CreateFile( fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	m_file = ::open( TO_PLATFORMCODE(fileName), dwCreationDisposition, S_IRUSR | S_IWUSR );

	// If not created then create path and try again
	if ( c_invalidDescriptor == m_file )
	{
		// Copy to local storage
		Char filePath[ 4096 ];
		Red::System::StringCopy( filePath, fileName, ARRAY_COUNT( filePath ) );

		// Create directories
		Char* pos = filePath;
		while ( *pos )
		{
			// Create path
			if ( *pos == '\\' || *pos == '/' )
			{
				Char org = *pos;
				*pos = 0;
				if ( 0 != mkdir( TO_PLATFORMCODE(filePath), S_IRWXU ) )
				{
					// Unable to create final path
					return false;
				}
				*pos = org;
			}

			// Next char
			pos++;
		}

		m_file = ::open( TO_PLATFORMCODE(fileName), dwCreationDisposition, S_IRUSR | S_IWUSR );
	}
	Red::System::StringCopy(m_fileName, fileName, MAX_FILE_NAME);

	return operator Bool();
}

Bool CSystemFile::CreateReader(const Char* fileName)
{
	//m_file = ::CreateFile( fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	m_file = ::open( TO_PLATFORMCODE(fileName), O_RDONLY );

	Red::System::StringCopy( m_fileName, fileName, MAX_FILE_NAME );

	return operator Bool();
}

Bool CSystemFile::Close()
{
	if( m_file == c_invalidDescriptor )
	{
		return false;
	}

	const int ret = ::close(m_file);

	m_file = c_invalidDescriptor;

	return 0 == ret;
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

Uint64 CSystemFile::GetSize() const
{
	struct stat stats;
	if ( ::fstat( m_file, &stats ) != 0 )
	{
		return 0;
	}

	return stats.st_size >= 0 ? static_cast< Uint64 >( stats.st_size ) : 0;
}

size_t CSystemFile::Read( void* buf, size_t bytesToRead )
{
	if( m_lastPointer != (Uint64)m_currentPointer )
	{
		if ( ::lseek( m_file, m_currentPointer, SEEK_SET ) == -1 )
		{
			Int32 errorCode = errno;
			char errorBuffer[256];
			strerror_r( errorCode, errorBuffer, 256 );

			LOG_CORE( TXT("[IO]: Low level file pointer change to %I64u failed, error: %ls"), m_currentPointer, errorBuffer );
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	size_t nr = ::read( m_file, buf, bytesToRead );
	if( nr < 0 )
	{
		//Int32 errorCode = errno;
		//char errorBuffer[256];
		//strerror_r( errorCode, errorBuffer, 256 );
		//LOG_CORE( TXT("[IO]: Low level read failed for descriptor %d, error: %ls"), m_file, errorBuffer );
		return 0;
	}

	m_currentPointer += nr;
	m_lastPointer = m_currentPointer;

	return nr;
}

uintptr_t CSystemFile::Write( const void* buf, size_t bytesToWrite )
{
	if( m_lastPointer != (Uint64)m_currentPointer )
	{
		if ( ::lseek( m_file, m_currentPointer, SEEK_SET ) == -1 )
		{
			Int32 errorCode = errno;
			char errorBuffer[256];
			strerror_r( errorCode, errorBuffer, 256 );

			LOG_CORE( TXT("[IO]: Low level file pointer change to %I64u failed, error: %ls"), m_currentPointer, errorBuffer );
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	uintptr_t nw = ::write( m_file, buf, bytesToWrite );
	if( nw < 0 )
	{
		nw = 0;
	}

	m_currentPointer += nw;
	m_lastPointer = m_currentPointer;

	return nw;
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
	String normalizedFilePath = fileName;
	normalizedFilePath.ReplaceAll( TXT("\\"), TXT("/") );

	String path;
	String pattern;
	if ( !normalizedFilePath.Split( TXT("/"), &path, &pattern, true ) ) // SplitFromRight
	{
		// theres no pattern
		path = normalizedFilePath;
	}

	if ( pattern == TXT("*") || pattern == TXT("*.") )
	{
		// its searching for everything, so no need to pattern match anything
		pattern.Clear();
	}

	m_findFile.m_usePattern = !pattern.Empty();

	Red::System::StringCopy( m_findFile.m_path, TO_PLATFORMCODE(path.AsChar()), PATH_MAX );
	if ( m_findFile.m_usePattern )
	{
		Red::System::StringCopy( m_findFile.m_pattern, TO_PLATFORMCODE(pattern.AsChar()), PATH_MAX );
	}

	m_findFile.m_folder = ::opendir( m_findFile.m_path );
	if ( m_findFile.m_folder == nullptr )
	{
#if 0
		LogLastError( TXT("CSystemFindFile"), fileName );
#endif
		return;
	}

	// find first valid entry
	++( *this );
}

CSystemFindFile::~CSystemFindFile()
{
	if ( m_findFile.m_folder != nullptr )
	{
		::closedir( m_findFile.m_folder );
		m_findFile.m_folder = nullptr;
		m_findFile.m_currentEntry = nullptr;
	}
}

CSystemFindFile::operator Bool() const
{
	return m_findFile.m_folder != nullptr && m_findFile.m_currentEntry != nullptr;
}

const Char* CSystemFindFile::GetFileName()
{
	Red::System::StringCopy( m_findFile.m_currentEntryName, ANSI_TO_UNICODE(m_findFile.m_currentEntry->d_name), NAME_MAX );
	return m_findFile.m_currentEntryName;
}

const char* CSystemFindFile::GetAnsiFileName()
{
	return m_findFile.m_currentEntry->d_name;
}

Bool CSystemFindFile::IsDirectory() const
{
	// if theres no support for d_type use stat
	if ( m_findFile.m_currentEntry->d_type == DT_UNKNOWN )
	{
		// stat requires the full path
		StringAnsi fullPath( m_findFile.m_path );
		fullPath += m_findFile.m_currentEntry->d_name;

		struct stat stats;
		stat( fullPath.AsChar(), &stats );
		return S_ISDIR( stats.st_mode );
	}

	return (m_findFile.m_currentEntry->d_type == DT_DIR );
}

Uint32 CSystemFindFile::GetSize()
{
	// stat requires the full path
	StringAnsi fullPath( m_findFile.m_path );
	fullPath += GetAnsiFileName();

	struct stat stats;
	if ( ::stat( fullPath.AsChar(), &stats ) != 0 )
	{
		return 0;
	}

	return stats.st_size;
}

void CSystemFindFile::operator ++()
{
	m_findFile.m_currentEntry = ::readdir( m_findFile.m_folder );

	// if theres a pattern, read entries until the first match appears
	if ( m_findFile.m_usePattern )
	{
		while ( Bool( *this ) )
		{
			const char* currentFileName = GetAnsiFileName();
			Int32 status = fnmatch( m_findFile.m_pattern, currentFileName, 0 );
			if ( status == 0 )
			{
				return;
			}

			m_findFile.m_currentEntry = ::readdir( m_findFile.m_folder );
		}
	}
}

#endif // RED_PLATFORM_LINUX
