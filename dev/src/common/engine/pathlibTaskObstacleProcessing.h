/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibObstacleMapper.h"
#include "pathlibTaskManager.h"


namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Asynchronous dynamic obstacle processing.
// Notice we are not spawning any new obstacles here, but just
// enabling/disabling pregenerated ones. There are few 'layers' of
// processing, and they are achieved through use of processing contexts
// (see CComponentRuntimeProcessingContext for reference).
// Layers of processing:
// - mark all obstacles on collision data (NOTICE synchronization
//   requirements as collision data are accessible asynchronously)
// - for each area, for each of navgraph effected, update node flags & links
// - update consistency for nodes that were effected
////////////////////////////////////////////////////////////////////////////
class CTaskObstacleProcessing : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	CObstaclesMapper::EventList				m_eventList;
	CObstaclesMapper*						m_obstaclesMapper;
public:
							CTaskObstacleProcessing( PathLib::CTaskManager& taskManager );

	// subclass interface task 
	virtual void			Process() override;
	virtual Bool			PreProcessingSynchronous() override;
	virtual void			PostProcessingSynchronous() override;

	virtual void			DescribeTask( String& outName ) const override;
};


};			// namespace PathLib