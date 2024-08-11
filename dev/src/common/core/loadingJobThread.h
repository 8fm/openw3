/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CLoadingJobManager;

/// Thread for the loading manager
class CLoadingJobThread : public Red::Threads::CThread
{
public:
	CLoadingJobThread( CLoadingJobManager* manager, const Uint32 jobAffinityMask );

	virtual void ThreadFunc();
	void SetKillFlag();

protected:
	CLoadingJobManager*	m_manager;				//!< Job manager (owner)
	Uint32				m_jobAffinityMask;		//!< Affinity mask for jobs
	volatile Bool		m_killFlag;				//!< Set this flag to kill the thead
};
