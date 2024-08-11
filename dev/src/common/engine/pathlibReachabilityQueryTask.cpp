/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibReachabilityQueryTask.h"

#include "pathlibAreaDescription.h"
#include "pathlibCentralNodeFinder.h"
#include "pathlibNavgraph.h"
#include "pathlibWorld.h"

#include "pathlibWalkableSpotQueryImplementation.inl"

namespace PathLib
{

const Float CMultiReachabilityTask::REACHABILITY_TEST_DISTANCE_LIMIT_MAGIC_MULTIPLIER = 1.4142135623730950488016887242097f;		// sqrt(2)

///////////////////////////////////////////////////////////////////////////////
// IReachabilityTaskBase
///////////////////////////////////////////////////////////////////////////////
IReachabilityTaskBase::IReachabilityTaskBase( CTaskManager& taskManager, IReachabilityDataBase* searchData, EPriority priority )
	: Super( taskManager, T_PATHFIND, FLAGS_DEFAULT, priority )
	, m_searchData( searchData )
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	, m_failureReason( IReachabilityDataBase::PATHFAIL_OK )
#endif
{

}

Bool IReachabilityTaskBase::QueryPathEndsPrecise( ComputationContext& context, const Vector3& destination )
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();

	Vector3 posDestination = destination;
	CollisionFlags defaultCollisionFlags = m_searchData->m_defaultCollisionFlags;
	NodeFlags forbiddenNodeFlags = m_searchData->m_forbiddenPathfindFlags;
	PathLib::AreaId hintId = context.m_areaId;
	Bool isOutputPrecise = true;
	Float personalSpace = m_searchData->m_personalSpace;

	if ( !context.m_isDestinationTested && !pathlib.TestLocation( hintId, posDestination, personalSpace, defaultCollisionFlags ) )
	{
		if ( m_searchData->m_searchTolerance <= 0.1f )
		{
			SetFailureReason( CSearchData::PATHFAIL_INVALID_DESTINATION_POS );
			return false;
		}
		Float minZ = posDestination.Z - 1.f;
		Float maxZ = posDestination.Z + 1.f;
		if( !pathlib.FindSafeSpot( hintId, posDestination, Min( personalSpace*2.f, m_searchData->m_searchTolerance ), personalSpace, posDestination, &minZ, &maxZ, defaultCollisionFlags ) )
		{
			SetFailureReason( CSearchData::PATHFAIL_INVALID_DESTINATION_POS );
			return false;
		}
		isOutputPrecise = false;
	}

	PathLib::CAreaDescription* destinationArea = pathlib.GetAreaAtPosition( posDestination, hintId );
	if ( !destinationArea || !destinationArea->IsReady() )
	{
		// destination unaccessible
		SetFailureReason( CSearchData::PATHFAIL_DESTINATION_OUT_OF_NAVDATA );
		return false;
	}

	

	PathLib::CNavGraph* destinationNavGraph = destinationArea->GetNavigationGraph( m_searchData->m_agentCategory );
	if ( !destinationNavGraph )
	{
		// destination area don't support our size
		SetFailureReason( CSearchData::PATHFAIL_DESTINATION_NO_NAVGRAPH );
		return false;
	}

	const CCentralNodeFinder& nf = context.m_startingNavgraph->GetNodeFinder();
	const CCentralNodeFinder& nfDest = destinationNavGraph->GetNodeFinder();
	PathLib::CNavNode* startingNode = context.m_startingNode;

	CCentralNodeFinder::RegionAcceptor region = nf.ReacheableRegion( startingNode->GetAreaRegionId(), true, forbiddenNodeFlags );
	

	// find destination node
	PathLib::CNavNode* destinationNode = nfDest.FindClosestNodeWithLinetest( posDestination, destinationNavGraph->GetMaxNodesDistance(), personalSpace, region, defaultCollisionFlags, forbiddenNodeFlags );
	if ( !destinationNode )
	{
		// no reachable node found, we will try a fallback with reversed search
		destinationNode = nfDest.FindClosestNodeWithLinetest( posDestination, destinationNavGraph->GetMaxNodesDistance(), personalSpace, CCentralNodeFinder::AnyRegion( false, true ), defaultCollisionFlags, forbiddenNodeFlags );
		if ( !destinationNode )
		{
			// couldn't find proper destination
			SetFailureReason( CSearchData::PATHFAIL_NO_DESTINATION_NODE );
			return false;
		}
		region = nfDest.ReacheableRegion( destinationNode->GetAreaRegionId(), true, forbiddenNodeFlags );

		startingNode = nf.FindClosestNodeWithLinetest( m_searchData->m_searchOrigin, context.m_startingNavgraph->GetMaxNodesDistance(), personalSpace, region, defaultCollisionFlags, forbiddenNodeFlags );

		if ( !startingNode )		
		{
			SetFailureReason( CSearchData::PATHFAIL_NO_PATH );
			return false;
		}
	}

	context.m_outputStartingNode = startingNode;
	context.m_outputDestinationNode = destinationNode;
	context.m_outputDestinationPosition = posDestination;
	context.m_outputIsPrecise = isOutputPrecise;

	SetFailureReason( CSearchData::PATHFAIL_OK );

	return true;
}
Bool IReachabilityTaskBase::QueryPathEndsWithTolerance( ComputationContext& context, const Vector3& destination )
{
	struct Implementation
	{
		CPathLibWorld&				m_pathlib;
		IReachabilityDataBase&		m_searchData;
		CNavGraph*					m_baseNavgraph;
		CNavNode*					m_baseNode;
		Box							m_testBBox;
		Vector3						m_destination;
		Vector3						m_outPosition;
		Float						m_testMaxDistance;
		Bool						m_bailOutOnSuccess;

		Implementation( CPathLibWorld& pathlib, IReachabilityDataBase& searchData, const Vector3& destination, CNavGraph* baseNavgraph, CNavNode* baseNode, Bool bailOutOnSuccess )
			: m_pathlib( pathlib )
			, m_searchData( searchData )
			, m_baseNavgraph( baseNavgraph )
			, m_baseNode( baseNode )
			, m_testBBox( Vector( destination ), searchData.m_searchTolerance )
			, m_destination( destination )
			, m_outPosition( destination )
			, m_testMaxDistance( searchData.m_searchTolerance )
			, m_bailOutOnSuccess( bailOutOnSuccess )
		{

		}

		CPathLibWorld&				Query_GetPathLib()
		{
			return m_pathlib;
		}
		Box&						Query_GetBBox()
		{
			return m_testBBox;
		}
		Float&						Query_GetMaxDist()
		{
			return m_testMaxDistance;
		}
		Uint32						Query_GetCategory()
		{
			return m_searchData.m_agentCategory;
		}
		NodeFlags					Query_NodeForbiddenFlags()
		{
			return m_searchData.m_forbiddenPathfindFlags;
		}
		Bool						Query_BailOutOnSuccess()
		{
			return m_bailOutOnSuccess;

		}

		const Vector3&				Query_GetDestination()
		{
			return m_destination;
		}

		void						Query_SetupRegionAcceptor( CCentralNodeFinder::RegionAcceptor& regionSetup )
		{
			regionSetup = m_baseNavgraph->GetNodeFinder().ReacheableRegion( m_baseNode->GetAreaRegionId(), m_bailOutOnSuccess, m_searchData.m_forbiddenPathfindFlags );
		}

		static CAreaDescription*	Query_LimitToBaseArea()
		{
			return nullptr;
		}
		CNavNode*					Query_FindClosestNode( CAreaDescription* area, const CCentralNodeFinder::RegionAcceptor& regionAcceptor )
		{
			CNavGraph* navgraph = area->GetNavigationGraph( m_searchData.m_agentCategory );
			if ( !navgraph )
			{
				return nullptr;
			}
			return navgraph->GetNodeFinder().FindClosestNodeWithLinetestAndTolerance( m_destination, m_testMaxDistance, m_searchData.m_personalSpace, m_outPosition, regionAcceptor, m_searchData.m_defaultCollisionFlags, m_searchData.m_forbiddenPathfindFlags, false );
		}
	};

	// use walkable spot query implementation to find spot in specified tolerance
	Implementation implementation( m_taskManager.GetPathLib(), *m_searchData, destination, context.m_startingNavgraph, context.m_startingNode, context.m_bailOutOnSuccess );

	CNavNode* destinationNode = WalkableSpot::PerformQuery( implementation );
	if ( destinationNode )
	{
		context.m_outputStartingNode = context.m_startingNode;
		context.m_outputDestinationNode = destinationNode;
		context.m_outputDestinationPosition = implementation.m_outPosition;
		context.m_outputIsPrecise = false;

		SetFailureReason( IReachabilityDataBase::PATHFAIL_WITH_TOLERANCE );
		return true;
	}

	return false;
}


EPathfindResult IReachabilityTaskBase::QueryPathEnds( const Vector3& destination, CNavNode*& outStartingNode, CNavNode*& outDestinationNode, Vector3& outDestination, Bool& useTolerance, Bool getAnyOutput )
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();
	IReachabilityDataBase& searchData = *m_searchData;

	ComputationContext context;
	context.m_areaId = searchData.m_currentArea;
	context.m_bailOutOnSuccess = getAnyOutput;
	context.m_isDestinationTested = false;
	const Vector3& posOrigin = searchData.m_searchOrigin;
	const Float personalSpace = searchData.m_personalSpace;
	CollisionFlags defaultCollisionFlags = searchData.m_defaultCollisionFlags;
	NodeFlags forbiddenNodeFlags = searchData.GetForbiddenPathfindFlags();

	
	PathLib::CAreaDescription* area = pathlib.GetAreaAtPosition( posOrigin, context.m_areaId );

	if ( !area )
	{
		// if we are out of walkable ground - fail
		SetFailureReason( CSearchData::PATHFAIL_OUT_OF_NAVDATA );
		return PathLib::PATHRESULT_FAILED_OUTOFNAVDATA;
	}

	// find starting node
	PathLib::CNavGraph* navGraph = area->GetNavigationGraph( searchData.m_agentCategory );
	if ( !navGraph )
	{
		// current area don't support our agent category
		SetFailureReason( CSearchData::PATHFAIL_NO_NAVGRAPH );
		return PathLib::PATHRESULT_FAILED;
	}
	context.m_startingNavgraph = navGraph;
	const PathLib::CCentralNodeFinder& nf = navGraph->GetNodeFinder();

	PathLib::CNavNode* startingNode = nf.FindClosestNodeWithLinetest( posOrigin, navGraph->GetMaxNodesDistance(), personalSpace, CCentralNodeFinder::AnyRegion( false, true ), defaultCollisionFlags, forbiddenNodeFlags );
	if ( !startingNode )
	{
		// couldn't find graph entry point
		SetFailureReason( CSearchData::PATHFAIL_NO_STARTING_NODE );
		return PathLib::PATHRESULT_FAILED;
	}
	context.m_startingNode = startingNode;

	// temporary store output starting node, in case we fails path-end search
	outStartingNode = startingNode;

	if ( !QueryPathEndsPrecise( context, destination ) )
	{
		if ( m_searchData->m_searchTolerance < 0.5f || !QueryPathEndsWithTolerance( context, destination ) )
		{
			return PathLib::PATHRESULT_FAILED;
		}
	}

	// store output
	outStartingNode = context.m_outputStartingNode;
	outDestinationNode = context.m_outputDestinationNode;
	outDestination = context.m_outputDestinationPosition;
	useTolerance = !context.m_outputIsPrecise;

	return PathLib::PATHRESULT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// CReachabilityTask
///////////////////////////////////////////////////////////////////////////////
void CReachabilityTask::Process()
{
	CNavNode* startingNode;
	CNavNode* destinationNode;
	Vector3 destination;
	Bool isPointReachablePrecisely;

	CReachabilityData& reachabilityData = static_cast< CReachabilityData& >( *m_searchData );	

	Bool outcome = QueryPathEnds( reachabilityData.m_searchDestination, startingNode, destinationNode, destination, isPointReachablePrecisely, true ) == PathLib::PATHRESULT_SUCCESS;

	reachabilityData.m_outputDestination = destination;
	reachabilityData.m_outputSuccess = outcome;

#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	// its a little unsafe, but its just a debug code that just won't brake
	reachabilityData.m_failureReason = m_failureReason;
	reachabilityData.m_lastFailedPathfindingDestination = reachabilityData.m_searchDestination;
#endif

	reachabilityData.m_asyncTaskResultPending = true;
	reachabilityData.m_isAsyncTaskRunning.SetValue( false );
}



CReachabilityTask::Ptr CReachabilityTask::Request( CPathLibWorld& pathlib, CReachabilityData* searchData )
{
	CTaskManager* taskManager = pathlib.GetTaskManager();
	Ptr task = new CReachabilityTask( *taskManager, searchData );
	taskManager->AddTask( task.Get() );
	task->Release();			// as ptr already boosted ref count to 2
	return task;
}

void CReachabilityTask::DescribeTask( String& outName ) const
{
	outName = TXT("Reachability query");
}

///////////////////////////////////////////////////////////////////////////////
// CMultiReachabilityTask
///////////////////////////////////////////////////////////////////////////////
Bool CMultiReachabilityTask::QueryReachability()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();

	CMultiReachabilityData& reachabilityData = static_cast< CMultiReachabilityData& >( *m_searchData );	

	ComputationContext context;
	context.m_areaId = reachabilityData.m_currentArea;
	context.m_bailOutOnSuccess = true;
	context.m_isDestinationTested = true;
	const Vector3& posOrigin = reachabilityData.m_searchOrigin;
	const Float personalSpace = reachabilityData.m_personalSpace;
	CollisionFlags defaultCollisionFlags = reachabilityData.m_defaultCollisionFlags;
	NodeFlags forbiddenNodeFlags = reachabilityData.GetForbiddenPathfindFlags();
	Bool usePathfindDistance = reachabilityData.m_usePathfindDistanceLimit;
	CMultiReachabilityData::EQueryType queryType = reachabilityData.m_queryType;

	reachabilityData.m_closestTargetIdx = -1;
	reachabilityData.m_closestTargetDistance = -1.f;

	PathLib::CAreaDescription* area = pathlib.GetAreaAtPosition( posOrigin, context.m_areaId );

	if ( !area )
	{
		// if we are out of walkable ground - fail
		SetFailureReason( CSearchData::PATHFAIL_OUT_OF_NAVDATA );
		return false;
	}

	// find starting node
	PathLib::CNavGraph* startingNavGraph = area->GetNavigationGraph( reachabilityData.m_agentCategory );
	if ( !startingNavGraph )
	{
		// current area don't support our agent category
		SetFailureReason( CSearchData::PATHFAIL_NO_NAVGRAPH );
		return false;
	}
	context.m_startingNavgraph = startingNavGraph;
	const PathLib::CCentralNodeFinder& nf = startingNavGraph->GetNodeFinder();

	PathLib::CNavNode* startingNode = nf.FindClosestNodeWithLinetest( posOrigin, startingNavGraph->GetMaxNodesDistance(), personalSpace, CCentralNodeFinder::AnyRegion( false, true ), defaultCollisionFlags, forbiddenNodeFlags );
	if ( !startingNode )
	{
		if ( queryType == CMultiReachabilityData::REACH_ANY )
		{
			if ( QueryReachabilitySimplified() )
				return true;
		}
		// couldn't find graph entry point
		SetFailureReason( CSearchData::PATHFAIL_NO_STARTING_NODE );
		return false;
	}
	context.m_startingNode = startingNode;
	
	Bool outcome = false;

	TDynArray< CMultiReachabilityData::Destination >& destinationList = reachabilityData.m_searchDestinations;

	for ( auto it = destinationList.Begin(), end = destinationList.End(); it != end; ++it )
	{
		CMultiReachabilityData::Destination& dest = *it;

		// perform base destination test
		AreaId areaId = context.m_areaId;
		if ( !pathlib.TestLocation( areaId, dest.m_dest, defaultCollisionFlags ) )
		{
			if ( !reachabilityData.m_correctDestinationPositions || !pathlib.MoveAwayFromWall( context.m_areaId, dest.m_dest, personalSpace, dest.m_dest, defaultCollisionFlags ) )
			{
				dest.m_result = PATHRESULT_FAILED;
			}
			
		}

		// some destinations may be already processed
		if ( dest.m_result == PATHRESULT_PENDING )
		{
			// perform trivial test
			CWideLineQueryData::MultiArea query( CWideLineQueryData( defaultCollisionFlags, posOrigin, dest.m_dest, personalSpace ) );
			if ( area->TMultiAreaQuery( query ) )
			{
				dest.m_result = PATHRESULT_SUCCESS;
			}
			else
			{
				dest.m_result = QueryPathEndsPrecise( context, dest.m_dest )
					? PATHRESULT_SUCCESS
					: PATHRESULT_FAILED;

				if ( dest.m_result == PATHRESULT_SUCCESS )
				{
					dest.m_originNode = context.m_outputStartingNode;
					dest.m_destinationNode = context.m_outputDestinationNode;
				}
			}
		}

		if ( dest.m_result == PATHRESULT_SUCCESS )
		{
			outcome = true;
			
			// success
			if ( queryType == CMultiReachabilityData::REACH_ANY && !usePathfindDistance )
			{
				break;
			}
		}
		else
		{
			// failure
			if ( queryType == CMultiReachabilityData::REACH_ALL )
			{
				outcome = false;
				if ( !usePathfindDistance )
				{
					break;
				}
			}
		}
	}

	// inner search
	if ( outcome && usePathfindDistance )
	{
		CSearchEngine& searchEngine = pathlib.GetSearchEngine();

		CMultiQueryData pathfindQuery;
		pathfindQuery.m_start = startingNode;
		pathfindQuery.m_graph = startingNavGraph;
		pathfindQuery.m_flagsForbidden = forbiddenNodeFlags;
		pathfindQuery.m_searchLimit = CalculatePathCost( reachabilityData.m_pathfindDistanceLimit * REACHABILITY_TEST_DISTANCE_LIMIT_MAGIC_MULTIPLIER );
		pathfindQuery.m_breakIfAnyNodeIsFound = queryType == CMultiReachabilityData::REACH_ANY;

		Bool hasDirectConnections = false;
		for ( auto it = destinationList.Begin(), end = destinationList.End(); it != end; ++it )
		{
			CMultiReachabilityData::Destination& dest = *it;

			if ( dest.m_result == PATHRESULT_SUCCESS )
			{
				if ( dest.m_destinationNode )
				{
					pathfindQuery.m_destinationList.PushBackUnique( dest.m_destinationNode );
					if ( dest.m_originNode != startingNode )
					{
						pathfindQuery.m_originsList.PushBackUnique( dest.m_originNode );
					}
				}
				else
				{
					hasDirectConnections = true;
				}
			}
		}

		CSearchNode::Marking marking( searchEngine.QueryMultipleDestinations( pathfindQuery ) );
		outcome = hasDirectConnections;
		for ( auto it = destinationList.Begin(), end = destinationList.End(); it != end; ++it )
		{
			CMultiReachabilityData::Destination& dest = *it;
			if ( dest.m_result == PATHRESULT_SUCCESS && dest.m_destinationNode )
			{
				dest.m_result = marking.CheckMarking( *dest.m_destinationNode )
					? PATHRESULT_SUCCESS
					: PATHRESULT_FAILED;

				if ( dest.m_result == PATHRESULT_SUCCESS )
				{
					outcome = true;
				}
				else
				{
					// failure
					if ( queryType == CMultiReachabilityData::REACH_ALL )
					{
						outcome = false;
					}
				}
			}
		}

		if ( outcome && reachabilityData.m_computeClosestTargetDistance )
		{
			Float lowestCost = FLT_MAX;
			Int32 closestDestinationIdx = -1;
			CSearchNode* bestNode = nullptr;
			
			for ( Int32 destinationIdx = 0, n = destinationList.Size(); destinationIdx != n; ++destinationIdx )
			{
				CMultiReachabilityData::Destination& dest = destinationList[ destinationIdx ];
				if ( dest.m_destinationNode )
				{
					if ( marking.CheckMarking( *dest.m_destinationNode ) )
					{
						Float nodeCost = ConvertPathCostToDistance( dest.m_destinationNode->GetCurrentCost() );
						
						if ( nodeCost < lowestCost )
						{
							closestDestinationIdx = destinationIdx;
							lowestCost = nodeCost;
						}
					}
				}
				else
				{
					Float nodeCost = (posOrigin - dest.m_dest).Mag();
					if ( nodeCost < lowestCost )
					{
						closestDestinationIdx = destinationIdx;
						lowestCost = nodeCost;
					}
				}
				
			}

			if ( closestDestinationIdx >= 0 )
			{
				reachabilityData.m_closestTargetIdx = closestDestinationIdx;

				CMultiReachabilityData::Destination& dest = destinationList[ closestDestinationIdx ];

				if ( dest.m_destinationNode )
				{
					reachabilityData.m_closestTargetDistance = CSearchEngine::ComputeOptimizedPathLength( pathlib, dest.m_dest, posOrigin, dest.m_destinationNode, personalSpace, defaultCollisionFlags );
				}
				else
				{
					reachabilityData.m_closestTargetDistance = lowestCost;
				}

				if ( reachabilityData.m_closestTargetDistance > reachabilityData.m_pathfindDistanceLimit )
				{
					outcome = false;
				}
			}
			else
			{
				reachabilityData.m_closestTargetIdx = -1;
				reachabilityData.m_closestTargetDistance = reachabilityData.m_pathfindDistanceLimit;
			}
		}
	}

	return outcome;
}

Bool CMultiReachabilityTask::QueryReachabilitySimplified()
{
	CPathLibWorld& pathlib = m_taskManager.GetPathLib();
	CMultiReachabilityData& reachabilityData = static_cast< CMultiReachabilityData& >( *m_searchData );	

	AreaId areaId = reachabilityData.m_currentArea;
	const Vector3& posOrigin = reachabilityData.m_searchOrigin;
	const Float personalSpace = reachabilityData.m_personalSpace;
	const CollisionFlags defaultCollisionFlags = reachabilityData.m_defaultCollisionFlags;
	const NodeFlags forbiddenNodeFlags = reachabilityData.GetForbiddenPathfindFlags();
	const Bool usePathfindDistance = reachabilityData.m_usePathfindDistanceLimit;
	const CMultiReachabilityData::EQueryType queryType = reachabilityData.m_queryType;

	Bool outcome = false;
	Float lowestCost = FLT_MAX;
	Int32 closestDestinationIdx = -1;

	TDynArray< CMultiReachabilityData::Destination >& destinationList = reachabilityData.m_searchDestinations;

	for ( Int32 destinationIdx = 0, n = destinationList.Size(); destinationIdx != n; ++destinationIdx )
	{
		CMultiReachabilityData::Destination& dest = destinationList[ destinationIdx ];

		if ( dest.m_result == PATHRESULT_PENDING )
		{
			const Float distanceSquared = ( posOrigin - dest.m_dest ).SquareMag();
			
			// perform trivial test
			if ( pathlib.TestLine( areaId, posOrigin, dest.m_dest, personalSpace, defaultCollisionFlags ) )
			{
				if ( !usePathfindDistance || ( distanceSquared < reachabilityData.m_pathfindDistanceLimit * reachabilityData.m_pathfindDistanceLimit ) )
				{
					dest.m_result = PATHRESULT_SUCCESS;
				}
			}
			if ( reachabilityData.m_computeClosestTargetDistance )
			{
				const Float nodeCost = ( posOrigin - dest.m_dest ).Mag();
				if ( nodeCost < lowestCost )
				{
					closestDestinationIdx = destinationIdx;
					lowestCost = nodeCost;
				}
			}
		}

		if ( dest.m_result == PATHRESULT_SUCCESS )
		{
			outcome = true;

			// success
			if ( !reachabilityData.m_computeClosestTargetDistance )
			{
				break;
			}
		}

		if ( closestDestinationIdx != -1 )
		{
			reachabilityData.m_closestTargetIdx = closestDestinationIdx;
			reachabilityData.m_closestTargetDistance = lowestCost;
		}
		else
		{
			reachabilityData.m_closestTargetIdx = -1;
			reachabilityData.m_closestTargetDistance = reachabilityData.m_pathfindDistanceLimit;
		}
	}

	return outcome;
}

void CMultiReachabilityTask::Process()
{
	CMultiReachabilityData& reachabilityData = static_cast< CMultiReachabilityData& >( *m_searchData );	
	Bool outcome = QueryReachability();

	reachabilityData.m_outputSuccess = outcome;

	reachabilityData.m_asyncTaskResultPending = true;
	reachabilityData.m_isAsyncTaskRunning.SetValue( false );
}

CMultiReachabilityTask::Ptr CMultiReachabilityTask::Request( CPathLibWorld& pathlib, CMultiReachabilityData* searchData )
{
	CTaskManager* taskManager = pathlib.GetTaskManager();
	Ptr task = new CMultiReachabilityTask( *taskManager, searchData );
	taskManager->AddTask( task.Get() );
	task->Release();			// as ptr already boosted ref count to 2
	return task;
}

void CMultiReachabilityTask::DescribeTask( String& outName ) const
{
	outName = TXT("Multireachability query");
}



};			// namespace PathLib

