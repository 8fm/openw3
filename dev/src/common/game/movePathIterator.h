/**
 * Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "movementObject.h"
#include "moveGlobalPathPlanner.h"


enum ERelPathPosition : CEnum::TValueType;
class IDebugFrame;
class CNavigationGoalsQueue;
class IMoveNavigationSegment;
class INavigable;
class IMoveStateController;

///////////////////////////////////////////////////////////////////////////////

enum ENavigationStatus : CEnum::TValueType
{
	NS_InProgress,		//!< Navigation is still in progress
	NS_Completed,		//!< Navigation has completed
	NS_Failed			//!< Navigation has failed
};

///////////////////////////////////////////////////////////////////////////////

//! The path iterator executes navigation queries.
class CMovePathIterator : public IPathPlannerCallback, public IMovementObject
{
private:
	INavigable&								m_agent;
	IMovePathPlannerQuery*					m_query;
	Bool									m_useReplanning;

	ENavigationStatus						m_status;
	Bool									m_paused;
	Bool									m_navigableIsWaitingForSegment;

	CMoveGlobalPathPlanner*					m_planner;
	CNavigationGoalsQueue*					m_queue;

	TDynArray< IMoveStateController* >		m_controllers;

public:
	//! Constructor.
	CMovePathIterator( INavigable& agent, IMovePathPlannerQuery* query, Bool useReplanning = false );
	~CMovePathIterator();

	//! Adds a new movement state controller
	void AddController( IMoveStateController* controller );

	//! Starts the path finding process
	void Start();

	//! Stops the iteration process
	void Stop();

	//! Cancels the iteration process
	void Cancel();

	//! Updates the iterator ( called by the global path planner )
	void Update();

	//! Returns the iteration status
	RED_INLINE ENavigationStatus GetStatus() const { return m_status; }

	// ------------------------------------------------------------------------
	// Movement related queries
	// ------------------------------------------------------------------------

	//! Returns the agent that uses the iterator
	RED_INLINE INavigable& GetNavigable() { return m_agent; }

	//! Checks where the 'checkedPos' is located on the queried path with respect
	//! to the specified 'anchorPos'
	ERelPathPosition GetRelativePosition( const Vector& checkedPos ) const;

	//! Casts the specified position onto the queried path.
	Vector CastPosition( const Vector& checkedPos ) const;

	// ------------------------------------------------------------------------
	// IPathPlannerCallback implementation
	// ------------------------------------------------------------------------
	virtual void OnPathFound( Uint32 callbackData, const TDynArray< IMoveNavigationSegment* >& path );
	virtual void OnPathNotFound( Uint32 callbackData );
	virtual void OnNoPathRequired( Uint32 callbackData );
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) const;

	// ------------------------------------------------------------------------
	// INavigable communication interface
	// ------------------------------------------------------------------------
	//! Call this method to indicate that processing of a nav segment has completed successfully.
	void OnNavigationSegmentCompleted();

	//! Call this method to indicate that processing of a nav segment has failed.
	void OnNavigationSegmentFailed();

	// -------------------------------------------------------------------------
	// Additional debug
	// -------------------------------------------------------------------------
	void DebugDraw( IDebugFrame& debugFrame ) const;
	String GetDescription() const;

private:
	//! Reprocesses currently traversed navigation segment
	void ReplanSegment( Int32 goalIdx );

	//! Called when there's a new segment available for processing
	void ProcessNextSegment();

	//! Processes a batch of goals
	void ProcessGoals();
};

///////////////////////////////////////////////////////////////////////////////

class CNavigationGoalsQueue : public IMovementObject
{
private:
	TDynArray< IMoveNavigationSegment* >	m_path;
	Int32										m_currentGoalIdx;
	Int32										m_transactionGoalIdx;
	TDynArray< Uint32 >						m_goalSegsCount;

public:
	CNavigationGoalsQueue();
	~CNavigationGoalsQueue();

	//! Tells if the queue is empty
	Bool Empty() const;

	//! Returns index of the goal that's currently being processed
	RED_INLINE Int32 GetCurrentGoalIdx() const { return m_currentGoalIdx; }

	//! Returns index of a goal that's most recently been added to the queue
	RED_INLINE Int32 GetLastProcessedGoalIdx() const { return m_currentGoalIdx + (Int32)m_goalSegsCount.Size() -  ( m_goalSegsCount.Size() > 0 ? 1 : 0 ); }

	//! Starts the goal-related navigation segments addition transaction.
	Bool BeginSegmentsAddition( Int32 goalIdx, Bool force = false );

	//! Finishes the goal-related navigation segments addition transaction.
	void FinishSegmentsAddition( Int32 goalIdx, const TDynArray< IMoveNavigationSegment* >& path );

	//! Retrieves next navigation segment
	IMoveNavigationSegment* GetNextSeg();

	//! Retrieves next navigation segment without removing it
	IMoveNavigationSegment* PeekNextSeg() const;

	//! Clears the entire queue
	void Clear();

	// -------------------------------------------------------------------------
	// Debug
	// -------------------------------------------------------------------------
	void GenerateDebugFragments( CRenderFrame* frame ) const;
	void DebugDraw( IDebugFrame& debugFrame ) const;
};

///////////////////////////////////////////////////////////////////////////////

//! A move state controller performs a continuous control over some movement 
//! aspect.
class IMoveStateController : public IMovementObject
{
public:
	virtual ~IMoveStateController() {}

	//! Called when the controller is activated
	virtual void Activate( CMovePathIterator& it ) = 0;

	//! Called in order to update the controller's state
	virtual void Update( CMovePathIterator& it ) = 0;
};

///////////////////////////////////////////////////////////////////////////////
