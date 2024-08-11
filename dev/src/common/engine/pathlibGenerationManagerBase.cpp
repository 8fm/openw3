/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibGenerationManagerBase.h"
#include "pathlib.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// ITaskManagerBase::CAsyncTask
////////////////////////////////////////////////////////////////////////////
IGenerationManagerBase::CAsyncTask::CAsyncTask( const AnsiChar* threadName )
	: Red::Threads::CThread( threadName )
	, m_taskManager( nullptr )
	, m_startWork( 0, 1 )
	, m_refCount( 1 )
	, m_taskProgress( 0.f )
{
}

Bool IGenerationManagerBase::CAsyncTask::ShouldTerminate()
{
	return m_taskManager->TerminationRequest();
}
void IGenerationManagerBase::CAsyncTask::Run( IGenerationManagerBase* taskManager )
{
	m_taskManager = taskManager;
	m_startWork.Release();
}
void IGenerationManagerBase::CAsyncTask::ThreadFunc()
{
	m_startWork.Acquire();

	m_startTime = EngineTime::GetNow();
	Bool result = ProcessPathLibTask();
	m_taskManager->OnTaskFinished( this );
	{
		EngineTime now = EngineTime::GetNow();
		String taskDescription;
		DescribeTask( taskDescription );

		PATHLIB_LOG( TXT("Task: '%ls' completed in %f seconds.\n"), taskDescription.AsChar(), Float((now-m_startTime)) );
	}
}

Bool IGenerationManagerBase::CAsyncTask::Begin_SynchronousSection()
{
	m_taskManager->RequestSynchronousProcessing();
	return !m_taskManager->TerminationRequest();
}
void IGenerationManagerBase::CAsyncTask::End_SynchronousSection()
{
	m_taskManager->FinishSynchronousProcessing();
}

void IGenerationManagerBase::CAsyncTask::AddRef()
{
	++m_refCount;
}
void IGenerationManagerBase::CAsyncTask::Release()
{
	if ( --m_refCount <= 0 )
	{
		delete this;
	}
}

CAreaDescription* IGenerationManagerBase::CAsyncTask::GetTargetArea() const { return NULL; }

Bool IGenerationManagerBase::CAsyncTask::PreProcessingSync()
{
	return true;
}
IGenerationManagerBase::CAsyncTask* IGenerationManagerBase::CAsyncTask::PostProcessingSync()
{
	return NULL;
}



////////////////////////////////////////////////////////////////////////////
// ITaskManagerBase
////////////////////////////////////////////////////////////////////////////
IGenerationManagerBase::IGenerationManagerBase()
	: m_terminationRequest( false )
{

}

IGenerationManagerBase::~IGenerationManagerBase()
{

}

void IGenerationManagerBase::OnTaskFinished( CAsyncTask* task )
{

}




};			// namespace PathLib