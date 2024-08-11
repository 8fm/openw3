#include "build.h"
#include "fileDecompression.h"
#include "fileDecompressionImpl.h"
#include "configVar.h"
#include "profiler.h"
#include "ioTagResolver.h"
#include "memoryHelpers.h"

//----

namespace Config
{
	TConfigVar< Int32, Validation::IntRange<10, 256> >		cvMaxAsyncRequestsInFlight( "ResourceLoading/Decompression", "MaxRequests", 200, eConsoleVarFlag_Developer );
	TConfigVar< Int32, Validation::IntRange<1, 16> >		cvRequestsPriorityReserve( "ResourceLoading/Decompression", "RequestsPriorityReserve", 8, eConsoleVarFlag_Developer );	
	TConfigVar< Int32, Validation::IntRange<1, 4> >			cvNumDecompressionThreads( "ResourceLoading/Decompression", "NumThreads", 2, eConsoleVarFlag_Developer | eConsoleVarFlag_ReadOnly );
	TConfigVar< Int32, Validation::IntRange<1, 256> >		cvMaxCompressedBudget( "ResourceLoading/Decompression", "MaxCompressedMemory", 50, eConsoleVarFlag_Developer );
	TConfigVar< Int32, Validation::IntRange<256, 100000> >	cvMaxCompressedBlocks( "ResourceLoading/Decompression", "MaxCompressedBlocks", 4096, eConsoleVarFlag_Developer );
	TConfigVar< Int32, Validation::IntRange<1, 256> >		cvMaxUncompressedBudget( "ResourceLoading/Decompression", "MaxUncompressedMemory", 200, eConsoleVarFlag_Developer );
	TConfigVar< Int32, Validation::IntRange<256, 100000> >	cvMaxUncompressedBlocks( "ResourceLoading/Decompression", "MaxUncompressedBlocks", 4096, eConsoleVarFlag_Developer );
}

//----

IFileDecompressionTask::IFileDecompressionTask()
	: m_refCount( 1 )
	, m_cleanup( nullptr )
{
}

IFileDecompressionTask::~IFileDecompressionTask()
{
	RED_FATAL_ASSERT( m_refCount.GetValue() == 0, "Invalid refcount in destructor (%d)", m_refCount.GetValue() );

	if ( m_cleanup )
	{
		m_cleanup();
	}
}

void IFileDecompressionTask::AddRef()
{
	m_refCount.Increment();
}

void IFileDecompressionTask::Release()
{
	if ( 0 == m_refCount.Decrement() )
	{
		delete this;
	}
}

void IFileDecompressionTask::SetCleanupFunction( const FileDecompressionCleanupFunction& cleanupFunction )
{
	RED_FATAL_ASSERT( !m_cleanup, "Cleanup function already set" );
	m_cleanup = cleanupFunction;
}

//----

CFileDecompression::CFileDecompression()
	: m_isFlushing( false )
	, m_numTotalRequests( 0 )
	, m_numPendingRequests( 0 )
{
	// create the thread
	m_thread = new CFileDecompressionThread();
	m_thread->InitThread();
	m_thread->SetPriority( Red::Threads::TP_TimeCritical ); // decompression must complete as soon as possible - it's holding on to resources

	// offload thread to second cluster
#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
	Red::Threads::TAffinityMask affinityMasks = (1 << 5);
	m_thread->SetAffinityMask( affinityMasks );
#endif	

	// create the pools, NOTE: the memory is not allocated in here
	m_compressedPool = new CFileDecompressionMemoryPool( Config::cvMaxCompressedBudget.Get() << 20, Config::cvMaxCompressedBlocks.Get(), MC_IOCompressedData );
	m_uncompressedPool = new CFileDecompressionMemoryPool( Config::cvMaxUncompressedBudget.Get() << 20, Config::cvMaxUncompressedBlocks.Get(), MC_IODecompressedData );
}

CFileDecompression::~CFileDecompression()
{
	// kill the decompression thread
	if ( m_thread )
	{
		m_thread->SendKillSignal();
		m_thread->JoinThread();
		delete m_thread;
	}

	// release the memory pool
	if ( m_compressedPool )
	{
		delete m_compressedPool;
		m_compressedPool = nullptr;
	}

	// release the memory pool
	if ( m_uncompressedPool )
	{
		delete m_uncompressedPool;
		m_uncompressedPool = nullptr;
	}
}

const CFileDecompression::EResult CFileDecompression::DecompressAsyncFile( const CFileDecompression::TaskInfo& taskInfo, IFileDecompressionTask*& outTask )
{
	// NOTE: until sure of the lock below, do nothing that can allocate from the IO pool

	// check file
	if ( !taskInfo.m_asyncFile )
		return eResult_Failed;

	// validate sizes
	if ( taskInfo.m_compressionType == Red::Core::Bundle::CT_Uncompressed )
	{
		// uncompressed data should have the same compressed and uncompressed size
		if ( taskInfo.m_compressedSize != taskInfo.m_uncompressedSize )
			return eResult_Failed;
	}
	// Because ChainedZLib contains a fixed-size header per chunk, it's possible that in might be marginally bigger on small data :/
	else if ( taskInfo.m_compressionType != Red::Core::Bundle::CT_ChainedZlib )
	{
		// compressed data should actually be compressed
		if ( taskInfo.m_compressedSize >= taskInfo.m_uncompressedSize )
			return eResult_Failed;
	}

	// make sure we do not saturate async IO to much
	const Uint32 requestCount = m_numPendingRequests.Increment();
	
	// do not accept new tasks if flushing
	// check for flushing lock after incrementing num pending requests in order to avoid a race in FlushAndLock where this task got in after the lock,
	// but before the increment. Otherwise the flush could miss waiting for this.
	if ( m_isFlushing.GetValue() )
	{
		m_numPendingRequests.Decrement();
		return eResult_NotReady;
	}

	// resolve priority map
	const auto priority = GFileSysPriorityResovler.Resolve( (EIOTag) taskInfo.m_ioTag );

	const Uint32 maxCount = Config::cvMaxAsyncRequestsInFlight.Get() - (Config::cvRequestsPriorityReserve.Get() * priority); // make sure there's always some space for high priority stuff
	if ( requestCount > maxCount )
	{
		m_numPendingRequests.Decrement();
		return eResult_NotReady;
	}

	// allocate target buffer if not specified
	bool isBufferAllocated = false;
	void* uncompressedBuffer = taskInfo.m_uncompressedMemory;
	if ( !uncompressedBuffer )
	{
		isBufferAllocated = true;
		uncompressedBuffer = m_uncompressedPool->Allocate( taskInfo.m_uncompressedSize );
		if ( !uncompressedBuffer )
		{
			WARN_CORE( TXT("Decompression: Internal decompression pool for uncompressed data is full (request=%d)"), taskInfo.m_uncompressedSize );

			m_numPendingRequests.Decrement();
			return eResult_NotReady;
		}
	}

	// allocate compression buffer (only for compressed data)
	void* compressionBuffer = nullptr;
	if ( taskInfo.m_compressionType != Red::Core::Bundle::CT_Uncompressed )
	{
		compressionBuffer = m_compressedPool->Allocate( taskInfo.m_compressedSize );
		if ( !compressionBuffer )
		{
			WARN_CORE( TXT("Decompression: Internal decompression pool for compressed data is full (request=%d)"), taskInfo.m_compressedSize );

			// release the uncompressed memory if it was allocated
			if ( uncompressedBuffer != taskInfo.m_uncompressedMemory )
				m_uncompressedPool->Free( uncompressedBuffer );

			// we are not ready yet
			m_numPendingRequests.Decrement();
			return eResult_NotReady;
		}
	}

	// OK, we have memory, create the task and start async loading
	CFileDecompressionTaskAsyncFile* task = new CFileDecompressionTaskAsyncFile();
	if ( compressionBuffer != nullptr )
	{
		task->SetupCompressedData( taskInfo.m_compressedSize, compressionBuffer, m_compressedPool );
		task->SetupCompression( m_thread, taskInfo.m_compressionType );
	}
	else
	{
		task->SetupCompression( nullptr, Red::Core::Bundle::CT_Uncompressed );
	}

	// setup target buffer
	RED_FATAL_ASSERT( uncompressedBuffer != nullptr, "No buffer" );
	task->SetupUncompressedData( taskInfo.m_uncompressedSize, uncompressedBuffer, isBufferAllocated ? m_uncompressedPool : nullptr );

	// start reading
	task->SetupReadTaskCounter( &m_numPendingRequests ); // sync
	task->SetupFileAndStart( taskInfo.m_asyncFile, taskInfo.m_offset, taskInfo.m_ioTag );

	// count requests submitted
	m_numTotalRequests.Increment();

	// done!
	outTask = task;
	return eResult_OK;
}

void CFileDecompression::FlushAndLock()
{
	CTimeCounter timer;

	// lock the decompression engine - we will not accept new tasks
	m_isFlushing.SetValue( true );

	// get memory status BEFORE the flush
#ifndef RED_FINAL_BUILD
	const Uint32 ioMemoryStartBlocks = (Uint32) Memory::GetPoolTotalAllocations< MemoryPool_IO >();
	const Uint32 ioMemoryStartSize = (Uint32) Memory::GetPoolTotalBytesAllocated< MemoryPool_IO >();
#endif

	// wait for us to finish all of the requests
	while ( m_numPendingRequests.GetValue() > 0 )
	{
		Red::Threads::YieldCurrentThread();
	}

	// W3 hack: wait for the memory to be freed
	for ( ;; )
	{
		const Uint32 allocatedSize = m_compressedPool->GetAllocatedMemory();
		if ( allocatedSize == 0 )
			break;

		WARN_CORE( TXT("IO LEAK: Memory from compressed pool is still used (%d bytes, %d blocks)"),
			allocatedSize, m_compressedPool->GetAlloctedBlocks() );
	}

	// W3 hack: wait for the memory to be freed
	for ( ;; )
	{
		const Uint32 allocatedSize = m_uncompressedPool->GetAllocatedMemory();
		if ( allocatedSize == 0 )
			break;

		WARN_CORE( TXT("IO LEAK: Memory from decompressed pool is still used (%d bytes, %d blocks)"),
			allocatedSize, m_uncompressedPool->GetAlloctedBlocks() );
	}

	// end of the flush stats
#ifndef RED_FINAL_BUILD
	const Uint32 ioMemoryEndSize = (Uint32) Memory::GetPoolTotalBytesAllocated< MemoryPool_IO >();

	// final stats
	LOG_CORE( TXT("Decompression thread flush took %1.2fms (%1.2f KB -> %1.2f KB, %d->%d)"), 
		timer.GetTimePeriodMS(), 
		ioMemoryStartSize / 1024.0f, ioMemoryEndSize / 1024.0f,
		ioMemoryStartBlocks, ioMemoryEndSize );
#endif
}

void CFileDecompression::Unlock()
{
	// unlock the decompression engine
	m_isFlushing.SetValue( false );
}