#pragma once

#include "pathlibSearchData.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CAgent is additional abstraction for path following
////////////////////////////////////////////////////////////////////////////
class CAgent : public CSearchData
{
	typedef CSearchData Super;
protected:
	enum eState
	{
		STATE_IDLE,
		STATE_PATHFIND_PENDING,
		STATE_PATHFOLLOW
	};
	Vector3					m_position;
	EulerAngles				m_orientation;									// I don't care about anything else than yaw, but I need to support existing moveable agent interface
	Uint32					m_currentWaypoint;
	Int16					m_currentMetalink;
	Bool					m_metalinkIsRunning;
	Bool					m_pathIsLimitedBecauseOfStreamingRange;
	Vector3					m_lastFollowPosition;
	Vector3					m_lastFollowUpdateOriginPosition;
	EngineTime				m_nextFollowUpdate;
	Float					m_nextFollowUpdateDistanceSq;
	eState					m_state			: 8;
	Bool					m_isOnNavdata;
	Uint16					m_pathfollowFlags;
	Float					m_approachDistance;
	Float					m_targetTolerance;
	CPathLibWorld*			m_world;
	Waypoints				m_waypoints;
	Metalinks				m_metalinksStack;
	THandle< CNode >		m_road;

	virtual void				OnPathCollectionSync();

	CAreaDescription*			UpdateCurrentArea();
	Bool						UpdatePreciseFollowPoint( Vector3& outFollowPosition );
	Bool						UpdateFollowPoint( Vector3& outFollowPosition );
	void						OnPathfindResult( PathLib::EPathfindResult result );
	Bool						StayOnNavdataRec( Vector3& inOutDeltaMovement, Int32 recursionLimit = 4 );
	Bool						ReturnToNavdataRec( Vector3& outPosition, Int32 recursionLimit = 3 );
	Uint32						GetOptimizableWaypointsBounding( Bool& isBoundingOptimizable );			// tricky. Function returns index of last waypoint before getting up to next metalink
public:
	static const ClassId CLASS_ID = CLASS_CAgent;

	enum ePathfollowFlags
	{
		PATHFOLLOW_DEFAULT					= 0,
		PATHFOLLOW_LINE_TEST				= FLAG(0),
		PATHFOLLOW_MOVING_WITH_TOLERANCE	= FLAG(1),
		PATHFOLLOW_PRECISE					= FLAG(2),
		PATHFOLLOW_SUPPORT_ROADS			= FLAG(3),
		PATHFOLLOW_USE_ROAD					= FLAG(4),

		PATHFOLLOW_USER_FLAGS				= PATHFOLLOW_LINE_TEST,
		PATHFOLLOW_PATHFINDING_FLAGS		= PATHFOLLOW_MOVING_WITH_TOLERANCE | PATHFOLLOW_USE_ROAD,
		PATHFOLLOW_PERSISTANT_FLAGS			= PATHFOLLOW_SUPPORT_ROADS | PATHFOLLOW_PRECISE,
	};

	CAgent( CPathLibWorld* world, Float personalSpace, ClassId classId = 0 );
	~CAgent()																{}
	
	////////////////////////////////////////////////////////////////////////
	// Base interface
	const Vector3&				GetPosition() const							{ return m_position; }
	const EulerAngles&			GetOrientation() const						{ return m_orientation; }

	Uint32						GetWaypointsCount() const					{ return m_waypoints.Size(); }
	const Vector3&				GetWaypoint( Uint32 waypoint ) const		{ return m_waypoints[ waypoint ]; }
	Uint32						GetMetalinksCount()	const					{ return m_metalinksStack.Size(); }
	const MetalinkInteraction&	GetMetalink( Uint32 metalink ) const		{ return m_metalinksStack[ metalink ]; }
	Bool						HasPath() const								{ return !m_waypoints.Empty(); }
	void						ClearPath();

	Uint32						GetCurrentWaypointIdx() const				{ return m_currentWaypoint; }
	Int32						GetCurrentMetalinkIdx() const				{ return m_currentMetalink; }
	Bool						IsPathLimitedBecauseOfStreamingRange() const{ return m_pathIsLimitedBecauseOfStreamingRange; }
	RED_INLINE Bool				NextWaypoint()								{ if ( m_currentWaypoint+1 < m_waypoints.Size() ) { m_currentWaypoint += 1; return true; } return false; }
	RED_INLINE Bool				PrevWaypoint()								{  if ( m_currentWaypoint > 0 ) { m_currentWaypoint -= 1; return true; } return false; }
	RED_INLINE void				SetCurrentWaypoint( Uint32 waypoint )		{ m_currentWaypoint = waypoint; }
	RED_INLINE const Vector3&	GetDestination() const						{ return GetWaypoint( GetWaypointsCount() -1 ); }
	RED_INLINE Bool				IsGoingToLastWaypoint() const				{ return m_currentWaypoint + 1 >= m_waypoints.Size(); }
	RED_INLINE Bool				IsPathfollowing() const						{ return m_state == STATE_PATHFOLLOW; }
	RED_INLINE Bool				IsOnNavdata() const							{ return m_isOnNavdata; }
	Uint16						GetPathfollowFlags() const					{ return m_pathfollowFlags; }
	const Vector3&				GetCurrentFollowPosition() const			{ return m_lastFollowPosition; }
	Bool						NextMetalink();

	CPathLibWorld*				GetPathLib() const							{ return m_world; }

	RED_INLINE void				SetPrecisePathFollowing( Bool val ) 		{ m_pathfollowFlags = val ? m_pathfollowFlags | PATHFOLLOW_PRECISE : m_pathfollowFlags & (~PATHFOLLOW_PRECISE); }
	RED_INLINE void				SupportRoads( Bool val ) 					{ m_pathfollowFlags = val ? m_pathfollowFlags | PATHFOLLOW_SUPPORT_ROADS : m_pathfollowFlags & (~PATHFOLLOW_SUPPORT_ROADS); }

	////////////////////////////////////////////////////////////////////////
	// pathfinding related queries
	PathLib::EPathfindResult	PlotPath( const Vector3& destinationPos, Float tolerance = 0.f );

	Bool						UpdatePathDestination( const Vector3& newDestination, Bool testForTrivialCase = true );

	Bool						FollowPath( Vector3& outFollowPosition );

	Bool						IsPathfollowingOnMetalink();

	void						LookAheadForBetterWaypoint( Float distance );

	Float						UpdateFollowingBasedOnClosestPositionOnPath( Vector3& outClosestPosition );	// Update current waypoint based on distance to path. Can break pathfollowing, but also can sort out pathfollowing after some more complicated shit. Returns squared distance to closest position.

	Bool						UpdateMetalinkAwareness();					// metalinks awareness. NOTICE: this function MAY change your current atomic action so its good to call it externally (eg. from AI)

	void						SetupPathfollowing( Float tolerateDistance, Uint16 pathfollowFlags );
	void						DynamicEnablePathfollowingFlags( Uint32 pathfollowFlags );
	void						DynamicDisablePathfollowingFlags( Uint32 pathfollowFlags );

	void						StopMovement();

	void						MoveToPosition( const Vector3& newPosition );

	Bool						StayOnNavdata( Vector3& inOutDeltaMovement );

	Bool						ReturnToNavdata( Vector3& outPosition );

	Bool						ComputeHeightBelow( const Vector3& pos, Float& outHeight );

	Bool						GetPathPointInDistance( Float fDistance, Vector3& vOut, Bool computePreciseZ = false );
	Bool						IsPointOnPath( const Vector3& vPoint, Float fToleranceRadius );
	Bool						CheckIfIsOnNavdata();

	void						UseRoad( CNode* road );
	void						DontUseRoad();
	CNode*						GetUsedRoad();

	Bool						GetCurrentMetalinkWaypoints( Vector3& outOrigin, Vector3& outDestination ) const;
	const MetalinkInteraction*	GetCurrentMetalinkInterraction() const;

	Bool						UpdatePathfollowFromVirtualPosition( const Vector3& virtualPosition, Float acceptWaypointDistance = 0.1f );
////////////////////////////////////////////////////////////////////////
	// Debug stuff
#ifdef PATHLIB_AGENT_DEBUG_PATHFINDING
	Bool						HasFailedRecently( Vector3& outFollowPoint ) const { if ( m_failedRecently ) { outFollowPoint = m_lastFailedPathfindingDestination; return true; } return false;}
#endif

	////////////////////////////////////////////////////////////////////////
	// Advanced spacial queries
	Bool						FindRandomPositionInRadius( const Vector3& pos, Float radius, Vector3& posOut );
	Bool						FindSafeSpot( const Vector3& pos, Float radius, Vector3& outPos, PathLib::CollisionFlags flags = PathLib::CT_DEFAULT );

	////////////////////////////////////////////////////////////////////////
	// Spacial queries
	using CSearchData::TestLine;
	using CSearchData::TestLocation;
	Bool						TestLine( const Vector3& pos1, const Vector3& pos2, Float personalSpace )	{ return TestLine( m_world, pos1, pos2, personalSpace ); }
	Bool						TestLine( const Vector3& pos1, const Vector3& pos2 )						{ return TestLine( m_world, pos1, pos2 ); }
	Bool						TestLine( const Vector3& posDestination )									{ return TestLine( m_position, posDestination ); }
	Bool						TestLocation( const Vector3& pos )											{ return TestLocation( m_world, pos ); }
	Bool						TestLocation( const Vector3& pos, Float radius )							{ return TestLocation( m_world, pos, radius ); }
	Float						GetClosestObstacle( Float radius, Vector3& outPos );
	Float						GetClosestObstacle(const Vector3& pos1, const Vector3& pos2, Float radius, Vector3& outGeometryPos, Vector3& outLinePos );
	Bool						ComputeHeight( const Vector3& pos, Float& z );
};

};			// namespace PathLib
