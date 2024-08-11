/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

#include "../core/loadingJob.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CScaleformTaskManager;

//////////////////////////////////////////////////////////////////////////
// CScaleformTask
//////////////////////////////////////////////////////////////////////////
class CScaleformTask : public ILoadJob
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Scaleform );

private:
	SF::Ptr<GFx::Task> m_task;

protected:
	//! Process the job
	virtual EJobResult Process();

public:
	CScaleformTask( GFx::Task* ptask );

	virtual const Char* GetDebugName() const override { return TXT("GFx::Task"); }
};

//////////////////////////////////////////////////////////////////////////
// CScaleformTaskManager
//////////////////////////////////////////////////////////////////////////
class CScaleformTaskManager : public GFx::TaskManager
{
public:
	CScaleformTaskManager();
	virtual ~CScaleformTaskManager();

	virtual Bool AddTask( GFx::Task* ptask ) override;
	virtual Bool AbandonTask( GFx::Task* ptask ) override;
	virtual void RequestShutdown() override;

private:
	volatile Bool m_acceptNewTasks;
};

#endif // USE_SCALEFORM
