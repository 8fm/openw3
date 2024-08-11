#include "build.h"
#include "fileDecompression.h"
#include "fileDecompressionImpl.h"
#include "bundleFileReaderDecompression.h"
#include "deferredDataBufferKickoff.h"
#include "fileSystemProfilerWrapper.h"
#include "ioTagResolver.h"
#include "profiler.h"

//----

#define NUM_GUARD_BYTES 0

//----

CFileDecompressionMemoryPool::CFileDecompressionMemoryPool( const Uint32 maxSize, const Uint32 maxBlocks, const EMemoryClass memoryClass )
	: m_maxBlocks( maxBlocks )
	, m_currentMemoryUsed( 0 )
	, m_maxMemoryUsed( maxSize )
	, m_memoryClass( memoryClass )
{
	m_blocks.Reserve( maxBlocks );
}

CFileDecompressionMemoryPool::~CFileDecompressionMemoryPool()
{
}

void* CFileDecompressionMemoryPool::Allocate( const Uint32 size )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	RED_FATAL_ASSERT( size != 0, "Invalid buffer size" );

	// prevent to many allocations
	void* mem = nullptr;
	if ( m_blocks.Size() <= m_maxBlocks )
	{
		if ( size > m_maxMemoryUsed )
		{
			// special case: the memory block is so big it would never fit into this pool
			// we need to allocate it regardless because we would fail the loading, but still - do not allocate it until everything else is freed
			WARN_CORE( TXT("Allocating exceptional memory block for decompression, size=%d, limit=%d"), size, m_maxMemoryUsed );
#ifdef RED_PLATFORM_ORBIS
			fprintf( stderr, "IO: Allocating exceptional memory block for decompression, size=%d, limit=%d\n", size, m_maxMemoryUsed );
#endif
			mem = InternalAllocate( size + NUM_GUARD_BYTES );
		}
		else if ( m_currentMemoryUsed + size <= m_maxMemoryUsed)
		{
			// allocate the block only if within the budget
			mem = InternalAllocate( size + NUM_GUARD_BYTES );
		}
	}

	// register allocation
	if ( mem )
	{
		m_blocks.Insert( mem, size );
		m_currentMemoryUsed += size;

		// fill the guard bytes
		Red::MemorySet( (Uint8*)mem + size, 0xCA, NUM_GUARD_BYTES );

		// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileVarAllocMemoryBlock( size );
#endif	
	}

	// return allocated memory
	return mem;
}

void CFileDecompressionMemoryPool::Free( void* memory )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// get size of the block
	Uint32 allocatedSize = 0;
	m_blocks.Find( memory, allocatedSize );

	// make sure it was allocated :)
	RED_FATAL_ASSERT( memory != nullptr, "Invalid buffer pointer" );
	RED_FATAL_ASSERT( allocatedSize > 0, "Invalid size of the allocated block" );
	RED_FATAL_ASSERT( allocatedSize <= m_currentMemoryUsed, "Size of the memory used is invalid" );

	// check the guard bytes
	const Uint8* guardArea = (const Uint8*) memory + allocatedSize;
	for ( Uint32 i=0; i<NUM_GUARD_BYTES; ++i )
	{
		if ( guardArea[i] != 0xCA )
		{
			fprintf( stderr, "CRAP! Addr=0x%p\n", guardArea );
			for (;;) {};
		}
	}

	// free the memory
	m_currentMemoryUsed -= allocatedSize;
	m_blocks.Erase( memory );
	InternalFree( memory );

	// update stats
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarFreeMemoryBlock( allocatedSize );
#endif
}

void* CFileDecompressionMemoryPool::InternalAllocate( const Uint32 size )
{
	return RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_IO, m_memoryClass, size, 16 );
}

void CFileDecompressionMemoryPool::InternalFree( void* ptr )
{
	RED_MEMORY_FREE( MemoryPool_IO, m_memoryClass, ptr );
}

//----

CFileDecompressionTaskAsyncFile::CFileDecompressionTaskAsyncFile()
	: m_compressedSize( 0 )
	, m_compressedMemory( nullptr )
	, m_uncompressedSize( 0 )
	, m_uncompressedMemory( nullptr )
	, m_compressedPool( nullptr )
	, m_uncompressedPool( nullptr )
	, m_compressionType( Red::Core::Bundle::CT_Max )
	, m_thread( nullptr )
	, m_readCompleted( 0 )
	, m_decompressionCompleted( 0 )
	, m_numPendingReadTasks( nullptr )
{
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarAllocDecompressionTask();
#endif
}

CFileDecompressionTaskAsyncFile::~CFileDecompressionTaskAsyncFile()
{
	// wait for the task to complete before killing it
	Sync();

	// release memory back to pool
	// this memory may already be released by the Decompress()
	// this memory will not be allocated for tasks without compression
	if ( m_compressedPool )
	{
		m_compressedPool->Free( m_compressedMemory );
		m_compressedMemory = nullptr;
		m_compressedPool = nullptr;
	}

	// release memory back to pool
	// this memory will not be allocated from pool in case of inplace loading
	if ( m_uncompressedPool )
	{
		m_uncompressedPool->Free( m_uncompressedMemory  );
		m_uncompressedMemory = nullptr;
		m_uncompressedPool = nullptr;
	}

	// stats
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileVarFreeDecompressionTask();
#endif
}

void CFileDecompressionTaskAsyncFile::SetupCompressedData( const Uint32 size, void* memory, CFileDecompressionMemoryPool* pool )
{
	RED_FATAL_ASSERT( m_compressedPool == nullptr, "SetupCompressedData already called" );
	RED_FATAL_ASSERT( m_compressedMemory == nullptr, "SetupCompressedData already called" );
	RED_FATAL_ASSERT( m_compressedSize == 0, "SetupCompressedData already called" );
	RED_FATAL_ASSERT( pool != nullptr, "SetupCompressedData call is invalid" );
	RED_FATAL_ASSERT( memory != nullptr, "SetupCompressedData call is invalid" );
	RED_FATAL_ASSERT( size != 0, "SetupCompressedData call is invalid" );
	m_compressedPool = pool;
	m_compressedMemory = memory;
	m_compressedSize = size;
}

void CFileDecompressionTaskAsyncFile::SetupUncompressedData( const Uint32 size, void* memory, CFileDecompressionMemoryPool* pool )
{
	RED_FATAL_ASSERT( m_uncompressedPool == nullptr, "SetupUncompressedData already called" );
	RED_FATAL_ASSERT( m_uncompressedMemory == nullptr, "SetupUncompressedData already called" );
	RED_FATAL_ASSERT( m_uncompressedSize == 0, "SetupUncompressedData already called" );
	RED_FATAL_ASSERT( memory != nullptr, "SetupUncompressedData call is invalid" );
	RED_FATAL_ASSERT( size != 0, "SetupUncompressedData call is invalid" );
	m_uncompressedPool = pool;
	m_uncompressedMemory = memory;
	m_uncompressedSize = size;
}

void CFileDecompressionTaskAsyncFile::SetupCompression( CFileDecompressionThread* thread, CompressionType compression )
{
	m_thread = thread;
	m_compressionType = compression;
}

void CFileDecompressionTaskAsyncFile::SetupReadTaskCounter( Red::Threads::CAtomic< Uint32 >* couter )
{
	m_numPendingReadTasks = couter;
}

void CFileDecompressionTaskAsyncFile::SetupFileAndStart( FileHandle handle, const Uint64 offset, const Uint8 ioTag )
{
	RED_FATAL_ASSERT( handle != 0, "SetupFileAndStart call is invalid" );

	// setup async task
	m_readToken.m_callback = &CFileDecompressionTaskAsyncFile::OnDataLoaded;
	m_readToken.m_offset = offset;
	m_readToken.m_userData = this;

	// setup buffer
	if ( m_compressionType == Red::Core::Bundle::CT_Uncompressed )
	{
		RED_FATAL_ASSERT( m_uncompressedMemory != nullptr, "Invalid or incomplete setup" );
		m_readToken.m_buffer = m_uncompressedMemory;
		m_readToken.m_numberOfBytesToRead = m_uncompressedSize;
	}
	else
	{
		RED_FATAL_ASSERT( m_compressedMemory != nullptr, "Invalid or incomplete setup" );
		m_readToken.m_buffer = m_compressedMemory;
		m_readToken.m_numberOfBytesToRead = m_compressedSize;
	}

	// keep extra reference wile we are reading this shit via the AsyncIO
	AddRef();

	// resolve priority and start
	const auto priority = GFileSysPriorityResovler.Resolve( (EIOTag) ioTag );
	Red::IO::GAsyncIO.BeginRead( handle, m_readToken, priority, (Uint32)ioTag );
}

CFileDecompressionTaskAsyncFile::EResult CFileDecompressionTaskAsyncFile::GetData( void*& outDataPointer ) const
{
	// failed ?
	if ( m_failed.GetValue() == 1 )
		return eResult_Failed;

	// still processing ?
	if ( m_decompressionCompleted.GetValue() != 1 )
		return eResult_NotReady;

	// done
	outDataPointer = m_uncompressedMemory;
	return eResult_OK;
}

const Uint32 CFileDecompressionTaskAsyncFile::GetSize() const
{
	return m_uncompressedSize;
}

void CFileDecompressionTaskAsyncFile::Sync()
{
	// wait for the read to complete before destroying
	{
		CTimeCounter timer;
		Bool waiting = false;
		while ( 1 != m_readCompleted.CompareExchange( 1, 1 ) )
		{
			if ( !waiting )
			{
				WARN_CORE( TXT("Decompression: Waiting for read started" ) );
				waiting = true;
			}

			Red::Threads::YieldCurrentThread();
		}

		// log any waiting in here
		if ( waiting )
		{
			WARN_CORE( TXT("Decompression: Waiting for read took %1.2fms" ), timer.GetTimePeriodMS() );
		}
	}

	// wait for the decompression to complete
	{
		CTimeCounter timer;
		Bool waiting = false;
		while ( 1 != m_decompressionCompleted.CompareExchange( 1, 1 ) )
		{
			if ( !waiting )
			{
				WARN_CORE( TXT("Decompression: Waiting for decompression started" ) );
				waiting = true;
			}

			Red::Threads::YieldCurrentThread();
		}

		// log any waiting in here
		if ( waiting )
		{
			WARN_CORE( TXT("Decompression: Waiting for decompression took %1.2fms" ), timer.GetTimePeriodMS() );
		}
	}
}

void CFileDecompressionTaskAsyncFile::SignalFinishedTask()
{
	if ( m_numPendingReadTasks != nullptr )
	{
		m_numPendingReadTasks->Decrement();
		m_numPendingReadTasks = nullptr;
	}
}

void CFileDecompressionTaskAsyncFile::ProcessLoadedData()
{
	// if the data is compressed add it to the decompression thread
	if ( m_compressionType != Red::Core::Bundle::CT_Uncompressed )
	{
		// send to decompression thread
		RED_FATAL_ASSERT( m_thread != nullptr, "Compressed data but no decompression thread" );
		m_thread->SendDecompressionTask( this );
	}
	else
	{
		// data is not compressed
		m_decompressionCompleted.SetValue(1);

		// release the refcount
		SignalFinishedTask();
	}
}

void CFileDecompressionTaskAsyncFile::Decompress()
{
	RED_FATAL_ASSERT( m_compressedMemory != nullptr, "Decompressed called with crappy data" );
	RED_FATAL_ASSERT( m_uncompressedMemory != nullptr, "Decompressed called with crappy data" );
	RED_FATAL_ASSERT( m_compressedSize != 0, "Decompressed called with crappy data" );
	RED_FATAL_ASSERT( m_uncompressedSize != 0, "Decompressed called with crappy data" );
	RED_FATAL_ASSERT( m_compressionType == Red::Core::Bundle::CT_ChainedZlib || m_compressedSize < m_uncompressedSize, "Decompressed called with crappy data" );
	RED_FATAL_ASSERT( m_compressionType != Red::Core::Bundle::CT_Uncompressed, "Decompressed called with crappy data" );

	// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileAsyncDecompressStart( m_uncompressedSize, m_compressionType );
#endif	

	// decompress the data
	BundleFileReaderDecompression::DecompressFileBufferSynch( 
		m_compressionType, 
		m_compressedMemory, m_compressedSize,
		m_uncompressedMemory, m_uncompressedSize );

	// update profiling stats
#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileAsyncDecompressEnd();
#endif	

	// release the compressed data buffer (if owned)
	if ( m_compressedPool )
	{
		m_compressedPool->Free( m_compressedMemory );
		m_compressedMemory = nullptr;
		m_compressedPool = nullptr;
	}

	// marked as done
	m_decompressionCompleted.SetValue(1);

	// we can free the memory now
	SignalFinishedTask();
}

Red::IO::ECallbackRequest CFileDecompressionTaskAsyncFile::OnDataLoaded(Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CFileDecompressionTaskAsyncFile* task = (CFileDecompressionTaskAsyncFile*) asyncReadToken.m_userData;
	RED_FATAL_ASSERT( task != nullptr, "Invalid task" );
	RED_FATAL_ASSERT( &task->m_readToken == &asyncReadToken, "Invalid task" );

	// IO has completed
	task->m_readCompleted.SetValue(1);

	// failed ?
	if ( asyncResult != Red::IO::eAsyncResult_Success )
	{
		task->m_failed.SetValue(1);
		task->m_decompressionCompleted.SetValue(1); // will not be done

		// reading done, update global state
		task->SignalFinishedTask();
	}
	else
	{
		// process the results
		task->ProcessLoadedData();
	}

	// reading is done, we can release the extra reference
	task->Release();

	// done
	return Red::IO::eCallbackRequest_Finish;
}

//----

CFileDecompressionThread::CFileDecompressionThread()
	: CThread( "DecompressionThread", Red::Threads::SThreadMemParams( 512 << 10 ) ) // 512 KB of stack
	, m_taskCount( 0, INT_MAX )
	, m_exit( false )
{
}

CFileDecompressionThread::~CFileDecompressionThread()
{
}

void CFileDecompressionThread::SendDecompressionTask( CFileDecompressionTaskAsyncFile* task )
{
	// keep internal reference
	task->AddRef();

	// add to list
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_taskLock );
		m_taskList.PushBack( task );
	}

	// signal the decompression thread to start
	m_taskCount.Release(1);
}

void CFileDecompressionThread::SendKillSignal()
{
	m_exit = true;
	m_taskCount.Release();
}

CFileDecompressionTaskAsyncFile* CFileDecompressionThread::PopTask()
{
	m_taskCount.Acquire();

	CFileDecompressionTaskAsyncFile* task = nullptr;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_taskLock );
		if ( !m_exit && !m_taskList.Empty() )
		{
			task = m_taskList[0];
			m_taskList.RemoveAt(0);
		}
	}

	return task;
}

void CFileDecompressionThread::ThreadFunc()
{
	Memory::RegisterCurrentThread();

	LOG_CORE( TXT("Decompression thread started") );

	while ( !m_exit )
	{
		// pop the task
		CFileDecompressionTaskAsyncFile* task = PopTask();
		if ( !task )
			continue;

		// decompress the data
		task->Decompress();
		task->Release();

		// Try to keep DDB loading busy
		SDeferredDataBufferKickOffList::GetInstance().KickNewJobs();
	}

	LOG_CORE( TXT("Decompression thread finished") );
}

//----
