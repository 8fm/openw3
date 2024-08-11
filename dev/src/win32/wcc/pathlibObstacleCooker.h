/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/pathlibObstacleAsyncProcessing.h"
#include "../../common/engine/layerInfo.h"

class CPathLibTaskPool;
class CPathLibCooker;

class CNavigationCookingContext;

class CObstacleCooker
{
protected:
	typedef Red::Threads::CMutex						Mutex;
	typedef Red::Threads::CScopedLock< Mutex >			Lock;
	typedef TDynArray< CLayerInfo* >					LayerList;

	CWorld*						m_world;
	CPathLibWorld*				m_pathlib;
	CPathLibTaskPool*			m_taskPool;
	CPathLibCooker*				m_cooker;
	CNavigationCookingContext* 	m_context;		
	LayerList					m_layersToTrash;
	Uint32						m_layersProcessing;
	Mutex						m_mutex;

public:
	CObstacleCooker( CWorld* world, CPathLibWorld* pathlib, CPathLibTaskPool* taskPool, CPathLibCooker* cooker, CNavigationCookingContext* context )
		: m_world( world )
		, m_pathlib( pathlib )
		, m_taskPool( taskPool )
		, m_cooker( cooker )
		, m_context( context )
		, m_layersProcessing( 0 )												{}

	void				DoStuff();
	void				TrashLayer( CLayerInfo* layer );
	void				TryUnloadStuff();
};

class CObstacleCookerLayerTask : public PathLib::CObstaclesLayerAsyncProcessing
{
	typedef PathLib::CObstaclesLayerAsyncProcessing Super;
protected:
	CObstacleCooker*			m_cooker;
	CLayerInfo*					m_layerInfo;
public:
	CObstacleCookerLayerTask( PathLib::CObstaclesMapper* obstaclesMapper, CLayerInfo* layerInfo, CObstacleCooker* cooker )
		: Super( obstaclesMapper, layerInfo->GetLayer() )
		, m_cooker( cooker )
		, m_layerInfo( layerInfo )												{}

	Bool											PreProcessingSync() override;
	PathLib::IGenerationManagerBase::CAsyncTask*	PostProcessingSync() override;
};
