/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskLayersVisibilityProcessing.h"

#include "pathlibAreaDescription.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstaclesMap.h"
#include "pathlibWorld.h"
#include "pathlibWorldLayersMapping.h"

namespace PathLib
{

CTaskLayerVisibilityProcessing::CTaskLayerVisibilityProcessing( PathLib::CTaskManager& taskManager, CWorldLayersMapping* worldLayers )
	: Super( taskManager, T_MODYFICATION, FLAG_USE_PREPROCESSING | FLAG_USE_POSTPROCESSING, EPriority::LayerVisibilityProcessing )
	, m_worldLayers( worldLayers )
{
}

Bool CTaskLayerVisibilityProcessing::PreProcessingSynchronous()
{
	m_updatedLayers = Move( m_worldLayers->StealPendingLayers() );
	return true;
}

void CTaskLayerVisibilityProcessing::Process()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();
	auto& nodeSetRequestsList = pathlib.GetObstaclesMapper()->GetComponentProcessingContext();

	nodeSetRequestsList.BeginRequestsCollection( pathlib );

	for ( const SLayerMapping& mapping : m_updatedLayers )
	{
		const auto* layerInfo = m_worldLayers->GetLayerInfo( mapping );
		ASSERT( layerInfo );
		Bool enabled = layerInfo->m_enabled;
		for ( AreaId areaId : layerInfo->m_areaList )
		{
			CAreaDescription* areaDescription = pathlib.GetAreaDescription( areaId );
			if ( !areaDescription || !areaDescription->IsReady() )
			{
				continue;
			}

			CObstaclesMap* obstaclesMap = areaDescription->GetObstaclesMap();
			if ( !obstaclesMap )
			{
				continue;
			}
			CObstacleGroup* obstacleGroup = obstaclesMap->GetObstacleGroups().GetObstacleGroup( mapping );
			if ( !obstacleGroup )
			{
				continue;
			}
			if ( enabled )
			{
				obstacleGroup->Attach( obstaclesMap, areaDescription, nodeSetRequestsList );
			}
			else
			{
				obstacleGroup->Detach( obstaclesMap, areaDescription, nodeSetRequestsList );
			}
			obstaclesMap->MarkVersionDirty();
		}
	}

	nodeSetRequestsList.EndRequestsCollection( pathlib );
}

void CTaskLayerVisibilityProcessing::PostProcessingSynchronous()
{
	m_worldLayers->LayerProcessingFinished( m_taskManager.GetPathLib() );
}

void CTaskLayerVisibilityProcessing::DescribeTask( String& outName ) const
{
	outName = TXT("Layer visibility processing");
}

};			// namespace PathLib

