/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibCollectCollisionPointsSpatialQuery.h"


#include "renderFrame.h"


namespace PathLib
{

CCollectCollisionPointsInCircleProxy::CCollectCollisionPointsInCircleProxy()
{
	
}

void CCollectCollisionPointsInCircleProxy::AddPoint( const Vector2& diff )
{
	Float dist = diff.Mag();
	if ( dist > NumericLimits< Float >::Epsilon() )
	{
		CollisionPoint p;
		p.m_dist = dist;
		p.m_normal = diff / dist;
		p.m_weight = p.m_normal.Y >= 0.f ? p.m_normal.X : -p.m_normal.X - 2.f;
		m_collisions.PushBack( p );
	}
}

void CCollectCollisionPointsInCircleProxy::ProcessOutput()
{
	struct Local
	{
		RED_INLINE static Bool IsOccluded( const CollisionPoint& p0, const CollisionPoint& p1 )
		{
			return p0.m_dist * ( p0.m_normal.Dot( p1.m_normal ) + NumericLimits< Float >::Epsilon() ) >= p1.m_dist;
		}

		RED_INLINE static Bool Occlude( CollisionPoint& p, CollisionPoint*& lastDominantPoint )
		{
			if ( IsOccluded( p, *lastDominantPoint ) )
			{
				p.m_weight = -1024.f;
				return true;
			}
			else
			{
				lastDominantPoint = &p;
				return false;
			}
		}
		
		static Uint32 RemoveSpacesImpl( Collection& collection, Uint32 size )
		{
			Uint32 i = 0;
			for ( ; i < size; ++i )
			{
				if ( collection[ i ].m_weight < -8.f )
				{
					break;
				}
			}

			Uint32 freeIndex = i;

			for ( ++i; i < size; ++i )
			{
				if ( collection[ i ].m_weight > -8.f )
				{
					collection[ freeIndex++ ] = collection[ i ];
				}
			}

			return freeIndex;
		}
		RED_INLINE static void RemoveSpaces( Collection& collection, Uint32 maxElems )
		{
			Uint32 shrinkedSize = RemoveSpacesImpl( collection, maxElems );
			if ( shrinkedSize < maxElems )
			{
				collection.Erase( collection.Begin() + shrinkedSize, collection.Begin() + maxElems );
			}
		}

		RED_INLINE static void RemoveSpaces( Collection& collection )
		{
			Uint32 newSize = RemoveSpacesImpl( collection, collection.Size() );
			collection.ResizeFast( newSize );
		}
	};

	// Simplify output by limiting it to dominant point. We run through all collision points and occlude them one by another.
	// Its not perfect solution, there might be cases where occluded point would occlude other point that won't get occluded that way,
	// but away of this, its cool enough.

	// lazy processing
	if ( m_outputIsProcessed )
	{
		return;
	}
	m_outputIsProcessed = true;

	if ( m_collisions.Size() < 2 )
	{
		return;
	}

	// sort collision output
	m_collisions.Sort();
	ASSERT ( m_collisions.IsSorted() );

	CollisionPoint* lastDominantPoint = &m_collisions[ 0 ];

	// find occluders by two scans
	
	// left-to-right
	{
		for ( Int32 i = 1, n = m_collisions.Size(); i < n; ++i )
		{
			CollisionPoint& p = m_collisions[ i ];
			Local::Occlude( p, lastDominantPoint );
		}

		CollisionPoint lastDominantTmp = *lastDominantPoint;
		lastDominantPoint = &lastDominantTmp;
		Local::RemoveSpaces( m_collisions );
		ASSERT ( m_collisions.IsSorted() );

		Int32 i = 0;
		for ( Int32 n = m_collisions.Size()-1; i < n; ++i )
		{
			CollisionPoint& p = m_collisions[ i ];
			if ( !Local::Occlude( p, lastDominantPoint ) )
			{
				break;
			}
		}
		Local::RemoveSpaces( m_collisions, i );
		ASSERT ( m_collisions.IsSorted() );
	}
	
	// right-to-left
	lastDominantPoint = &m_collisions[ m_collisions.Size() - 1 ];
	{
		for ( Int32 i = m_collisions.Size()-2; i >= 0; --i )
		{
			CollisionPoint& p = m_collisions[ i ];
			Local::Occlude( p, lastDominantPoint );
		}

		CollisionPoint lastDominantTmp = *lastDominantPoint;
		lastDominantPoint = &lastDominantTmp;
		Local::RemoveSpaces( m_collisions );
		ASSERT ( m_collisions.IsSorted() );

		Int32 i = m_collisions.Size()-1;
		for ( ; i >= 1; --i )
		{
			CollisionPoint& p = m_collisions[ i ];
			if ( !Local::Occlude( p, lastDominantPoint ) )
			{
				break;
			}
		}
		// removal of occluded last elements is somehow simplified..
		if ( i+1 < Int32( m_collisions.Size() ) )
		{
			m_collisions.ResizeFast( i+1 );
			ASSERT ( m_collisions.IsSorted() );
		}
	}
}


void CCollectCollisionPointsInCircleProxy::GetRepulsionSpot( Float maxDist, Vector2& outRepulsionDir, Float& outRepulsionRange )
{
	ASSERT( !m_collisions.Empty() );

	ProcessOutput();

	if ( m_collisions.Size() == 1 )
	{
		outRepulsionRange = maxDist;
		outRepulsionDir = m_collisions[ 0 ].m_normal;
		return;
	}

	Float furthestSpotSq = NumericLimits< Float >::Epsilon();
	Vector2 bestRepultionDir( 0.f, 0.f );


	const CollisionPoint* prevPoint = &m_collisions.Back();
	for ( const CollisionPoint& currPoint : m_collisions )
	{
		if ( prevPoint->m_normal.CrossZ( currPoint.m_normal ) > 0.f )
		{
			bestRepultionDir = MathUtils::GeometryUtils::PerpendicularR( currPoint.m_normal ) * currPoint.m_dist + MathUtils::GeometryUtils::PerpendicularL( prevPoint->m_normal ) * prevPoint->m_dist;
			outRepulsionDir = bestRepultionDir.Normalized();
			outRepulsionRange = maxDist;
			return;
		}

		Vector2 p00 = prevPoint->m_normal * -prevPoint->m_dist;
		Vector2 diff0 = MathUtils::GeometryUtils::PerpendicularL( prevPoint->m_normal ) * 128.f;
		Vector2 p01 = p00 + diff0;
		Vector2 p10 = currPoint.m_normal * -currPoint.m_dist;
		Vector2 diff1 = MathUtils::GeometryUtils::PerpendicularR( currPoint.m_normal ) * 128.f;
		Vector2 p11 = p00 + diff1;

		Float ratio0, ratio1;

		MathUtils::GeometryUtils::ClosestPointsLineLine2D( p00, p01, p10, p11, ratio0, ratio1 );

		// rare case that should normal get into that CrossZ test up thereif points are moving away from each other
		if ( ratio0 < NumericLimits< Float >::Epsilon() )
		{
			bestRepultionDir = MathUtils::GeometryUtils::PerpendicularR( currPoint.m_normal ) * currPoint.m_dist + MathUtils::GeometryUtils::PerpendicularL( prevPoint->m_normal ) * prevPoint->m_dist;
			outRepulsionDir = bestRepultionDir.Normalized();
			outRepulsionRange = maxDist;
			return;
		}

		Vector2 v0 = p00 + diff0 * ratio0;
		Vector2 v1 = p10 + diff1 * ratio1;

		Vector2 v = (v0 + v1) * 0.5f;
		Float distSq = v.SquareMag();
		if ( distSq > furthestSpotSq )
		{
			furthestSpotSq = distSq;
			bestRepultionDir = v;
		}
		prevPoint = &currPoint;
	}

	Float furthestSpot = MSqrt( furthestSpotSq );
	outRepulsionDir = bestRepultionDir * ( 1.f / furthestSpot );
	outRepulsionRange = Min( maxDist, furthestSpot );
}

Float CCollectCollisionPointsInCircleProxy::GetClosestCollision( Vector2& outNormal )
{
	ASSERT( !m_collisions.Empty() );

	ProcessOutput();

	Float closestSpot = FLT_MAX;
	Vector2 normal;

	for ( const CollisionPoint& currPoint : m_collisions )
	{
		if ( currPoint.m_dist < closestSpot )
		{
			closestSpot = currPoint.m_dist;
			normal = currPoint.m_normal;
		}
	}

	outNormal = normal;
	return closestSpot;
}

#ifndef NO_EDITOR_FRAGMENTS
void CCollectCollisionPointsInCircleProxy::DebugRender( CRenderFrame* frame, const Vector& centralSpot, Float testRadius )
{
	ProcessOutput();

	for ( const CollisionPoint& p : m_collisions )
	{
		Vector spot = centralSpot;
		spot.AsVector2() -= p.m_normal * p.m_dist;
		Vector2 perpedicular = MathUtils::GeometryUtils::PerpendicularL( p.m_normal ) * 2.f;
		Vector spotL = spot;
		Vector spotR = spot;
		spotL.AsVector2() += perpedicular;
		spotR.AsVector2() -= perpedicular;
		frame->AddDebugLine( spotL, spotR, Color::GREEN, true );
	}

	frame->AddDebugCircle( centralSpot, testRadius, Matrix::IDENTITY, Color::GREEN, 12, true );

	if ( !m_collisions.Empty() )
	{
		Vector2 repulsionDir;
		Float repulsionDist;
		GetRepulsionSpot( testRadius,repulsionDir, repulsionDist );
		Vector newPos = centralSpot;
		newPos.AsVector2() += repulsionDir * repulsionDist;
		frame->AddDebugLine( centralSpot, newPos, Color::RED, true );
	}
}
#endif


void CCollectCollisionPointsInCircleQueryData::NoticeSpot( const Vector2& point )
{
	Vector2 diff = m_circleCenter.AsVector2() - point;
	Float distSq = diff.SquareMag();
	if ( distSq < m_radius*m_radius && distSq > NumericLimits< Float >::Epsilon()  )
	{
		Float dist = MSqrt( distSq );
		Data::CollisionPoint p;
		p.m_dist = dist;
		p.m_normal = diff / dist;
		p.m_weight = p.m_normal.Y >= 0.f ? p.m_normal.X : -p.m_normal.X - 2.f;
		m_output.m_collisions.PushBack( p );
	}
}


};				// namespace PathLib