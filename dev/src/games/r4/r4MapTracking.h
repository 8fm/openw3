/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/pathlibAgent.h"

class CR4MapTracking;

class CR4TrackingAgent : public PathLib::CAgent
{
	typedef PathLib::CAgent Super;

protected:
	CR4MapTracking&				m_owner;
	TDynArray< Uint8 >			m_waypointFlags;

	virtual void				OnPathCollectionSync() override;
public:
	enum PathlibAgents : ClassId
	{
		CLASS_CR4MapTracking		= FLAG( 2 )
	};
	static const ClassId CLASS_ID = CLASS_CR4MapTracking;

	CR4TrackingAgent( CR4MapTracking& owner, CPathLibWorld* world, Float personalSpace );
	~CR4TrackingAgent();
};

class CR4MapTracking
{
protected:
	CCommonMapManager&								m_manager;
	CR4TrackingAgent*								m_agent;													// pathlib agent doing queries
	// pathfinding state
	Bool											m_pathFollowing;
	Bool											m_updatePathRequest;
	Bool											m_hasSafeSpot;
	Bool											m_forceWaypointsUpdate;
	Bool											m_pathLimitedByStreaming;
	Uint16											m_pathStreamingVersionStamp;
	EngineTime										m_pathfindingTimeout;										// if pathfinding have failed - timeout b4 running next query
	EngineTime										m_lookAheadTimeout;
	EngineTime										m_lastDetailedUpdate;
	Vector3											m_lastSafeSpot;
	Vector3											m_startPos;
	Vector3											m_targetPos;
	// setup
	Float											m_parameterRemovalDistance;
	Float											m_parameterPlacingDistance;
	Float											m_parameterRefreshInterval;
	Float											m_parameterPathfindingTolerance;
	Uint32											m_parameterMaxCount;
	Float											m_mapZoom;
	// precomputed setup
	Float											m_computedRemovalDistance;
	Float											m_computedPlacingDistance;
	// output
	Float											m_closestWaypoint;
	TDynArray< Vector >								m_waypoints;

	// stuff for obtaining path point position
	struct SPathPointTracking
	{
		Vector3		m_basePosition;
		Vector3		m_currentDestination;
		Uint32		m_currentWaypoint;
		Uint32		m_currentMetalink;

		SPathPointTracking( const CR4TrackingAgent& agent );
	};

	Bool			GetPathPointInDistance( Float distance, SPathPointTracking& tracking, Vector3& vOut );

	Bool			PlotPath( const Vector3& destination, Float targetRadius );
	void			UpdateWaypoints();
	void			UpdateMetalinkAwareness();

	void			OnParametersModified();

public:
	CR4MapTracking( CCommonMapManager& mapManager );
	~CR4MapTracking();

#ifndef NO_EDITOR_FRAGMENTS
	void			OnGenerateDebugFragments( CRenderFrame* frame );
#endif

	void			UpdateTracking( const Vector& startPos, const Vector& destination, Float targetRadius );
	void			ClearTracking();

	void			CreateSearchData();
	void			DestroySearchData();

	void			SetParameters( Float removalDistance, Float placingDistance, Float refreshInterval, Float pathfindTolerance, Uint32 maxCount );
	void			OnChangedMapZoom( Float zoom );
	Float			GetParameterPlacingDistance() const															{ return m_computedPlacingDistance; }
};