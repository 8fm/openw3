/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "hashmap.h"
#include "fileDecompression.h"

#include "../redIO/redIOAsyncIO.h"
#include "../redIO/redIOAsyncFileHandleCache.h"
#include "../redIO/redIOAsyncReadToken.h"

//------

class CFileDecompressionTaskAsyncFile;

//------

/// Decompression memory allocator
class CFileDecompressionMemoryPool
{
public:
	CFileDecompressionMemoryPool( const Uint32 maxSize, const Uint32 maxBlocks, const EMemoryClass memoryClass );
	~CFileDecompressionMemoryPool();

	// get the amount of allocated memory
	RED_FORCE_INLINE const Uint32 GetAllocatedMemory() const { return m_currentMemoryUsed; }

	// get number of allocated blocks
	RED_FORCE_INLINE const Uint32 GetAlloctedBlocks() const { return m_blocks.Size(); }

	// allocate a block of memory for decompression
	// this can return NULL if we are out of budget
	void* Allocate( const Uint32 size );

	// release block of memory
	void Free( void* memory );

private:
	// naive way to keep track of allocated memory
	typedef THashMap< void*, Uint32, DefaultHashFunc<void*>, DefaultEqualFunc<void*>, MC_Depot >		TBufferHandles;
	TBufferHandles			m_blocks;

	// size of the memory allocated so far
	volatile Uint32			m_currentMemoryUsed;
	Uint32					m_maxMemoryUsed;

	// maximum number of allocated blocks
	Uint32					m_maxBlocks;

	// internal pool type
	EMemoryClass			m_memoryClass;

	// internal lock
	Red::Threads::CMutex	m_lock;

	// internal allocation
	void* InternalAllocate( const Uint32 size );
	void InternalFree( void* ptr );
};

//------

/// Internal decompression thread
class CFileDecompressionThread : public Red::Threads::CThread
{
public:
	CFileDecompressionThread();
	~CFileDecompressionThread();

	void SendDecompressionTask( CFileDecompressionTaskAsyncFile* task );
	void SendKillSignal();

private:
	typedef TDynArray< CFileDecompressionTaskAsyncFile* >		TaskList;

	volatile Bool						m_exit;
	Red::Threads::CSemaphore			m_taskCount;
	Red::Threads::CMutex				m_taskLock;
	TaskList							m_taskList;	

	CFileDecompressionTaskAsyncFile* PopTask();

	virtual void ThreadFunc();
};

//------

/// Internal decompression task from 
/// File load + decompression task
class CFileDecompressionTaskAsyncFile : public IFileDecompressionTask
{
public:
	CFileDecompressionTaskAsyncFile();

	typedef Red::IO::CAsyncFileHandleCache::TFileHandle	FileHandle;
	typedef Red::Core::Bundle::ECompressionType CompressionType;

	// IFileDecompressionTask interface
	virtual const Uint32 GetSize() const override;
	virtual EResult GetData( void*& outDataPointer ) const override;

	// Setup task
	void SetupCompressedData( const Uint32 size, void* memory, CFileDecompressionMemoryPool* pool );
	void SetupUncompressedData( const Uint32 size, void* memory, CFileDecompressionMemoryPool* pool );
	void SetupCompression( CFileDecompressionThread* thread, CompressionType compression );
	void SetupReadTaskCounter( Red::Threads::CAtomic< Uint32 >* couter );
	void SetupFileAndStart( FileHandle handle, const Uint64 offset, const Uint8 ioTag );

	// Decompress (should be called from thread)
	void Decompress();

	// Wait until done (or failed)
	void Sync();

private:
	virtual ~CFileDecompressionTaskAsyncFile();

	// memory buffers
	Uint32			m_compressedSize;
	void*			m_compressedMemory;
	Uint32			m_uncompressedSize;
	void*			m_uncompressedMemory;

	// memory pools owning the buffers (null if memory not owned by pool)
	CFileDecompressionMemoryPool*	m_compressedPool;
	CFileDecompressionMemoryPool*	m_uncompressedPool;

	// decompression thread we can use to schedule decompression
	CFileDecompressionThread*		m_thread;

	// async task for the async IO
	Red::IO::SAsyncReadToken	m_readToken;

	// internal states
	mutable Red::Threads::CAtomic< Int32 >	m_failed;
	mutable Red::Threads::CAtomic< Int32 >	m_readCompleted;
	mutable Red::Threads::CAtomic< Int32 >	m_decompressionCompleted;

	// external state
	Red::Threads::CAtomic< Uint32 >*		m_numPendingReadTasks;

	// type of the compression
	CompressionType	m_compressionType;

	// loading done
	void ProcessLoadedData();

	// signal the fact that the task has finished
	void SignalFinishedTask();

	// loading callback
	static Red::IO::ECallbackRequest OnDataLoaded(Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred );
};

//------

