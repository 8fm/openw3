/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibAreaProcessingJob.h"

#include "pathlibNavgraph.h"
#include "pathlibTaskManager.h"
#include "pathlibWorld.h"

namespace PathLib
{

#ifndef NO_EDITOR_PATHLIB_SUPPORT
////////////////////////////////////////////////////////////////////////////
// CAreaProcessingJob
////////////////////////////////////////////////////////////////////////////
CAreaProcessingJob::CAreaProcessingJob( CAreaDescription* area )
	: IGenerationManagerBase::CAsyncTask( "PathLib area processing" )
	, m_loadRequest( area, *area->GetPathLib().GetStreamingManager() )
	, m_area( area )
{
}
CAreaProcessingJob::~CAreaProcessingJob()
{

}
CAreaDescription* CAreaProcessingJob::GetTargetArea() const
{
	return m_area;
}
IGenerationManagerBase::CAsyncTask* CAreaProcessingJob::PostProcessingSync()
{
	m_loadRequest.Release();
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// CAreaGenerationJob
////////////////////////////////////////////////////////////////////////////
CAreaLoadPreSynchronization::CAreaLoadPreSynchronization( CAreaDescription* area )
	: CAreaProcessingJob( area )
{

}

Bool CAreaLoadPreSynchronization::PreProcessingSync()
{
	m_area->GetPathLib().GetTaskManager()->UnlockProcessing();

	m_area->StartProcessing();
	return true;
}

Bool CAreaLoadPreSynchronization::ProcessPathLibTask()
{
	Bool isLoaded = false;
	CTaskManager* taskManager = m_area->GetPathLib().GetTaskManager();
	do 
	{
		CSynchronousSection section( this );
		isLoaded = m_area->IsLoaded();
		if ( ShouldTerminate() )
		{
			return false;
		}
	}
	while ( !isLoaded );

	Bool isLocked = false;

	do
	{
		CSynchronousSection section( this );
		isLocked = taskManager->LockProcessing();
		if ( ShouldTerminate() )
		{
			return false;
		}
	}
	while ( isLocked );

	return true;
}

IGenerationManagerBase::CAsyncTask* CAreaLoadPreSynchronization::PostProcessingSync()
{
	IGenerationManagerBase::CAsyncTask* job = NULL;

	m_area->EndProcessing();
	m_area->SyncProcessing( &job );

	Super::PostProcessingSync();

	return job;
}

void CAreaLoadPreSynchronization::DescribeTask( String& task )
{
	String areaDesc;
	m_area->Describe( areaDesc );
	task = String::Printf( TXT("Preloading area %s"), areaDesc.AsChar() );
}

////////////////////////////////////////////////////////////////////////////
// CAreaGenerationJob
////////////////////////////////////////////////////////////////////////////
CAreaGenerationJob::CAreaGenerationJob( CAreaDescription* area, Uint16 taskFlags, Bool runPrePreocessingSync, Bool runSyncProcessingAfter )
	: CAreaProcessingJob(area )
	, m_flags( taskFlags )
	, m_runPreProcessingSync( runPrePreocessingSync )
	, m_runSyncProcessingAfter( runSyncProcessingAfter )
{
}
Bool CAreaGenerationJob::ShouldTerminate()
{
	//if ( m_area->IsDirty() & m_flags )
	//{
	//	return true;
	//}
	return IGenerationManagerBase::CAsyncTask::ShouldTerminate();
}
Bool CAreaGenerationJob::ProcessPathLibTask()
{
	m_area->AsyncProcessing( this );

	return true;
}
Bool CAreaGenerationJob::PreProcessingSync()
{
	if ( m_runPreProcessingSync )
	{
		m_area->PreGenerateSync();
	}
	return true;
}
IGenerationManagerBase::CAsyncTask* CAreaGenerationJob::PostProcessingSync()
{
	Super::PostProcessingSync();
	m_area->PostGenerationSyncProcess();
	if ( m_runSyncProcessingAfter )
	{
		m_area->SyncProcessing();
	}

	return nullptr;
}
void CAreaGenerationJob::DescribeTask( String& task )
{
	m_area->DescribeProcessingTasks( task, m_flags );
}

////////////////////////////////////////////////////////////////////////////
// CAreaWaterProcessing
////////////////////////////////////////////////////////////////////////////
CSpecialZonesProcessing::CSpecialZonesProcessing( CAreaDescription* area, CSpecialZonesMap* specialZones )
	: Super( area )
	, m_water( area->GetPathLib().GetWorld()->GetGlobalWater() )
	, m_specialZones( specialZones )
{

}
Bool CSpecialZonesProcessing::PreProcessingSync()
{
	return m_water != nullptr;
}
Bool CSpecialZonesProcessing::ProcessPathLibTask()
{
	struct Functor
	{
		CGlobalWater*			m_water;
		CSpecialZonesMap*		m_specialZones;
		Bool					m_ret;

		Functor( CGlobalWater* water, CSpecialZonesMap* specialZones )
			: m_water( water )
			, m_specialZones( specialZones )
			, m_ret( true ) {}

		void operator()( CNavGraph* navgraph )
		{
			m_ret = navgraph->MarkSpecialZones( m_water, m_specialZones ) && m_ret;
		}

	} fun( m_water, m_specialZones );

	m_area->GetNavgraphs()->IterateGraphs( fun );

	return true;
}

void CSpecialZonesProcessing::DescribeTask( String& task )
{
	String areaDesc;
	m_area->Describe( areaDesc );
	task = String::Printf( TXT("Computing water at area %s"), areaDesc.AsChar() );
}


////////////////////////////////////////////////////////////////////////////
// CAreaCoherentRegionsComputationJob
////////////////////////////////////////////////////////////////////////////

//Bool CAreaCoherentRegionsComputationJob::ProcessPathLibTask()
//{
//	struct Functor
//	{
//		void operator()( CNavGraph* navgraph ) const
//		{
//			navgraph->InitialCoherentRegionsMarking();
//		}
//	} f;
//
//	const auto* navgraphsList = m_area->GetNavgraphs();
//	navgraphsList->IterateGraphs( f );
//
//	return true;
//}
//Bool CAreaCoherentRegionsComputationJob::PreProcessingSync()
//{
//	m_area->StartProcessing();
//
//	return true;
//}
//IGenerationManagerBase::CAsyncTask* CAreaCoherentRegionsComputationJob::PostProcessingSync()
//{
//	m_area->EndProcessing();
//
//	return nullptr;
//}
//void CAreaCoherentRegionsComputationJob::DescribeTask( String& task )
//{
//	String areaDesc;
//	m_area->Describe( areaDesc );
//	task = String::Printf( TXT("Compute coherent regions for %s"), areaDesc.AsChar() );
//}



#endif // NO_EDITOR_PATHLIB_SUPPORT

};			// namespace PathLib
