#pragma once

class CPathLibWorld;

#include "../core/heap.h"
#include "../core/refCountPointer.h"

#ifndef NO_EDITOR
#define PROFILE_PATHLIB_TASKS
#endif

namespace PathLib
{

class CTaskManager
{
protected:

	typedef Red::Threads::CMutex					Mutex;
	typedef Red::Threads::CScopedLock< Mutex >		Lock;

public:

	class CAsyncTask
	{
		friend class CTaskManager;
	protected:
		enum ETaskType
		{
			T_MODYFICATION						= FLAG( 0 ),
			T_PATHFIND							= FLAG( 1 ),
			T_QUERY								= FLAG( 2 ),

			T_ALL								= T_MODYFICATION | T_PATHFIND | T_QUERY
		};
	
		enum EFlags
		{
			FLAG_USE_PREPROCESSING				= FLAG( 0 ),
			FLAG_USE_POSTPROCESSING				= FLAG( 1 ),
			FLAGS_DEFAULT						= 0
		};

		enum class EPriority
		{
			LayerVisibilityProcessing,
			PlotPath,
			ReachabilityQuery,
			RemoveImmediateObstacle,
			ProcessImmediateObstacle,
			ObstacleProcessing,
			PostStreaming,
			WalkableSpotQuery,
			UnloadStreamingItems,
		};

		CTaskManager&					m_taskManager;
		Red::Threads::CAtomic< Int32 >	m_lifeRefCount;
		ETaskType						m_taskType;
		Uint32							m_taskFlags;
		EPriority						m_priority;

	public:
		typedef TRefCountPointer< CAsyncTask > Ptr;

		struct TaskOrder
		{
			static Bool			Less( const CAsyncTask::Ptr& t0, const CAsyncTask::Ptr& t1 )	{ return t0->m_priority > t1->m_priority; }
		};

								CAsyncTask( PathLib::CTaskManager& taskManager, ETaskType taskType, Uint32 flags, EPriority priority );
		virtual					~CAsyncTask() {}

		// subclass interface task 
		virtual Bool			PreProcessingSynchronous();
		virtual void			Process();
		virtual void			PostProcessingSynchronous();
		virtual void			DescribeTask( String& outName ) const;

		// main task async implementation
		void					Run();

		Bool					IsUsingPreprocessing() const						{ return (m_taskFlags & FLAG_USE_PREPROCESSING) != 0; }
		Bool					IsUsingPostprocessing() const						{ return (m_taskFlags & FLAG_USE_POSTPROCESSING) != 0; }

		void					Release();
		void					AddRef();
	};

	class CPathLibThread : public Red::Threads::CThread
	{
	protected:
		CTaskManager&						m_taskManager;
		CAsyncTask::Ptr						m_activeTask;
		Red::Threads::CAtomic< Bool >		m_isTaskRunning;
		Red::Threads::CAtomic< Bool >		m_shutdownRequested;
		Red::Threads::CMutex				m_mutex;

	public:
		Red::Threads::CConditionVariable	m_condition;

	public:
						CPathLibThread( CTaskManager& taskManager );

		void			RunTask( CAsyncTask* m_task );
		void			ResetActiveTask();
		void			RequestShutdown();

		CAsyncTask*		GetActiveTask() const;
		Bool			IsTaskRunning() const { return m_isTaskRunning.GetValue(); }

	protected:
		virtual void	ThreadFunc() override;
	};

	class CScopedProcessingLock : public Red::System::NonCopyable
	{
	private:
		CTaskManager*				m_man;
		Bool						m_valid;
	public:
								CScopedProcessingLock( CTaskManager* taskManager )
									: m_man( taskManager )
									, m_valid( taskManager->LockProcessing() )		{}
								~CScopedProcessingLock()							{ if ( m_valid ) m_man->UnlockProcessing(); }
								CScopedProcessingLock( CScopedProcessingLock&& s )
									: m_man( s.m_man )
									, m_valid( s.m_valid )							{ s.m_valid = false; }

		Bool					IsValid()											{ return m_valid; }
	};

protected:

	Mutex													m_mutex;
	CPathLibWorld&											m_pathlib;
	THeap< CAsyncTask::Ptr, CAsyncTask::TaskOrder >			m_pendingActivationQueue;
	CAsyncTask::Ptr											m_pendingPostProcessingTask;
	Int32													m_lockCount;
	CPathLibThread											m_thread;

public:
								CTaskManager( CPathLibWorld& pathlib );

	void						Initialize();
	void						Shutdown();

	void						Update();

	Bool						LockProcessing();
	void						UnlockProcessing();

	void						AddTask( CAsyncTask* task );

	CPathLibWorld&				GetPathLib() const { return m_pathlib; }

	Bool						HasTasks() const { return !m_pendingActivationQueue.Empty() || m_thread.IsTaskRunning(); }

	void						OnTaskFinished( CAsyncTask::Ptr& task );
};

};		// namespace PathLib

