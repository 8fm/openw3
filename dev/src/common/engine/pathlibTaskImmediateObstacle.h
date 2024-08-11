/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibObstacleSpawnContext.h"
#include "pathlibTaskManager.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Each immediate obstacle got its own 'task' for processing
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
// On attachment we need to first obtain synchronously input data. And then
// create obstacle.
////////////////////////////////////////////////////////////////////////////
class CTaskProcessImmediateObstacle : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	THandle< CComponent >		m_engineComponent;
	CObstaclesMapper&			m_mapper;
	CObstacleSpawnData			m_spawnData;
public:
							CTaskProcessImmediateObstacle( CTaskManager& taskManager, CObstaclesMapper& mapper, const THandle< CComponent >& engineComponent );

	virtual Bool			PreProcessingSynchronous() override;
	virtual void			Process() override;

	virtual void			DescribeTask( String& outName ) const override;
};

////////////////////////////////////////////////////////////////////////////
// Removing immediate obstacle is separate task, as we need to modify graph
// and then remove obstacle from structure, which would fuckup all context
// post processing data if we would try to do it on normal obstacle
// processing task.
////////////////////////////////////////////////////////////////////////////
class CTaskRemoveImmediateObstacle : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	CObstaclesMapper&			m_mapper;
	SComponentMapping			m_mapping;
public:
							CTaskRemoveImmediateObstacle( CTaskManager& taskManager, CObstaclesMapper& mapper, const SComponentMapping& mapping );

	virtual void			Process() override;

	virtual void			DescribeTask( String& outName ) const override;
};

};			// namespace PathLib