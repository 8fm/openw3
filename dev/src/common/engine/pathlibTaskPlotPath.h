/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibReachabilityQueryTask.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Main pathfinding logic implementation.
// Notice, that pathfinding (search algorithm) is actually using
// each node's search context data, and so its not really read only
// task, and there can't be any simultanous searches going on.
////////////////////////////////////////////////////////////////////////////
class CTaskPlotPath : public IReachabilityTaskBase
{
	typedef IReachabilityTaskBase Super;
protected:
	EPathfindResult				PlotPath();
public:
	CTaskPlotPath( CTaskManager& taskManager, CSearchData* searchData )
		: Super( taskManager, searchData, EPriority::PlotPath )								{}

	virtual void				Process() override;

	virtual void				DescribeTask( String& outName ) const override;
};

};			// namespace PathLib