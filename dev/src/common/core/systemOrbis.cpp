/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifdef RED_PLATFORM_ORBIS

#include "string.h"
#include "filePath.h"

#include <time.h>
#include <rtc.h>

#pragma comment ( lib, "libSceRtc_stub_weak.a" )

#define INVALID_FD_VALUE (-1)

/************************************************************************/
/* Globals																*/
/************************************************************************/
CSystemIO		GSystemIO;

/************************************************************************/
/* System I/O implementation                                            */
/************************************************************************/
static String Stupid8DirLimit( const Char* fileName )
{
	String stupid = fileName;

	// UrlBuilder will send \app0\, so will other things
	stupid.ReplaceAll(TXT("\\"),TXT("/"));

	return stupid;
}

Bool CSystemIO::CopyFile(const Char* existingFileName, const Char* newFileName, Bool failIfExists) const
{
	RED_UNUSED( existingFileName );
	RED_UNUSED( newFileName );
	RED_UNUSED( failIfExists );
	return false;
}

Bool CSystemIO::CreateDirectory(const Char* pathName) const
{
	//FIXME: Doesn't create intermediate directories. This is just using the "old" code before CL# 219192 changed it.
	// Also, confirm what 'group' and 'other' modes are really needed on Orbis
	return ::sceKernelMkdir( TO_PLATFORMCODE( Stupid8DirLimit(pathName).AsChar() ), SCE_KERNEL_S_IRWXU ) == SCE_OK;
}

Bool CSystemIO::DeleteFile(const Char* fileName) const
{
	return ::sceKernelUnlink( TO_PLATFORMCODE( Stupid8DirLimit(fileName).AsChar() ) ) == SCE_OK;
}

Bool CSystemIO::IsFileReadOnly(const Char* fileName) const
{
	// files on cooked platforms are always read only
	RED_UNUSED( fileName );
	return true;
}

Bool CSystemIO::FileExist(const Char* fileName) const
{
	WARN_CORE(TXT("Someone tried to check if file exists, probably this is causing a bug now: %s"), fileName);
	return false;
}

Bool CSystemIO::SetFileReadOnly(const Char* fileName, Bool readOnlyFlag )
{
	return ::sceKernelChmod( TO_PLATFORMCODE( Stupid8DirLimit(fileName).AsChar() ), readOnlyFlag ? SCE_KERNEL_S_IRU : SCE_KERNEL_S_IRWU ) == SCE_OK;
}

Bool CSystemIO::MoveFile(const Char* existingFileName, const Char* newFileName) const
{
	return ::sceKernelRename( TO_PLATFORMCODE( Stupid8DirLimit(existingFileName).AsChar() ), TO_PLATFORMCODE( Stupid8DirLimit(newFileName).AsChar() ) )== SCE_OK;
}

Bool CSystemIO::RemoveDirectory(const Char* pathName) const
{
	return ::sceKernelRmdir( TO_PLATFORMCODE( Stupid8DirLimit(pathName).AsChar() ) ) == SCE_OK;
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
				return false;
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

	// 	SceKernelTimespec on Orbis:
	// 	This structure represents the time. The time elapsed from January 1 1970 12:00 a.m. will be stored separately in the number of seconds and the fraction less than one second.

	SceKernelStat stat;
	if ( ::sceKernelStat( TO_PLATFORMCODE( Stupid8DirLimit(pathName).AsChar() ), &stat ) == SCE_OK )
	{
		// Convert to format that can easily be passed through to 
		SceRtcDateTime orbisFileTime;
		if( ::sceRtcSetTime_t( &orbisFileTime, stat.st_mtim.tv_sec ) == SCE_OK )
		{
			fileTime.SetYear			( static_cast< Uint32 >( orbisFileTime.year ) );
			fileTime.SetMonth			( static_cast< Uint32 >( orbisFileTime.month ) - 1 );
			fileTime.SetDay				( static_cast< Uint32 >( orbisFileTime.day ) - 1 );
			fileTime.SetHour			( static_cast< Uint32 >( orbisFileTime.hour ) );
			fileTime.SetMinute			( static_cast< Uint32 >( orbisFileTime.minute ) );
			fileTime.SetSecond			( static_cast< Uint32 >( orbisFileTime.second ) );
			fileTime.SetMilliSeconds	( static_cast< Uint32 >( stat.st_mtim.tv_nsec ) / 1000000 );
		}
	}

	return fileTime;
}

Uint64 CSystemIO::GetFileSize( const Char* pathName ) const
{
	SceKernelStat stat;
	if ( ::sceKernelStat( TO_PLATFORMCODE(pathName), &stat ) != SCE_OK )
	{
		return 0;
	}

	return stat.st_size >= 0 ? static_cast< Uint64 >( stat.st_size ) : 0;
}

/************************************************************************/
/* System File implementation                                           */
/************************************************************************/

CSystemFile* CSystemFile::m_first = 0;

CSystemFile::CTime::CTime()
{
	//FIXME>>>>
#if 0
	m_fileTime.dwHighDateTime = 0;
	m_fileTime.dwLowDateTime = 0;
#endif
}

CSystemFile::CTime::CTime(Uint64 time)
{
	//FIXME>>>>
	RED_UNUSED( time );
#ifndef RED_PLATFORM_ORBIS
	m_fileTime.dwHighDateTime = (DWORD)(time >> 32);
	m_fileTime.dwLowDateTime = (DWORD)(time & 0xffffffff);
#endif
}

String CSystemFile::CTime::ToString()
{
	//FIXME>>>>
#ifndef RED_PLATFORM_ORBIS
	SYSTEMTIME st;

	FileTimeToLocalFileTime( &m_fileTime, &m_fileTime );
	FileTimeToSystemTime( &m_fileTime, &st );

	// Build a string showing the date and time.
	return String::Printf(TXT("%02d/%02d/%d  %02d:%02d"),
		st.wMonth, st.wDay, st.wYear,
		st.wHour, st.wMinute);
#else
	return TXT( "TODO" );
#endif
}

void CSystemFile::CTime::Extract( Int32& year, Int32& month, Int32& day, Int32& hour, Int32& minute, Int32& second )
{
	RED_UNUSED( year );
	RED_UNUSED( month );
	RED_UNUSED( day );
	RED_UNUSED( hour );
	RED_UNUSED( minute );
	RED_UNUSED( second );
#if 0
	RED_MESSAGE("FIXME>>>>>>>>>>>>>>>>>>>>>>>")
	// No sce wrapper? Except for dealing with the rtc
	// Windows impl is broken, using m_fileTime for both args...?!
	struct tm localFileTime;
	RED_VERIFY( ::localtime_s( &m_fileTime.tv_sec, &localFileTime ) != nullptr );

	// Make the same as Windows
	year = tm.tm_year + 1900;
	month = tm.tm_mon + 1;
	day = tm.tm_wday + 0;
	hour = tm.tm_hour + 0;
	minute = tm.tm_min + 0;
	second = tm.tm_sec > 59 ? 59 : tm.tm_sec; // think we can live without the possibility of leap seconds on Orbis, which is better than returning 0-61 and potentially crash by going out of some bounds
#endif
}

Uint64 CSystemFile::CTime::ToUint64() const
{
#if 0
	return (( Uint64 ) m_fileTime.dwHighDateTime << 32) | ( m_fileTime.dwLowDateTime );
#endif
	return 0;
}

Bool CSystemFile::CTime::operator >(const CSystemFile::CTime& time) const
{
	RED_UNUSED( time );
	// FIXME>>>
#ifndef RED_PLATFORM_ORBIS
	return 1 == ::CompareFileTime(&m_fileTime, &time.m_fileTime);
#endif
	return false;
}

CSystemFile::CSystemFile()
	: m_file(INVALID_FD_VALUE )
	, m_next( 0 )
	, m_lastPointer( 0 )
	, m_currentPointer( 0 )
{
	m_fileName[0] = TXT('\0');
}

CSystemFile::CSystemFile( const CSystemFile& other )
	: m_file( other.m_file )
	, m_next( other.m_next )
	, m_lastPointer( 0 )
	, m_currentPointer( 0 )
{
	Red::System::StringCopy( m_fileName, other.m_fileName, MAX_FILE_NAME);
	if( m_file == INVALID_FD_VALUE )
	{
		return;
	}
}

CSystemFile::~CSystemFile()
{
}

CSystemFile::operator Bool() const
{
	return m_file > INVALID_FD_VALUE;
}

Bool CSystemFile::CreateWriter( const Char* fileName, Bool append )
{
	// Check for GFileManager because this can actually be called in the CFileManager ctor from the profiler!
	if ( GFileManager && GFileManager->IsReadOnly() )
	{
		const size_t fileLen = Red::System::StringLength( fileName );
		const size_t appLen = Red::System::StringLengthCompileTime(TXT("/app0"));
		if ( fileLen >= appLen && Red::System::StringCompareNoCase( fileName, TXT("/app0"), appLen ) == 0 )
		{
			// The devkit will actually shut down
			RED_LOG_ERROR( PlayGo, TXT("Can't create a writer to readonly /app0 {File '%ls'} with PlayGo enabled!"), fileName );
			return false;
		}
	}
	
	// Don't use SCE_KERNEL_O_APPEND since that will always move the file cursor to the end before writing
	// but that's not how the systemWin32 only sets it to the end once when opening the file.
	const SceInt32 flags = SCE_KERNEL_O_RDWR | SCE_KERNEL_O_CREAT | ( append ? 0 : SCE_KERNEL_O_TRUNC );

	RED_MESSAGE( "FIXME: Leak handles like Windows or close these ones..." )
	RED_ASSERT( m_file <= INVALID_FD_VALUE, TXT("File reader/writer already created. Can close current fd") );
	m_file = ::sceKernelOpen( TO_PLATFORMCODE( Stupid8DirLimit(fileName).AsChar() ), flags, SCE_KERNEL_S_IRWU );
	if ( m_file < 0 )
	{
		m_file = INVALID_FD_VALUE;
		return false;
	}

	if ( append )
	{
		m_currentPointer = GetSize();
	}

	Red::System::StringCopy(m_fileName, fileName, MAX_FILE_NAME);

	//CSystem::SystemFileAccessUnlock();

	return operator Bool();
}

Bool CSystemFile::CreateReader(const Char* fileName)
{
	//CSystem::SystemFileAccessLock();
	//FIXME: bother with file locks?

	RED_MESSAGE( "FIXME: Leak handles like Windows or close these ones..." )
	RED_ASSERT( m_file <= INVALID_FD_VALUE, TXT("File reader/writer already created. Can close current fd") );
	m_file =  ::sceKernelOpen( TO_PLATFORMCODE( Stupid8DirLimit(fileName).AsChar() ), SCE_KERNEL_O_RDONLY, 0 );

#ifndef RED_FINAL_BUILD
	if ( m_file == SCE_KERNEL_ERROR_ENOBLK )
	{
		RED_LOG_ERROR( PlayGo, TXT("Failed to open file '%ls' for reading since it's on the network locus!"), fileName );
	}
	else if ( m_file == SCE_KERNEL_ERROR_ENOPLAYGOENT )
	{
		RED_LOG_ERROR( PlayGo, TXT("Failed to open file '%ls' for reading since it's not in the PlayGo definition file!"), fileName );
	}
#endif

	Red::System::StringCopy(m_fileName, fileName, MAX_FILE_NAME );

	//CSystem::SystemFileAccessUnlock();

	return operator Bool();
}

Bool CSystemFile::Close()
{
	if( m_file == INVALID_FD_VALUE ) 
	{
		return false;
	}

	//CSystem::SystemFileAccessLock();

	const SceInt32 sceErr = ::sceKernelClose( m_file );

	//CSystem::SystemFileAccessUnlock();

	m_file = INVALID_FD_VALUE;

	return sceErr == SCE_OK;
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
	return ::sceKernelFsync( m_file ) == SCE_OK;
}

Uint64 CSystemFile::GetSize() const
{
	SceKernelStat stat;
	if ( ::sceKernelFstat( m_file, &stat ) == SCE_OK )
	{
		RED_ASSERT( stat.st_size >= 0 );
		return stat.st_size;
	}

	return 0;
}

CSystemFile::CTime CSystemFile::GetCreationTime() const
{


	CTime out;

#ifndef RED_PLATFORM_ORBIS
	::GetFileTime(m_file, &out.m_fileTime, NULL, NULL);
#endif

	return out;
}

CSystemFile::CTime CSystemFile::GetLastAccessTime() const
{
	CTime out;

#ifndef RED_PLATFORM_ORBIS
	::GetFileTime(m_file, NULL, &out.m_fileTime, NULL);
#endif

	return out;
}

CSystemFile::CTime CSystemFile::GetLastWriteTime() const
{
	CTime out;

#ifndef RED_PLATFORM_ORBIS
	::GetFileTime(m_file, NULL, NULL, &out.m_fileTime);
#endif

	return out;
}

size_t CSystemFile::Read( void* buf, size_t bytesToRead )
{
	//CSystem::SystemFileAccessLock();

	if( m_lastPointer != (uintptr_t)m_currentPointer )
	{
		if ( ::sceKernelLseek( m_file, m_currentPointer, SCE_KERNEL_SEEK_SET ) < 0 )
		{
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	ssize_t nr = ::sceKernelRead( m_file, buf, bytesToRead );
	if ( nr < 0 )
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
	//CSystem::SystemFileAccessLock();

	if( m_lastPointer != (uintptr_t)m_currentPointer )
	{
		if ( ::sceKernelLseek( m_file, m_currentPointer, SCE_KERNEL_SEEK_SET ) < 0 )
		{
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	ssize_t nw = ::sceKernelWrite( m_file, buf, bytesToWrite );
	if ( nw < 0 )
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
		if ( ::sceKernelLseek( m_file, m_currentPointer, SCE_KERNEL_SEEK_SET ) < 0 )
		{
			return 0;
		}
		m_lastPointer = m_currentPointer;
	}

	const off_t length = m_currentPointer + 1;
	const SceInt32 sceErr = ::sceKernelFtruncate( m_file, length );

	//CSystem::SystemFileAccessUnlock();

	return sceErr == SCE_OK;
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
class COrbisDirWalker : Red::System::NonCopyable
{
private:
	static const String DIR_SEPARATOR;

private:
	String		m_wildcard;

private:
	size_t		m_blockReadSize;
	SceChar8*	m_buf;
	SceChar8*	m_curBuf;
	SceChar8*	m_endBuf;
	SceInt32	m_fd;

private:
	String				m_curBasePath;	
#if 0
	TQueue< String >	m_direntQueue;
#endif
	String				m_curDirentPath;

public:
	COrbisDirWalker( const String& wildcard );
	~COrbisDirWalker();
	Bool OpenDir( const String& path );
	Bool IsValid() const;
	Bool NextDirent();
	SceKernelDirent* CurDirent();
	void CloseDir();

public:
	const String& GetWildcard() const { return m_wildcard; }

public:
	const Char* GetCurDirentPath() const;

private:
	Bool FetchDirents();
	Bool FillBuf();
	void UpdateCurrentDirent();
	void Cleanup();
	void Reset();
};

CSystemFindFile::CSystemFindFile(const Char* fileName)
{
	String normalizedBasePath;
	String wildcard;

	String normalizedFilePath = fileName;

	// Stupid but CFilePath path treats /hostapp/ as "rootdir hostapp/"
	const Bool replacedHostApp = normalizedFilePath.Replace(TXT("/hostapp/"), TXT("") );

	normalizedFilePath.ReplaceAll(TXT("\\"), TXT("/") );
	if ( ! normalizedFilePath.Split(TXT("*"), &normalizedBasePath, &wildcard, true ) )
	{
		normalizedBasePath = normalizedFilePath;
	}
	else if ( wildcard == TXT("*") || wildcard == TXT(".*") )
	{
		wildcard.Clear();
	}

	CFilePath filePath( normalizedBasePath );

	//TODO Exact match... so foo.* etc or foo.txt
//	const String& fileWithExt = filePath.GetFileNameWithExt();

	const String& basePath = replacedHostApp ? TXT("/hostapp/" ) + filePath.GetPathString( TXT("/") ) : filePath.GetPathString();

	// HACK: not much choice
	m_findFile = new COrbisDirWalker( wildcard );

	m_findFile->OpenDir( basePath );

	if ( wildcard.Empty() )
	{
		return;
	}

	//size_t wildcardStrLen = wildcard.Size() - 1; // minus one for NULL
	while ( m_findFile->IsValid() && m_findFile->CurDirent()->d_type == SCE_KERNEL_DT_REG )
	{
// 		const Char* curDirentPath = m_findFile->GetCurDirentPath();
// 		Char* pch = Red::System::StringSearchLast( curDirentPath, wildcard.AsChar() );
// 		if ( *(pch + wildcardStrLen + 1 ) == '\0' ) // matches end of string
// 		{
// 			break;
// 		}

		String curDirentPath = m_findFile->GetCurDirentPath();
		if ( curDirentPath.EndsWith(wildcard) )
		{
			break;
		}

		m_findFile->NextDirent();
	}
}

CSystemFindFile::~CSystemFindFile()
{
	delete m_findFile;
}

CSystemFindFile::operator Bool() const
{
	return m_findFile->IsValid();
}

const Char* CSystemFindFile::GetFileName()
{
	return m_findFile->GetCurDirentPath();
}

Bool CSystemFindFile::IsDirectory() const
{
	const SceKernelDirent* dirent = m_findFile->CurDirent();
	return dirent ? m_findFile->CurDirent()->d_type == SCE_KERNEL_DT_DIR : false;
}

Uint32 CSystemFindFile::GetSize()
{
	return 0;
}

void CSystemFindFile::operator ++()
{
	m_findFile->NextDirent();

	const String& wildcard = m_findFile->GetWildcard();
	if ( wildcard.Empty() )
	{
		return;
	}

//	size_t wildcardStrLen = wildcard.Size() - 1; // minus one for NULL
	while ( m_findFile->IsValid() && m_findFile->CurDirent()->d_type == SCE_KERNEL_DT_REG )
	{
// 		const Char* curDirentPath = m_findFile->GetCurDirentPath();
// 		Char* pch = Red::System::StringSearchLast( curDirentPath, wildcard.AsChar() );
// 		if ( *(pch + wildcardStrLen + 1 ) == '\0' ) // matches end of string
// 		{
// 			break;
// 		}
		
		String curDirentPath = m_findFile->GetCurDirentPath();
		if ( curDirentPath.EndsWith(wildcard) )
		{
			break;
		}

		m_findFile->NextDirent();
	}
}

const String COrbisDirWalker::DIR_SEPARATOR = TXT("/");

COrbisDirWalker::COrbisDirWalker( const String& wildcard )
	: m_wildcard( wildcard )
	, m_blockReadSize( 0 )
	, m_buf( nullptr )
	, m_curBuf( nullptr )
	, m_endBuf( nullptr )
	, m_fd( INVALID_FD_VALUE )
{
}

Bool COrbisDirWalker::OpenDir( const String& path )
{
	String normalizedBasePath = path;

	normalizedBasePath.ReplaceAll(TXT("\\"), TXT("/"));

	if ( normalizedBasePath.EndsWith(TXT("/")) )
	{
		normalizedBasePath.RemoveAtFast(normalizedBasePath.Size()-1);
	}

	CloseDir();

	const String stupid = Stupid8DirLimit( normalizedBasePath.AsChar() );
	m_fd = ::sceKernelOpen( TO_PLATFORMCODE( stupid.AsChar() ), SCE_KERNEL_O_DIRECTORY | SCE_KERNEL_O_RDONLY, 0 );
	if ( m_fd <= INVALID_FD_VALUE )
	{
		m_fd = INVALID_FD_VALUE;

#ifndef RED_FINAL_BUILD
		if ( m_fd == SCE_KERNEL_ERROR_ENOBLK )
		{
			RED_LOG_ERROR( PlayGo, TXT("Failed to open directory '%ls' since it's on the network locus!"), path.AsChar() );
		}
		else if ( m_fd == SCE_KERNEL_ERROR_ENOPLAYGOENT )
		{
			RED_LOG_ERROR( PlayGo, TXT("Failed to open directory '%ls' since it's not in the PlayGo definition file!"), path.AsChar() );
		}
#endif

		return false;
	}

	SceKernelStat dirStat;
	const SceInt32 err = ::sceKernelFstat( m_fd, &dirStat );
	if ( err < SCE_OK )
	{
		RED_HALT( "sceKernelFstat failed: 0x%08x", err );
		Reset();
		return false;
	}

	m_blockReadSize = dirStat.st_blksize;
	m_buf = static_cast< SceChar8* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Engine, m_blockReadSize ) );
	RED_ASSERT( m_buf );
	if ( ! m_buf )
	{
		Reset();
		return false;
	}

	m_curBasePath = normalizedBasePath;

	if ( FillBuf() )
	{
		UpdateCurrentDirent();
		return true;
	}

	return false;
}

COrbisDirWalker::~COrbisDirWalker()
{
	Reset();
}

Bool COrbisDirWalker::IsValid() const
{ 
	return m_curBuf != nullptr;
}
	
Bool COrbisDirWalker::NextDirent()
{
	// Go to the next entry or clear buffer
	if ( m_curBuf )
	{
		const SceKernelDirent* dirent = reinterpret_cast< SceKernelDirent* >( m_curBuf );
		if ( m_curBuf + dirent->d_reclen >= m_endBuf )
		{
			m_curBuf = nullptr;
		}
		else
		{
			m_curBuf += dirent->d_reclen;
		}
	}

	const Bool hasNext = m_curBuf || FetchDirents();
	UpdateCurrentDirent();

	return hasNext;
}

SceKernelDirent* COrbisDirWalker::CurDirent()
{
	if ( ! m_curBuf )
	{
		return nullptr;
	}

	SceKernelDirent* dirent = reinterpret_cast< SceKernelDirent* >( m_curBuf );
	return dirent;
}

void COrbisDirWalker::CloseDir()
{
	m_blockReadSize = 0;

	if ( m_fd > INVALID_FD_VALUE )
	{
		::sceKernelClose( m_fd );
		m_fd = INVALID_FD_VALUE;
	}

	if ( m_buf )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_Engine, m_buf );
	}

	m_buf = m_curBuf = m_endBuf = nullptr;

	UpdateCurrentDirent();
}

Bool COrbisDirWalker::FetchDirents()
{
	if ( m_fd <= INVALID_FD_VALUE )
	{
		return false;
	}

	if ( FillBuf() )
	{
		return true;
	}

#if 0
	// OpenDir calls FillBuf()
	if ( ! m_direntQueue.Empty() )
	{
		String path = m_direntQueue.Front();
		m_direntQueue.Pop();
		return OpenDir( path );
	}
#endif

	return false;
}

Bool COrbisDirWalker::FillBuf()
{
	m_curBuf = m_endBuf = nullptr;

	SceInt32 ret = ::sceKernelGetdents( m_fd, m_buf, m_blockReadSize );
	if ( ret < SCE_OK )
	{
		RED_HALT( "sceKernelGetdents failed: 0x%08x", ret );
		return false;
	}

	if ( ret == 0 )
	{
		// End of directory
		return false;
	}

	const SceInt32 numRead = ret;

	m_curBuf = m_buf;
	m_endBuf = m_buf + numRead;

	return true;
}

void COrbisDirWalker::UpdateCurrentDirent()
{
	if ( ! m_curBuf )
	{
		m_curDirentPath.Clear();
		return;
	}

	SceKernelDirent* dirent = reinterpret_cast< SceKernelDirent* >( m_curBuf );
	
	m_curDirentPath = ANSI_TO_UNICODE(dirent->d_name);//String::Printf(TXT("%ls%ls%ls"), m_curBasePath.AsChar(), DIR_SEPARATOR.AsChar(), ANSI_TO_UNICODE(dirent->d_name) );

#if 0
	if ( dirent->d_type == SCE_KERNEL_DT_DIR )
	{
		String dirPath = m_curBasePath + DIR_SEPARATOR + ANSI_TO_UNICODE(dirent->d_name);
		m_direntQueue.Push( dirPath );
	}
#endif
}

void COrbisDirWalker::Reset()
{
	CloseDir();

	m_curBasePath.ClearFast();
#if 0
	m_direntQueue.Clear();
#endif
}

const Char* COrbisDirWalker::GetCurDirentPath() const
{
	return m_curDirentPath.AsChar();
}

#endif // RED_PLATFORM_ORBIS
