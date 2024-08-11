/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "collisionCacheAsyncReader.h"
#include "collisionCacheDataFormat.h"
#include "../physics/compiledCollision.h"

#include "../core/fileSystemProfilerWrapper.h"
#include "../core/memoryFileReader.h"
#include "../core/compression/zlib.h"
#include "../core/ioTagResolver.h"
#include "../core/configVar.h"

#define ONE_MB (1024.0f * 1024.0f)

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >		cvSmallCollisionDecompressionTreshold( "Collision", "SmallCollisionDecompressionTreshold", 4096, eConsoleVarFlag_Developer );
}


CCollsionCacheAsyncReader::CCollsionCacheAsyncReader()
	: m_isLoading( false )
{
#ifdef NO_EDITOR	
	// ctremblay. About to ship game, and those are the needed value. 
	m_readBuffer.Resize( 3 * 1024 * 1024 );
	m_loadBuffer.Resize( 4 * 1024 * 1024 + 512 * 1024 );
#else
	// reserve some memory (1MB) for each buffer
	m_readBuffer.Resize( 1 << 20 );
	m_loadBuffer.Resize( 1 << 20 );
#endif
	// create decompression thread
	m_thread = new CDecompressionThread();
	m_thread->InitThread();
	m_thread->SetPriority( Red::Threads::TP_Normal );

	// offload thread to second cluster
#if defined( RED_PLATFORM_DURANGO )
	Red::Threads::TAffinityMask affinityMasks = (1 << 6) | (1 << 5) | (1 << 4);
	m_thread->SetAffinityMask( affinityMasks );
#elif defined( RED_PLATFORM_ORBIS )
	Red::Threads::TAffinityMask affinityMasks = (1 << 0) | (1 << 1) | (1 << 4) | (1 << 5);
	m_thread->SetAffinityMask( affinityMasks );
#endif
}

CCollsionCacheAsyncReader::~CCollsionCacheAsyncReader()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	RED_FATAL_ASSERT( m_isLoading, "Trying to destroy collision cache loader during loading phase" );

	// kill the thread
	if ( m_thread )
	{
		m_thread->SendKillSignal();
		m_thread->JoinThread();
		delete m_thread;
		m_thread = nullptr;
	}
}

void CCollsionCacheAsyncReader::EnsureSupport( const SLoadingTask& loadingTask )
{
	// make sure the loading will be possible for specified cache
	// what it means is that the loading and read buffers must be big enough to accommodate biggest asset in the cache
	if ( loadingTask.m_loadBufferSize > m_loadBuffer.Size() )
	{
		LOG_ENGINE( TXT("Collision cache AsyncIO: load buffer resized %1.2f MB -> %1.2f MB"), 
			m_loadBuffer.Size() / ONE_MB,
			loadingTask.m_loadBufferSize / ONE_MB );

		m_loadBuffer.Resize( loadingTask.m_loadBufferSize );
	 }

	// the same for read buffer (decompression data + deserialization)
	if ( loadingTask.m_readBufferSize > m_readBuffer.Size() )
	{
		LOG_ENGINE( TXT("Collision cache AsyncIO: read buffer resized %1.2f MB -> %1.2f MB"), 
			m_readBuffer.Size() / ONE_MB, 
			loadingTask.m_readBufferSize / ONE_MB );

		m_readBuffer.Resize( loadingTask.m_readBufferSize );
	}
}

CCollsionCacheAsyncReader::EResult CCollsionCacheAsyncReader::StartLoading( const SLoadingTask& loadingTask )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// we are already loading
	if ( m_isLoading )
		return eResult_Busy;

	// - if we are here than we NOW that the there are no tasks running on the thread

	// validation - if anything is wrong here we would crash anyway
	RED_FATAL_ASSERT( (loadingTask.m_file != Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE) || (loadingTask.m_memory != nullptr), "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_offset >= 0, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_sizeOnDisk > 0, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_sizeInMemory > 0, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_sizeInMemory >= loadingTask.m_sizeOnDisk, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_sizeInMemory <= loadingTask.m_loadBufferSize, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_sizeOnDisk <= loadingTask.m_readBufferSize, "Invalid collision cache loading task" );
	RED_FATAL_ASSERT( loadingTask.m_callback, "Invalid collision cache loading task. You need to provide a callback" );

	// make sure we are able to support reading of the data
	EnsureSupport( loadingTask );

	// we are loading from now on
	m_task = loadingTask;
	m_isLoading = true;
	RED_THREADS_MEMORY_BARRIER();

	// resolve collision loading priority
	const auto prio = GFileSysPriorityResovler.Resolve( eIOTag_CollisionNormal );

	// start loading of the collision data into the read buffer
	if ( loadingTask.m_file !=  Red::IO::CAsyncFileHandleCache::INVALID_FILE_HANDLE )
	{
		m_readToken.m_buffer = m_readBuffer.Data();
		m_readToken.m_callback = &CCollsionCacheAsyncReader::OnDataLoaded;
		m_readToken.m_numberOfBytesToRead = loadingTask.m_sizeOnDisk;
		m_readToken.m_offset = loadingTask.m_offset;
		m_readToken.m_userData = this;
		Red::IO::GAsyncIO.BeginRead( loadingTask.m_file, m_readToken, prio, eIOTag_CollisionNormal );

		// - we are during async op in here - the state is not certain, IE - the m_isLoading may be false again
		// return eResult_Started and hope that the called will handle the task result at some point in the future

		// started loading
		return eResult_StartedAndWaiting;
	}
	else if ( loadingTask.m_memory != nullptr )
	{
		// TEMPSHIT, TODO: match the interface better
		Red::MemoryCopy( m_readBuffer.Data(), loadingTask.m_memory, loadingTask.m_sizeOnDisk );

		// there's no reading, send the decompression task directly
		const auto ret = m_thread->SendTask( m_task, m_readBuffer.Data(), m_loadBuffer.Data(), &m_isLoading );
		if ( ret == CDecompressionThread::eSendResult_Finished )
		{
			// decompression finished immediately
			m_isLoading = false;
			return eResult_StartedAndFinished;
		}
		else
		{
			// we need to wait for the decompression
			return eResult_StartedAndWaiting;
		}
	}
	else
	{
		// unsupported case
		return eResult_Failed;
	}
}

Red::IO::ECallbackRequest CCollsionCacheAsyncReader::OnDataLoaded( Red::IO::SAsyncReadToken& asyncReadToken, Red::IO::EAsyncResult asyncResult, Uint32 numberOfBytesTransferred )
{
	CCollsionCacheAsyncReader* reader = static_cast< CCollsionCacheAsyncReader* >( asyncReadToken.m_userData );
	RED_FATAL_ASSERT( reader->m_isLoading == true, "Async loading job finished but we are not loading. WTF?" );

	// loaded ?
	if ( asyncResult == Red::IO::eAsyncResult_Success )
	{
		RED_FATAL_ASSERT( numberOfBytesTransferred == asyncReadToken.m_numberOfBytesToRead, "Partial reads are not supported here" );

		// start decompression/deserialization
		const auto ret = reader->m_thread->SendTask( reader->m_task, reader->m_readBuffer.Data(), reader->m_loadBuffer.Data(), &reader->m_isLoading );

		// simple decompressions can finish right away
		if ( ret == CDecompressionThread::eSendResult_Finished )
		{
			reader->m_isLoading = false;
		}
	}
	else
	{
		// the loading failed - we will not decompress anything, mark task as failed
		reader->m_task.m_callback( CompiledCollisionPtr() );
		RED_THREADS_MEMORY_BARRIER();

		// release the fence
		reader->m_isLoading = false;
	}

	// no more data needed
	return Red::IO::eCallbackRequest_Finish;
}

//----

CCollsionCacheAsyncReader::CDecompressionThread::CDecompressionThread()
	: CThread( "CollisionCacheDecompression", Red::Threads::SThreadMemParams( 512 << 10 ) ) // 512 KB of stack
	, m_exit( false )
	, m_task( nullptr )
	, m_readBuffer( nullptr )
	, m_loadBuffer( nullptr )
	, m_isLoading( nullptr )
	, m_start( 0, INT_MAX )
{
}

CCollsionCacheAsyncReader::CDecompressionThread::~CDecompressionThread()
{
}

CCollsionCacheAsyncReader::CDecompressionThread::ESendResult CCollsionCacheAsyncReader::CDecompressionThread::SendTask( SLoadingTask& task, void* readBuffer, void* loadBuffer, volatile Bool* isLoading )
{
	RED_FATAL_ASSERT( m_task == nullptr, "Decompression thread contention" );
	RED_FATAL_ASSERT( *isLoading == true, "Inconsistent thread state" );

	/*// decompression tasks is so tiny it should happen synchronously to save time
	const Uint32 smallTaskSize = Config::cvSmallCollisionDecompressionTreshold.Get();
	if ( task.m_sizeInMemory < smallTaskSize )
	{
		// decompress the mesh synchronously
		auto mesh = DecompressData( task, loadBuffer, readBuffer );

		// return the mesh
		task.m_callback( mesh );

		RED_THREADS_MEMORY_BARRIER();

		// we are done
		return eSendResult_Finished;
	}*/

	// start task
	m_task = &task;
	m_isLoading = isLoading;
	m_loadBuffer = loadBuffer;
	m_readBuffer = readBuffer;
	m_start.Release(1);

	// task was scheduled and is pending
	return eSendResult_Pending;
}

void CCollsionCacheAsyncReader::CDecompressionThread::SendKillSignal()
{
	m_exit = true;
	m_start.Release(1);
}

CompiledCollisionPtr CCollsionCacheAsyncReader::CDecompressionThread::DecompressData( const SLoadingTask& task, void* loadData, void* readData )
{
	// decompress (only if bigger in memory than on disk)
	void* deserializationMem;
	if ( task.m_sizeInMemory > task.m_sizeOnDisk )
	{
#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileSyncDecompressStart( task.m_sizeInMemory, Red::Core::Bundle::CT_Zlib );
#endif

		Red::Core::Decompressor::CZLib zlib;
		auto ret = zlib.Initialize( readData, loadData, task.m_sizeOnDisk, task.m_sizeInMemory );
		RED_FATAL_ASSERT( ret == Red::Core::Decompressor::Status_Success, "Internal decompression error" );

		auto ret2 = zlib.Decompress();
		RED_FATAL_ASSERT( ret2 == Red::Core::Decompressor::Status_Success, "Internal decompression error" );

#ifdef RED_PROFILE_FILE_SYSTEM
		RedIOProfiler::ProfileDiskFileSyncDecompressEnd();
#endif

		// deserialize from decompressed memory
		deserializationMem = loadData;
	}
	else
	{
		// deserialize from reading buffer since we have uncompressed data
		deserializationMem = readData;
	}

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileDeserializeStart( TXT("Collision") );
#endif

	// prepare reader
	CMemoryFileReader reader( (const Uint8*) deserializationMem, task.m_sizeInMemory, 0 );

	// create and load mesh
	CompiledCollisionPtr mesh( new CCompiledCollision() );
	mesh->SerializeToCache( reader );

#ifdef RED_PROFILE_FILE_SYSTEM
	RedIOProfiler::ProfileDiskFileDeserializeEnd( TXT("Collision") );
#endif

	return mesh;
}

void CCollsionCacheAsyncReader::CDecompressionThread::ThreadFunc()
{
	Memory::RegisterCurrentThread();

	// debug log
	LOG_ENGINE( TXT("Collision cache decompression thread started") );

	// thread loop
	for ( ;; )
	{
		// wait for tasks
		m_start.Acquire();

		// exit requested
		if ( m_exit )
			break;

		// keep kickstart function
		TKickstartFunction kickstart = m_task->m_kickstart;

		// state validation
		RED_FATAL_ASSERT( m_task != nullptr, "Thread waken up but there are no tasks" );
		RED_FATAL_ASSERT( *m_isLoading == true, "Inconsistent thread state" );

		// validate CRC
		const Uint64 crc = Red::System::CalculateHash64( m_readBuffer, m_task->m_sizeOnDisk );
		if ( crc != m_task->m_dataCRC )
		{
			ERR_ENGINE( TXT("Collision cache AsyncIO: CRC error in loaded data") );

			// fail task (PRESERVE ORDER)
			m_task->m_callback( CompiledCollisionPtr() );
		
			RED_THREADS_MEMORY_BARRIER();
			m_task = nullptr;

			// release loading fence on the whole thread
			*m_isLoading = false; // always last
			continue;
		}

		// decompress the mesh
		auto mesh = DecompressData( (const SLoadingTask&) *m_task, m_loadBuffer, m_readBuffer );

		// done, write result (PRESERVE ORDER)
		m_task->m_callback( mesh );
		RED_THREADS_MEMORY_BARRIER();
		m_task = nullptr;

		// release loading fence on the whole thread
		*m_isLoading = false; // always last

		// kick start some new loading
		if ( kickstart )
			kickstart();
	}

	// debug log
	LOG_ENGINE( TXT("Collision cache decompression thread finished") );
}

Bool CCollsionCacheAsyncReader::IsBusy() const 
{ 
	return m_isLoading; 
}
