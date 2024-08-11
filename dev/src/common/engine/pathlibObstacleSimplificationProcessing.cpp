/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstacleSimplificationProcessing.h"

#include "pathlibObstaclesGroup.h"
#include "pathlibObstaclesMap.h"



namespace PathLib
{


CObstacleSimplificationProcessingThread::CObstacleSimplificationProcessingThread( CAreaDescription* area )
	: Super ( area )
{

}

Bool CObstacleSimplificationProcessingThread::ProcessPathLibTask()
{
	CAreaDescription* area = m_area;
	CObstaclesMap* obstacles = area->GetObstaclesMap();
	if ( obstacles )
	{
		CObstacleGroupsCollection& groups = obstacles->GetObstacleGroups();
		groups.SimplifyShapes( obstacles );
	}
	return true;
}

void CObstacleSimplificationProcessingThread::DescribeTask( String& task )
{
	task = TXT( "Obstacle simplification" );
}





};		// namespace PathLib


