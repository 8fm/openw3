/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "pathlibFoliageCooker.h"

#include "../../common/engine/foliageCell.h"

#include "cookerSpeedTreeSupport.h"
#include "pathlibCooker.h"
#include "pathlibTaskPool.h"
#include "../../common/engine/pathlibWorld.h"


Bool CPathLibCooker::ProcessFoliage()
{
	m_taskPool->StartProcessing();

	CPathLibWorld::TerrainAreasIterator it( *m_pathlib );
	while ( it )
	{
		PathLib::CTerrainAreaDescription* terrainArea = *it;
		if ( terrainArea )
		{
			CPathLibTaskPool::CAsyncTask* task = new CTerrainCookerFoliageProcessingThread( terrainArea );
			m_taskPool->AddTask( task );
		}

		++it;
	}

	m_taskPool->CompleteProcessing();
	TryGarbageCollect();

	return true;
}

CTerrainCookerFoliageProcessingThread::~CTerrainCookerFoliageProcessingThread()
{

}

void CTerrainCookerFoliageProcessingThread::GetTreeCollisionShapes( const CSRTBaseTree* tree, TDynArray< Sphere >& outShapes )
{
	Bool grass;
	CookerSpeedTreeSupport::GetTreeCollisionShapes( tree, grass, outShapes );
	if ( grass )
	{
		outShapes.ClearFast();
	}
}
