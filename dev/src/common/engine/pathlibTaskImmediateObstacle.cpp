/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskImmediateObstacle.h"

#include "component.h"
#include "pathlibAreaDescription.h"
#include "pathlibComponent.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstaclesMap.h"
#include "pathlibWorld.h"

namespace PathLib
{



////////////////////////////////////////////////////////////////////////////
// CTaskProcessImmediateObstacle
////////////////////////////////////////////////////////////////////////////
CTaskProcessImmediateObstacle::CTaskProcessImmediateObstacle( CTaskManager& taskManager, CObstaclesMapper& mapper, const THandle< CComponent >& engineComponent )
	: Super( taskManager, T_MODYFICATION, FLAG_USE_PREPROCESSING, EPriority::ProcessImmediateObstacle )
	, m_mapper( mapper )
	, m_engineComponent( engineComponent )
{
	ASSERT( engineComponent.Get()->AsPathLibComponent() && engineComponent.Get()->AsPathLibComponent()->AsObstacleComponent() && engineComponent.Get()->AsPathLibComponent()->AsObstacleComponent()->GetPathLibCollisionGroup() == PLC_Immediate );
}

Bool CTaskProcessImmediateObstacle::PreProcessingSynchronous()
{
	CComponent* engineComponent = m_engineComponent.Get();
	if ( !engineComponent )
	{
		return false;
	}
	if ( !engineComponent->IsAttached() )
	{
		return false;
	}
	IComponent* pathlibComponent = engineComponent->AsPathLibComponent();

	if ( !m_spawnData.Initialize( engineComponent, m_taskManager.GetPathLib(), nullptr ) )
	{
		return false;
	}
	
	return true;
}
void CTaskProcessImmediateObstacle::Process()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();

	auto& mapping = m_mapper.RequestMapping( m_spawnData.m_mapping );
	//auto& processingContext = m_mapper.GetComponentProcessingContext();

	auto& areaList = mapping.m_areaInfo;

	// fully remove obstacle
	if ( !areaList.Empty() )
	{
		//processingContext.BeginRequestsCollection( pathlib );
		//for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
		//{
		//	CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
		//	CObstaclesMap* obstacles = area->GetObstaclesMap();
		//	if ( obstacles )
		//	{
		//		obstacles->HideObstacle( it->m_obstacleId, processingContext );
		//	}
		//}
		//processingContext.EndRequestsCollection( pathlib );

		for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
		{
			CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
			CObstaclesMap* obstacles = area->GetObstaclesMap();
			if ( obstacles )
			{
				obstacles->RemoveObstacle( it->m_obstacleId );
			}
		}
	}
	

	areaList.ClearFast();

	mapping.m_isRuntimeObstacleEnabled = true;

	// re-add obstacle
	CAreaDescription* areasInVicinity[ 16 ];
	Uint32 areasFound = pathlib.CollectAreasAt( m_spawnData.m_bbox, areasInVicinity, 16 );

	//processingContext.BeginRequestsCollection( pathlib );

	for ( Uint32 i = 0; i < areasFound; ++i )
	{
		CAreaDescription* area = areasInVicinity[ i ];
		if ( area->IsReady() )
		{
			CObstaclesMap* obstaclesMap = area->LazyInitializeObstaclesMap();
			CObstacle* obstacle = obstaclesMap->CreateObstacle( m_spawnData );
			if ( obstacle )
			{
				CObstaclesMapper::ObstacleAreaInfo areaInfo;
				areaInfo.m_areaId = area->GetId();
				// this is possibly heavy as it should modify the navgraphs
				areaInfo.m_obstacleId = obstaclesMap->AddObstacle( obstacle, m_spawnData );
				//obstaclesMap->ShowObstacle( areaInfo.m_obstacleId, processingContext );
				areaList.PushBack( areaInfo );
			}
		}
	}

	//processingContext.EndRequestsCollection( pathlib );
}

void CTaskProcessImmediateObstacle::DescribeTask( String& outName ) const
{
	outName = TXT("Add immediate obstacle");
}



////////////////////////////////////////////////////////////////////////////
// CTaskRemoveImmediateObstacle
////////////////////////////////////////////////////////////////////////////
CTaskRemoveImmediateObstacle::CTaskRemoveImmediateObstacle( CTaskManager& taskManager, CObstaclesMapper& mapper, const SComponentMapping& mapping )
	: Super( taskManager, T_MODYFICATION, FLAGS_DEFAULT, EPriority::RemoveImmediateObstacle )
	, m_mapper( mapper )
	, m_mapping( mapping )
{

}

void CTaskRemoveImmediateObstacle::Process()
{
	auto* info = m_mapper.GetMapping( m_mapping );
	if ( !info )
	{
		// obstacle not registered anyhow yet
		return;
	}

	info->m_isRuntimeObstacleEnabled = false;

	CPathLibWorld& pathlib = m_mapper.GetPathLib();
	auto& areaList = info->m_areaInfo;

	for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
	{
		CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
		CObstaclesMap* obstacles = area->GetObstaclesMap();
		obstacles->RemoveObstacle( it->m_obstacleId );
	}

	m_mapper.ForgetMapping( m_mapping );
}

void CTaskRemoveImmediateObstacle::DescribeTask( String& outName ) const
{
	outName = TXT("Remove immediate obstacle");
}

};			// namespace PathLib