/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskObstacleProcessing.h"

#include "component.h"
#include "pathlibComponent.h"
#include "pathlibObstacleMapper.h"
#include "pathlibNodeSetRequestsList.h"
#include "pathlibWorld.h"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CTaskObstacleProcessing
///////////////////////////////////////////////////////////////////////////////
CTaskObstacleProcessing::CTaskObstacleProcessing( PathLib::CTaskManager& taskManager )
	: Super( taskManager, T_MODYFICATION, FLAG_USE_PREPROCESSING, EPriority::ObstacleProcessing )
	, m_obstaclesMapper( taskManager.GetPathLib().GetObstaclesMapper() )
{

}

Bool CTaskObstacleProcessing::PreProcessingSynchronous()
{
	m_obstaclesMapper->StealAllEvents( m_eventList );

	if ( m_eventList.Empty() )
	{
		return false;
	}

	return true;
}

void CTaskObstacleProcessing::Process()
{
	auto& nodeSetRequestsList = m_obstaclesMapper->GetComponentProcessingContext();
	CPathLibWorld& pathlib = m_obstaclesMapper->GetPathLib();

	nodeSetRequestsList.BeginRequestsCollection( pathlib );

	for ( auto it = m_eventList.Begin(), end = m_eventList.End(); it != end; ++it )
	{
		CProcessingEvent& e = *it;
		if ( e.m_generalProcessingImplementation )
		{
			e.m_generalProcessingImplementation->RuntimeProcessing( nodeSetRequestsList, e );
		}
		else
		{
			CComponent* component = e.m_component.Get();
			if ( component )
			{
				component->AsPathLibComponent()->RuntimeProcessing( nodeSetRequestsList, e );
			}
		}
	}

	nodeSetRequestsList.EndRequestsCollection( pathlib );
}

void CTaskObstacleProcessing::PostProcessingSynchronous()
{
	
}

void CTaskObstacleProcessing::DescribeTask( String& outName ) const
{
	outName = TXT("Obstacle processing");
}

};			// namespace PathLib