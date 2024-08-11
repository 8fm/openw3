/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskPool.h"



///////////////////////////////////////////////////////////////////////////////
// CPathLibTaskPool::CManagerThread
///////////////////////////////////////////////////////////////////////////////
CPathLibTaskPool::CManagerThread::CManagerThread( CPathLibTaskPool& owner )
	: Red::Threads::CThread( "Pathlib cooker pool manager" )
	, m_owner( owner )
{

}

Bool CPathLibTaskPool::CManagerThread::RunTask( CAsyncTask* task )
{
	m_owner.m_syncProcessingSection.Acquire();
	Bool preSyncOk = task->PreProcessingSync();
	m_owner.m_syncProcessingSection.Release();
	if ( preSyncOk )
	{
		{
			Lock lock( m_owner.m_mutex );
			m_owner.m_activeTasks.PushBack( task );
		}

		task->InitThread();		

		task->Run( &m_owner );

		return true;
	}
	else
	{
		task->Release();
		return false;
	}
}

void CPathLibTaskPool::CManagerThread::ThreadFunc()
{
	while ( true )
	{
		// wait until there is something to process
		// > other version of this shit Sleep( 5 );
		m_owner.m_actionsSemaphore.TryAcquire( 5 );

		{
			m_owner.m_mutex.Acquire();
			//Lock lock( m_owner.m_mutex );

			{
				// find yourself a request
				if ( m_owner.m_shutdownRequest )
				{
					m_owner.m_mutex.Release();
					return;
				}
				
				if ( m_owner.m_freeTasksSemaphore.TryAcquire() )
				{
					// process pending free task action
					CAsyncTask* finishedTask = m_owner.m_finishedTasks.PopBackFast();
					m_owner.m_mutex.Release();

					m_owner.m_syncProcessingSection.Acquire();
					finishedTask->PostProcessingSync();
					m_owner.m_syncProcessingSection.Release();
					finishedTask->JoinThread();
					finishedTask->Release();
					
					m_owner.m_mutex.Acquire();
					if ( m_owner.m_pendingTasks.Empty() && m_owner.m_activeTasks.Empty() && m_owner.m_finishedTasks.Empty() )
					{
						m_owner.m_allTasksCompleted.WakeAll();
					}
				}

				if ( m_owner.CanRunTask()  )
				{
					CAsyncTask* newTask = m_owner.m_pendingTasks.PopBackFast();
					m_owner.m_mutex.Release();
					if ( !RunTask( newTask ) )
					{
						if ( m_owner.m_pendingTasks.Empty() && m_owner.m_activeTasks.Empty() && m_owner.m_finishedTasks.Empty() )
						{
							m_owner.m_allTasksCompleted.WakeAll();
						}
					}
					continue;
				}


			}
		}
		m_owner.m_mutex.Release();
	}
}


///////////////////////////////////////////////////////////////////////////////
// CPathLibTaskPool
///////////////////////////////////////////////////////////////////////////////
CPathLibTaskPool::CPathLibTaskPool()
	: m_thread( nullptr )
	, m_actionsSemaphore( 0, 1024*1024 )
	, m_freeTasksSemaphore( 0, MAX_ACTIVE_TASKS )
	, m_shutdownRequest( false )
{
}
CPathLibTaskPool::~CPathLibTaskPool()
{
	// system is requested to shut down
	for ( auto it = m_pendingTasks.Begin(), end = m_pendingTasks.End(); it != end; ++it )
	{
		(*it)->Release();
	}
}
void CPathLibTaskPool::OnTaskFinished( CAsyncTask* task )
{
	{
		Lock lock( m_mutex );
		m_activeTasks.RemoveFast( task );
		m_finishedTasks.PushBack( task );
		m_freeTasksSemaphore.Release();
	}
	m_actionsSemaphore.Release();
}

void CPathLibTaskPool::StartProcessing( Uint32 taskLimit )
{
	ASSERT( taskLimit <= MAX_ACTIVE_TASKS );
	m_concurrentTaskLimit = taskLimit;
	if ( !m_thread )
	{
		m_shutdownRequest = false;
		m_thread = new CManagerThread( *this );
		m_thread->InitThread();
	}
}
void CPathLibTaskPool::InterruptProcessing()
{
	if ( m_thread )
	{
		m_shutdownRequest = true;
		m_thread->JoinThread();
		delete m_thread;
		m_thread = nullptr;
	}
}

void CPathLibTaskPool::ProcessAllPendingTasks()
{
	while ( true ) 
	{
		{
			Lock lock( m_mutex );
			if ( m_pendingTasks.Empty() && m_activeTasks.Empty() && m_finishedTasks.Empty() )
			{
				break;
			}
			m_allTasksCompleted.Wait( m_mutex );
		}
	}
}

void CPathLibTaskPool::CompleteProcessing()
{
	ProcessAllPendingTasks();
	m_shutdownRequest = true;
	m_actionsSemaphore.Release();
	m_thread->JoinThread();
	delete m_thread;
	m_thread = nullptr;
}
void CPathLibTaskPool::AddTask( CAsyncTask* task )
{
	{
		Lock lock( m_mutex );
		m_pendingTasks.PushBack( task );
	}
	m_actionsSemaphore.Release();
}
void CPathLibTaskPool::RequestSynchronousProcessing()
{
	m_syncProcessingSection.Acquire();
}
void CPathLibTaskPool::FinishSynchronousProcessing()
{
	m_syncProcessingSection.Release();
}