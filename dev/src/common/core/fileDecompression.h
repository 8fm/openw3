/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../core/bundleheader.h"

/// Function used when there are resources that need to be released only when the decompression taks finished
typedef std::function< void() > FileDecompressionCleanupFunction;

/// File load + decompression task
class IFileDecompressionTask
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	void AddRef();
	void Release();

	enum EResult
	{
		eResult_OK,			// Task was accepted
		eResult_Failed,		// Task failed (invalid IO, CRC error, or decompression error)
		eResult_NotReady,	// Task has not finished yet
	};

	// get size of data
	virtual const Uint32 GetSize() const = 0;

	// get data, this will WAIT for the data to be there
	// note: the data pointer is still owned by this class until it's released
	virtual EResult GetData( void*& outDataPointer ) const = 0;

	// set cleanup function
	void SetCleanupFunction( const FileDecompressionCleanupFunction& cleanupFunction );

protected:
	IFileDecompressionTask();
	virtual ~IFileDecompressionTask();

private:
	// refcount
	Red::Threads::CAtomic< Int32 >	m_refCount;

	// delayed release action
	FileDecompressionCleanupFunction	m_cleanup;
};

/// AsyncIO + Decompression helper
class CFileDecompression
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	CFileDecompression();
	~CFileDecompression();

	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	AsyncFileHandle;
	typedef Red::Core::Bundle::ECompressionType CompressionType;

	struct TaskInfo
	{
		AsyncFileHandle		m_asyncFile;          // required
		Uint64				m_offset;             // required
		Uint32				m_compressedSize;     // required
		CompressionType		m_compressionType;    // required, can by CT_Uncompressed
		Uint32				m_uncompressedSize;   // required
		void*				m_uncompressedMemory; // optional, required only for inplace loading
		Uint8				m_ioTag;			  // required, io tag of the operation (from ioTag.h)
	};

	enum EResult
	{
		eResult_OK,			// Task was accepted
		eResult_Failed,		// Task could not be accepted at all (invalid params)
		eResult_NotReady,	// Internal decompression resources are full, task cannot be processed now
	};

	// Create decompression task for given task description
	// You need to have async or sync file handle
	// You need to know the compressed and uncompressed size
	// You can provide the memory for the uncompressed data, if not it will be allocated
	// Memory for the internal compressed data is handled automatically
	// This can return eResult_NotReady which means you need to retry
	const EResult DecompressAsyncFile( const TaskInfo& taskInfo, IFileDecompressionTask*& outTask );

	// Flush all pending decompression tasks
	// This will return all of the IO memory that is in use by the decompression thread
	void FlushAndLock();

	// Unlock the decompression system
	void Unlock();

private:
	// thread doing the decompression, loading is done using the AsyncIO
	class CFileDecompressionThread*		m_thread;

	// number of asynchronous requests in flight
	Red::Threads::CAtomic< Uint32 >		m_numPendingRequests;

	// total number of requests processed
	Red::Threads::CAtomic< Uint32 >		m_numTotalRequests;

	// "pool" for compressed memory - uses the IO memory pool
	class CFileDecompressionMemoryPool*	m_compressedPool;

	// "pool" for uncompressed memory - uses the IO memory pool
	class CFileDecompressionMemoryPool*	m_uncompressedPool;

	// are we flushing the decompression task list ?
	Red::Threads::CAtomic<Bool>			m_isFlushing;
};

