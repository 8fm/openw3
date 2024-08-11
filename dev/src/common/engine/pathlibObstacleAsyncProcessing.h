#pragma once

#include "pathlibObstacleMapper.h"
#include "pathlibStreamingItem.h"

namespace PathLib
{

#ifndef NO_EDITOR_PATHLIB_SUPPORT 

class CObstaclesLayerAsyncProcessing : public IGenerationManagerBase::CAsyncTask
{
	typedef IGenerationManagerBase::CAsyncTask Super;
protected:
	CObstaclesMapper::EventList					m_eventList;
	CObstaclesMapper*							m_obstaclesMapper;

	THandle< CLayer >							m_layer;

	Bool								ProcessEventList();
public:
	CObstaclesLayerAsyncProcessing( CObstaclesMapper* obstaclesMapper, CLayer* layer );

	Bool								PreProcessingSync() override;
	Bool								ProcessPathLibTask() override;
	IGenerationManagerBase::CAsyncTask*	PostProcessingSync() override;

	void								DescribeTask( String& task ) override;

	CLayerGroup*						GetProcessedLayerGroup();

	CObstaclesMapper*					GetObstacleMapper() const						{ return m_obstaclesMapper; }

	CLayer*								GetLayer() const								{ return m_layer.Get(); }
};

#endif		// NO_EDITOR_PATHLIB_SUPPORT


};			// namespace PathLib