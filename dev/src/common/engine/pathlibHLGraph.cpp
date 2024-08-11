/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibHLGraph.h"

#include "pathlibAreaDescription.h"
#include "pathlibNavgraph.h"
#include "pathlibWorld.h"

namespace PathLib
{

namespace HLGraph
{

CHLNode* FindNode( CPathLibWorld& pathlib, Uint32 category, CoherentRegion regionId )
{
	AreaId areaId = AreaIdFromCoherentRegion( regionId );
	AreaRegionId areaRegionId = AreaRegionIdFromCoherentRegion( regionId );

	CAreaDescription* area = pathlib.GetAreaDescription( areaId );
	if ( !area )
	{
		return nullptr;
	}

	CNavGraph* navgraph = area->GetNavigationGraph( category );
	if ( !navgraph )
	{
		return nullptr;
	}
	return navgraph->GetHLGraph().FindHLNode( areaRegionId );
}

Bool MarkAccessibleNodes( CPathLibWorld& pathlib, Uint32 category, CoherentRegion regionId, CAccessibleNodesMarking& outAccessibleNodes )
{
	return false;
}

};			// namespace HLGraph


};			// namespace PathLib