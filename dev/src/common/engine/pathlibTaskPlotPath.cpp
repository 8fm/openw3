/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTaskPlotPath.h"

#include "pathlibAreaDescription.h"
#include "pathlibCentralNodeFinder.h"
#include "pathlibNavgraph.h"
#include "pathlibSearchEngine.h"
#include "pathlibTerrain.h"
#include "pathlibWorld.h"


namespace PathLib
{

EPathfindResult CTaskPlotPath::PlotPath()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();
	CSearchData& searchData = static_cast< CSearchData& > ( *m_searchData );

	CNavNode* startingNode = nullptr;
	CNavNode* destinationNode = nullptr;
	Vector3 posDestination = searchData.m_searchDestination;
	Bool useTolerance = false;

	EPathfindResult result = QueryPathEnds( searchData.m_searchDestination, startingNode, destinationNode, posDestination, useTolerance );
	if ( result != PATHRESULT_SUCCESS )
	{
		if ( searchData.m_findClosestSpotPathfindingEnabled && startingNode )
		{
			CFindClosestSpotQueryData queryInput;
			queryInput.m_start					= startingNode;
			queryInput.m_flagsForbidden			= searchData.m_forbiddenPathfindFlags;
			queryInput.m_destinationPosition	= searchData.m_searchDestination;
			queryInput.m_destination			= nullptr;
			queryInput.m_lookForConnectors		= true;
			queryInput.m_distanceLimit			= searchData.m_closestSpotPathfindingDistanceLimit;

			CTerrainAreaDescription* terrain = pathlib.GetTerrainAreaAtPosition( posDestination );
			if ( terrain )
			{
				queryInput.m_lookForConnectors = false;
			}


			if ( !pathlib.GetSearchEngine().FindPathToClosestSpot( queryInput ) )
			{
				// path not found
				SetFailureReason( CSearchData::PATHFAIL_FAILED_PLOTPATH );
				return PathLib::PATHRESULT_FAILED;
			}

			// collect outcome data
			searchData.m_outputPathUseTolerance = useTolerance;
			searchData.m_outputPathDestinationWasOutsideOfStreamingRange = queryInput.m_lookForConnectors;
			searchData.CollectPath( &pathlib, queryInput, false );

			SetFailureReason( CSearchData::PATHFAIL_WITH_TOLERANCE );
			return PathLib::PATHRESULT_SUCCESS;
		}

		return result;
	}

	// search path
	CQueryData queryInput;
	queryInput.m_start					= startingNode;
	queryInput.m_flagsForbidden			= searchData.m_forbiddenPathfindFlags;
	queryInput.m_destinationPosition	= posDestination;
	queryInput.m_destination			= destinationNode;
#ifdef PATHLIB_SUPPORT_PATH_RATER
	queryInput.m_pathRater				= searchData.m_pathRater.Get();
#endif
	if ( !pathlib.GetSearchEngine().FindPath( queryInput ) )
	{
		// path not found
		SetFailureReason( CSearchData::PATHFAIL_FAILED_PLOTPATH );
		return PathLib::PATHRESULT_FAILED;
	}
	// collect outcome data
	searchData.m_outputPathUseTolerance = useTolerance;
	searchData.m_outputPathDestinationWasOutsideOfStreamingRange = false;
	searchData.CollectPath( &pathlib, queryInput );

	SetFailureReason( CSearchData::PATHFAIL_OK );
	return PathLib::PATHRESULT_SUCCESS;
}

void CTaskPlotPath::Process()
{
	CSearchData& searchData = static_cast< CSearchData& > ( *m_searchData );

	// NOTICE: use 'friend' access to touch search data protected properties
	searchData.m_asyncTaskResult = PlotPath();
	searchData.m_asyncTaskResultPending = true;
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	// its a little unsafe, but its just a debug code that just won't brake
	searchData.m_failureReason = m_failureReason;
	searchData.m_lastFailedPathfindingDestination = searchData.m_searchDestination;
#endif
	searchData.m_isAsyncTaskRunning.SetValue( false );

	//Float timeEnd = Float( Red::System::Clock::GetInstance().GetTimer().GetSeconds() );
	//PATHLIB_LOG( TXT("Plotpath completed Took %f"), timeEnd - timeStart );
}

void CTaskPlotPath::DescribeTask( String& outName ) const
{
	outName = TXT("Plot path");
}

};			// namespace PathLib