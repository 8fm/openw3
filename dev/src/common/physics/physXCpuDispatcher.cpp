/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"


#include "physXCpuDispatcher.h"

#include "../core/taskManager.h"
#include "physicsEngine.h"

#ifdef USE_PHYSX

using namespace physx;

//////////////////////////////////////////////////////////////////////////
// CPhysXCpuDispatcher::CPhysXTask
//////////////////////////////////////////////////////////////////////////
#ifndef NO_DEBUG_PAGES
CPhysXCpuDispatcher::CPhysXTask::CPhysXTask( physx::PxBaseTask& physXTask, Bool isCritical )
#else
CPhysXCpuDispatcher::CPhysXTask::CPhysXTask( physx::PxBaseTask& physXTask )
#endif
	: m_physXTask( physXTask )
{
#ifndef NO_DEBUG_PAGES
	m_isCritical = isCritical;
	m_namePtr = physXTask.getName();
#endif
}

CPhysXCpuDispatcher::CPhysXTask::~CPhysXTask()
{
}

void CPhysXCpuDispatcher::CPhysXTask::Run()
{

	m_physXTask.run();
	m_physXTask.release();
}

//////////////////////////////////////////////////////////////////////////
// CPhysXCpuDispatcher
//////////////////////////////////////////////////////////////////////////
void CPhysXCpuDispatcher::submitTask( physx::PxBaseTask& task )
{
	const char* name = task.getName();

	static const char* m_destructibleSceneBeforeTickPtr = 0;
	if( !m_destructibleSceneBeforeTickPtr && !strcmp( name, "DestructibleScene::BeforeTick" ) )
	{
		m_destructibleSceneBeforeTickPtr = name;
	}

	static const char* m_apexSceneLODComputeBenefit = 0;
	if( !m_apexSceneLODComputeBenefit && !strcmp( name, "ApexScene::LODComputeBenefit" ) )
	{
		m_apexSceneLODComputeBenefit = name;
	}

	
#ifndef NO_DEBUG_PAGES
	CPhysXTask* physXTask =  new ( CTask::Root ) CPhysXTask( task, m_priority == TSP_Critical );
#else
	CPhysXTask* physXTask = new ( CTask::Root ) CPhysXTask( task );
#endif
	if( m_destructibleSceneBeforeTickPtr == name )
	{
		m_collectedTask.SetValue( physXTask );
		return;
	}

	if( m_apexSceneLODComputeBenefit == name )
	{
		RED_FATAL_ASSERT( m_beginTask.GetValue() == nullptr, "physx tasks collision");
		m_beginTask.SetValue( physXTask );
		return;
	}

	PxTaskManager* taskManager = task.getTaskManager();
	GTaskManager->Issue( *physXTask, m_priority );
	physXTask->Release();
}

physx::PxU32 CPhysXCpuDispatcher::getWorkerCount() const 
{
	return static_cast< physx::PxU32 >( GTaskManager->GetNumDedicatedTaskThreads() );
}

void CPhysXCpuDispatcher::ProcessCollectedTask()
{
	CPhysXTask* task = m_collectedTask.Exchange( nullptr );
	if( !task ) return;

	PC_SCOPE_PHYSICS( CPhysXCpuDispatcherProcessCollectedTask );

	task->Run();
	task->Release();
}

void CPhysXCpuDispatcher::StartBeginTask()
{
	CPhysXTask* task = m_beginTask.Exchange( nullptr );

	if( !task ) return;
	PxTaskManager* taskManager = task->m_physXTask.getTaskManager();
	GTaskManager->Issue( *task, m_priority );
	task->Release();

}

#endif // USE_PHYSX