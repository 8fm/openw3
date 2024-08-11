#pragma once

#include "sortedmap.h"
#include "staticarray.h"

#include "../redIO/redIO.h"

// NOTE: SYNCHRONOUS READING ONLY, for Asynchrnous file stuff look into the AsyncIO.

/// A wrapper for native file handle for SYNCHRONOUS READING
// TODO: move to RedIO after David's changes are in
class CNativeFileHandleWrapper : public Red::NonCopyable
{
public:
	CNativeFileHandleWrapper( const String& absolutePath );
	~CNativeFileHandleWrapper();

	// get absolute file path
	RED_INLINE const String& GetAbsolutePath() const { return m_absolutePath; }

	// request an actual, physical access point for given thread, if thread ID is not specified current thread ID is used
	// you can release the reader once you're done with it
	class CNativeFileReader* CreateReader( Uint32 userThreadID = 0 );

private:
	String					m_absolutePath;

	typedef Red::IO::CNativeFileHandle		PhysicalHandle;

	struct VirtualHandle
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

	public:
		VirtualHandle( const String& path, const Uint32 thread );
		~VirtualHandle();

		RED_INLINE const String GetAbsolutePath() const { return m_absolutePath; }
		RED_INLINE const Uint32 GetOwningThread() const { return m_owningThread; }
		RED_INLINE const Uint32 GetLRUMarker() const { return m_lastUsed; }
		RED_INLINE const Uint32 GetReaderCount() const { return m_readerCount; }
		RED_INLINE const Bool IsInvalid() const { return m_invalid; }
		RED_INLINE const Bool HasReaders() const { return m_readerCount > 0; }

		void AddRef();
		void Release();

		PhysicalHandle* Open();
		void Close();

	public:
		void RecycleForThread_NoLock( const Uint32 thread );

	private:
		String								m_absolutePath;		// file path
		Uint32								m_owningThread;		// only thread that is allowed to access this handle
		PhysicalHandle*						m_handle;			// opened handle
		Uint32								m_lastUsed;			// when was it last used
		Uint32								m_readerCount;		// valid readers
		Red::Threads::CAtomic< Uint32 >		m_refCount;			// valid instances
		Bool								m_invalid;			// this handle is invalid (file does not exist, etc)
	};

	// global access lock
	static Red::Threads::CMutex			st_lock;

	// global opened handles
	static const Uint32 MAX_OPENED_HANDLES		= 64;
	typedef TStaticArray< VirtualHandle*, MAX_OPENED_HANDLES >		TOpenedHandles; 
	static TOpenedHandles				st_openedHandles;
	static Uint32						st_counter;

	// local per-thread handles
	static const Uint32 MAX_THREADS				= 4;
	typedef TStaticArray< VirtualHandle*, MAX_THREADS >				TLocalReaders; 
	TLocalReaders						m_localHandles;

	friend class CNativeFileReader;
	friend class CDebugPageFileHandleCache;
	friend class CDebugPageFios2;
};

/// An actual physical access point to the file for SYNCHRONOUS READING
// TODO: move to RedIO after David's changes are in
class CNativeFileReader : public Red::NonCopyable
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );

public:
	~CNativeFileReader();

	//! Blocking read. Returns true upon success.
	const Bool Read( void* dest, const Uint32 length, /*[out]*/ Uint32& outNumberOfBytesRead );

	//! Seek to file position. Returns true upon success.
	const Bool Seek( const Uint32 offset, Red::IO::ESeekOrigin seekOrigin );

	//! Get file offset. Returns -1 upon error.
	const Uint32 Tell() const;

private:
	CNativeFileReader( CNativeFileHandleWrapper::VirtualHandle* virtualHandle, CNativeFileHandleWrapper::PhysicalHandle* physicalHandle );

	Uint32										m_owningThread;		// only thread that is allowed to access this handle
	CNativeFileHandleWrapper::PhysicalHandle*	m_physicalHandle;	// physical file handle
	CNativeFileHandleWrapper::VirtualHandle*	m_virtualHandle;	// wrapper interface

	friend class CNativeFileHandleWrapper;
};
