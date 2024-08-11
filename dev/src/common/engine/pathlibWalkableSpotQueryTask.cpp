/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibWalkableSpotQueryTask.h"

#include "pathlibAreaDescription.h"
#include "pathlibCentralNodeFinder.h"
#include "pathlibNavgraph.h"
#include "pathlibWorld.h"

namespace PathLib
{


CWalkableSpotQueryTask::CWalkableSpotQueryTask( CTaskManager& taskManager, CWalkableSpotQueryRequest* request )
	: Super( taskManager, T_QUERY, request->IsRequiringSynchronousCompletionCallback() ? FLAG_USE_POSTPROCESSING : 0, EPriority::WalkableSpotQuery )
	, m_request( request )
{

}

Bool CWalkableSpotQueryTask::PerformQuery()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();
	CWalkableSpotQueryRequest& request = *m_request;

	// setup test bbox
	Box& bbox = request.m_testBox;
	Float& maxDist = request.m_maxDist;
	Vector destination = request.m_destinationPos;

	auto funUpdateBBox =
		[ &bbox, &destination, &maxDist ] ()
	{
		Box cropBox( Vector( destination ), maxDist );
		bbox.Crop( cropBox );
	};

	funUpdateBBox();

	// consider if we have starting region
	CAreaDescription* baseArea;
	CCentralNodeFinder::RegionAcceptor regionSetup;
	Bool onlyReacheableFromSource = request.IsQueringReachableFromSource();
	Bool limitToBaseArea = request.IsLimitedToBaseArea();
	Bool bailOutOnSuccess = request.ShouldBailOutOnSuccess();

	if ( onlyReacheableFromSource || limitToBaseArea )
	{
		baseArea = pathlib.GetAreaAtPosition( request.m_sourcePos );
		if ( !baseArea )
		{
			// couldn't determine base Area
			return false;
		}
	}
	if ( onlyReacheableFromSource )
	{
		CNavGraph* navgraph = baseArea->GetNavigationGraph( request.m_category );
		if ( !navgraph )
		{
			// area doesn't support this agent category
			return false;
		}

		regionSetup = navgraph->GetNodeFinder().ReachableFrom( request.m_sourcePos, request.m_personalSpace, true, request.m_collisionFlags, request.m_forbiddenNodeFlags );

		if ( regionSetup.IsInvalid() )
		{
			return false;
		}
	}
	else
	{
		baseArea = nullptr;
		regionSetup = CCentralNodeFinder::AnyRegion( false, bailOutOnSuccess );
	}

	// base area test (as it is most obvious answer)
	auto funTestArea =
		[ &request, &regionSetup, &maxDist, &funUpdateBBox ] ( CAreaDescription* area ) -> Bool
	{
		CNavGraph* navgraph = area->GetNavigationGraph( request.m_category );
		if ( !navgraph )
		{
			return false;
		}

		if ( navgraph->GetNodeFinder().FindClosestNode( request, regionSetup ) )
		{
			funUpdateBBox();
			return true;
		}
		return false;
	};

	if ( limitToBaseArea )
	{
		funTestArea( baseArea );

		return true;
	}

	// collect areas to iterate on
	CAreaDescription* areasList[ 16 ];
	Uint32 areasCount = pathlib.CollectAreasAt( bbox, areasList, 16, true );

	Bool areasListProcessPending[ 16 ];
	for ( Uint32 i = 0; i < areasCount; ++i )
	{
		areasListProcessPending[ i ] = true;
	}

	// in first run iterate through areas that contains destination
	for ( Uint32 i = 0; i < areasCount; ++i )
	{
		if ( areasList[ i ]->GetBBox().Contains( destination ) )
		{
			if ( funTestArea( areasList[ i ] ) )
			{
				if ( bailOutOnSuccess )
				{
					return true;
				}
			}
			areasListProcessPending[ i ] = false;
		}
	}

	// in second run iterate through all remaining areas
	for ( Uint32 i = 0; i < areasCount; ++i )
	{
		if ( areasListProcessPending[ i ] && areasList[ i ]->GetBBox().Touches( bbox ) )
		{
			if ( funTestArea( areasList[ i ] ) )
			{
				if ( bailOutOnSuccess )
				{
					return true;
				}
			}
		}
	}

	return true;
}

void CWalkableSpotQueryTask::Process()
{
	CWalkableSpotQueryRequest& request = *m_request;

	PerformQuery();

	request.QueryCompleted();
	// if request doesnt require synchronous callback - run it async
	if ( !request.IsRequiringSynchronousCompletionCallback() )
	{
		request.CompletionCallback();
	}
}

void CWalkableSpotQueryTask::PostProcessingSynchronous()
{
	m_request->CompletionCallback();
}

void CWalkableSpotQueryTask::DescribeTask( String& outName ) const
{
	outName = TXT("Walkable spot query");
}

};			// namespace PathLib

