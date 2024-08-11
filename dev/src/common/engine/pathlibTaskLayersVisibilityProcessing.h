/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "pathlibObstacleSpawnContext.h"
#include "pathlibTaskManager.h"

namespace PathLib
{

class CWorldLayersMapping;

////////////////////////////////////////////////////////////////////////////
// Asynchronous processing of updated world layer groups.
// Involves turning on/off obstacle groups, which in turn involves
// enabling/disabling obstacles and nodesets.
////////////////////////////////////////////////////////////////////////////
class CTaskLayerVisibilityProcessing : public CTaskManager::CAsyncTask
{
	typedef CTaskManager::CAsyncTask Super;
protected:
	TDynArray< SLayerMapping >				m_updatedLayers;
	CWorldLayersMapping*					m_worldLayers;
public:
	CTaskLayerVisibilityProcessing( PathLib::CTaskManager& taskManager, CWorldLayersMapping* worldLayers );

	// subclass interface task 
	virtual Bool			PreProcessingSynchronous() override;
	virtual void			Process() override;
	virtual void			PostProcessingSynchronous() override;

	virtual void			DescribeTask( String& outName ) const override;
};



};			// namespace PathLib

