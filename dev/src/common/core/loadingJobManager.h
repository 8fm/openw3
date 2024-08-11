/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "loadingJob.h"

class ILoadJob;
class ILoadingJobMonitor;
class CLoadingJobThread;

/// Specific job manager only for the loading jobs
/// This is a relic from the old job system
class CLoadingJobManager
{
public:
	CLoadingJobManager();
	~CLoadingJobManager();

	//! Is locked ?
	RED_INLINE Bool IsLocked() const { return m_isLocked.GetValue(); }

	//! Is the job manager idle ?
	RED_INLINE Bool IsIdle() const { return m_isIdle.GetValue(); }

	//! Is the job manager blocking garbage collector ?
	RED_INLINE Bool IsBlockingGC() const { return m_isBlockingGC.GetValue(); }

	//! Initialize job manager
	void InitializeThreads();

	//! Shutdown job manager
	void ShutDownThreads();

	//! Issue single loading
	void Issue( ILoadJob* job );

	//! Flush all pending jobs up to given priority
	void FlushPendingJobs( EJobPriority priorityHigherOrEqualThan = JP_MAX );

	//! Lock the manager - will prevent processing any jobs
	void Lock();

	//! Unlock manager
	void Unlock();

	//! Flush until there are no jobs being processed
	void FlushProcessingJobs();

	//! Attach/detach job manager monitor
	void AttachMonitor( ILoadingJobMonitor* monitorInterface );

private:
	//! Pop a job, called by CJobThread
	ILoadJob* PopJob( const Uint32 jobAffinityMask );

	//! Called by job thread to inform that job was finished
	void JobFinished( ILoadJob* job, EJobResult result );

	//! Internal reporting
	void ReportJobIssued( ILoadJob* job );
	void ReportJobStarted( ILoadJob* job );
	void ReportJobFinished( ILoadJob* job, const EJobResult result );
	void ReportJobCanceled( ILoadJob* job );

	// Internal flags
	Red::Threads::CAtomic< Bool >	m_isLocked;
	Red::Threads::CAtomic< Bool >	m_isIdle;
	Red::Threads::CAtomic< Bool >	m_isBlockingGC;

	// Internal lock states
	Red::Threads::CAtomic< Int32 >	m_lockCount;			//!< Lock on the job manager - will stop processing any new jobs
	Red::Threads::CAtomic< Int32 >	m_processingCount;		//!< Number of jobs being processed right now
	Red::Threads::CAtomic< Int32 >	m_gcBlockCount;			//!< Number of jobs blocking garbage collector
	Red::Threads::CAtomic< Int32 >	m_issueCounter;			//!< Number of jobs issued so far

	// Job list - per priority
	Red::Threads::CMutex			m_lock;
	Red::Threads::CSemaphore		m_queueCount;
	TDynArray< ILoadJob* >			m_queueJobs[ JP_MAX ];
	Red::Threads::CAtomic< Bool >	m_queueEmpty[ JP_MAX ];

	// Loading thread
	TDynArray< CLoadingJobThread* >	m_threads;

	// Loading job monitor
	ILoadingJobMonitor*				m_monitor;

	// Lock/Unlock GC
	void LockGC();
	void UnlockGC();

	friend class CLoadingJobThread;
	friend class CObjectGCLocker;
};

/// Loading manager
typedef TSingleton< CLoadingJobManager > SJobManager;