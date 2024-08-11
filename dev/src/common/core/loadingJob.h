/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "engineTime.h"

///////////////////////////////////////////////////////////////

/// Internal job state
enum EJobState
{
	JOB_Initialized,		//!< Job was initialized ( created ) but not yet added to the queue
	JOB_Pending,			//!< Job is waiting in the job queue
	JOB_Canceled,			//!< Job was canceled
	JOB_Failed,				//!< Job failed ( internally )
	JOB_Processing,			//!< Job is being processed
	JOB_Finished,			//!< Job has finished
};

/// Internal job processing result
enum EJobResult
{
	JR_Failed,				//!< We have failed to process the job
	JR_Finished,			//!< We finished processing the job
};

/// Priorities for jobs. Keeped globally here to maintain clearness
enum EJobPriority
{	
	JP_Immediate,
	JP_Cutscene,
	JP_Video,
	JP_SoundStream,
	JP_EnvProbe,
	JP_SpawnEntity,
	JP_StreamingResource,
	JP_StreamingNPC,
	JP_StreamingObject,
	JP_Mesh,
	JP_Animset,
	JP_Animation,
	JP_Resources,
	JP_Speech,
	JP_Default,
	JP_Texture,
	JP_SpawnEntityBackground,
	JP_PathLib,

	JP_MAX,
};

///////////////////////////////////////////////////////////////

/// Generic job class
class ILoadJob : public Red::System::NonCopyable
{
public:
	//! Get job priority
	RED_INLINE EJobPriority GetPriority() const { return (EJobPriority) m_priority; }

	//! Is this job a GC blocker ?
	RED_INLINE Bool IsGCBlocker() const { return m_isGCBlocker; }

	//! Is this job canceled
	RED_INLINE Bool IsCanceled() const { return m_isCanceled; }

	ILoadJob( EJobPriority priority, const Bool blockGC = false );
	virtual ~ILoadJob();

	//! Add internal reference
	void AddRef();

	//! Remove internal reference
	void Release();

	//! Cancel the job
	void Cancel();

	//! Returns true if job has reached it's final state ( finished, failed or canceled )
	Bool HasEnded() const;

	//! Returns true if job was finished without errors
	Bool HasFinishedWithoutErrors() const;

	//! Job debug information
	virtual const Char* GetDebugName() const { return TXT("Loading job"); }

	void* operator new( size_t size );	
	void  operator delete( void* ptr );	

protected:
	//! Process the job, is called from job thread
	virtual EJobResult Process()=0;

private:
	Red::Threads::CAtomic< Int32 >	m_refCount;			//!< Internal reference count
	Red::Threads::CAtomic< Uint32 >	m_state;			//!< Internal job state
	Uint8							m_priority;			//!< Job priority
	Uint8							m_isCanceled : 1;	//!< This job was canceled
	Uint8							m_isGCBlocker : 1;	//!< This job is a blocker for garbage collector

	void ReportIssued();
	void ReportStarted();
	void ReportFinished( const EJobResult result );
	void ReportCanceled();

	friend class CLoadingJobManager;
	friend class CLoadingJobThread;
};

///////////////////////////////////////////////////////////////
