/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "fileHandleCache.h"
#include "fileSystemProfilerWrapper.h"

Red::Threads::CMutex CNativeFileHandleWrapper::st_lock;
Uint32 CNativeFileHandleWrapper::st_counter = 0; // for LRU
CNativeFileHandleWrapper::TOpenedHandles CNativeFileHandleWrapper::st_openedHandles;

//----

CNativeFileHandleWrapper::VirtualHandle::VirtualHandle( const String& path, const Uint32 thread )
	: m_absolutePath( path )
	, m_owningThread( thread )
	, m_lastUsed( 0 )
	, m_readerCount( 0 )
	, m_refCount( 1 )
	, m_invalid( false )
{
}

CNativeFileHandleWrapper::VirtualHandle::~VirtualHandle()
{
	RED_FATAL_ASSERT( m_readerCount == 0, "Trying to close file '%ls' that is still being read", m_absolutePath.AsChar() );

	// close physical access point when destroying the virtual handle
	if ( m_handle )
	{
		delete m_handle;
		m_handle = nullptr;

		// remove from the list of active handles
		{
			Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );
			st_openedHandles.Remove( this );
		}
	}
}

void CNativeFileHandleWrapper::VirtualHandle::RecycleForThread_NoLock( const Uint32 thread )
{
	RED_FATAL_ASSERT( !HasReaders(), "Can't recycle virtual handle while it has readers" );

	m_owningThread = thread;
}

void CNativeFileHandleWrapper::VirtualHandle::AddRef()
{
	m_refCount.Increment();
}

void CNativeFileHandleWrapper::VirtualHandle::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

CNativeFileHandleWrapper::PhysicalHandle* CNativeFileHandleWrapper::VirtualHandle::Open()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	// is it opened ? if so, use it
	if ( m_handle )
	{
		st_counter += 1;
		m_lastUsed = st_counter; // update LRU
		m_readerCount += 1; // update active readers count
		return m_handle;
	}

	// file is not valid
	if ( m_invalid )
	{
		return nullptr;
	}

	// not yet opened, open new access point, first check if there's space left
	if ( st_openedHandles.Full() )
	{
		// we have to many opened file handles - try to free one that was used last
		VirtualHandle* bestHandle = nullptr;
		Uint32 bestLastUsed = st_counter;
		for ( Uint32 i=0; i<st_openedHandles.Size(); ++i )
		{
			// we can only release handles that have NO READERS
			VirtualHandle* handle = st_openedHandles[i];
			if ( handle->m_readerCount == 0 )
			{
				// better ?
				if ( handle->m_lastUsed < bestLastUsed )
				{
					bestHandle = handle;
					bestLastUsed = handle->m_lastUsed;
				}
			}

			// no handles to release found - probably we leaked the reader somewhere
			if ( !bestHandle )
			{
				RED_HALT( "To many active synchronous readers. Unable to create new one." );
				return nullptr;
			}

			// close the physical file handle
			RED_FATAL_ASSERT( bestHandle->m_readerCount == 0, "Trying to close handle that is still in use" );
			delete bestHandle->m_handle;
			bestHandle->m_handle = nullptr;

			// remove from the list of opened handles
			st_openedHandles.Remove( bestHandle );
			st_counter += 1;
		}
	}

	// create new physical handle - may fail
	RED_FATAL_ASSERT( m_handle == nullptr, "Trying to reopen already opened handle to '%ls'", m_absolutePath.AsChar() );
	m_handle = new PhysicalHandle;
	if ( !m_handle->Open( m_absolutePath.AsChar(), Red::IO::eOpenFlag_Read ) )
	{
		ERR_CORE( TXT("IO error: failed to open physcail file '%ls'"), m_absolutePath.AsChar() );
		m_invalid = true;
		return nullptr;
	}

	// we've successfully opened a physical handle, add ourselves to the list of opened handles
	RED_FATAL_ASSERT( !st_openedHandles.Full(), "No place for new handle" );
	st_openedHandles.PushBack( this );

	// return physical handle for reading
	st_counter += 1;
	m_lastUsed = st_counter; // update LRU
	m_readerCount += 1; // update active readers count
	return m_handle;
}

void CNativeFileHandleWrapper::VirtualHandle::Close()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	RED_FATAL_ASSERT( m_readerCount > 0, "File handle '%ls' close more times it was opened", m_absolutePath.AsChar() );
	m_readerCount -= 1;
}

//----

CNativeFileHandleWrapper::CNativeFileHandleWrapper( const String& absolutePath )
	: m_absolutePath( absolutePath )
{
}

CNativeFileHandleWrapper::~CNativeFileHandleWrapper()
{
	for ( auto it = m_localHandles.Begin(); it != m_localHandles.End(); ++it )
	{
		VirtualHandle* handle = *it;
		handle->Release();
	}
}

class CNativeFileReader* CNativeFileHandleWrapper::CreateReader( Uint32 userThreadID /*= 0*/ )
{	
	// Swap thread ID if not specified
	if ( !userThreadID )
		userThreadID = Red::System::Internal::ThreadId::CurrentThread().AsNumber();

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_lock );

	// find the matching virtual handle for given thread
	VirtualHandle* virtualHandle = nullptr;
	for ( Uint32 i=0; i<m_localHandles.Size(); ++i )
	{
		if ( m_localHandles[i]->GetOwningThread() == userThreadID )
		{
			virtualHandle = m_localHandles[i];
			break;
		}
	}

	// not found, add new one or recycle unused
	if ( !virtualHandle )
	{
		// Too many different threads
		if ( m_localHandles.Full() )
		{
			// Try to recycle an unused handle
			for ( Uint32 i = 0; i<m_localHandles.Size(); ++i )
			{
				if ( !m_localHandles[i]->HasReaders() )
				{
					m_localHandles[i]->RecycleForThread_NoLock( userThreadID );
					virtualHandle = m_localHandles[i];
					break;
				}
			}
		}
		else
		{
			virtualHandle = new VirtualHandle( m_absolutePath, userThreadID );
			m_localHandles.PushBack( virtualHandle );
		}
	}

	// still can't get a handle
	if ( !virtualHandle )
	{
		ERR_CORE( TXT("Too many different threads try to read file '%ls'. Access denied."), m_absolutePath.AsChar() );
		return nullptr;
	}

	// request physical handle
	PhysicalHandle* physicalHandle = virtualHandle->Open();
	if ( !physicalHandle )
		return nullptr;

	// create wrapper
	return new CNativeFileReader( virtualHandle, physicalHandle );
}

//---

CNativeFileReader::~CNativeFileReader()
{
	// release the virtual handle access
	m_virtualHandle->Close();
	m_virtualHandle->Release();

	// cleanup
	m_owningThread = 0;
	m_physicalHandle = nullptr;
	m_virtualHandle = nullptr;
}

const Bool CNativeFileReader::Read( void* dest, const Uint32 length, /*[out]*/ Uint32& outNumberOfBytesRead )
{
	return m_physicalHandle->Read( dest, length, outNumberOfBytesRead );
}

const Bool CNativeFileReader::Seek( const Uint32 offset, Red::IO::ESeekOrigin seekOrigin )
{
	return m_physicalHandle->Seek( offset, seekOrigin );
}

const Uint32 CNativeFileReader::Tell() const
{
	return (Uint32) m_physicalHandle->Tell();
}

CNativeFileReader::CNativeFileReader( CNativeFileHandleWrapper::VirtualHandle* virtualHandle, CNativeFileHandleWrapper::PhysicalHandle* physicalHandle )
	: m_virtualHandle( virtualHandle )
	, m_physicalHandle( physicalHandle )
	, m_owningThread( virtualHandle->GetOwningThread() )
{
	m_virtualHandle->AddRef();
}

//---