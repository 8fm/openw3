/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "redIOFile.h"
#include "redIOAsyncReadToken.h"

// shitty reference to Core (for now - the redIO is always used with Core)
using namespace Red;
#include "../redThreads/redThreadsAtomic.h"
#include "../core/fileSystemProfilerWrapper.h"

REDIO_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////
// CNativeFileHandle
//////////////////////////////////////////////////////////////////////////
CNativeFileHandle::CNativeFileHandle()
	: m_async( false )
{
}

CNativeFileHandle::~CNativeFileHandle()
{
	Close();
}

Bool CNativeFileHandle::Open( const Char* path, Uint32 openFlags )
{
	m_async = (openFlags & eOpenFlag_Async) != 0;
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_async )
		RedIOProfiler::ProfileAsyncIOOpenFileStart( path );
	else
		RedIOProfiler::ProfileSyncIOOpenFileStart( path );
#endif

	const Bool ret = m_file.Open( path, openFlags );

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_async )
		RedIOProfiler::ProfileAsyncIOOpenFileEnd( m_file.GetFileHandle() );
	else
		RedIOProfiler::ProfileSyncIOOpenFileEnd( m_file.GetFileHandle() );
#endif
	return ret;
}

Bool CNativeFileHandle::Close()
{
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_async )
		RedIOProfiler::ProfileAsyncIOCloseFileStart( m_file.GetFileHandle() );
	else
		RedIOProfiler::ProfileSyncIOCloseFileStart( m_file.GetFileHandle() );
#endif

	const Bool ret = m_file.Close();

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_async )
		RedIOProfiler::ProfileAsyncIOCloseFileEnd();
	else
		RedIOProfiler::ProfileSyncIOCloseFileEnd();
#endif
	return ret;
}

Bool CNativeFileHandle::Read( void* dest, Uint32 length, Uint32& /*[out]*/ outNumberOfBytesRead )
{
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( !m_async )
		RedIOProfiler::ProfileSyncIOReadStart( m_file.GetFileHandle(), length );
#endif

	Bool ret = m_file.Read( dest, length, outNumberOfBytesRead );

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( !m_async )
		RedIOProfiler::ProfileSyncIOReadEnd();
#endif

	return ret;
}

Bool CNativeFileHandle::Write( const void* src, Uint32 length, Uint32& /*[out]*/ outNumberOfBytesWritten )
{
	return m_file.Write( src, length, outNumberOfBytesWritten );
}

Bool CNativeFileHandle::Seek( Int64 offset, ESeekOrigin seekOrigin )
{
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( !m_async && seekOrigin == eSeekOrigin_Set )
	{
		RedIOProfiler::ProfileSyncIOSeekStart( m_file.GetFileHandle(), (Uint64)offset );
	}
#endif
	const Bool ret = m_file.Seek( offset, seekOrigin );

#ifdef RED_PROFILE_FILE_SYSTEM
	if ( !m_async && seekOrigin == eSeekOrigin_Set )
	{
		RedIOProfiler::ProfileSyncIOSeekEnd();
	}
#endif

	return ret;
}

Int64 CNativeFileHandle::Tell() const
{
	return m_file.Tell();
}

Bool CNativeFileHandle::IsValid() const
{
	return m_file.IsValid();
}

Bool CNativeFileHandle::Flush()
{
	return m_file.Flush();
}

Uint64 CNativeFileHandle::GetFileSize() const
{
	return m_file.GetFileSize();
}

Uint32 CNativeFileHandle::GetFileHandle() const
{
	return m_file.GetFileHandle();
}

REDIO_NAMESPACE_END