#include "build.h"
#include "moveNavigationPath.h"
#include "movementGoal.h"
#include "moveGlobalPathPlanner.h"
#include "moveLocomotion.h"
#include "moveSteeringLocomotionSegment.h"
#include "../engine/renderFrame.h"

RED_DEFINE_STATIC_NAME( CMoveNavigationOrientation );
RED_DEFINE_STATIC_NAME( CMoveNavigationPath );

///////////////////////////////////////////////////////////////////////////////

CMoveNavigationOrientation::CMoveNavigationOrientation( Float orientation )
	: m_orientation( orientation )
{
}

CName CMoveNavigationOrientation::GetTypeID() const
{
	return CNAME( CMoveNavigationOrientation );
}

void CMoveNavigationOrientation::Process( INavigable& navigable )
{
	navigable.OnProcessNavSegment( *this );
}

void CMoveNavigationOrientation::GenerateDebugFragments( CRenderFrame* frame ) const
{
}

void CMoveNavigationOrientation::DebugDraw( IDebugFrame& debugFrame ) const
{
}

///////////////////////////////////////////////////////////////////////////////

CMoveNavigationPath::CMoveNavigationPath( Float radius )
	: m_radius( radius )
	, m_totalPathLength( 0.0f )
	, m_hasOrientation( false )
	, m_isLastSegment( true )
{
}

CMoveNavigationPath::~CMoveNavigationPath()
{
	Clear();
}

void CMoveNavigationPath::AddWaypoint( const Vector& pos )
{
	if ( m_points.Size() >= 1 )
	{
		// we're about to have more than 2 points - which means we're adding a new segment
		const Vector& segStart = m_points.Back();

		m_lengths.PushBack( pos.DistanceTo( segStart ) );
		m_normals.PushBack(  ( pos - segStart ).Normalized3() );

		m_totalPathLength += m_lengths.Back();
	}

	// finally - add the new point
	m_points.PushBack( pos );
}

void CMoveNavigationPath::Clear()
{
	m_points.Clear();
	m_lengths.Clear();
	m_normals.Clear();
	m_totalPathLength = 0.0f;
}

Float CMoveNavigationPath::MapPointToPathDistance( const Vector& pos ) const
{
	Float minDistance = FLT_MAX;
	Float segmentLengthTotal = 0.0f;
	Float pathDistance = 0.0f;
	Float segmentProjection = 0.0f;
	Float dist = 0.0f;

	Uint32 count = m_points.Size();
	for ( Uint32 i = 1; i < count; ++i )
	{
		Uint32 segIdx = i - 1;
		PointToSegmentDistance( pos, segIdx, segmentProjection, dist );
		if ( dist < minDistance )
		{
			minDistance = dist;
			pathDistance = segmentLengthTotal + segmentProjection;
		}
		segmentLengthTotal += m_lengths[segIdx];
	}

	return pathDistance;
}

void CMoveNavigationPath::MapPointToPath( const Vector& pos, Vector& outPathPos, Float& outDistFromPath ) const
{
	Float minDistance = FLT_MAX;
	Float segmentProjection = 0.0f;
	Float dist = 0.0f;

	// loop over all segments, find the one nearest to the given point
	Uint32 count = m_points.Size();
	for ( Uint32 i = 1; i < count; ++i )
	{
		Uint32 segIdx = i - 1;
		PointToSegmentDistance( pos, segIdx, segmentProjection, dist );
		if ( dist < minDistance )
		{
			minDistance = dist;
			outPathPos = m_points[segIdx] + ( m_points[segIdx + 1] - m_points[segIdx] ) * segmentProjection;
		}
	}

	// measure how far original point is outside the Pathway's "tube"
	outDistFromPath = outPathPos.DistanceTo( pos) - m_radius;
}

void CMoveNavigationPath::PointToSegmentDistance( const Vector& pos, Uint32 segIdx, Float& outSegProjection, Float& outDist ) const
{
	// convert the test point to be "local" to ep0
	Vector local = pos - m_points[segIdx];

	// find the projection of "local" onto "segmentNormal"
	outSegProjection = Vector::Dot3( m_normals[segIdx], local );

	// handle boundary cases: when projection is not on segment, the
	// nearest point is one of the endpoints of the segment
	if ( outSegProjection < 0.0f )
	{
		outSegProjection = 0;
		outDist = pos.DistanceTo( m_points[segIdx] );
	}
	else if ( outSegProjection > m_lengths[segIdx] )
	{
		outSegProjection = m_lengths[segIdx];
		outDist = pos.DistanceTo( m_points[segIdx + 1] );
	}

	// otherwise nearest point is projection point on segment
	Vector ptInSeg = m_normals[segIdx] * outSegProjection;
	ptInSeg += m_points[segIdx];
	outDist = pos.DistanceTo( ptInSeg );
}

Vector CMoveNavigationPath::MapPathDistanceToPoint( Float distance ) const
{
	// clip or wrap given path distance according to cyclic flag
	Float remainingDist = distance;
	if ( distance < 0 ) 
	{
		return m_points[0];
	}
	if ( distance >= m_totalPathLength ) 
	{
		return m_points.Back();
	}

	// step through segments, subtracting off segment lengths until
	// locating the segment that contains the original pathDistance.
	// Interpolate along that segment to find 3d point value to return.
	Vector result;
	Uint32 count = m_points.Size();
	for ( Uint32 i = 1; i < count; ++i )
	{
		Uint32 segIdx = i - 1;
		if ( m_lengths[segIdx] < remainingDist )
		{
			remainingDist -= m_lengths[segIdx];
		}
		else
		{
			float ratio = remainingDist / m_lengths[segIdx];
			result = m_points[i-1] + ( m_points[i] - m_points[i-1] ) * ratio;
			break;
		}
	}
	return result;
}

void CMoveNavigationPath::GenerateDebugFragments( CRenderFrame* frame ) const
{
	Uint32 count = m_points.Size();
	for ( Uint32 i = 1; i < count; ++i )
	{
		frame->AddDebugFatLine( m_points[i - 1], m_points[i], Color::GREEN, 0.05f, true );
	}
}

void CMoveNavigationPath::DebugDraw( IDebugFrame& debugFrame ) const
{
	Uint32 count = m_points.Size();
	for ( Uint32 i = 1; i < count; ++i )
	{
		debugFrame.AddLine( m_points[i - 1], m_points[i], Color::GREEN );
	}
}

void CMoveNavigationPath::SetNextSeg( IMoveNavigationSegment* nextSeg )
{
	if ( nextSeg != NULL )
	{
		m_isLastSegment = nextSeg->GetTypeID() != CNAME( CMoveNavigationPath );
	}
	else
	{
		m_isLastSegment = true;
	}
}

CName CMoveNavigationPath::GetTypeID() const
{
	return CNAME( CMoveNavigationPath );
}

void CMoveNavigationPath::Process( INavigable& navigable )
{
	navigable.OnProcessNavSegment( *this );
}

///////////////////////////////////////////////////////////////////////////////
