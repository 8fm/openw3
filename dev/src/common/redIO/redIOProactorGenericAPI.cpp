/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _HAS_EXCEPTIONS 
#define _HAS_EXCEPTIONS 0
#endif // !_HAS_EXCEPTIONS 

#include "../redSystem/crt.h"
#include "../redSystem/error.h"
#include "../redSystem/utility.h"

#include "redIOPlatform.h"
#include "redIOAsyncReadToken.h"
#include "redIOProactorGenericAPI.h"

#include "../core/fileSystemProfilerWrapper.h"

#ifdef RED_USE_NEW_MEMORY_SYSTEM 
	#include "../redMemory/include/utils.h"
#endif

//#pragma optimize("",off)

REDIO_GENERICAPI_NAMESPACE_BEGIN

#if defined( RED_PLATFORM_DURANGO ) 
	static const Red::Threads::TAffinityMask THREAD_AFFINITY_MASK = ( 1ULL << 4 | 1ULL << 5 | 1ULL << 6 );
#elif defined( RED_PLATFORM_ORBIS )
	static const Red::Threads::TAffinityMask THREAD_AFFINITY_MASK = ( 1ULL << 4 | 1ULL << 5 );
#endif

#ifndef RED_FINAL_BUID
# define DEBUG_CALLBACKS 1
#else
# define DEBUG_CALLBACKS 0
#endif

// Has a simpler thread check, since the callback won't happen on the caller's thread with the generic API
#if DEBUG_CALLBACKS
static RED_TLS Bool GDebugIsCallbackThread;
# define CALLBACK_CHECK(x) do { RED_FATAL_ASSERT( ! GDebugIsCallbackThread, "Cannot perform %ls during callback!", MACRO_TXT(RED_STRINGIFY(x)) ); } while(false)
#else
# define CALLBACK_CHECK(x)
#endif

//////////////////////////////////////////////////////////////////////////
// CIOProactor
//////////////////////////////////////////////////////////////////////////

CIOProactor::SAsyncQueue::SAsyncQueue()
	: m_count( 0 )
{
#ifndef RED_FINAL_BUILD
	Red::System::MemoryZero( m_list, sizeof( m_list) );
#endif
}

void CIOProactor::SAsyncQueue::Put( const SAsyncOp& put )
{
	RED_FATAL_ASSERT( m_count < MAX_ASYNC_OPS, "Number of async ops (%u) exceeded", MAX_ASYNC_OPS );

	// add stuff to queue
	m_list[ m_count ] = put;
	m_count += 1;
}

CIOProactor::SAsyncOp CIOProactor::SAsyncQueue::GetBest( CAsyncFile* lastFile, const Uint64 lastOffset, const Uint32 currentTick )
{
	RED_FATAL_ASSERT( m_count > 0, "Trying to access empty queue" );

	Int32 entryIndex = 0;

	// find entry with the same file and smallest forward distance
	if ( lastFile != nullptr )
	{
		Int64 bestForwardDistanceVal = INT64_MAX;
		Int32 bestForwardDistanceEntry = -1;
		Int64 bestTotalDistanceVal = INT64_MAX;
		Int32 bestTotalDistanceEntry = -1;
		Int64 bestSizeVal = 0;
		Int32 bestSizeEntry = -1;
		Uint32 bestAgeVal = 0;
		Int32 bestAgeEntry = -1;
		
		for ( Uint32 i=0; i<m_count; ++i )
		{
			const auto& info = m_list[i];
			if ( info.m_asyncFile == lastFile )
			{
				const auto offset = (Int64)info.m_pAsyncReadToken->m_offset;
				const auto size = (Int64)info.m_pAsyncReadToken->m_numberOfBytesToRead;

				// forward offset stuff
				if ( offset > (Int64)lastOffset )
				{
					const Int64 diff = (offset - lastOffset);
					if ( diff < bestForwardDistanceVal )
					{
						bestForwardDistanceEntry = i;
						bestForwardDistanceVal = diff;
					}
				}

				// general distance
				{
					const Int64 diff = abs( (Int64)offset - (Int64)lastOffset );
					if ( diff < bestTotalDistanceVal )
					{
						bestTotalDistanceVal = diff;
						bestTotalDistanceEntry = i;
					}
				}

				// block size
				if ( info.m_pAsyncReadToken->m_numberOfBytesToRead > bestSizeVal )
				{
					bestSizeVal = size;
					bestSizeEntry = i;
				}
			}

			// should be fine even if currentTick has wrapped around, assuming it doesn't wrap around multiple times
			// before an item is popped.
			const Uint32 age = currentTick - info.m_startTick;
			if ( age > bestAgeVal )
			{
				bestAgeVal = age;
				bestAgeEntry = i;
			}
		}

		// choose the best
		if ( bestForwardDistanceEntry != -1 )
		{
			entryIndex = bestForwardDistanceEntry;
		}
		else if ( bestTotalDistanceEntry != -1 )
		{
			entryIndex = bestTotalDistanceEntry;
		}
		else if ( bestSizeEntry != -1 )
		{
			entryIndex = bestSizeEntry;
		}
		else if ( bestAgeEntry != -1 )
		{
			entryIndex = bestAgeEntry;
		}
	}

	// pop the entry from the array
	RED_FATAL_ASSERT( entryIndex < (Int32)m_count, "Invalid entry index" );
	const auto ret = m_list[ entryIndex ];

	// patch
	m_count -= 1;
	if ( entryIndex != m_count )
	{
		m_list[ entryIndex ] = m_list[ m_count ];
	}

	// return best entry
	return ret;
}

//////////////////////////////////////////////////////////////////////////
// CIOProactor
//////////////////////////////////////////////////////////////////////////
CIOProactor::CIOProactor()
	: CThread( "RedIO"  )
	, m_asyncFileHandleCache( REDIO_MAX_FILE_HANDLES, REDIO_MAX_FILE_HANDLES )
	, m_totalOpsInQueue( 0 )
	, m_lastRequestFile( nullptr )
	, m_lastRequestOffset( 0 )
	, m_currentTick( 0 )
	, m_shutdownFlag( false )
{
}

CIOProactor::~CIOProactor()
{
}

Bool CIOProactor::Init()
{
	// Shouldn't call twice anyway
	CALLBACK_CHECK(Init);

	InitThread();

	return true;
}

void CIOProactor::Shutdown()
{
	CALLBACK_CHECK(Shutdown);

	{
		CScopedLock lock( m_queueMutex );
		m_shutdownFlag = true;
		// For now don't cancel, since we could be processing some I/O right now
		// and we don't have the queue mutex locked at that exact point
	}
	m_condQueueWait.WakeAny();

	JoinThread();
}

CIOProactor::TFileHandle CIOProactor::OpenFile( const Char* filePath, Uint32 asyncFlags /*= eAsyncFlag_None*/ )
{
	return m_asyncFileHandleCache.Open( filePath, asyncFlags );
}

void CIOProactor::ReleaseFile( TFileHandle fh )
{
	m_asyncFileHandleCache.Release( fh );
}

void CIOProactor::BeginRead( TFileHandle fh, SAsyncReadToken& asyncReadToken, EAsyncPriority priority /*= eAsyncPriority_Normal*/, Uint32 ioTag )
{
	CAsyncFile* asyncFile = m_asyncFileHandleCache.GetAsyncFile_AddRef( fh );
	if ( ! asyncFile )
	{
		const FAsyncOpCallback callback = asyncReadToken.m_callback;
		if ( callback )
		{
			callback( asyncReadToken, eAsyncResult_Error, 0 );
		}
		return;
	}

#ifdef RED_PROFILE_FILE_SYSTEM
	const Uint32 asyncOpId = RedIOProfiler::ProfileAllocRequestId();
	RedIOProfiler::ProfileAsyncIOReadScheduled( asyncFile->GetFileHandle(), asyncOpId, asyncReadToken.m_offset, asyncReadToken.m_numberOfBytesToRead, ioTag );
#endif

	{
		CScopedLock lock( m_queueMutex );

		// Check under the lock vs atomic so cancellation doesn't have to care about something being added to the queue
		if ( m_shutdownFlag )
		{
			m_asyncFileHandleCache.Release( fh );
			const FAsyncOpCallback callback = asyncReadToken.m_callback;
			if ( callback )
			{
				callback( asyncReadToken, eAsyncResult_Canceled, 0 );
			}
			return;
		}

		// FULL QUEUE
		RED_FATAL_ASSERT( m_totalOpsInQueue < MAX_ASYNC_OPS, "Internal IO queue is full. This is a hard limit - decrease number of pending IO operations" );

		SAsyncOp asyncOp;
		asyncOp.m_asyncFile = asyncFile;
		asyncOp.m_cacheFileHandle = fh;
		asyncOp.m_pAsyncReadToken = &asyncReadToken;
		asyncOp.m_ioTag = ioTag;
#ifdef RED_PROFILE_FILE_SYSTEM
		asyncOp.m_operationId = asyncOpId;
#else
		asyncOp.m_operationId = 0;
#endif
		asyncOp.m_startTick = m_currentTick;

		RED_FATAL_ASSERT( priority < eAsyncPriority_Count, "Invalid priority for async op (%d)", priority );
		m_queues[ priority ].Put( asyncOp );
		m_totalOpsInQueue += 1;
	}

	m_condQueueWait.WakeAny();
}

Uint64 CIOProactor::GetFileSize( TFileHandle fh )
{
	// FIXME: Should cache it, or have a way of knowing it ahead of time
	CAsyncFile* asyncFile = m_asyncFileHandleCache.GetAsyncFile_AddRef( fh );
	if ( ! asyncFile )
	{
		return 0;
	}

	const Uint64 size = asyncFile->GetFileSize();
	m_asyncFileHandleCache.Release( fh );

	return size;
}

const Char* CIOProactor::GetFileName( TFileHandle fh ) const
{
	return m_asyncFileHandleCache.GetFileName( fh );
}

void CIOProactor::ThreadFunc()
{
#ifdef RED_USE_NEW_MEMORY_SYSTEM
	red::memory::RegisterCurrentThread();
#endif

#ifdef RED_PLATFORM_DURANGO
	PROCESSOR_NUMBER nProc;
	{
		nProc.Group = 0;
		nProc.Number = 6;
		nProc.Reserved = 0;
	}
	::SetThreadIdealProcessorEx( ::GetCurrentThread(), &nProc, nullptr );
#endif

#if DEBUG_CALLBACKS
	GDebugIsCallbackThread = true;
#endif

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
	SetAffinityMask( THREAD_AFFINITY_MASK );
#endif

	DoLoop();
}

void CIOProactor::GetBestOp( SAsyncOp& asyncOp )
{
	Int32 bestPrio = -1;

	RED_FATAL_ASSERT( m_totalOpsInQueue > 0, "Queue is empty" );

	// what is the best priority ?
	for ( Uint32 i=eAsyncPriority_Critical; i<=eAsyncPriority_Low; ++i )
	{
		if ( m_queues[i].HasData() )
		{
			bestPrio = i;
			break;
		}
	}

	RED_FATAL_ASSERT( bestPrio != -1, "All queues are empty but the thread was woken up" );

	// TODO: anti starvation ?
	// TODO: maybe we should try to select best file ?

	asyncOp = m_queues[ bestPrio ].GetBest( m_lastRequestFile, m_lastRequestOffset, m_currentTick );

	++m_currentTick;

	m_lastRequestFile = asyncOp.m_asyncFile;
	m_lastRequestOffset = asyncOp.m_pAsyncReadToken->m_offset;

	m_totalOpsInQueue -= 1; // should be actually AFTER ProcessBeginRead, we will see what happens
}

void CIOProactor::DoLoop()
{
	for (;;)
	{
		SAsyncOp pendingAsyncOp;

		// wait for op
		{
			CScopedLock lock( m_queueMutex );
			while ( m_totalOpsInQueue < 1 && ! m_shutdownFlag )
			{
				m_condQueueWait.Wait( m_queueMutex );
			}

			// request to exit
			if ( m_shutdownFlag )
				break;

			// get the op from the queue
			GetBestOp( pendingAsyncOp );
		}
		
		// process the op
		ProcessBeginRead( pendingAsyncOp );	

		// release FH that was used
		m_asyncFileHandleCache.Release( pendingAsyncOp.m_cacheFileHandle );
	}
}

void CIOProactor::ProcessBeginRead( SAsyncOp& asyncOp )
{
	CAsyncFile* asyncFile = asyncOp.m_asyncFile;
	SAsyncReadToken& asyncReadToken = *asyncOp.m_pAsyncReadToken;

	Int64 expectedNextOffset = asyncReadToken.m_offset;
	if ( ! asyncFile->Seek( expectedNextOffset, eSeekOrigin_Set ) )
	{
		REDIO_ERR( TXT("Failed to seek to offset %lld, fd %d"), expectedNextOffset, asyncOp.m_cacheFileHandle );
		FAsyncOpCallback callback = asyncReadToken.m_callback;
		if ( callback )
		{
			callback( asyncReadToken, eAsyncResult_Error, 0 );
		}
		return;
	}

	for(;;)
	{
		void* const buffer = asyncReadToken.m_buffer;
		const Int64 offset = asyncReadToken.m_offset;
		const Uint32 numberOfBytesToRead = asyncReadToken.m_numberOfBytesToRead;
		FAsyncOpCallback callback = asyncReadToken.m_callback;

#ifdef RED_PROFILE_FILE_SYSTEM
		if ( asyncOp.m_operationId )
		{
			RedIOProfiler::ProfileAsyncIOReadStart( asyncOp.m_operationId );
		}
#endif

		if ( offset != expectedNextOffset )
		{
			if ( ! asyncFile->Seek( offset, eSeekOrigin_Set ) )
			{
				REDIO_ERR( TXT("Failed to seek to offset %lld, fd %d"), offset, asyncOp.m_cacheFileHandle );
				if ( callback )
				{
					callback( asyncReadToken, eAsyncResult_Error, 0 );
				}
				break;
			}
		}


		Uint32 nbytes = 0;
		if ( ! asyncFile->Read( buffer, numberOfBytesToRead, nbytes ) || nbytes != numberOfBytesToRead )
		{
			REDIO_ERR( TXT("Failed to read %u bytes, fd %d"), numberOfBytesToRead, asyncOp.m_cacheFileHandle );
			if ( callback )
			{
				callback( asyncReadToken, eAsyncResult_Error, 0 );
			}
			break;
		}

		// TBD: could always reset to -1 if would exceed INT64_MAX
		expectedNextOffset += nbytes;

		const ECallbackRequest callbackRequest = callback ? callback( asyncReadToken, eAsyncResult_Success, nbytes ) : eCallbackRequest_Finish;

#ifdef RED_PROFILE_FILE_SYSTEM
		if ( asyncOp.m_operationId )
		{
			RedIOProfiler::ProfileAsyncIOReadEnd( asyncOp.m_operationId );
		}
#endif

		if ( callbackRequest == eCallbackRequest_Finish )
		{
			break;
		}
		else if ( callbackRequest == eCallbackRequest_More )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			const Uint32 asyncOpId = RedIOProfiler::ProfileAllocRequestId();
			asyncOp.m_operationId = asyncOpId;
			RedIOProfiler::ProfileAsyncIOReadScheduled( asyncOp.m_asyncFile->GetFileHandle(), asyncOpId, asyncReadToken.m_offset, asyncReadToken.m_numberOfBytesToRead, asyncOp.m_ioTag );
#endif

			continue;
		}
		else if ( callbackRequest == eCallbackRequest_Defer )
		{
			// FIXME: for the moment
			REDIO_ERR( TXT("Failed to defer callback.") );
			if ( callback )
			{
				callback( asyncReadToken, eAsyncResult_Canceled, 0 );
			}
			break;
		}
		else
		{
			RED_HALT( "Unexpected control flow" );
			break;
		}
	}
}

REDIO_GENERICAPI_NAMESPACE_END

