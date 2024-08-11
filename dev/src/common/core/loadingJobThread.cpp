/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "loadingJob.h"
#include "loadingJobThread.h"
#include "loadingJobManager.h"
#include "fileSystemProfilerWrapper.h"

///////////////////////////////////////////////////////////////

Bool RED_TLS GIsLoadingJobThread;

CLoadingJobThread::CLoadingJobThread( CLoadingJobManager* manager, const Uint32 jobAffinityMask )
	: CThread( "LoadingJobThread" )
	, m_jobAffinityMask( jobAffinityMask )
	, m_manager( manager )
	, m_killFlag( false )
{
}

void CLoadingJobThread::ThreadFunc()
{
	Memory::RegisterCurrentThread();

#ifdef RED_PLATFORM_DURANGO
	PROCESSOR_NUMBER nProc;
	{
		nProc.Group = 0;
		nProc.Number = 6;
		nProc.Reserved = 0;
	}
	::SetThreadIdealProcessorEx( ::GetCurrentThread(), &nProc, nullptr );
#endif

	GIsLoadingJobThread = true;

	// Process the jobs
	while ( !m_killFlag )
	{
		// Pop the job
		ILoadJob* job = m_manager->PopJob( m_jobAffinityMask );
		if ( job )
		{
			// Profiling
#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileLoadingJobStart( job->GetDebugName(), (Uint32)job->GetPriority() );
#endif

			// Process job
			const EJobResult result = job->Process();

			// Profiling
#ifdef RED_PROFILE_FILE_SYSTEM
			RedIOProfiler::ProfileLoadingJobEnd();
#endif

			// Process job result
			m_manager->JobFinished( job, result );
		}
		else
		{
			// Transient condition - we cannot process the jobs right now, limit the CPU usage
			Red::Threads::SleepOnCurrentThread( 1 );
		}
	}
}

void CLoadingJobThread::SetKillFlag()
{
	m_killFlag = true;
}

///////////////////////////////////////////////////////////////
