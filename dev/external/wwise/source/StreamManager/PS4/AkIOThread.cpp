//////////////////////////////////////////////////////////////////////
//
// AkIOThread.h
//
// Platform-specific layer of high-level I/O devices of the Stream Manager.
// Implements the I/O thread function, manages all thread related 
// synchronization.
// Specific to POSIX API.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkIOThread.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <errno.h>
#include <sys/time.h>


using namespace AK::StreamMgr;

//-----------------------------------------------------------------------------
    // Name: class CAkClientThreadAware
    // Desc: Platform-specific layer for stream tasks which handles blocking client
	//		 threads for blocking I/O.
	//		 Client threads are blocked using a conditional variable
	//		 and signaling it for a specific thread (sys_cond_signal_to).
    //-----------------------------------------------------------------------------

CAkClientThreadAware::CAkClientThreadAware()
: m_idBlockedThread( AK_NULL_THREAD )
{
}

CAkClientThreadAware::~CAkClientThreadAware()
{
	AKASSERT( !IsBlocked() );
}

// Set task as "blocked, waiting for I/O completion".
void CAkClientThreadAware::SetBlockedStatus() 
{ 
	// Get current thread's id. Having a non-zero m_idBlockedThread indicates that this stream is waiting for a signal.
	AKASSERT( m_idBlockedThread == AK_NULL_THREAD );
	m_idBlockedThread = pthread_self();	
}


//-----------------------------------------------------------------------------
// Name: CAkIOThread
// Desc: Implements the I/O thread, and synchronization services. Namely,
//		 thread control based on standard and automatic streams semaphores,
//		 memory full, max number of concurrent low-level transfers. It also 
//		 works with client thread blocking (partly implemented in 
//		 CAkClientThreadAware).
//       The I/O thread calls pure virtual method PerformIO(), that has to be
//       implemented by derived classes according to how they communicate with
//       the Low-Level IO.
//-----------------------------------------------------------------------------

// Device construction/destruction.
CAkIOThread::CAkIOThread()
: m_hIOThread( NULL )
, m_mutexPendingStmsSem( NULL )
, m_condAreTasksPending( NULL )
, m_cPendingStdStms( 0 )
, m_cRunningAutoStms( 0 )
, m_bDoWaitMemoryChange( false )
#ifdef _DEBUG
, m_bIsAutoSemSignaled( false )
#endif
, m_uNumConcurrentIO( 0 )
, m_mutexBlockingIO( NULL )
, m_condBlockingIODone( NULL )
{	
	AKPLATFORM::AkClearThread( &m_hIOThread );
}

CAkIOThread::~CAkIOThread()
{
}

// Init/term scheduler objects.
AKRESULT CAkIOThread::Init( 
    const AkThreadProperties & in_threadProperties // Platform-specific thread properties. Can be NULL (uses default).
    )
{
	// Initialize client thread blocking synchronization objects.
	{
		ScePthreadMutexattr mutex_attr;
		AKVERIFY(! scePthreadMutexattrInit( &mutex_attr ));
		
		int err = scePthreadMutexInit( &m_mutexBlockingIO, &mutex_attr, NULL );
		AKVERIFY(! scePthreadMutexattrDestroy(&mutex_attr) );
		if ( err != 0 )
			return AK_Fail;

		ScePthreadCondattr cond_attr;
		AKVERIFY(!scePthreadCondattrInit( &cond_attr ));
		
		err = scePthreadCondInit( &m_condBlockingIODone, &cond_attr, NULL );
		AKVERIFY(! scePthreadCondattrDestroy(&cond_attr) );
		if ( err != 0 )
			return AK_Fail;
	}

	// Create scheduler semaphore.
	{
		ScePthreadMutexattr mutex_attr;

		AKVERIFY(!scePthreadMutexattrInit( &mutex_attr ));
		
		AKVERIFY(!scePthreadMutexattrSettype( &mutex_attr, PTHREAD_MUTEX_RECURSIVE ));

		ScePthreadCondattr cond_attr;
		AKVERIFY(!scePthreadCondattrInit( &cond_attr ));

		int err = scePthreadMutexInit( &m_mutexPendingStmsSem, &mutex_attr, NULL );
		AKVERIFY(! scePthreadMutexattrDestroy(&mutex_attr));
		if ( err != 0 )
			return AK_Fail;
			
		err = scePthreadCondInit( &m_condAreTasksPending, &cond_attr, NULL );
		AKVERIFY(! scePthreadCondattrDestroy(&cond_attr));
		if ( err != 0 )
			return AK_Fail;
	}

	m_cPendingStdStms	= 0;
    m_cRunningAutoStms	= 0;
	m_uNumConcurrentIO	= 0;
    
    // Launch the scheduler/IO thread.
    // Create and start the worker IO thread with default stack size.
	
	m_bDoRun = true;
	AKPLATFORM::AkCreateThread(
		CAkIOThread::IOSchedThread,			// Start address
		this,								// Parameter
		in_threadProperties,				// Properties
		&m_hIOThread,						// Handle
		"AK::IOThread" );	

	if ( AKPLATFORM::AkIsValidThread( &m_hIOThread ) )
		return AK_Success;		
	return AK_Fail;
}

void CAkIOThread::Term()
{
	// It is ok that the functions return EINVAL 
	// because we might pass twice in the Term function
	int ret;

	if (m_mutexPendingStmsSem)
	{
		ret = scePthreadMutexLock( &m_mutexPendingStmsSem );
		AKASSERT( ret == 0 || ret == EINVAL );
		m_bDoRun = false;
		ret = scePthreadCondSignal(&m_condAreTasksPending);
		AKASSERT( ret == 0 || ret == EINVAL );
		ret = scePthreadMutexUnlock( &m_mutexPendingStmsSem );
		AKASSERT( ret == 0 || ret == EINVAL );
	}
	
	
	if ( AKPLATFORM::AkIsValidThread( &m_hIOThread ) )
    {
		AKPLATFORM::AkWaitForSingleThread( &m_hIOThread );
		m_hIOThread = AK_NULL_THREAD;
	}
	
	if (m_mutexPendingStmsSem)
	{
		ret = scePthreadMutexDestroy( &m_mutexPendingStmsSem );
		AKASSERT( ret == 0 || ret == EINVAL );
		m_mutexPendingStmsSem = NULL;
	}

	if (m_condAreTasksPending)
	{
		ret = scePthreadCondDestroy( &m_condAreTasksPending );
		AKASSERT( ret == 0 || ret == EINVAL );
		m_condAreTasksPending = NULL;
	}

	if (m_mutexBlockingIO)
	{
		ret = scePthreadMutexDestroy( &m_mutexBlockingIO );
		AKASSERT( ret == 0 || ret == EINVAL );
		m_mutexBlockingIO = NULL;
	}

	if (m_condBlockingIODone)
	{
		ret = scePthreadCondDestroy( &m_condBlockingIODone );
		AKASSERT( ret == 0 || ret == EINVAL );
		m_condBlockingIODone = NULL;
	}
}

// Scheduler thread.
// --------------------------------------------------------
AK_DECLARE_THREAD_ROUTINE( CAkIOThread::IOSchedThread )
{
    CAkIOThread * pDevice = AK_GET_THREAD_ROUTINE_PARAMETER_PTR( CAkIOThread );

	struct timespec   ts;
	struct timeval    tp;
	
	pDevice->OnThreadStart();
    
    while ( true )
    {
		// Enter critical section.
		// The I/O thread checks if tasks are waiting to be serviced. If there are none, wait for CondVar signal.
		pDevice->Lock();
		
		while ( !pDevice->CanSchedule()
				&& pDevice->m_bDoRun )
		{
			int ret = scePthreadCondWait( &pDevice->m_condAreTasksPending,  &pDevice->m_mutexPendingStmsSem );
			AKASSERT( ret == 0 || !"Error while waiting for condition variable" );
		}

		bool bStop = !( pDevice->m_bDoRun );
		
		pDevice->Unlock();
		
		if ( bStop )
		{
			if ( pDevice->ClearStreams() )
			{
				AkExitThread(AK_RETURN_THREAD_OK);
			}
			else
				AKPLATFORM::AkSleep( 100 );
		}

		// Schedule.
        pDevice->PerformIO();
    }
}

//
// Methods used by stream objects: 
// Scheduler thread control.
// -----------------------------------

// Increment pending standard streams count.
// Sync: Standard streams semaphore interlock.
void CAkIOThread::StdSemIncr()
{
    AkAutoLock<CAkIOThread> chgSem( *this );
	
	if ( ++m_cPendingStdStms == 1 )
	{
		// We just incremented it from 0 to 1. Signal condition variable.
		AKVERIFY( scePthreadCondSignal( &m_condAreTasksPending ) == 0 );
	}
}

// Decrement pending standard streams count.
// Sync: Standard streams semaphore interlock.
void CAkIOThread::StdSemDecr()
{
    AkAutoLock<CAkIOThread> chgSem( *this );
	AKASSERT( m_cPendingStdStms > 0 );
    --m_cPendingStdStms;
}

// Increment pending automatic streams count.
// Note: Event remains unsignaled if there is no memory ("Mem Idle" state).
// Sync: Streams semaphore lock (mutex). Signals CondVar if I/O thread was idle.
void CAkIOThread::AutoSemIncr()
{
	AkAutoLock<CAkIOThread> chgSem( *this );

	m_cRunningAutoStms++;
    if ( m_cRunningAutoStms == 1 &&
         !m_bDoWaitMemoryChange )
    {
        // We just incremented it from 0 to 1 and memory is available. 
		// Signal CondVar to release the I/O if necessary.
#ifdef _DEBUG
        m_bIsAutoSemSignaled = true;
#endif
		AKVERIFY( scePthreadCondSignal( &m_condAreTasksPending ) == 0 );
    }
}

// Decrement pending automatic streams count.
// Sync: Automatic streams semaphore lock.
void CAkIOThread::AutoSemDecr()
{
	AkAutoLock<CAkIOThread> chgSem( *this );

    AKASSERT( m_cRunningAutoStms > 0 );
    m_cRunningAutoStms--;    
#ifdef _DEBUG
	// We just decremented it from 1 to 0. I/O thread will stop performing I/O on automatic streams.
    m_bIsAutoSemSignaled = ( m_cRunningAutoStms > 0 );
#endif
}

// Notify that memory was freed, or memory usage must be reviewed.
// Un-inhibates automatic streams semaphore. Event is signaled if pending automatic streams count is greater than 0.
// IMPORTANT Sync: None. Locking must be handled on the caller side, to enclose calls to Memory Manager
// and protect automatic streams semaphore. Tasks use LockMem().
void CAkIOThread::NotifyMemChange()
{
    if ( m_bDoWaitMemoryChange )
    {
        m_bDoWaitMemoryChange = false;
        if ( m_cRunningAutoStms > 0 )
        {
            // Auto streams are running and we just notified that some memory is available. 
			// Signal CondVar to release the I/O if necessary.
#ifdef _DEBUG
            m_bIsAutoSemSignaled = true;
#endif
			AKVERIFY( scePthreadCondSignal( &m_condAreTasksPending ) == 0 );
        }
    }
}

// Notify that memory is idle. I/O thread should not wake up to service automatic streams until memory usage
// changes (until someone calls NotifyMemChange).
// Inhibates automatic streams semaphore.
// IMPORTANT Sync: None. Locking must be handled on the caller side, to enclose calls to Memory Manager
// and protect automatic streams semaphore. Tasks use LockMem().
void CAkIOThread::NotifyMemIdle()
{
    m_bDoWaitMemoryChange = true;
#ifdef _DEBUG
	if ( m_cRunningAutoStms > 0 )
    {
        // Auto streams are running but we just notified that memory is not available. I/O thread will stop
		// performing I/O on automatic streams.
        m_bIsAutoSemSignaled = false;
    }
#endif
}

// Concurrent I/O requests.
// --------------------------------------------------------
void CAkIOThread::IncrementIOCount()
{
	AkAutoLock<CAkIOThread> chgSem( *this );
	++m_uNumConcurrentIO;
}

void CAkIOThread::DecrementIOCount()
{
	AkAutoLock<CAkIOThread> chgSem( *this );
	--m_uNumConcurrentIO;
	AKVERIFY( scePthreadCondSignal( &m_condAreTasksPending ) == 0 );
}

// Blocking I/O.
// --------------------------------------------------------
void CAkIOThread::WaitForIOCompletion( 
	CAkClientThreadAware * in_pWaitingTask
	)
{
	AKVERIFY( scePthreadMutexLock( &m_mutexBlockingIO ) == 0 );

	// Now that we aquired the mutex, check if the task is still waiting for I/O (its thread ID would be NULL).
	// Since all blocked threads are awaken when a blocking I/O completes, they need to check their status again
	// (BlockedThreadID() not being null) before leaving.
	while ( in_pWaitingTask->BlockedThreadID() != AK_NULL_THREAD )
	{
		// Still waiting for I/O: wait on conditional variable signal.
		AKVERIFY( scePthreadCondWait( &m_condBlockingIODone, &m_mutexBlockingIO ) == 0 );		
	}

	AKVERIFY( scePthreadMutexUnlock( &m_mutexBlockingIO ) == 0 );
}

void CAkIOThread::SignalIOCompleted(
	CAkClientThreadAware * in_pWaitingTask	// The caller's thread id must be checked after the mutex was obtained to avoid race conditions.
	)								// CAkStmTask::ClearBlockedStatus() will be called from within the critical section.
{
	AKVERIFY( scePthreadMutexLock( &m_mutexBlockingIO ) == 0 );

	// Now that we aquired the mutex, release the blocked thread. 
	AKASSERT( in_pWaitingTask->BlockedThreadID() != AK_NULL_THREAD );

	// Clear io_idBlockedThread: done inside locks to avoid race conditions with the client thread requesting
	// another blocking transfer.
	in_pWaitingTask->ClearBlockedStatus();

	// We cannot signal a specific thread. Thus, we signal all threads waiting for this variable. Only
	// the thread stuck in the task for which we cleared the BlockedStatus above will be released. Others will
	// go back to sleep.
	int eSignalResult = scePthreadCondBroadcast( &m_condBlockingIODone ); // balary TODO
	AKASSERT( eSignalResult == 0 );	
	
	AKVERIFY( scePthreadMutexUnlock( &m_mutexBlockingIO ) == 0 );
}

