#include "build.h"
#include "pathlibGenerationManager.h"

#ifndef NO_EDITOR_PATHLIB_SUPPORT

#include "pathlibConst.h"
#include "pathlibWorld.h"
#include "pathlibAreaDescription.h"
#include "pathlibAreaProcessingJob.h"
#include "pathlibTerrain.h"
#include "pathlibNavmeshComponent.h"
#include "pathlibTaskManager.h"
#include "pathlibTerrain.h"
#include "pathlibVisualizer.h"
#include "pathlibNavmeshArea.h"
#include "pathlibObstacleMapper.h"
#include "pathlibStreamingManager.h"

#include "clipMap.h"
#include "gameResource.h"
#include "world.h"
#include "baseEngine.h"

namespace PathLib
{


CGenerationManager::CStatusListener::CStatusListener( CWorld* world )
	: m_world( world )
{}

void CGenerationManager::CStatusListener::UnregisterLocked()
{
	CWorld* world = m_world.Get();
	if ( !world )
	{
		return;
	}
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( !pathlib )
	{
		return;
	}
	pathlib->GetGenerationManager()->UnregisterStatusListener();
}

void CGenerationManager::CStatusListener::TaskProcessed( CAreaDescription* areaDescription )
{
	Uint16 dirtyMask = areaDescription->IsDirty();
	
	String taskDescription;
	ETaskStatus status;
	if ( dirtyMask != 0 )
	{
		status = Status_Pending;
		areaDescription->DescribeProcessingTasks( taskDescription, dirtyMask );
	}
	else
	{
		status = Status_Complete;
	}
	
	TaskStatusChanged( areaDescription->GetId(), status, taskDescription );
}
void CGenerationManager::CStatusListener::TaskIsActive( CAreaDescription* areaDescription )
{
	String taskDescription;
	areaDescription->DescribeProcessingTasks( taskDescription, areaDescription->IsDirty() );
	TaskStatusChanged( areaDescription->GetId(), Status_Active, taskDescription );
}

void CGenerationManager::CStatusListener::ProcessAllTasksLocked()
{
	CWorld* world = m_world.Get();
	if ( !world )
	{
		return;
	}
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( !pathlib )
	{
		return;
	}
	pathlib->GetGenerationManager()->RegisterStatusListener( this );
}


////////////////////////////////////////////////////////////////////////////
// PathLib::CGenerationManager
////////////////////////////////////////////////////////////////////////////
CGenerationManager::CGenerationManager( CPathLibWorld& owner )
	: m_owner( owner )
	, m_isDisabled( 0 )
	, m_isDirty( false )
	, m_recalculateWaypoints( false )
	, m_updateObstacles( false )
	, m_checkObsolateObstacles( false )
	, m_isProcessing( false )
	, m_modyficationDelay()
	, m_syncProcessingRequestsCount( 0 )
	, m_syncProcessingRequests( 0, MAX_SUPPORTED_THREAS_COUNT )
	, m_syncProcessingRequestsDone( 0, 1 )
	, m_currentAsyncTask( NULL )
	, m_statusListener( NULL )
{
}

CGenerationManager::~CGenerationManager()
{
	ASSERT( !m_isProcessing && !m_currentAsyncTask );
}
void CGenerationManager::Initialize()
{
}
void CGenerationManager::BreakProcessing()
{
	m_terminationRequest = true;
	while ( m_isProcessing )
	{
		ProcessThreadSyncSection();
		Red::Threads::SleepOnCurrentThread( 100 );
	}
	if ( m_currentAsyncTask )
	{
		m_currentAsyncTask->Release();
		m_currentAsyncTask = NULL;
	}
	m_terminationRequest = false;
}
void CGenerationManager::Shutdown()
{
	BreakProcessing();
}
void CGenerationManager::HandleFinishedTask()
{
	if ( m_currentAsyncTask && !m_isProcessing )
	{
		CAsyncTask* nextTask = m_currentAsyncTask->PostProcessingSync();
		m_currentAsyncTask->Release();
		m_currentAsyncTask = NULL;
		if ( nextTask )
		{
			RunTask( nextTask );
		}
	}
}
void CGenerationManager::ProcessThreadSyncSection()
{
	while ( m_syncProcessingRequestsCount.GetValue() )
	{
		m_syncProcessingRequests.Release();
		m_syncProcessingRequestsDone.Acquire();
	}
}

void CGenerationManager::RequestSynchronousProcessing()
{
	m_syncProcessingRequestsCount.Increment();
	m_syncProcessingRequests.Acquire();
}
void CGenerationManager::FinishSynchronousProcessing()
{
	m_syncProcessingRequestsCount.Decrement();
	m_syncProcessingRequestsDone.Release();
}

Bool CGenerationManager::GameTick()
{
	HandleFinishedTask();
	return m_currentAsyncTask == nullptr;
}
void CGenerationManager::Tick()
{
	if ( m_isProcessing )
	{
		ProcessThreadSyncSection();
	}
	else
	{
		HandleFinishedTask();
	}

	if ( m_isProcessing || m_isDisabled )
	{
		return;
	}

	CTaskManager* taskManager = m_owner.GetTaskManager();
	CTaskManager::CScopedProcessingLock lock( taskManager );
	if ( !lock.IsValid() )
	{
		return;
	}
	
	EngineTime engineTime = GEngine->GetRawEngineTime();

	if ( m_recalculateWaypoints && m_recalculateWaypointsDelay < engineTime )
	{
		CWorld* world = m_owner.GetWorld();
		ASSERT( world );
		world->RefreshAllWaypoints( m_recalculateWaypointsBox );

		if ( m_statusListener )
		{
			m_statusListener->TaskStatusChanged( INVALID_AREA_ID, CStatusListener::Status_Complete, TXT("") );
		}

		m_recalculateWaypoints = false;
	}

	if ( m_updateObstacles && m_updateObstaclesDelay < engineTime )
	{
		for ( Uint32 i = 0, n = m_updateObstaclesAreas.Size(); i != n; ++i )
		{
			m_owner.UpdateObstacles( m_updateObstaclesAreas[ i ] );
		}
		m_updateObstacles = false;
	}

	if ( m_checkObsolateObstacles && m_checkObsolateObstaclesDelay < engineTime )
	{
		m_owner.GetObstaclesMapper()->CheckForObsolateObstacles();
		m_checkObsolateObstacles = false;
	}

	if ( m_isDirty && m_modyficationDelay < engineTime )
	{
		m_modyficationDelay = EngineTime::ZERO;
		// we do cascade test insteady of some events list - this simplifies the mechanism
		// but blow up code below
		Bool hasDelayedTasks = false;
		EngineTime currentTime = GEngine->GetRawEngineTime();
		CAsyncTask* job = NULL;
		Bool foundTask;
		do 
		{
			foundTask = false;
			auto handleArea =
				[&] ( CAreaDescription* area ) -> Bool
			{
				if ( area->IsDirty() )
				{
					if ( !area->IsProcessing() && area->GetProcessingDelay() < currentTime )
					{
						if ( area->IsLoaded() )
						{
							if ( area->SyncProcessing( &job ) )
							{
								if ( m_statusListener )
								{
									if ( job )
									{
										m_statusListener->TaskIsActive( area );
									}
									else
									{
										m_statusListener->TaskProcessed( area );
									}
								}
								foundTask = true;
								return job != NULL;
							}
						}
						else
						{
							job = new PathLib::CAreaLoadPreSynchronization( area );
							foundTask = true;
							return true;
						}
					}
					else
					{
						hasDelayedTasks = true;
						m_modyficationDelay =
							(m_modyficationDelay == EngineTime::ZERO) ?
							area->GetProcessingDelay() :
							(Min( area->GetProcessingDelay(), m_modyficationDelay ) );
					}
				}
				return false;
			};

			Bool breakProcessing = false;
			// instance areas
			const auto& instances = m_owner.m_instanceAreas;
			for ( auto it = instances.Begin(), end = instances.End(); it != end; ++it )
			{
				CNavmeshAreaDescription* area = (*it).m_area;
				if ( handleArea( area ) )
				{
					breakProcessing = true;
					break;
				}
			}

			if ( !breakProcessing )
			{
				// terrain areas
				const auto& terrain = m_owner.m_terrainAreas;
				for ( PathLib::AreaId index = 0, n = PathLib::AreaId(terrain.Size()); index < n; ++index )
				{
					auto area = terrain[ index ];
					if ( area && handleArea( area ) )
					{
						break;
					}
				}
			}
		}
		while( foundTask && !job );
			
		// if we found task - manager will stay dirty to check for new task when current one is finished
		m_isDirty = foundTask || hasDelayedTasks;

		if ( job )
		{
			RunTask( job );
			return;
		}
	}
}

void CGenerationManager::RecalculateWaypoints( const Box& box )
{
	if ( !m_recalculateWaypoints )
	{
		m_recalculateWaypoints = true;
		m_recalculateWaypointsBox = box;
		if ( m_statusListener )
		{
			m_statusListener->TaskStatusChanged( INVALID_AREA_ID, CStatusListener::Status_Pending, TXT("Recalculate waypoints") );
		}
	}
	else
	{
		m_recalculateWaypointsBox.AddBox( box );
	}
	m_recalculateWaypointsDelay = GEngine->GetRawEngineTime() + 10.f;
}

void CGenerationManager::UpdateObstacles( const Box& bbox, Float updateDelay )
{
	if ( !m_isDisabled )
	{
		m_updateObstaclesDelay = GEngine->GetRawEngineTime() + updateDelay;
		m_updateObstacles = true;
		for ( Uint32 i = 0, n = m_updateObstaclesAreas.Size(); i != n; ++i )
		{
			if ( m_updateObstaclesAreas[ i ].Touches( bbox ) )
			{
				m_updateObstaclesAreas[ i ].AddBox( bbox );
				return;
			}
		}
		m_updateObstaclesAreas.PushBack( bbox );
	}
}

void CGenerationManager::CheckObsolateObstacles()
{
	m_checkObsolateObstacles = true;
	m_checkObsolateObstaclesDelay = GEngine->GetRawEngineTime() + 10.f;
}

void CGenerationManager::MarkDirty( AreaId dirtyArea )
{
	m_isDirty = true;
	m_modyficationDelay = EngineTime::ZERO;
	if ( m_statusListener )
	{
		CAreaDescription* area = m_owner.GetAreaDescription( dirtyArea );
		if ( area )
		{
			String taskDescription;
			area->DescribeProcessingTasks( taskDescription, area->IsDirty() );
			m_statusListener->TaskStatusChanged( dirtyArea, CStatusListener::Status_Pending, taskDescription );
		}
	}
}

void CGenerationManager::GetPendingTasks( CStatusListener* listener )
{
	auto handleArea =
		[ listener ] ( CAreaDescription* area )
	{
		if ( !area->IsProcessing() && area->IsDirty() )
		{
			String taskDescription;
			area->DescribeProcessingTasks( taskDescription );
			listener->TaskStatusChanged( area->GetId(), CGenerationManager::CStatusListener::Status_Pending, taskDescription );
		}
	};

	// instance areas
	const auto& instances = m_owner.m_instanceAreas;
	for ( auto it = instances.Begin(), end = instances.End(); it != end; ++it )
	{
		CNavmeshAreaDescription* area = (*it).m_area;
		handleArea( area );
	}

	// terrain areas
	const auto& terrain = m_owner.m_terrainAreas;
	for ( PathLib::AreaId index = 0, n = PathLib::AreaId(terrain.Size()); index < n; ++index )
	{
		auto area = terrain[ index ];
		if ( area )
		{
			handleArea( area );
		}
	}

	if ( m_recalculateWaypoints )
	{
		listener->TaskStatusChanged( INVALID_AREA_ID, CStatusListener::Status_Pending, String( TXT("Recalculate waypoints") ) );
	}

}
void CGenerationManager::OnTaskFinished( CAsyncTask* task )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
	if ( m_statusListener )
	{
		CAreaDescription* area = m_currentAsyncTask->GetTargetArea();
		if ( area )
		{
			m_statusListener->TaskProcessed( area );
		}
	}
	
	//m_currentJob->Release();
	//m_currentJob = NULL;
	m_isProcessing = false;
	m_owner.GetTaskManager()->UnlockProcessing();
}
void CGenerationManager::RunTask( CAsyncTask* task )
{
	if ( !m_owner.GetTaskManager()->LockProcessing() )
	{
		task->Release();
		ASSERT( false );
		ASSUME( false );
	}
	if ( task->PreProcessingSync() )
	{
		task->InitThread();
		task->DetachThread();

		m_currentAsyncTask = task;
		m_isProcessing = true;
		

		m_currentAsyncTask->Run( this );
		{
			String taskDescription;
			task->DescribeTask( taskDescription );
			PATHLIB_LOG( TXT("Task: '%ls' started.\n"), taskDescription.AsChar() );
		}
	}
	else
	{
		task->Release();
		m_owner.GetTaskManager()->UnlockProcessing();
	}
}

void CGenerationManager::RegisterStatusListener( CStatusListener* lisener )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );

	m_statusListener = lisener;
	
	if ( m_isProcessing )
	{
		CAreaDescription* area = m_currentAsyncTask->GetTargetArea();
		String taskDescription;
		m_currentAsyncTask->DescribeTask( taskDescription );
		lisener->TaskStatusChanged( area ? area->GetId() : INVALID_AREA_ID, CStatusListener::Status_Active, taskDescription );
	}

	GetPendingTasks( lisener );
}


};				// namespace PathLib


#endif // NO_EDITOR_PATHLIB_SUPPORT
