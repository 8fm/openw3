#include "build.h"
#include "movePathIterator.h"
#include "movePathPlannerQuery.h"
#include "moveNavigationPath.h"

///////////////////////////////////////////////////////////////////////////////

CMovePathIterator::CMovePathIterator( INavigable& agent, IMovePathPlannerQuery* query, Bool useReplanning )
	: m_agent( agent )
	, m_query( query )
	, m_useReplanning( useReplanning )
	, m_status( NS_InProgress )
	, m_paused( false )
	, m_navigableIsWaitingForSegment( true )
	, m_queue( new CNavigationGoalsQueue() )
	, m_planner( NULL )
{
	agent.OnNavigationStart();

	m_planner = GCommonGame->GetPathPlanner();
	m_planner->RegisterIterator( m_agent, *this );
}

CMovePathIterator::~CMovePathIterator()
{
	m_planner = GCommonGame->GetPathPlanner();
	if ( m_planner )
	{
		m_planner->RemoveQuery( *this );
		m_planner->UnregisterIterator( m_agent );
		m_planner = NULL;
	}

	for ( TDynArray< IMoveStateController* >::iterator it = m_controllers.Begin(); it != m_controllers.End(); ++it )
	{
		(*it)->Release();
	}
	m_controllers.Clear();

	m_agent.OnNavigationEnd();

	if ( m_query )
	{
		m_query->Release();
		m_query = NULL;
	}

	m_queue->Release();
	m_queue = NULL;
}

// ----------------------------------------------------------------------------
// move controllers management
// ----------------------------------------------------------------------------
void CMovePathIterator::AddController( IMoveStateController* controller )
{
	if ( controller )
	{
		m_controllers.PushBack( controller );
		controller->Activate( *this );
	}
}

void CMovePathIterator::Update()
{
	for ( TDynArray< IMoveStateController* >::iterator it = m_controllers.Begin(); it != m_controllers.End(); ++it )
	{
		(*it)->Update( *this );
	}
}

// ----------------------------------------------------------------------------
// Goals processing
// ----------------------------------------------------------------------------
void CMovePathIterator::Start()
{
	m_paused = false;

	// schedule a path finding query 
	ProcessGoals();
}

void CMovePathIterator::Stop()
{
	m_paused = true;
}

void CMovePathIterator::Cancel()
{
	m_status = NS_Failed;
}

void CMovePathIterator::ProcessGoals()
{
	ASSERT( m_status == NS_InProgress );
	Int32 lastGoalIdx = m_queue->GetLastProcessedGoalIdx();

	Bool keepProcessing = true;
	while( keepProcessing && lastGoalIdx < ( (Int32)m_query->GetGoalsCount() - 1 ) )
	{
		Vector startPos;

		// establish the start position for the segment processing
		if ( lastGoalIdx < 0 )
		{
			startPos = m_agent.GetNavAgentPosition();
		}
		else
		{
			startPos = m_query->GetGoal( lastGoalIdx ).position;
		}

		// get the next goal
		++lastGoalIdx;
		ASSERT( lastGoalIdx >= 0 );
		const SNavigationGoal& newGoal = m_query->GetGoal( lastGoalIdx );

		if ( newGoal.position.DistanceTo( startPos ) > newGoal.radius )
		{
			//// we need to deffer the request to the path finder in order to fulfill the goal
			//IPathEngineQueryAgent* peAgent = m_agent.GetNavQueryAgent();
			//ASSERT( peAgent ); // the agent used to have a PE representation, so it should have it still...

			//if ( m_queue->BeginSegmentsAddition( lastGoalIdx ) )
			//{
			//	m_planner->FindPath( *peAgent, startPos, newGoal.position, *this, lastGoalIdx );
			//}
			keepProcessing = false;
		}
		else if ( newGoal.hasOrientation )
		{
			// we're on the spot - so we may just need to adjust the orientation
			if ( m_queue->BeginSegmentsAddition( lastGoalIdx ) )
			{
				TDynArray< IMoveNavigationSegment *> path;
				path.PushBack( new CMoveNavigationOrientation( newGoal.orientation ) );
				m_queue->FinishSegmentsAddition( lastGoalIdx, path );

				keepProcessing = true;
			}
			else
			{
				keepProcessing = false;
			}
		}
		else
		{
			// there's nothing to be done
			keepProcessing = true;
		}
	}

	// tick the path processor
	ProcessNextSegment();
}

void CMovePathIterator::OnPathFound( Uint32 callbackData, const TDynArray< IMoveNavigationSegment* >& path )
{
	ASSERT( !path.Empty() );
	if ( path.Empty() )
	{
		m_status = NS_Failed;
		return;
	}
	
	// if the goal specifies an orientation, annotate the last navigation segment with it
	const SNavigationGoal& goal = m_query->GetGoal( callbackData );
	if ( goal.hasOrientation )
	{
		path.Back()->SetOrientation( goal.orientation );
	}

	// add the segments to the queue
	m_queue->FinishSegmentsAddition( callbackData, path );

	// plan the remaining part of the path
	ProcessGoals();
}

void CMovePathIterator::OnPathNotFound( Uint32 callbackData )
{
	if ( m_useReplanning )
	{
		ReplanSegment( callbackData );
	}
	else
	{
		Cancel();
	}
}

void CMovePathIterator::OnNoPathRequired( Uint32 callbackData )
{
	// if the goal specifies an orientation, annotate the last navigation segment with it
	const SNavigationGoal& goal = m_query->GetGoal( callbackData );
	CMoveNavigationPath* navPathSeg = new CMoveNavigationPath();

	// get the previous waypoint
	Vector startPos;
	if ( callbackData == 0 )
	{
		startPos = m_agent.GetNavAgentPosition();
	}
	else
	{
		startPos = m_query->GetGoal( callbackData - 1 ).position;
	}
	navPathSeg->AddWaypoint( startPos );
	navPathSeg->AddWaypoint( goal.position );

	if ( goal.hasOrientation )
	{
		navPathSeg->SetOrientation( goal.orientation );
	}

	// add the segments to the queue
	TDynArray< IMoveNavigationSegment* > path;
	path.PushBack( navPathSeg );
	m_queue->FinishSegmentsAddition( callbackData, path );

	// plan the remaining part of the path
	ProcessGoals();
}

// ----------------------------------------------------------------------------
// INavigable communication interface
// ----------------------------------------------------------------------------

void CMovePathIterator::OnNavigationSegmentCompleted()
{
	m_navigableIsWaitingForSegment = true;

	// process the next segment
	ProcessNextSegment();
}

void CMovePathIterator::OnNavigationSegmentFailed()
{	
	m_navigableIsWaitingForSegment = m_useReplanning;
	if ( m_useReplanning )
	{
		Int32 currentGoalIdx = m_queue->GetCurrentGoalIdx();
		if ( currentGoalIdx >= 0 )
		{
			ReplanSegment( currentGoalIdx );
		}
		else
		{
			ProcessGoals();
		}
	}
	else
	{
		Cancel();
	}
}

void CMovePathIterator::ReplanSegment( Int32 failedGoalIdx )
{
	ASSERT( false, TXT("OBSOLATE CODE!") );
}

void CMovePathIterator::ProcessNextSegment()
{
	if ( m_paused )
	{
		return;
	}

	if ( m_queue->Empty() )
	{
		m_status = NS_Completed;
	}
	else if ( m_navigableIsWaitingForSegment )
	{
		IMoveNavigationSegment* seg = m_queue->GetNextSeg();
		IMoveNavigationSegment* nextSeg = m_queue->PeekNextSeg();
		if ( seg )
		{
			seg->SetNextSeg( nextSeg );
			seg->Process( m_agent );
			seg->Release();
			m_navigableIsWaitingForSegment = false;
		}
	}
}

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------

void CMovePathIterator::OnGenerateDebugFragments( CRenderFrame* frame ) const
{
	m_queue->GenerateDebugFragments( frame );
}

void CMovePathIterator::DebugDraw( IDebugFrame& debugFrame ) const
{
	m_queue->DebugDraw( debugFrame );
	if ( m_query )
	{
		Int32 currGoalIdx = ::Min( 0, m_queue->GetCurrentGoalIdx() );
		m_query->DebugDraw( m_agent.GetNavAgentPosition(), currGoalIdx, debugFrame );
	}
}

String CMovePathIterator::GetDescription() const
{
	String desc;

	if ( m_query )
	{
		desc = String::Printf( TXT("<b>Query</b>: %s<br>"), m_query->GetDescription().AsChar() );
	}
	else
	{
		desc = TXT( "no pathfinding query used" );
	}

	return desc;
}

// ----------------------------------------------------------------------------
// Movement related queries
// ----------------------------------------------------------------------------

ERelPathPosition CMovePathIterator::GetRelativePosition( const Vector& checkedPos ) const
{
	return m_query->GetRelativePosition( m_agent.GetNavAgentPosition(), checkedPos );
}

Vector CMovePathIterator::CastPosition( const Vector& checkedPos ) const
{
	return m_query->CastPosition( m_agent.GetNavAgentPosition(), checkedPos );
}

///////////////////////////////////////////////////////////////////////////////

CNavigationGoalsQueue::CNavigationGoalsQueue()
	: m_currentGoalIdx( -1 )
	, m_transactionGoalIdx( -1 )
{
}

CNavigationGoalsQueue::~CNavigationGoalsQueue()
{
	Clear();
}

Bool CNavigationGoalsQueue::Empty() const
{
	return m_path.Empty() && m_transactionGoalIdx < 0 ;
}

Bool CNavigationGoalsQueue::BeginSegmentsAddition( Int32 goalIdx, Bool force )
{
	if ( force || m_transactionGoalIdx < 0 || m_transactionGoalIdx == goalIdx )
	{
		m_transactionGoalIdx = goalIdx;
		return true;
	}
	else
	{
		return false;
	}
}

void CNavigationGoalsQueue::FinishSegmentsAddition( Int32 goalIdx, const TDynArray< IMoveNavigationSegment* >& path )
{
	ASSERT( goalIdx >= 0 && "Finishing a transaction for a non-existing goal." );
	ASSERT( m_transactionGoalIdx == goalIdx && "finishing a non-existing navigation goal transaction." );
	if ( m_transactionGoalIdx != goalIdx )
	{
		return;
	}
	m_transactionGoalIdx = -1;

	// We assume that segments that belong to the currently processed goal
	// may be invalidated and replaced - but that only applies to the currently 
	// processed goal - all other goals we have segments for are considered valid and 
	// if a replacement request comes - it means there's a bug in the code.

	Int32 lastProcessedGoalIdx = GetLastProcessedGoalIdx();
	if ( goalIdx == m_currentGoalIdx )
	{
		// if there's some processing going on, it means that there must be a goal there
		ASSERT( !m_goalSegsCount.Empty() && "There's no goal currently being processed - so there's nothing to replace" );

		// in case the goal we are adding segments for is the currently processed
		// goal, remove all segments for it
		for ( Uint32 i = 0; i < m_goalSegsCount[0]; ++i )
		{
			if ( m_path[0] == NULL )
			{
				HALT( "Invalid path navigation segment - call Paksas" );
				break;
			}
			m_path[0]->Release();
			m_path.Erase( m_path.Begin() );
		}

		Uint32 segsAdded = 0;
		for ( TDynArray< IMoveNavigationSegment* >::const_iterator it = path.Begin(); it != path.End(); ++it )
		{
			if ( *it != NULL )
			{
				(*it)->AddRef();
				m_path.Insert( segsAdded, *it );
				++segsAdded;
			}
		}
		m_goalSegsCount[0] = segsAdded;
	}
	else if ( goalIdx - 1 == lastProcessedGoalIdx )
	{
		// insert the segments 
		Uint32 segsAdded = 0;
		for ( TDynArray< IMoveNavigationSegment* >::const_iterator it = path.Begin(); it != path.End(); ++it )
		{
			if ( *it != NULL )
			{
				(*it)->AddRef();
				m_path.PushBack( *it );
				++segsAdded;
			}
		}
		m_goalSegsCount.PushBack( segsAdded );
	}
	else
	{
		ASSERT( false && TXT( "Trying to replace segments for a goal that's neither the current nor new one" ) );
	}

	if ( m_currentGoalIdx < 0 )
	{
		m_currentGoalIdx = 0;
	}
}

IMoveNavigationSegment* CNavigationGoalsQueue::GetNextSeg()
{
	if ( m_path.Empty() )
	{
		return NULL;
	}
	ASSERT( m_currentGoalIdx >= 0 );
	ASSERT( m_currentGoalIdx <= GetLastProcessedGoalIdx() );
	ASSERT( !m_goalSegsCount.Empty() );

	while( !m_goalSegsCount.Empty() && m_goalSegsCount[0] <= 0 )
	{
		m_goalSegsCount.Erase( m_goalSegsCount.Begin() );
		++m_currentGoalIdx;
	}

	if ( m_goalSegsCount.Empty() )
	{
		// there are no more segments to process
		ASSERT( m_path.Empty() ); // ... therefore the path must be empty as well
		return NULL;
	}
	else
	{
		ASSERT( !m_path.Empty() ); // the path can't be empty

		IMoveNavigationSegment* seg = m_path[0];

		m_path.Erase( m_path.Begin() );
		--m_goalSegsCount[0];

		return seg;
	}
}

IMoveNavigationSegment* CNavigationGoalsQueue::PeekNextSeg() const
{
	if ( m_path.Empty() )
	{
		return NULL;
	}
	else
	{
		return m_path[0];
	}
}

void CNavigationGoalsQueue::Clear()
{
	for ( TDynArray< IMoveNavigationSegment* >::iterator it = m_path.Begin(); it != m_path.End(); ++it )
	{
		(*it)->Release();
	}
	m_path.Clear();
	m_goalSegsCount.Clear();

	m_currentGoalIdx = -1;
}

// ----------------------------------------------------------------------------
// Debug
// ----------------------------------------------------------------------------

void CNavigationGoalsQueue::GenerateDebugFragments( CRenderFrame* frame ) const
{
	for ( TDynArray< IMoveNavigationSegment* >::const_iterator it = m_path.Begin(); it != m_path.End(); ++it )
	{
		(*it)->GenerateDebugFragments( frame );
	}
}

void CNavigationGoalsQueue::DebugDraw( IDebugFrame& debugFrame ) const
{
	for ( TDynArray< IMoveNavigationSegment* >::const_iterator it = m_path.Begin(); it != m_path.End(); ++it )
	{
		(*it)->DebugDraw( debugFrame );
	}
}

///////////////////////////////////////////////////////////////////////////////
