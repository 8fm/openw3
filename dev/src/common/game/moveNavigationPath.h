/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "movementObject.h"
#include "movementTargeter.h"


///////////////////////////////////////////////////////////////////////////////

class CMovingAgentComponent;
struct SMoveLocomotionGoal;
class CPathPointLock;
class CMoveLocomotion;
class INavigable;

///////////////////////////////////////////////////////////////////////////////

class IDebugFrame
{
public:
	virtual ~IDebugFrame() {}

	virtual void AddSphere( const Vector& pos, Float radius, const Color& color ) = 0;

	virtual void AddLine( const Vector& start, const Vector& end, const Color& color ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IMoveNavigationSegment : public IMovementObject
{
public:
	virtual ~IMoveNavigationSegment() {}

	virtual CName GetTypeID() const = 0;

	virtual void SetNextSeg( IMoveNavigationSegment* nextSeg ) = 0;

	virtual void Process( INavigable& navigable ) = 0;

	virtual void SetOrientation( Float orientation ) = 0;

	virtual void GenerateDebugFragments( CRenderFrame* frame ) const = 0;

	virtual void DebugDraw( IDebugFrame& debugFrame ) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class CMoveNavigationOrientation : public IMoveNavigationSegment
{
private:
	Float				m_orientation;

public:
	CMoveNavigationOrientation( Float orientation );

	RED_INLINE Float GetOrientation() const { return m_orientation; }

	// ------------------------------------------------------------------------
	// IMoveNavigationSegment implementation
	// ------------------------------------------------------------------------
	virtual CName GetTypeID() const;
	virtual void SetNextSeg( IMoveNavigationSegment* nextSeg ) {}
	virtual void Process( INavigable& navigable );
	virtual void SetOrientation( Float orientation ) { m_orientation = orientation; }
	virtual void GenerateDebugFragments( CRenderFrame* frame ) const;
	virtual void DebugDraw( IDebugFrame& debugFrame ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CMoveNavigationPath : public IMoveNavigationSegment
{
private:
	Float					m_radius;
	Float					m_totalPathLength;
	TDynArray< Vector >		m_points;
	TDynArray< Float >		m_lengths;
	TDynArray< Vector >		m_normals;

	Bool					m_hasOrientation;
	Float					m_orientation;
	Bool					m_isLastSegment;

public:
	CMoveNavigationPath( Float radius = 0.0f );
	~CMoveNavigationPath();

	RED_INLINE Uint32 Size() const { return m_lengths.Size(); }

	RED_INLINE Bool Empty() const { return m_lengths.Empty(); }

	RED_INLINE const Vector& GetDestination() const { ASSERT( !Empty() ); return m_points.Back(); }

	RED_INLINE const Vector& GetStart() const { ASSERT( !Empty() ); return m_points[0]; }

	RED_INLINE Bool HasOrientation() const { return m_hasOrientation; }

	RED_INLINE Float GetOrientation() const { return m_orientation; }

	RED_INLINE Bool IsLastSegment() const { return m_isLastSegment; }

	//! Adds a path waypoint
	void AddWaypoint( const Vector& pos );
	
	//! Clears the path, removing all previously added waypoints
	void Clear();

	Float MapPointToPathDistance( const Vector& pos ) const;

	void MapPointToPath( const Vector& pos, Vector& outPathPos, Float& outDistFromPath ) const;

	Vector MapPathDistanceToPoint( Float distance ) const;

	// ------------------------------------------------------------------------
	// IMoveNavigationSegment implementation
	// ------------------------------------------------------------------------
	virtual CName GetTypeID() const;
	virtual void SetNextSeg( IMoveNavigationSegment* nextSeg );
	virtual void Process( INavigable& navigable );
	virtual void SetOrientation( Float orientation ) { m_hasOrientation = true; m_orientation = orientation; }
	virtual void GenerateDebugFragments( CRenderFrame* frame ) const;
	virtual void DebugDraw( IDebugFrame& debugFrame ) const;

private:
	void PointToSegmentDistance( const Vector& pos, Uint32 segIdx, Float& outSegProjection, Float& outDist ) const;
};

///////////////////////////////////////////////////////////////////////////////

class INavigable : public Red::System::NonCopyable
{
public:
	virtual ~INavigable() {}

	//! Returns the current position of the navigable agent.
	virtual Vector GetNavAgentPosition() const = 0;

	//! Called when the path iteration process begins
	virtual void OnNavigationStart() = 0;

	//! Called when the navigation process is over
	virtual void OnNavigationEnd() = 0;

	//! Sets a new movement speed
	virtual void SetNavMoveType( EMoveType moveType, Float absSpeed ) = 0;

	//! Sets a new movement speed
	virtual void GetNavMoveType( EMoveType& outMoveType, Float& outAbsSpeed ) const = 0;

	// ------------------------------------------------------------------------
	// Navigation segment visitor
	// ------------------------------------------------------------------------

	//! Called when the path iterator has a new nav segment to process
	virtual void OnProcessNavSegment( CMoveNavigationOrientation& seg ) = 0;

	//! Called when the path iterator has a new nav segment to process
	virtual void OnProcessNavSegment( CMoveNavigationPath& seg ) = 0;
};

///////////////////////////////////////////////////////////////////////////////
