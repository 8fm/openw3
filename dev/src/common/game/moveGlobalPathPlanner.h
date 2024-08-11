#pragma once

#include "movementObject.h"


///////////////////////////////////////////////////////////////////////////////

class IMovePathPlannerQuery;
class CMovePathIterator;
class IMoveNavigationSegment;
class INavigable;
class CPathPointLock;

///////////////////////////////////////////////////////////////////////////////

class IPathPlannerCallback
{
public:
	virtual ~IPathPlannerCallback() {}

	//! Called when a path query is complete and a valid path was found.
	virtual void OnPathFound( Uint32 callbackData, const TDynArray< IMoveNavigationSegment* >& path ) = 0;

	//! Called when a path query is complete and no valid path was found.
	virtual void OnPathNotFound( Uint32 callbackData ) = 0;

	//! Called when a path query is complete and turns out no path is needed to get to the
	//! requested destination, 'cause we're already there
	virtual void OnNoPathRequired( Uint32 callbackData ) = 0;

	//! Called in order to generate debug info
	virtual void OnGenerateDebugFragments( CRenderFrame* frame ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

enum EPathfindingQueryResult
{
	QR_InProgress,
	QR_PathFound,
	QR_PathNotFound,
	QR_PathNotNeeded
};

///////////////////////////////////////////////////////////////////////////////

//! A global facility for path planning activities.
class CMoveGlobalPathPlanner
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_AI );
private:
	struct SLock
	{
		Int32				m_lockId;
		Float			m_distanceToGoal;

		SLock( Int32 lockId, Float distanceToGoal ) : m_lockId( lockId ), m_distanceToGoal( distanceToGoal ) {}

		static int CompareFunc( const void* a, const void* b )
		{
			SLock* infoA = ( SLock* ) a;
			SLock* infoB = ( SLock* ) b;
			if ( infoA->m_distanceToGoal > infoB->m_distanceToGoal ) return -1;
			if ( infoA->m_distanceToGoal < infoB->m_distanceToGoal ) return 1;
			return 0;
		}
	};

	struct SQueryFailure
	{
		Vector		m_start;
		Vector		m_end;

		SQueryFailure( const Vector& start, const Vector& end ) : m_start( start ), m_end( end ) {}

		Bool operator==( const SQueryFailure& rhs ) const
		{
			// compare both positions with granularity of 1[cm] 
			// ( mind that we're using the squared version of the dist function due to
			// performance reasons )
			return m_start.DistanceSquaredTo( rhs.m_start ) <= 1e-6 && m_end.DistanceSquaredTo( rhs.m_end ) <= 1e-6;
		}
	};
	typedef TDynArray< SQueryFailure >			QueryFailures;

	friend class CPathPointLock;

private:
	typedef THashMap< Uint32, TDynArray< SLock > >	LockPointsMap;
	LockPointsMap									m_lockPoints;
	Int32												m_idsPool;

	typedef THashMap< const INavigable*, CMovePathIterator* >	IteratorsMap;
	IteratorsMap									m_iterators;

	QueryFailures									m_queryFailures;			//!< Debug shit

public:
	CMoveGlobalPathPlanner();
	~CMoveGlobalPathPlanner();

	//! Updates the planner
	void Tick( Float timeDelta );

	//! Removes a path finding query
	void RemoveQuery( const IPathPlannerCallback& callback );

	//! Registers a path iterator for an agent
	void RegisterIterator( const INavigable& agent, CMovePathIterator& iterator );

	//! Unregisters a path iterator for an agent
	void UnregisterIterator( const INavigable& agent );

	//! Returns a path planner an agent is currently using
	const CMovePathIterator* GetIteratorFor( const INavigable& agent ) const;

	//! Debug draw
	void GenerateDebugFragments( CRenderFrame* frame );

private:
	// ------------------------------------------------------------------------
	// Path finding
	// ------------------------------------------------------------------------
	//! Updates the scheduled path finding queries
	void UpdatePathfindingQueries();

	//! Updates the state of navigation iterators
	void UpdateIterators();

	// ------------------------------------------------------------------------
	// Point locks management
	// ------------------------------------------------------------------------
	//! Locks up a point
	Int32 AquireLockId( Uint32 key, Float dist );

	//! Unlocks a point
	void ReleaseLockId( Uint32 key, Int32 lockId );

	//! Returns the current key
	Float GetCurrentLock( Uint32 key, Int32 lockId, Float dist );

	//! Updates the point locks
	void UpdatePointLocks();
};


///////////////////////////////////////////////////////////////////////////////

class CPathPointLock : public IMovementObject 
{
private:
	CMoveGlobalPathPlanner&			m_host;
	Vector							m_target;

	Uint32							m_key;
	Int32								m_lockId;

public:
	CPathPointLock( CMoveGlobalPathPlanner& host, const Vector& target );
	~CPathPointLock();

	Float GetMultiplier( const Vector& currPos, Float agentRadius ) const;

	//! Debug draw
	void GenerateDebugFragments( const Vector& queryPos, CRenderFrame* frame );

private:
	static RED_INLINE Uint32 GetHash( Int32 x, Int32 y, Int32 z )
	{
		const Uint32 tx = x;
		const Uint32 ty = y;
		const Uint32 tz = z;

		const Uint32 t = tx + ( ty << 6 )+ ( tz << 12 );
		return t;
	}
};

