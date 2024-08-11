/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibSearchData.h"

#include "pathlibAgent.h"
#include "pathlibAreaDescription.h"
#include "pathlibTaskPlotPath.h"
#include "pathlibNavgraph.h"
#include "pathlibNodeSet.h"
#include "pathlibMetalink.h"
#include "pathlibMetalinkComponent.h"
#include "pathlibUtils.h"
#include "pathlibWorld.h"


namespace PathLib
{



////////////////////////////////////////////////////////////////////////////
// IReachabilityDataBase
////////////////////////////////////////////////////////////////////////////
IReachabilityDataBase::IReachabilityDataBase( Float personalSpace )
	: m_refCount( 0 )
	, m_personalSpace( personalSpace )
	, m_agentCategory( 0 )
	, m_outputPathUseTolerance( false )
	, m_currentArea( INVALID_AREA_ID )
	, m_defaultCollisionFlags( CT_DEFAULT )
	, m_searchOrigin( 0,0,0 )
	, m_searchTolerance( 0.f )
	, m_forbiddenPathfindFlags( NFG_FORBIDDEN_BY_DEFAULT )
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	, m_failedRecently( false )
	, m_failureReason( PATHFAIL_NOT_YET_PATHFIND )
#endif
{

}

IReachabilityDataBase::~IReachabilityDataBase()
{

}

void IReachabilityDataBase::Initialize( CPathLibWorld* pathlib, Float personalSpace )
{
	m_personalSpace = personalSpace;
	m_agentCategory = pathlib->GetGlobalSettings().ComputePSCategory( m_personalSpace );
}

Bool IReachabilityDataBase::TestLine( CPathLibWorld* world, const Vector3& pos1, const Vector3& pos2, CollisionFlags collisionFlags )
{
	return world->TestLine( m_currentArea, pos1, pos2, m_personalSpace, m_defaultCollisionFlags | collisionFlags );
}
Bool IReachabilityDataBase::TestLine( CPathLibWorld* world, const Vector3& pos1, const Vector3& pos2, Float personalSpace, CollisionFlags collisionFlags )
{
	if ( personalSpace == 0.f )
	{
		return world->TestLine( m_currentArea, pos1, pos2, m_defaultCollisionFlags | collisionFlags );
	}
	else
	{
		return world->TestLine( m_currentArea, pos1, pos2, personalSpace, m_defaultCollisionFlags | collisionFlags );
	}
}
Bool IReachabilityDataBase::TestLocation( CPathLibWorld* world, const Vector3& pos, CollisionFlags collisionFlags )
{
	return world->TestLocation( m_currentArea, pos, m_personalSpace, m_defaultCollisionFlags | collisionFlags );
}

Bool IReachabilityDataBase::TestLocation( CPathLibWorld* world, const Vector3& pos, Float radius, CollisionFlags collisionFlags )
{
	return world->TestLocation( m_currentArea, pos, radius, m_defaultCollisionFlags | collisionFlags );
}

#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
void IReachabilityDataBase::Debug_PathfindOutcome( String& outOutcome )
{
	switch( m_failureReason )
	{
	case PATHFAIL_NOT_YET_PATHFIND:
		outOutcome = TXT("");
		break;
	case PATHFAIL_OK:
		outOutcome = TXT("Pathfind ok");
		break;
	case PATHFAIL_WITH_TOLERANCE:
		outOutcome = TXT("Pathfind unprecise");
		break;
	case PATHFAIL_INVALID_STARTING_POS:
		outOutcome = TXT("Invalid starting pos");
		break;
	case PATHFAIL_INVALID_DESTINATION_POS:
		outOutcome = TXT("Invalid destination pos");
		break;
	case PATHFAIL_OUT_OF_NAVDATA:
		outOutcome = TXT("Starting pos out of navdata");
		break;
	case PATHFAIL_NO_NAVGRAPH:
		outOutcome = TXT("No navgraph");
		break;
	case PATHFAIL_NO_STARTING_NODE:
		outOutcome = TXT("No starting node");
		break;
	case PATHFAIL_DESTINATION_OUT_OF_NAVDATA:
		outOutcome = TXT("Destination pos out of navdata");
		break;
	case PATHFAIL_DESTINATION_NO_NAVGRAPH:
		outOutcome = TXT("No destination navgraph");
		break;
	case PATHFAIL_NO_DESTINATION_NODE:
		outOutcome = TXT("No destination node");
		break;
	case PATHFAIL_NO_PATH:
		outOutcome = TXT("No path");
		break;
	case PATHFAIL_FAILED_PLOTPATH:
		outOutcome = TXT("Failed plot path");
		break;
	default:
		ASSERT( false );
		ASSUME( false );
	}
}
#endif



////////////////////////////////////////////////////////////////////////////
// CReachabilityData
////////////////////////////////////////////////////////////////////////////
CReachabilityData::CReachabilityData( Float personalSpace )
	: IReachabilityDataBase( personalSpace )
	, m_isAsyncTaskRunning( false )
	, m_asyncTaskResultPending( false )
	, m_asyncTaskInvalidated( false )
	, m_asyncTaskResult( EPathfindResult::PATHRESULT_FAILED )
	, m_lastOutputSuccessful( false )
{

}

CReachabilityData::~CReachabilityData()
{
	ASSERT( m_isAsyncTaskRunning.GetValue() == 0, TXT("Async task should boost up reachability data ref count!") );
}

EPathfindResult CReachabilityData::QueryReachable( CPathLibWorld* world, const Vector3& startingPos, const Vector3& destinationPos, Float tolerance, Vector3* outDestinationPosition )
{
	if ( m_isAsyncTaskRunning.GetValue() )
	{
		return PATHRESULT_PENDING;
	}

	if ( m_asyncTaskResultPending )
	{
		m_lastOutputSource = m_searchOrigin;
		m_lastOutputDestination = m_outputSuccess ? m_outputDestination : m_searchDestination;
		m_lastOutputSuccessful = m_outputSuccess;

		m_asyncTaskResultPending = false;
	}

	Bool lastOutputValid = false;

	Bool isStartingPosSame = (startingPos - m_lastOutputSource).IsAlmostZero( 0.1f );
	// test starting position
	if ( isStartingPosSame || world->TestLine( m_currentArea, m_lastOutputSource, startingPos, m_personalSpace, m_defaultCollisionFlags ) )
	{
		// check if previous result is still valid
		if ( m_lastOutputSuccessful )
		{
			// Last time we have reached the target. Lets check if current requested position is inside tolerance distance from previous outcome or is accessible from it.
			// Also we need to test if current origin position is accessible from previous one
			Vector3 destDiff = destinationPos - m_lastOutputDestination;
			Float destDiffLenSq = destDiff.SquareMag();
			Bool isDestPosSame = destDiffLenSq < (0.1f*0.1f);

			// test destination position
			if ( isDestPosSame
				|| (tolerance > 0.f && destDiffLenSq < tolerance*tolerance )
				|| world->TestLine( m_currentArea, m_lastOutputDestination, destinationPos, m_personalSpace, m_defaultCollisionFlags )
				)
			{
				lastOutputValid = true;
			}
		}
		else
		{
			// last time we wasn't able to reach target. One of two things have to change - either we moved to place unreachable from initial origin
			// or destination was moved to different place
			Vector3 destDiff = destinationPos - m_lastOutputDestination;
			Float destDiffLenSq = destDiff.SquareMag();
			Bool isDestPosSame = destDiffLenSq < (0.1f*0.1f);

			Float ignoreDist = (tolerance*0.25f);

			if ( isDestPosSame
				|| ( tolerance > 0.f && destDiffLenSq < (ignoreDist*ignoreDist) ) )
			{
				lastOutputValid = true;
			}
		}
	}

	if ( lastOutputValid )
	{
		if ( outDestinationPosition )
		{
			(*outDestinationPosition) = m_lastOutputDestination;
		}
		return m_lastOutputSuccessful ? PATHRESULT_SUCCESS : PATHRESULT_FAILED;
	}

	// check for trivial case if starting pos or destination are unreachable
	// now check for trivial failure case (with is quite problematic to handle later on)
	if ( !world->TestLocation( m_currentArea, startingPos, m_personalSpace, m_defaultCollisionFlags ) )
	{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
		m_failedRecently = true;
		m_lastFailedPathfindingDestination = destinationPos;
		m_failureReason = PATHFAIL_INVALID_STARTING_POS;
#endif
		return PATHRESULT_FAILED;
	}
	// when tolerance is on we don't care it target is reachable...
	if ( tolerance <= 0.1f && !world->TestLocation( m_currentArea, destinationPos, m_personalSpace, m_defaultCollisionFlags ) )
	{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
		m_failedRecently = true;
		m_lastFailedPathfindingDestination = destinationPos;
		m_failureReason = PATHFAIL_INVALID_DESTINATION_POS;
#endif
		return PATHRESULT_FAILED;
	}

	m_searchOrigin = startingPos;
	m_searchDestination = destinationPos;
	m_searchTolerance = tolerance;

	if ( world->TestLine( m_currentArea, startingPos, destinationPos, m_personalSpace, m_defaultCollisionFlags ) )
	{
		m_outputSuccess = true;
		m_outputDestination = destinationPos;
		m_outputPathUseTolerance = false;

		return PATHRESULT_SUCCESS;
	}

	m_asyncTaskInvalidated = false;
	m_isAsyncTaskRunning.SetValue( true );
	CTaskManager* taskManager = world->GetTaskManager();
	CReachabilityTask* task = new CReachabilityTask( *taskManager, this );
	taskManager->AddTask( task );
	task->Release();
	return PATHRESULT_PENDING;

}



////////////////////////////////////////////////////////////////////////////
// CMultiReachabilityData
////////////////////////////////////////////////////////////////////////////

CMultiReachabilityData::CMultiReachabilityData( Float personalSpace )
	: IReachabilityDataBase( personalSpace )
	, m_isAsyncTaskRunning( false )
	, m_asyncTaskResultPending( false )
	, m_lastOutputSuccessful( false )
	, m_usePathfindDistanceLimit( false )
	, m_correctDestinationPositions( false )
	, m_lastOutputSource( 0.f, 0.f, 0.f )
{

}

CMultiReachabilityData::~CMultiReachabilityData()
{
	ASSERT( m_isAsyncTaskRunning.GetValue() == 0, TXT("Async task should boost up reachability data ref count!") );
}

CMultiReachabilityData::QuerySetupInterface CMultiReachabilityData::SetupReachabilityQuery( EQueryType queryType, const Vector3& startingPos, NodeFlags nf )
{
	// if query is running - setup must fail
	if ( m_isAsyncTaskRunning.GetValue() )
	{
		return QuerySetupInterface();
	}

	// as we changed setup - last outcome is broken
	m_asyncTaskInvalidated = true;

	// setup query
	m_queryType = queryType;
	m_usePathfindDistanceLimit = false;
	m_computeClosestTargetDistance = false;
	m_correctDestinationPositions = false;
	m_forbiddenPathfindFlags = nf;
	m_searchOrigin = startingPos;
	m_searchDestinations.ClearFast();

	// provide interface to further setup query
	return QuerySetupInterface( this );
}

EPathfindResult CMultiReachabilityData::QueryReachable( CPathLibWorld* world )
{
	// if query is running - bail out
	if ( m_isAsyncTaskRunning.GetValue() )
	{
		return PATHRESULT_PENDING;
	}

	// if result is pending - thats cool
	if ( m_asyncTaskResultPending )
	{
		m_asyncTaskResultPending = false;

		m_lastOutputSource = m_searchOrigin;
		m_lastOutputSuccessful = m_outputSuccess;
	}

	// return last result if its valid
	if ( !m_asyncTaskInvalidated )
	{
		return m_lastOutputSuccessful ? PATHRESULT_SUCCESS : PATHRESULT_FAILED;
	}

	m_asyncTaskInvalidated = false;

	// run async query
	m_isAsyncTaskRunning.SetValue( true );
	CTaskManager* taskManager = world->GetTaskManager();
	CMultiReachabilityTask* task = new CMultiReachabilityTask( *taskManager, this );
	taskManager->AddTask( task );
	task->Release();
	return PATHRESULT_PENDING;
}

////////////////////////////////////////////////////////////////////////////
// CSearchData
////////////////////////////////////////////////////////////////////////////
CSearchData::CSearchData( Float personalSpace, ClassId classMask )
	: IReachabilityDataBase( personalSpace )
	, m_searchDestination( 0.f, 0.f, 0.f )
	, m_isAsyncTaskRunning( false )
	, m_asyncTaskResultPending( false )
	, m_asyncTaskInvalidated( false )
	, m_asyncTaskResult( EPathfindResult::PATHRESULT_FAILED )
	, m_forcePathfindingInTrivialCase( false )
	, m_findClosestSpotPathfindingEnabled( false )
	, m_doHeavyPathOptimization( false )
	, m_outputPathDestinationWasOutsideOfStreamingRange( false )
	, m_classMask( classMask )
	, m_closestSpotPathfindingDistanceLimit( 64.f )
	, m_pathRater( nullptr )
{

}

CSearchData::~CSearchData()
{
	ASSERT( m_isAsyncTaskRunning.GetValue() == 0, TXT("Async task should boost up search data ref count!") );
}


Bool CSearchData::UpdatePathDestination( CPathLibWorld* world, const Vector3& startingPos, const Vector3& newDestination )
{
	if ( m_outputWaypoints.Size() >= 2 )
	{
		const Vector3& lastPoint = m_outputWaypoints[ m_outputWaypoints.Size() - 2 ];
		if ( TestLine( world, lastPoint, newDestination, m_personalSpace ) )
		{
			m_outputWaypoints[ m_outputWaypoints.Size() - 1 ] = newDestination;
			return true;
		}
	}
	return false;
}
EPathfindResult CSearchData::PlotPath( CPathLibWorld* world, const Vector3& startingPos, const Vector3& destinationPos, Float tolerance )
{
	//OnPathfindRequest();
	Bool isTaskRunning = m_isAsyncTaskRunning.GetValue();

	// if we are running plot path task now - bail out
	if ( isTaskRunning )
	{
		return PATHRESULT_PENDING;
	}

	// if recent plot path task has just completed
	if ( m_asyncTaskResultPending && !m_asyncTaskInvalidated )
	{
		m_asyncTaskResultPending = false;

		// check if given request is still valid
		Bool isStartingPosSame = (startingPos - m_searchOrigin).IsAlmostZero( 0.1f );
		Bool isDestPosSame = (destinationPos - m_searchDestination).IsAlmostZero( 0.1f );

		Bool isAsyncResultSuitable =
			(isStartingPosSame || world->TestLine( m_currentArea, m_searchOrigin, startingPos, m_personalSpace, m_defaultCollisionFlags ))
			&& (m_outputPathUseTolerance
			|| (isDestPosSame || world->TestLine( m_currentArea, m_searchDestination, destinationPos, m_personalSpace, m_defaultCollisionFlags )) );


		// if request is valid - use its output
		if ( isAsyncResultSuitable )
		{
			ASSERT( m_asyncTaskResult != PATHRESULT_SUCCESS || m_outputWaypoints.Size() > 0 );
			if ( m_asyncTaskResult == PATHRESULT_SUCCESS && !isDestPosSame )
			{
				if ( m_outputPathUseTolerance )
				{
					if ( ( m_outputWaypoints.Last() - destinationPos ).SquareMag() < tolerance*tolerance )
					{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
						m_failedRecently = false;
#endif
						return PATHRESULT_SUCCESS;
					}
				}
				else if ( UpdatePathDestination( world, startingPos, destinationPos ) )
				{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
					m_failedRecently = false;
#endif
					return PATHRESULT_SUCCESS;
				}
			}
			else
			{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
				m_failedRecently = m_asyncTaskResult != PATHRESULT_SUCCESS;
#endif
				return m_asyncTaskResult;
			}

		}

		// if request is not valid - make new one
	}

	// firstly check for trivial case (with is quite often the case)
	if ( !m_forcePathfindingInTrivialCase
		&& TestLine( world, startingPos, destinationPos, m_personalSpace, CT_DEFAULT ) )
	{
		m_outputMetalinksStack.ClearFast();

		m_outputWaypoints.ResizeFast( 2 );

		m_outputWaypoints[ 0 ] = startingPos;
		m_outputWaypoints[ 1 ] = destinationPos;


#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
		m_failedRecently = false;
		m_failureReason = PATHFAIL_OK;
#endif

		return PATHRESULT_SUCCESS;
	}

	// now check for trivial failure case (with is quite problematic to handle later on)
	if ( !TestLocation( world, startingPos, m_personalSpace, CT_DEFAULT ) )
	{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
		m_failedRecently = true;
		m_lastFailedPathfindingDestination = destinationPos;
		m_failureReason = PATHFAIL_INVALID_STARTING_POS;
#endif
		return PATHRESULT_FAILED;
	}
	
	// if we have closest spot pathfinding on, there is no trivial failure case
	if ( m_findClosestSpotPathfindingEnabled )
	{
	}
	// when tolerance is on we don't care it target is reachable...
	else if ( tolerance > 0.1f )
	{
		// ... unless target is in tolerance range
		if ( (startingPos - destinationPos).SquareMag() < tolerance*tolerance )
		{
			// already at target, don't come closer if the target is in unreachable zone
			if ( !TestLocation( world, destinationPos, m_personalSpace, CT_DEFAULT ) )
			{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
				m_failedRecently = true;
				m_lastFailedPathfindingDestination = destinationPos;
				m_failureReason = PATHFAIL_WITH_TOLERANCE;
#endif
				return PATHRESULT_FAILED;
			}
		}
	}
	// zer0 tolerance policy
	else if ( !TestLocation( world, destinationPos, m_personalSpace, CT_DEFAULT ) )
	{
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
		m_failedRecently = true;
		m_lastFailedPathfindingDestination = destinationPos;
		m_failureReason = PATHFAIL_INVALID_DESTINATION_POS;
#endif
		return PATHRESULT_FAILED;
	}

	// process new pathfinding request
	m_searchOrigin = startingPos;
	m_searchDestination = destinationPos;
	m_searchTolerance = tolerance;

	m_asyncTaskInvalidated = false;
	m_isAsyncTaskRunning.SetValue( true );
	CTaskManager* taskManager = world->GetTaskManager();
	CTaskPlotPath* task = new CTaskPlotPath( *taskManager, this );
	taskManager->AddTask( task );
	task->Release();
	return PATHRESULT_PENDING;
}

Bool CSearchData::SimplifyPath( CPathLibWorld* world, const Vector3* inWaypoints,Uint32 inWaypointsCount,Vector3* outWaypoints, Uint32& outWaypointsCount, Uint32 waypointsCountLimit )
{
	Float personalSpace = m_personalSpace;
	Uint32 collisionFlags = m_defaultCollisionFlags;

	// initial step
	outWaypointsCount = 1;
	Vector3 currentPosition = inWaypoints[ 0 ];
	outWaypoints[0] = currentPosition;

	// main optimalization loop
	Uint32 unoptimizedWaypoint = 1;

	PathLib::AreaId areaId = m_currentArea;

	do 
	{
		// We do special type of binary search. First we grow the searching range.
		Uint32 binarySearchStepMax = 1;									// Max- exclusive
		Uint32 binarySearchStepMin;										// Min- inclusive
		Vector3 testPosition;
		do
		{
			binarySearchStepMin = binarySearchStepMax;
			binarySearchStepMax <<= 1;
			if ( binarySearchStepMax > inWaypointsCount - unoptimizedWaypoint )
			{
				binarySearchStepMax = inWaypointsCount - unoptimizedWaypoint + 1;
				break;
			}
			testPosition = inWaypoints[ unoptimizedWaypoint + binarySearchStepMax - 1 ];
		}
		while ( world->TestLine( areaId, currentPosition, testPosition, personalSpace, collisionFlags ) );

		// Now we do standard binary search.
		while ( binarySearchStepMax - binarySearchStepMin > 1 )
		{
			int nBinarySearchStep = (binarySearchStepMax + binarySearchStepMin) / 2;
			testPosition = inWaypoints[ unoptimizedWaypoint + nBinarySearchStep - 1 ];
			if ( world->TestLine( areaId, currentPosition, testPosition, personalSpace, collisionFlags ) )
			{
				binarySearchStepMin = nBinarySearchStep;
			}
			else
			{
				binarySearchStepMax = nBinarySearchStep;
			}
		}

		// we store another waypoint
		unoptimizedWaypoint += binarySearchStepMin;
		currentPosition = inWaypoints[ unoptimizedWaypoint-1 ];
		outWaypoints[ outWaypointsCount ] = currentPosition;

		if ( ++outWaypointsCount >= waypointsCountLimit )
		{
			return false;
		}
	}
	while ( unoptimizedWaypoint < inWaypointsCount );

	return true;
}

void CSearchData::OptimizePath( CPathLibWorld* pathlib, Vector3* inOutWaypoints,Uint32 inOutWaypointsCount  )
{
	Float personalSpace = m_personalSpace;
	Uint32 collisionFlags = m_defaultCollisionFlags;

	Vector3 prevWPPos = inOutWaypoints[ 0 ];
	AreaId areaId = m_currentArea;

	for ( Uint32 wp = 1; wp+1 < inOutWaypointsCount; ++wp )
	{
		Vector3 newWPPos = inOutWaypoints[ wp ];
		const Vector3& nextWPPos = inOutWaypoints[ wp+1 ];

		CAreaDescription* area = pathlib->GetAreaAtPosition( newWPPos, areaId );
		ASSERT( area );
		if ( area )
		{
			// actually it would be uber bad case with area being something invalid. But who knows.
			{
				NavUtils::SBinSearchContext context( area, prevWPPos, newWPPos, nextWPPos, personalSpace, 1.f, m_defaultCollisionFlags );

				// move follow point as far as possible
				newWPPos = ::FunctionalBinarySearch( newWPPos, nextWPPos, context );
			}
			
			area = pathlib->GetAreaAtPosition( newWPPos, areaId );
			ASSERT( area );
			// sanity test
			if ( area )
			{
				NavUtils::SBinSearchContext context( area, nextWPPos, newWPPos, prevWPPos, personalSpace, 1.f, m_defaultCollisionFlags );

				// move follow point as close as possible
				newWPPos = ::FunctionalBinarySearch( newWPPos, prevWPPos, context );
			}
		}

		inOutWaypoints[ wp ] = newWPPos;
		prevWPPos = newWPPos;
	}
}

void CSearchData::PostProcessCollectedPath( CPathLibWorld* world, Uint32& pathToSimplifyBeginIndex )
{
	Int32 waypointsToSimplify = m_outputWaypoints.Size() - pathToSimplifyBeginIndex;
	if ( waypointsToSimplify > 2 )
	{
		Vector3 simplifiedPath[ 256 ];
		Uint32 simplifiedPathLength = 0;
		if ( SimplifyPath( world, &m_outputWaypoints[ pathToSimplifyBeginIndex ], waypointsToSimplify, simplifiedPath, simplifiedPathLength, 256 ) )
		{
			if ( m_doHeavyPathOptimization )
			{
				OptimizePath( world, simplifiedPath, simplifiedPathLength );
			}
			m_outputWaypoints.ResizeFast( pathToSimplifyBeginIndex + simplifiedPathLength );
			Red::System::MemoryCopy( &m_outputWaypoints[ pathToSimplifyBeginIndex ], &simplifiedPath, simplifiedPathLength * sizeof( Vector3 ) );
		}
	}
	pathToSimplifyBeginIndex = m_outputWaypoints.Size()+1;
}

Bool CSearchData::CollectPath( CPathLibWorld* pathlib, CQueryData& query, Bool includeDestination )
{
	Uint32 visitedNodes = query.m_path.Size();
	m_outputWaypoints.Reserve( visitedNodes+2 );
	m_outputWaypoints.ResizeFast( 1 );
	m_outputWaypoints[ 0 ] = m_searchOrigin;
	m_outputMetalinksStack.ClearFast();

	Uint32 pathToSimplifyBeginIndex = 0;

	for ( Uint32 i = 0; i < visitedNodes; ++i )
	{
		const CPathNode& node = *query.m_path[ i ];

		// metalink (eg. exploration, doors) support
		if ( node.HaveFlag( NF_IS_CUSTOM_LINK ) && i < visitedNodes - 1 )
		{
			const CPathNode& nextNode = *query.m_path[ i+1 ];
			const CPathLink* link = node.GetLinkTo( &nextNode );
			if ( link && link->HaveFlag( NF_IS_CUSTOM_LINK ) )
			{
				// This stuff is tricky and.. ugly, but I don't want to increase data side for nodes and links
				// basically we assume that custom links connect two nodes of some nav modyficator
				// and so through their nodeset we can get into modyfication itself, and ask it
				// how it want to influence agent movement.

				PathLib::CAreaDescription* area = pathlib->GetAreaDescription( node.GetAreaId() );
				CNavGraph* navgraph = area->GetNavigationGraph( m_agentCategory );
				PATHLIB_ASSERT( navgraph, TXT("Its gonna crash m'lord") );
				CNavgraphNodeSet* nodeSet = navgraph->GetNodeSet( node.GetNodesetIndex() );
				INodeSetPack* pack = nodeSet->GetOwner();
				if ( pack )
				{
					CNavModyfication* navMod = pack->AsNavModyfication();
					if ( navMod )
					{
						IMetalinkSetup* setup = navMod->GetSetup();

						CAgent* agent = As< CAgent >();
						if ( !agent || !setup->AgentPathfollowIgnore( agent ) )
						{
							// we are ready to push metalink, but before we do it we will simplify remaining path
							PostProcessCollectedPath( pathlib, pathToSimplifyBeginIndex );

							MetalinkInteraction m;
							m.m_waypointIndex = m_outputWaypoints.Size();
							m.m_metalinkFlags = setup->GetMetalinkPathfollowFlags();
							m.m_position = node.GetPosition();
							m.m_destination = nextNode.GetPosition();
							m.m_metalinkRuntime.m_mapping = navMod->GetMapping();
							m.m_metalinkSetup = setup;
							m_outputMetalinksStack.PushBack( Move( m ) );
						}
					}
				}
			}
		}

		m_outputWaypoints.PushBack( node.GetPosition() );
	}

	if ( includeDestination )
	{
		m_outputWaypoints.PushBack( query.m_destinationPosition );
	}

	PostProcessCollectedPath( pathlib, pathToSimplifyBeginIndex );

	return true;
}

};			// namespace PathLib

