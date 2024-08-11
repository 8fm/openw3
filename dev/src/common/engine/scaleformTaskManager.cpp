/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "guiGlobals.h"

#include "scaleformTaskManager.h"

#ifdef USE_SCALEFORM

#include "../core/loadingJobManager.h"
#include "../core/depot.h"

extern RED_TLS Bool GIsLoadingJobThread;

//////////////////////////////////////////////////////////////////////////
// CScaleformTask
//////////////////////////////////////////////////////////////////////////
CScaleformTask::CScaleformTask( GFx::Task* ptask )
	: ILoadJob( JP_Immediate, true /*block GC*/ )
	, m_task( ptask )
{
}

EJobResult CScaleformTask::Process()
{
	m_task->Execute();
	m_task.Clear();
	return JR_Finished;		
}

//////////////////////////////////////////////////////////////////////////
// CScaleformTaskManager
//////////////////////////////////////////////////////////////////////////
CScaleformTaskManager::CScaleformTaskManager()
	: m_acceptNewTasks( true )
{
}

CScaleformTaskManager::~CScaleformTaskManager()
{
	RequestShutdown();
}

Bool CScaleformTaskManager::AddTask(GFx::Task* ptask)
{
	RED_FATAL_ASSERT(ptask, "No task");

 	if (!m_acceptNewTasks)
 	{
 		return false;
 	}
	
	if ( !GDeferredInit && !GIsLoadingJobThread && ptask->GetTaskType() == SF::GFx::Task::Type_IO )
	{
		ILoadJob* job = new CScaleformTask(ptask);
		SJobManager::GetInstance().Issue(job);
		job->Release();
	}
	else
	{
		SF::Ptr< GFx::Task > task = ptask;
		task->Execute();
	}

	return true;
}

Bool CScaleformTaskManager::AbandonTask(GFx::Task* ptask)
{
	RED_UNUSED( ptask );

	return false;
}

void CScaleformTaskManager::RequestShutdown()
{
	SJobManager::GetInstance().FlushPendingJobs();
 	m_acceptNewTasks = false;
}

#endif // USE_SCALEFORM
