/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTerrainFoliage.h"

#include "foliageBroker.h"
#include "foliageCell.h"
#include "foliageCellIterator.h"
#include "foliageResource.h"
#include "foliageEditionController.h"
#include "pathlibTerrain.h"
#include "pathlibObstacleSpawnContext.h"
#include "pathlibObstaclesMap.h"
#include "renderer.h"
#include "world.h"
#include "baseTree.h"


#ifndef NO_EDITOR_PATHLIB_SUPPORT

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CTerrainFoliageProcessingThread
////////////////////////////////////////////////////////////////////////////
CTerrainFoliageProcessingThread::CTerrainFoliageProcessingThread( CTerrainAreaDescription* area )
	: Super( area )
{
}

RED_INLINE CTerrainAreaDescription* CTerrainFoliageProcessingThread::GetArea() const
{
	return static_cast< CTerrainAreaDescription* >( m_area );
}

void CTerrainFoliageProcessingThread::GetTreeCollisionShapes( const CSRTBaseTree* tree, TDynArray< Sphere >& outShapes )
{
	GRender->GetSpeedTreeResourceCollision( tree->AcquireRenderObject().Get(), outShapes );
}

Bool CTerrainFoliageProcessingThread::PreProcessingSync()
{
	CFoliageEditionController & controller = m_area->GetPathLib().GetWorld()->GetFoliageEditionController();

	controller.AcquireAndLoadFoliageAtArea( m_area->GetBBox(), m_foliageHandle );

	return !m_foliageHandle.Empty();
}

Bool CTerrainFoliageProcessingThread::ProcessPathLibTask()
{
	CTerrainAreaDescription* area = GetArea();

	for ( auto it = m_foliageHandle.Begin(), end = m_foliageHandle.End(); it != end; ++it )
	{
		CFoliageCell* foliageCel = it->Get();
		if ( !foliageCel->IsResourceValid() )
		{
			continue;
		}

		CFoliageResource * foliageResource = foliageCel->GetFoliageResource();
		if ( !foliageResource )
		{
			continue;
		}

		CObstaclesMap* obstaclesMap = area->LazyInitializeObstaclesMap();

		const CFoliageResource::InstanceGroupContainer & treez = foliageResource->GetAllTreeInstances();
		TDynArray< Sphere > collisionShapes;
		
		for ( auto itTreez = treez.Begin(), endTreez = treez.End(); itTreez != endTreez; ++itTreez )
		{
			const CSRTBaseTree* tree = itTreez->baseTree.Get();
			const FoliageInstanceContainer& instanceList = itTreez->instances;

			GetTreeCollisionShapes( tree, collisionShapes );

			if( collisionShapes.Empty() )
			{
				continue;
			}

			
			for ( auto itInstances = instanceList.Begin(), endInstances = instanceList.End(); itInstances != endInstances; ++itInstances )
			{
				const SFoliageInstance & foliageInstance = *itInstances;

				if ( area->GetBBox().SquaredDistance( foliageInstance.GetPosition() ) > (5.f*5.f) )
				{
					continue;
				}

				PathLib::CObstacleSpawnData instanceSpawnData;
				instanceSpawnData.InitializeTree( foliageInstance, collisionShapes );

				CObstacle* obstacle = obstaclesMap->CreateObstacle( instanceSpawnData );
				if ( obstacle )
				{
					obstaclesMap->AddObstacle( obstacle, instanceSpawnData );
				}
				

				//Vector					m_position;		//!< Position
				//EulerAngles				m_rotation;		//!< Rotation
				//Float					m_scale;		//!< Scale
			}

			collisionShapes.ClearFast();
		}
		
	}
	
	return true;
}
IGenerationManagerBase::CAsyncTask* CTerrainFoliageProcessingThread::PostProcessingSync()
{
	m_foliageHandle.Clear();

	Super::PostProcessingSync();
	
	return nullptr;
}
void CTerrainFoliageProcessingThread::DescribeTask( String& task )
{
	Int32 x,y;
	m_area->GetPathLib().GetTerrainInfo().GetTileCoordsFromId( m_area->GetId(), x, y );
	task = String::Printf( TXT( "Process foliage at %d x %d" ), x, y );
}


};			// namespace PathLib

#endif		// NO_EDITOR_PATHLIB_SUPPORT

