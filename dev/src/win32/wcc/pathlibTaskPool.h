/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/pathlibGenerationManagerBase.h"

///////////////////////////////////////////////////////////////////////////////
class CPathLibTaskPool : public PathLib::IGenerationManagerBase
{
protected:
	class CManagerThread : public Red::Threads::CThread
	{
	protected:
		CPathLibTaskPool&								m_owner;

		Bool		RunTask( CAsyncTask* task );
	public:

		CManagerThread( CPathLibTaskPool& owner );

		void		ThreadFunc() override;
	};


	typedef Red::Threads::CMutex						Mutex;
	typedef Red::Threads::CSemaphore					Semaphore;
	typedef Red::Threads::CConditionVariable			Condition;
	typedef Red::Threads::CScopedLock< Mutex >			Lock;

	static const Uint32 MAX_ACTIVE_TASKS = 8;									// TODO: make it dependent on project settings

	CManagerThread*										m_thread;

	Mutex												m_mutex;
	TDynArray< CAsyncTask* >							m_pendingTasks;
	TStaticArray< CAsyncTask*, MAX_ACTIVE_TASKS >		m_activeTasks;
	TStaticArray< CAsyncTask*, MAX_ACTIVE_TASKS >		m_finishedTasks;

	Semaphore											m_actionsSemaphore; // count of pending tasks + 
	Semaphore											m_freeTasksSemaphore;

	Mutex												m_syncProcessingSection;

	Condition											m_allTasksCompleted;

	Uint32												m_concurrentTaskLimit;
	Bool												m_shutdownRequest;

	Bool			CanRunTask()												{ return m_activeTasks.Size() < m_concurrentTaskLimit && !m_pendingTasks.Empty(); }

public:
	CPathLibTaskPool();
	~CPathLibTaskPool();

	// ITaskManagerBase interface
	void			OnTaskFinished( CAsyncTask* task ) override;

	// specific interface
	void			StartProcessing( Uint32 taskLimit = MAX_ACTIVE_TASKS );
	void			InterruptProcessing();

	void			ProcessAllPendingTasks();
	void			CompleteProcessing();
	void			AddTask( CAsyncTask* task );
	void			Shutdown()													{ InterruptProcessing(); }

	void			RequestSynchronousProcessing() override;
	void			FinishSynchronousProcessing() override;

};
