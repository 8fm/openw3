#include "build.h"
#include "pathlibTaskManager.h"

#include "pathlib.h"

#ifdef PROFILE_PATHLIB_TASKS
#include "../core/profilerTypes.h"
#endif

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CTaskManager::CAsyncTask
///////////////////////////////////////////////////////////////////////////////
CTaskManager::CAsyncTask::CAsyncTask( PathLib::CTaskManager& taskManager, ETaskType taskType, Uint32 flags, EPriority priority )
	: m_taskManager( taskManager )
	, m_taskType( taskType )
	, m_taskFlags( flags )
	, m_priority( priority )
	, m_lifeRefCount( 1 )
{
}

void CTaskManager::CAsyncTask::Process()
{
	// nothing by default
}

Bool CTaskManager::CAsyncTask::PreProcessingSynchronous()
{
	// nothing by default
	return true;
}

void CTaskManager::CAsyncTask::PostProcessingSynchronous()
{
	// nothing by default
}

void CTaskManager::CAsyncTask::Run()
{
	PC_SCOPE_PIX( PathLib );

#ifdef PROFILE_PATHLIB_TASKS
	CTimeCounter timeCounter;
	timeCounter.ResetTimer();
#endif
	Process();

#ifdef PROFILE_PATHLIB_TASKS
	Float ms = Float( timeCounter.GetTimePeriodMS() );
	String name;
	DescribeTask( name );
	PATHLIB_LOG( TXT("Pathlib task '%s' took %0.2f"), name.AsChar(), ms );
#endif

}

void CTaskManager::CAsyncTask::DescribeTask( String& outName ) const
{
	outName = TXT("Unnamed PathLib task");
}

void CTaskManager::CAsyncTask::Release()
{
	const Int32 newRefCount = m_lifeRefCount.Decrement();
	RED_FATAL_ASSERT( newRefCount >= 0,"" );

	if ( newRefCount == 0 )
	{
		delete this;
	}
}

void CTaskManager::CAsyncTask::AddRef()
{
	const Int32 newRefCount = m_lifeRefCount.Increment();
	RED_FATAL_ASSERT( newRefCount > 0, "" );
}

CTaskManager::CPathLibThread::CPathLibThread( CTaskManager& taskManager )
	: CThread( "PathLibThread" )
	, m_taskManager( taskManager )
	, m_isTaskRunning( true )
{
}

void CTaskManager::CPathLibThread::RunTask( CAsyncTask* task )
{
	RED_ASSERT( !IsTaskRunning() && !m_activeTask );

	m_isTaskRunning.SetValue( true );
	m_activeTask = task;

	Lock lock( m_mutex );
	m_condition.WakeAll();
}

void CTaskManager::CPathLibThread::ResetActiveTask()
{
	RED_ASSERT( !IsTaskRunning() );
	m_activeTask = nullptr;
}

void CTaskManager::CPathLibThread::RequestShutdown()
{
	Lock lock( m_mutex );
	m_shutdownRequested.SetValue( true );
	m_condition.WakeAll();
}

void CTaskManager::CPathLibThread::ThreadFunc()
{
	while ( true )
	{
		{
			Lock lock( m_mutex );
			m_isTaskRunning.SetValue( false );
			if ( m_shutdownRequested.GetValue () )
			{
				return;
			}
			m_condition.Wait( m_mutex );
		}

		while ( m_activeTask )
		{
			m_activeTask->Run();
			m_taskManager.OnTaskFinished( m_activeTask );
		}
	}
}

CTaskManager::CAsyncTask* CTaskManager::CPathLibThread::GetActiveTask() const
{
	RED_ASSERT( IsTaskRunning() );

	return m_activeTask.Get(); 
}

///////////////////////////////////////////////////////////////////////////////
// CTaskManager
///////////////////////////////////////////////////////////////////////////////
CTaskManager::CTaskManager( CPathLibWorld& pathlib )
	: m_pathlib( pathlib )
	, m_thread( *this )
{
}

void CTaskManager::Initialize()
{
	m_thread.InitThread();

#if defined( RED_PLATFORM_DURANGO ) || defined( RED_PLATFORM_ORBIS )
	m_thread.SetAffinityMask( ( 1 << 3 ) | ( 1 << 4 ) );
#endif
	m_thread.SetPriority( Red::Threads::TP_BelowNormal );
}

void CTaskManager::Shutdown()
{
	m_mutex.Acquire();

	m_thread.RequestShutdown();

	while( m_thread.IsTaskRunning() )
	{
		m_mutex.Release();
		Red::Threads::SleepOnCurrentThread( 2 );
		m_mutex.Acquire();
	}
	
	// release pending tasks
	m_pendingActivationQueue.ClearFast();
	m_pendingPostProcessingTask = nullptr;
	
	m_thread.JoinThread();

	m_mutex.Release();
}

void CTaskManager::Update()
{
	if ( m_thread.IsTaskRunning() )
	{
		return;
	}

	if ( m_pendingPostProcessingTask )
	{
		CAsyncTask* activeTask = m_pendingPostProcessingTask.Get();

		if ( activeTask->IsUsingPostprocessing() )
		{
			activeTask->PostProcessingSynchronous();
		}
		
		m_pendingPostProcessingTask = nullptr;
	}
	
	CAsyncTask::Ptr newTask;
	{
		Lock lock( m_mutex );
		if ( !m_pendingActivationQueue.Empty() )
		{
			newTask = m_pendingActivationQueue.Front();
			m_pendingActivationQueue.PopHeap();
		}
	}

	if ( newTask )
	{
		if ( newTask->IsUsingPreprocessing() )
		{
			if ( !newTask->PreProcessingSynchronous() )
			{
				return;
			}
		}
		m_thread.RunTask( newTask.Get() );
	}
}


Bool CTaskManager::LockProcessing()
{
	Lock lock( m_mutex );

	if ( m_lockCount == 0 && m_thread.IsTaskRunning() )
	{
		return false;
	}

	// mark it as we are conflicted with all types of tasks
	++m_lockCount;
	return true;
}
void CTaskManager::UnlockProcessing()
{
	--m_lockCount;
}

void CTaskManager::AddTask( CAsyncTask* task )
{
	Lock lock( m_mutex );
	if ( !task->IsUsingPreprocessing() )
	{
		if ( !m_thread.IsTaskRunning() )
		{
			if ( !m_thread.IsTaskRunning() && m_pendingActivationQueue.Empty() && !m_pendingPostProcessingTask )
			{
				m_thread.RunTask( task );
				return;
			}
		}
	}

	m_pendingActivationQueue.PushHeap( task );
}

void CTaskManager::OnTaskFinished( CAsyncTask::Ptr& task )
{
	if ( task->IsUsingPostprocessing() )
	{
		m_pendingPostProcessingTask = task;
	}
	else
	{
		Lock lock( m_mutex );

		while ( !m_pendingActivationQueue.Empty() )
		{
			CAsyncTask::Ptr newTask = m_pendingActivationQueue.Front();
			if ( newTask->IsUsingPreprocessing() )
			{
				break;
			}

			task = newTask;
			m_pendingActivationQueue.PopHeap();
			return;
		}
	}

	task = nullptr;
}

};				// namespace PathLib


