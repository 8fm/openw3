/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibSpatialQuery.h"

class CRenderFrame;

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// Data for storing collect collision points output. It should be stored
// externally for reuse, as it holds data in dynamic arrays which does
// dynamic reallocations at start
class CCollectCollisionPointsInCircleProxy
{
	friend class CCollectCollisionPointsInCircleQueryData;
protected:
	struct CollisionPoint // ctremblay minimum alignment for this type is 4. TSortedArray will handle it correctly.
	{
		Vector2 m_normal;
		Float	m_dist;
		Float	m_weight;

		Bool	operator<( const CollisionPoint& p ) const					{ return m_weight < p.m_weight; }
	};

	static_assert( __alignof( CollisionPoint ) == 4, "Alignment busted 4, please fix!" );

	typedef TSortedArray< CollisionPoint > Collection;

	Collection					m_collisions;
	Bool						m_outputIsProcessed;

	void		AddPoint( const Vector2& diff );
	void		ProcessOutput();

public:
	CCollectCollisionPointsInCircleProxy();
	~CCollectCollisionPointsInCircleProxy()									{}

	Bool		NoHits() const												{ return m_collisions.Empty(); }
	void		GetRepulsionSpot( Float maxDist, Vector2& outRepulsionDir, Float& outRepulsionRange );
	Float		GetClosestCollision( Vector2& outNormal );

#ifndef NO_EDITOR_FRAGMENTS
	void		DebugRender( CRenderFrame* frame, const Vector& centralSpot, Float testRadius );
#endif
	
};

class CCollectCollisionPointsInCircleQueryData : public CCircleQueryData
{
	typedef CCircleQueryData Super;
protected:
	typedef CCollectCollisionPointsInCircleProxy Data;
	
	Data&						m_output;

public:
	enum { AUTOFAIL = false };

	typedef TMultiAreaSpatialQuery< CCollectCollisionPointsInCircleQueryData > MultiArea;

	CCollectCollisionPointsInCircleQueryData( Uint32 flags, const Vector3& center, Float radius, CCollectCollisionPointsInCircleProxy& output )
		: Super( flags, center,radius )
		, m_output( output )												{ m_output.m_collisions.ClearFast(); m_output.m_outputIsProcessed = false; }

	RED_FORCE_INLINE void OnIntersection( const Vector2& point )			{ m_output.AddPoint( m_circleCenter.AsVector2() - point ); }
	void NoticeSpot( const Vector2& point );
};
////////////////////////////////////////////////////////////////////////////

};			// namespace PathLib