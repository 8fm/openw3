/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "loadingJob.h"
#include "loadingJobThread.h"
#include "loadingJobManager.h"
#include "loadingJobMonitor.h"

#include "configVar.h"
#include "messagePump.h"

#ifdef RED_PLATFORM_WINPC
#include <sysinfoapi.h>
#elif RED_PLATFORM_LINUX
#include <unistd.h>
#endif

namespace Helper
{
	Uint32 GetNumberOfUsableCores()
	{
#ifdef RED_PLATFORM_WINPC
		SYSTEM_INFO sysinfo;
		::GetSystemInfo( &sysinfo );

		Uint32 result = sysinfo.dwNumberOfProcessors;
		return result;
#elif defined(RED_PLATFORM_DURANGO)
		return 6;
#elif defined(RED_PLATFORM_ORBIS)
		return 6;
#elif defined(RED_PLATFORM_LINUX)
		Uint32 result = (Uint32)sysconf( _SC_NPROCESSORS_ONLN );
		return result;
#else
#error Unknown platform in Helper::GetNumberOfCores()
#endif
	}
}

// W3HACK needed to prevent infinite loading screen for TT#117688. Scene loading while loading world,
// scene system is flushing loading jobs while the job manager is locked. Need to force unlock it, hopefully
// without messing up normal lock refcounting.
// DO NOT use this for anything else!
Bool GW3Hack_UnlockOverride = false;


///////////////////////////////////////////////////////////////

namespace Config
{
	TConfigVar< Bool >		cvDebugForceSyncJobs( "Debug", "DebugForceSyncJobs", false );
}

///////////////////////////////////////////////////////////////

CLoadingJobManager::CLoadingJobManager()
	: m_isLocked( false )
	, m_isIdle( true )
	, m_isBlockingGC( false )
	, m_lockCount( 0 )
	, m_processingCount( 0 )
	, m_gcBlockCount( 0 )
	, m_queueCount( 0, INT_MAX )
	, m_monitor( nullptr )
	, m_issueCounter( 0 )
{
	// reset queue empty flags
	for ( Uint32 i=0; i<JP_MAX; ++i )
		m_queueEmpty[ i ].SetValue( true );
}

CLoadingJobManager::~CLoadingJobManager()
{

}

void CLoadingJobManager::InitializeThreads()
{
#ifndef RED_FULL_DETERMINISM

	Bool useAffinityMasks = true;
	const Uint32 numThreads = 1; // 1 generic 2 extra for streaming in stuff
	const Uint32 numAfinityMasks = 2;

#ifdef RED_PLATFORM_CONSOLE
# if defined( RED_PLATFORM_DURANGO )
	Red::Threads::TAffinityMask affinityMasks[numAfinityMasks] = { (1 << 4) | (1 << 5) | (1 << 6) };
#elif defined( RED_PLATFORM_ORBIS )
	Red::Threads::TAffinityMask affinityMasks[numAfinityMasks] = { (1 << 4) | (1 << 5) };
# else
#  error Unsupported console!
# endif
#endif

	const Uint32 threadMasks[] = { 0xFFFFFFFF };

	for ( Uint32 i=0; i<numThreads; ++i )
	{
		CLoadingJobThread* thread = new CLoadingJobThread( this, threadMasks[i] );
		thread->InitThread();

#ifdef RED_PLATFORM_CONSOLE
		if ( i < numAfinityMasks )
		{
			thread->SetAffinityMask( affinityMasks[i] );
		}

		thread->SetPriority( Red::Threads::TP_Highest );
#endif

		m_threads.PushBack( thread );
	}
#else
	// No threads created
	LOG_CORE( TXT("No job manager threads were created") );
#endif
}

void CLoadingJobManager::ShutDownThreads()
{
	// Lock the job execution
	Lock();

	// Finish all current task
	FlushProcessingJobs();

	// Stop threads
	for ( Uint32 i=0; i<m_threads.Size(); ++i )
	{
		// Try to exit the loop
		m_threads[i]->SetKillFlag();
		m_queueCount.Release(100);

		// Wait for the thread to end
		m_threads[i]->JoinThread();

		// Cleanup
		delete m_threads[i];
	}

	// done
	m_threads.ClearFast();
}

void CLoadingJobManager::ReportJobIssued( ILoadJob* job )
{
	job->ReportIssued();

	if ( m_monitor )
		m_monitor->OnJobIssued( job );
}

void CLoadingJobManager::ReportJobStarted( ILoadJob* job )
{
	job->ReportStarted();

	if ( m_monitor )
		m_monitor->OnJobStarted( job );
}

void CLoadingJobManager::ReportJobFinished( ILoadJob* job, const EJobResult result )
{
	job->ReportFinished( result );

	if ( m_monitor )
		m_monitor->OnJobFinished( job, result );
}

void CLoadingJobManager::ReportJobCanceled( ILoadJob* job )
{
	job->ReportCanceled();

	if ( m_monitor )
		m_monitor->OnJobCanceled( job );
}

void CLoadingJobManager::Issue( ILoadJob* job )
{
	RED_FATAL_ASSERT( job != nullptr, "NULL job is not schedulable" );

	// Synchronous loading option
	if ( m_threads.Empty() || Config::cvDebugForceSyncJobs.Get() )
	{
		ReportJobIssued( job );
		ReportJobStarted( job );
		const EJobResult result = job->Process();
		ReportJobFinished( job, result );
		return;
	}

	// Change internal state
	ReportJobIssued( job );

	// Add the job to the list
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		// add the job to the appropriate bucket based on it's priority
		const EJobPriority priority = job->GetPriority();
		RED_FATAL_ASSERT( (Int32)priority < (Int32)JP_MAX, "Invalid priority value jor job" );
		m_queueJobs[ priority ].PushBack( job );
		m_queueEmpty[ priority ].SetValue( false );

		// Add internal reference
		job->AddRef();
	}

	// Report issued job
	m_issueCounter.Increment();

	// Release the thread
	m_queueCount.Release(1);
}

void CLoadingJobManager::FlushPendingJobs( EJobPriority priorityHigherOrEqualThan /*= JP_MAX*/ )
{
	// calling the flush is allowed only on main thread
	if ( !::SIsMainThread() )
	{
		ERR_CORE( TXT("FlushPendingJobs called from non-main thread. This is not supported.") );
		return;
	}

	// loop until all jobs are done
	Bool hasPendingJobs = false;
	do
	{
		hasPendingJobs = false;
		for ( Uint32 i=0; i < (Uint32)priorityHigherOrEqualThan; ++i )
		{
			if ( !m_queueEmpty[i].GetValue() )
			{
				hasPendingJobs = true;
				break;
			}
		}

		// no more jobs in the queue - finish what we currently have
		if ( !hasPendingJobs )
		{
			FlushProcessingJobs(); 
		}

		PUMP_MESSAGES_DURANGO_CERTHACK();
	}
	// there are no new jobs and no new jobs were submitted
	while ( hasPendingJobs || m_issueCounter.Exchange( 0 ) != 0 ); 
}

void CLoadingJobManager::Lock()
{
	if ( 1 == m_lockCount.Increment() )
	{
		RED_FATAL_ASSERT( m_isLocked.GetValue() == false, "Internal state inconsistency" );
		m_isLocked.SetValue( true );
	}
}

void CLoadingJobManager::Unlock()
{
	if ( 0 == m_lockCount.Decrement() )
	{
		RED_FATAL_ASSERT( m_isLocked.GetValue() == true, "Internal state inconsistency" );
		m_isLocked.SetValue( false );
	}
}

void CLoadingJobManager::FlushProcessingJobs()
{
	if ( !::SIsMainThread() )
	{
		ERR_CORE( TXT("FlushProcessingJobs called from non-main thread. This is not supported.") );
		return;
	}

	// Disable job pumping
	Lock();

	// Wait until all jobs are processed
	while ( !IsIdle() ) 
	{
		PUMP_MESSAGES_DURANGO_CERTHACK();
		Red::Threads::SleepOnCurrentThread(1);
	}

	// Enable job pumping, if there's an external Lock job queue will remain empty
	Unlock();
}

ILoadJob* CLoadingJobManager::PopJob( const Uint32 jobAffinityMask )
{
	// manager is locked - no new jobs can be scheduled
	if ( IsLocked() && !GW3Hack_UnlockOverride )
		return nullptr;

	// wait for jobs
	m_queueCount.Acquire();

	// get the best job - check the priority queues
	bool hasJobs = false;
	ILoadJob* job = nullptr;
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

		for ( Uint32 i=0; i<JP_MAX; ++i )
		{
			// is this a 
			auto& queue = m_queueJobs[i];
			if ( !queue.Empty() )
			{
				hasJobs = true;

				// do we match thread affinity mask ?
				if ( 0 == (jobAffinityMask & (1 << i)) )
					continue; // job is not for this thread

				job = queue.Front();
				queue.Erase( m_queueJobs[i].Begin() );

				// last job was popped
				if ( queue.Empty() )
					m_queueEmpty[ i ].SetValue( true );

				// use the job
				break;
			}
		}
	}
	
	// no job fetched, reschedule the waking up of the thread
	if ( !job )
	{
		// we have jobs but not matching this thread
		if ( hasJobs )
		{
			Red::Threads::SleepOnCurrentThread(1);
			m_queueCount.Release(1); // retry
		}
	}

	// no job situation can actually happen
	if ( job )
	{
		if ( job->IsCanceled() )
		{
			// Cancel the job
			ReportJobCanceled( job );
			job->Release();
			job = nullptr;
		}
		else
		{
			// Update job status
			ReportJobStarted( job );

			// We have started processing a job
			if ( 1 == m_processingCount.Increment() )
				m_isIdle.SetValue( false );

			// Count GC blocking jobs
			if ( job->IsGCBlocker() )
			{
				LockGC();
			}
		}
	}
	

	// no jobs found
	return job;
}

void CLoadingJobManager::JobFinished( ILoadJob* job, EJobResult result )
{
	// Report that job has finished
	ReportJobFinished( job, result );

	// Decrement processing counter
	if ( 0 == m_processingCount.Decrement() )
		m_isIdle.SetValue( true );

	// Count IO jobs
	if ( job->IsGCBlocker() )
	{
		UnlockGC();
	}

	// Release internal handle to the job
	job->Release();
}

void CLoadingJobManager::AttachMonitor( ILoadingJobMonitor* monitorInterface )
{
	m_monitor = monitorInterface;
}

void CLoadingJobManager::LockGC()
{
	if ( 1 == m_gcBlockCount.Increment() )
	{
		m_isBlockingGC.SetValue( true );
	}
}

void CLoadingJobManager::UnlockGC()
{
	if ( 0 == m_gcBlockCount.Decrement() )
	{
		m_isBlockingGC.SetValue( false );
	}
}