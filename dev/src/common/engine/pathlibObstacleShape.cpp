/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstacleShape.h"

#include "pathlibAreaDescription.h"
#include "pathlibSimpleBuffers.h"


#define F_SQRT2                  (1.41421356237309504880f)


namespace PathLib
{

const Float CObstacleShape::SHAPE_COMPARISION_PRECISION = 0.1f;
const Float CObstacleShape::SHAPE_COMPARISION_PRECISION_SQ = SHAPE_COMPARISION_PRECISION * SHAPE_COMPARISION_PRECISION;

////////////////////////////////////////////////////////////////////////////
// Shapes - serialization stuff
////////////////////////////////////////////////////////////////////////////

CObstacleShape* CObstacleShape::NewFromBuffer( CSimpleBufferReader& reader )
{
	Int8 shapeType;
	if ( !reader.Get( shapeType ) )
	{
		return NULL;
	}
	CObstacleShape* shape;
	switch ( shapeType )
	{
	case CLASS_POLY:
		shape = new CObstacleShapePoly();
		break;
	case CLASS_COMPOSITE:
		shape = new CObstacleShapeComposite();
		break;
	case CLASS_LINE:
		shape = new CObstacleShapeLineSegment();
		break;
	case CLASS_CIRCLE:
		shape = new CObstacleShapeCircle();
		break;
	case CLASS_BOX:
		shape = new CObstacleShapeBox();
		break;

	default:
		return NULL;
		break;
	}
	if ( !shape->ReadFromBuffer( reader ) )
	{
		delete shape;
		return NULL;
	}
	return shape;
}
void CObstacleShape::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Int8 shapeType = Int8(GetClass());
	writer.Put( shapeType );
	writer.Put( m_bbox[ 0 ] );
	writer.Put( m_bbox[ 1 ] );
}
Bool CObstacleShape::ReadFromBuffer( CSimpleBufferReader& reader )
{
	Bool ret = reader.Get( m_bbox[ 0 ] );
	ret = ret && reader.Get( m_bbox[ 1 ] );
	return ret;
}

void CObstacleShapeLineSegment::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );
	writer.Put( m_v1 );
	writer.Put( m_v2 );
}
Bool CObstacleShapeLineSegment::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
		return false;

	return reader.Get( m_v1 ) && reader.Get( m_v2 );
}

void CObstacleShapeCircle::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );
	writer.Put( m_center );
	writer.Put( m_radius );
}
Bool CObstacleShapeCircle::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
		return false;

	Bool ret = reader.Get( m_center );
	ret = ret && reader.Get( m_radius );
	return ret;
}

void CObstacleShapePoly::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );
	Uint16 vertsCount = Uint16( m_verts.Size() );
	writer.Put( vertsCount );
	for( Uint16 i = 0; i < vertsCount; ++i )
	{
		writer.Put( m_verts[ i ] );
	}
}
Bool CObstacleShapePoly::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
		return false;

	Uint16 vertsCount;
	if ( !reader.Get( vertsCount ) )
	{
		return false;
	}
	m_verts.Resize( vertsCount );
	for( Uint16 i = 0; i < vertsCount; ++i )
	{
		if ( !reader.Get( m_verts[ i ] ) )
		{
			return false;
		}
	}
	return true;
}

void CObstacleShapeComposite::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );

	Uint16 subshapesCount = Uint16( m_subShapes.Size() );
	writer.Put( subshapesCount );
	for ( Uint16 i = 0; i < subshapesCount; ++i )
	{
		m_subShapes[ i ]->WriteToBuffer( writer );
	}
}
Bool CObstacleShapeComposite::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
		return false;

	Uint16 subshapesCount;
	if ( !reader.Get( subshapesCount ) )
		return false;
	m_subShapes.Reserve( subshapesCount );
	for ( Uint16 i = 0; i < subshapesCount; ++i )
	{
		CObstacleShape* shape = CObstacleShape::NewFromBuffer( reader );
		if ( !shape )
		{
			return false;
		}
		m_subShapes.PushBack( shape );
	}
	return true;
}

CObstacleShape::EClass CObstacleShapeLineSegment::GetClass() const { return CObstacleShape::EClass(POLYCLASS); }
CObstacleShape::EClass CObstacleShapeCircle::GetClass() const { return CObstacleShape::EClass(POLYCLASS); }
CObstacleShape::EClass CObstacleShapeBox::GetClass() const { return CObstacleShape::EClass(POLYCLASS); }
CObstacleShape::EClass CObstacleShapePoly::GetClass() const { return CObstacleShape::EClass(POLYCLASS); }
CObstacleShape::EClass CObstacleShapeComposite::GetClass() const { return CObstacleShape::EClass(POLYCLASS); }

////////////////////////////////////////////////////////////////////////////
// Shapes - Contains implementation
////////////////////////////////////////////////////////////////////////////
namespace
{
	struct DummyTester
	{
		static Bool Contains( CObstacleShape* shape1, CObstacleShape* shape2 ) { return false; }
	};

	template < class T1, class T2, class BaseClass >
	struct ShapeTester : public BaseClass
	{
		static Bool Contains( CObstacleShape* shape1, CObstacleShape* shape2 )
		{
			return BaseClass::Contains( static_cast< T1* >( shape1 ), static_cast< T2* >( shape2 ) );
		}
	};

	struct BoxTester
	{
		static Bool Contains( CObstacleShapeBox* shape1, CObstacleShape* shape2 )
		{
			return true;
		}
	};

	struct PolyPolyTester
	{
		static Bool Contains( CObstacleShapePoly* shape1, CObstacleShapePoly* shape2 )
		{
			const TDynArray< Vector2 >& verts1 = shape1->GetVerts();
			const TDynArray< Vector2 >& verts2 = shape2->GetVerts();
			// TODO: optimize (O(nlogn)) is possible
			for ( Uint32 i = 0, n = verts2.Size(); i != n; ++i )
			{
				if ( !MathUtils::GeometryUtils::IsPointInPolygon2D( verts1, verts2[ i ] ) )
				{
					return false;
				}
			}
			return true;
		}
	};

	struct ShapeCompositeTester
	{
		static Bool Contains( CObstacleShape* shape1, CObstacleShapeComposite* shape2 )
		{
			const auto& shapeList = shape2->GetSubShapes();
			for ( auto it = shapeList.Begin(), end = shapeList.End(); it != end; ++it )
			{
				if ( !shape1->Contains( *it ) )
				{
					return false;
				}
			}
			return true;
		}
	};

	struct CompositeShapeTester
	{
		static Bool Contains( CObstacleShapeComposite* shape1, CObstacleShape* shape2 )
		{
			// not very accurate but simplified
			const auto& shapeList = shape1->GetSubShapes();
			for ( auto it = shapeList.Begin(), end = shapeList.End(); it != end; ++it )
			{
				if ( (*it)->Contains( shape2 ) )
				{
					return true;
				}
			}
			return false;
		}
	};

	typedef Bool (*ShapeFunction) ( CObstacleShape* , CObstacleShape* );

	static ShapeFunction ShapeContainsFunctions[ CObstacleShape::CLASS_COUNT * CObstacleShape::CLASS_COUNT ] =
	{
		//CLASS_POLY
		&ShapeTester< CObstacleShapePoly, CObstacleShapePoly, PolyPolyTester >::Contains,						//CLASS_POLY
		&ShapeTester< CObstacleShape, CObstacleShapeComposite, ShapeCompositeTester >::Contains,				//CLASS_COMPOSITE
		&DummyTester::Contains,																					//CLASS_LINE
		&DummyTester::Contains,																					//CLASS_CIRCLE
		&DummyTester::Contains,																					//CLASS_BOX
		//CLASS_COMPOSITE
		&ShapeTester< CObstacleShapeComposite, CObstacleShape, CompositeShapeTester >::Contains,				//CLASS_POLY
		&ShapeTester< CObstacleShapeComposite, CObstacleShape, CompositeShapeTester >::Contains,				//CLASS_COMPOSITE
		&DummyTester::Contains,																					//CLASS_LINE
		&DummyTester::Contains,																					//CLASS_CIRCLE
		&ShapeTester< CObstacleShapeComposite, CObstacleShape, CompositeShapeTester >::Contains,				//CLASS_BOX
		//CLASS_LINE
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		//CLASS_CIRCLE
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		&DummyTester::Contains,
		//CLASS_BOX
		&ShapeTester< CObstacleShapeBox, CObstacleShape, BoxTester >::Contains,									//CLASS_POLY
		&ShapeTester< CObstacleShapeBox, CObstacleShape, BoxTester >::Contains,									//CLASS_COMPOSITE
		&ShapeTester< CObstacleShapeBox, CObstacleShape, BoxTester >::Contains,									//CLASS_LINE
		&ShapeTester< CObstacleShapeBox, CObstacleShape, BoxTester >::Contains,									//CLASS_CIRCLE
		&ShapeTester< CObstacleShapeBox, CObstacleShape, BoxTester >::Contains,									//CLASS_BOX
	};
};

Bool CObstacleShape::Contains( CObstacleShape* shape )
{
	const Vector3& bboxMin1 = this->GetBBoxMin();
	const Vector3& bboxMax1 = this->GetBBoxMax();
	const Vector3& bboxMin2 = shape->GetBBoxMin();
	const Vector3& bboxMax2 = shape->GetBBoxMax();
	if ( bboxMin1.X >= bboxMin2.X &&
		bboxMin1.Y >= bboxMin2.Y &&
		bboxMin1.Z >= bboxMin2.Z &&
		bboxMax1.X <= bboxMax2.X &&
		bboxMax1.Y <= bboxMax2.Y && 
		bboxMax1.Z <= bboxMax2.Z )
	{
		EClass class1 = GetClass();
		EClass class2 = shape->GetClass();
		return ShapeContainsFunctions[ class1 * CObstacleShape::CLASS_COUNT + class2 ]( this, shape );
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////
// Shapes
////////////////////////////////////////////////////////////////////////////
CObstacleShape::~CObstacleShape()
{}

Bool CObstacleShape::ComputeDetourPoint( CAreaDescription* area, Float personalSpace, const Vector2& baseLocalPoint, const Vector2& deltaPosition, Vector3& outDetourPoint )
{
	Float worldMinZ = area->VLocalToWorldZ( m_bbox[ 0 ].Z );
	Float worldMaxZ = area->VLocalToWorldZ( m_bbox[ 1 ].Z );
	Float baseZ;
	Bool baseNavigable = true;
	Vector2 basePoint = baseLocalPoint;
	area->VLocalToWorld( basePoint );						// obstacle map is managed in local coordinates so we need to transform them to world
	if ( !area->VComputeHeight( basePoint, worldMinZ, worldMaxZ, baseZ ) )
	{
		baseZ = m_bbox[ 0 ].Z;
		baseNavigable = false;
	}

	outDetourPoint.AsVector2() = baseLocalPoint + deltaPosition;
	area->VLocalToWorld( outDetourPoint.AsVector2() );		// transform output position separately (in case of rotation)
	outDetourPoint.Z = baseZ;
	// try to place our point on navmesh/terrain ( with navmesh we can have few problems)
	if ( !area->VComputeHeight( outDetourPoint, outDetourPoint.Z ) )
	{
		if ( !area->VComputeHeight( outDetourPoint.AsVector2(), worldMinZ, worldMaxZ, outDetourPoint.Z ) )
		{
			if ( !baseNavigable || !area->VComputeHeightFrom( outDetourPoint.AsVector2(), Vector3( basePoint.X, basePoint.Y, baseZ ), outDetourPoint.Z ) )
			{
				return false;
			}
		}
	}

	if ( !area->GetBBox().Contains( outDetourPoint ) )
	{
		return false;
	}

	{
		CCircleQueryData query( CT_DEFAULT, outDetourPoint, personalSpace );
		if ( !area->VSpatialQuery( query ) )
		{
			return false;
		}
	}

	return true;
}
Bool CObstacleShape::ComputeLineDetour( CAreaDescription* area, Float personalSpace, const Vector2& v1, const Vector2& v2, ObstacleDetour& outDetour )
{
	Vector2 diff = v2 - v1;
	Float distSq = diff.SquareMag();
	Float maxNodesDist = area->GetMaxNodesDistance();
	if ( distSq <= maxNodesDist*maxNodesDist )
	{
		return true;
	}
	Float dist = sqrt( distSq );
	Float divisions = MCeil( dist / maxNodesDist );

	Vector2 perpedicular = MathUtils::GeometryUtils::PerpendicularR( diff / dist );
	perpedicular *= personalSpace + 0.05f;

	Vector2 modifiedDiff = diff / divisions;		// set length to dist / divisions

	Uint32 newNodes = Uint32( divisions ) - 1;
	Vector2 nodePos = v1;
	for ( Uint32 i = 0; i < newNodes; ++i )
	{
		nodePos += modifiedDiff;
		Vector3 detourPoint;
		if ( ComputeDetourPoint( area, personalSpace, nodePos, perpedicular, detourPoint ) )
		{
			outDetour.PushBack( detourPoint );
			if ( outDetour.Size() == outDetour.Capacity() )
			{
				return false;
			}
		}
	}
	return true;
}
void CObstacleShape::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	ASSERT( false, TXT("Obstacle type is not yet implemented!\n") );
}

Bool CObstacleShape::CompareShape( CObstacleShape* shape )
{
	if ( GetClass() != shape->GetClass() )
	{
		return false;
	}
	if ( (m_bbox[0] - shape->m_bbox[0]).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ 
		|| (m_bbox[1] - shape->m_bbox[1]).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ )
	{
		return false;
	}
	return VCompareShape( shape );
}

void CObstacleShape::InitializeBBox( const Vector3& b1, const Vector3& b2 )
{
	m_bbox[ 0 ] = b1;
	m_bbox[ 1 ] = b2;

	ASSERT( m_bbox[ 0 ].X <= m_bbox[ 1 ].X, TXT( "Creating Box with min.X > max.X") );
	ASSERT( m_bbox[ 0 ].Y <= m_bbox[ 1 ].Y, TXT( "Creating Box with min.Y > max.Y") );
	ASSERT( m_bbox[ 0 ].Z <= m_bbox[ 1 ].Z, TXT( "Creating Box with min.Z > max.Z") );
}

CObstacleShapePoly* CObstacleShape::AsPoly()
{
	return nullptr;
}
CObstacleShapeLineSegment* CObstacleShape::AsLineSegment()
{
	return nullptr;
}
CObstacleShapeCircle* CObstacleShape::AsCircle()
{
	return nullptr;
}
CObstacleShapeBox* CObstacleShape::AsBox()
{
	return nullptr;
}
CObstacleShapeComposite* CObstacleShape::AsComposite()
{
	return nullptr;
}
////////////////////////////////////////////////////////////////////////////
// CObstalceShapeLineSegment
Bool CObstacleShapeLineSegment::VCompareShape( CObstacleShape* shape )
{
	CObstacleShapeLineSegment* s = static_cast< CObstacleShapeLineSegment* >( shape );
	if ( (s->m_v1 - m_v1).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ ||
		(s->m_v2 - m_v2).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ )
	{
		return false;
	}
	return true;
}

Bool CObstacleShapeLineSegment::VTestLocation( const Vector3& v ) const
{
	return true;
}

Bool CObstacleShapeLineSegment::VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const
{
	return !query.Intersect( m_v1, m_v2 );
}

Bool CObstacleShapeLineSegment::VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( query.m_circleCenter.AsVector2(), query.m_radius, m_v1, m_v2 );
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const
{
	return query.Intersect( m_v1, m_v2 );
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPos;
	MathUtils::GeometryUtils::TestClosestPointOnLine2D( query.m_circleCenter.AsVector2(), m_v1, m_v2, closestPos );
	Float distSq = (query.m_circleCenter.AsVector2() - closestPos).SquareMag();
	if ( distSq < query.m_closestDistSq )
	{
		query.m_pointOut.AsVector2() = closestPos;
		query.m_pointOut.Z = query.m_circleCenter.Z;
		query.m_closestDistSq = distSq;
		query.m_obstacleHit = true;
	}
	return true;
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestSpot;
	MathUtils::GeometryUtils::TestClosestPointOnLine2D( query.m_circleCenter.AsVector2(), m_v1, m_v2, closestSpot );
	query.NoticeSpot( closestSpot );
	return true;
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionLineLine2D( query.m_v1.AsVector2(), query.m_v2.AsVector2(), m_v1, m_v2 );
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const
{
	Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( query.m_v1.AsVector2(), query.m_v2.AsVector2(), m_v1, m_v2 );
	if ( distSq <= query.m_radius * query.m_radius )
	{
		return false;
	}
	return true;
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const
{
	Vector2 pointSegmentOut;
	Vector2 pointObstacleOut;
	MathUtils::GeometryUtils::ClosestPointsLineLine2D( query.m_v1.AsVector2(), query.m_v2.AsVector2(), m_v1, m_v2, pointSegmentOut, pointObstacleOut );
	Float distSq = (pointSegmentOut - pointObstacleOut).SquareMag();
	if ( distSq < query.m_closestDistSq )
	{
		query.m_closestDistSq = distSq;
		query.m_closestPointOnSegment.AsVector2() = pointSegmentOut;
		query.m_closestGeometryPoint.AsVector2() = pointObstacleOut;
		query.m_closestPointOnSegment.Z = query.m_v1.Z;
		query.m_closestGeometryPoint.Z = query.m_v1.Z;
		query.m_obstacleHit = true;
	}

	return true;
}
Bool CObstacleShapeLineSegment::VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const
{
	struct Fun
	{
		const CObstacleShapeLineSegment& m_me;

		Fun( const CObstacleShapeLineSegment* me )
			: m_me( *me ) {}

		RED_INLINE Bool operator()( const Vector2& v, Float ps ) const
		{
			return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( v, ps, m_me.m_v1, m_me.m_v2 );
		}
		RED_INLINE Bool operator()( const Vector2& v1, const Vector2& v2, Float ps ) const
		{
			Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( v1, v2, m_me.m_v1, m_me.m_v2 );
			return distSq > ps*ps;
		}
	} fun( this );

	if ( !fun( query.m_v1.AsVector2(), query.m_v2.AsVector2(), query.m_radius ) )
	{
		return query.OnIntersection( fun );
	}
	return true;
}


Bool CObstacleShapeLineSegment::VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const
{
	if ( MathUtils::GeometryUtils::TestIntersectionCircleLine2D( query.m_circleCenter.AsVector2(), query.m_radius, m_v1, m_v2 ) )
	{
		auto& o = query.m_output;
		o.Grow( 2 );
		o[ o.Size()-2 ] = Vector( m_v1.X, m_v1.Y, query.m_basePos.Z );
		o[ o.Size()-1 ] = Vector( m_v1.X, m_v1.Y, query.m_basePos.Z );
	}

	return true;
}

Bool CObstacleShapeLineSegment::VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const
{
	return !query.m_customTester->IntersectLine( m_v1, m_v2 );
}

void CObstacleShapeLineSegment::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	Vector2 dirVec = (m_v2 - m_v1).Normalized();
	dirVec *= personalSpace + 0.05f;

	auto funComputePoint =
		[ this, area, personalSpace, &outDetour ] ( const Vector2& v, const Vector2& delta ) -> Bool
	{
		Vector3 detourPoint;
		if ( this->ComputeDetourPoint( area, personalSpace, v, delta, detourPoint ) )
		{
			outDetour.PushBack( detourPoint );
			if ( outDetour.Size() == outDetour.Capacity() )
			{
				// out of capacity:(
				return false;
			}
		}
		return true;
	};
	if ( !ComputeLineDetour( area, personalSpace, m_v1, m_v2, outDetour ) )
	{
		return;
	}

	if ( !funComputePoint( m_v2, dirVec + MathUtils::GeometryUtils::PerpendicularL( dirVec ) ) )
	{
		return;
	}
	if ( !funComputePoint( m_v2, dirVec + MathUtils::GeometryUtils::PerpendicularR( dirVec ) ) )
	{
		return;
	}

	if ( !ComputeLineDetour( area, personalSpace, m_v2, m_v1, outDetour ) )
	{
		return;
	}

	if ( !funComputePoint( m_v1, -dirVec + MathUtils::GeometryUtils::PerpendicularL( dirVec ) ) )
	{
		return;
	}
	if ( !funComputePoint( m_v1, -dirVec + MathUtils::GeometryUtils::PerpendicularR( dirVec ) ) )
	{
		return;
	}
}

Vector3 CObstacleShapeLineSegment::GetVert( Uint32 i ) const
{
	ASSERT( i < 4 );
	Vector3 ret;
	Bool up = i > 1;
	Bool ind0 = i & 1;
	ret.AsVector2() = ind0 ? m_v1 : m_v2;
	ret.Z = m_bbox[ up ].Z;
	return ret;
}

CObstacleShapeLineSegment* CObstacleShapeLineSegment::AsLineSegment()
{
	return this;
}

////////////////////////////////////////////////////////////////////////////
// CObstacleShapeCircle
Bool CObstacleShapeCircle::VCompareShape( CObstacleShape* shape )
{
	CObstacleShapeCircle* s = static_cast< CObstacleShapeCircle* >( shape );
	if ( (s->m_radius - m_radius) > SHAPE_COMPARISION_PRECISION ||
		(s->m_center - m_center).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ )
	{
		return false;
	}
	return true;
}

Bool CObstacleShapeCircle::VTestLocation( const Vector3& v ) const
{
	return ( v.AsVector2() - m_center ).SquareMag() > ( m_radius*m_radius );
}
Bool CObstacleShapeCircle::VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const
{
	Float computedRadius = m_radius + query.m_radius;

	Float centerDistSq = (query.m_circleCenter.AsVector2() - m_center.AsVector2()).SquareMag();
	if ( centerDistSq < (computedRadius*computedRadius) )
		return false;

	return true;
}
Bool CObstacleShapeCircle::VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 diff = query.m_circleCenter.AsVector2() - m_center.AsVector2();
	Float centerDistSq = diff.SquareMag();
	if ( centerDistSq < (m_radius*m_radius) )
	{
		query.m_pointOut = query.m_circleCenter;
		query.m_closestDistSq = 0.f;
		query.m_obstacleHit = true;
		return true;
	}
	Float centerDist = sqrt( centerDistSq );
	Float dist = centerDist - m_radius;
	Float distSq = dist*dist;
	if( distSq < query.m_closestDistSq )
	{
		query.m_pointOut = m_center + diff * (m_radius / centerDist);
		query.m_closestDistSq = distSq;
		query.m_obstacleHit = true;
	}
	return true;
}
Bool CObstacleShapeCircle::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestSpot;
	query.ClosestSpotCircle( m_center.AsVector2(), m_radius, closestSpot );
	query.NoticeSpot( closestSpot );
	return true;
}


Bool CObstacleShapeCircle::VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const
{
	Float computedRadius = m_radius + query.m_radius;

	Float centerDistSq = (query.m_circleCenter.AsVector2() - m_center.AsVector2()).SquareMag();
	if ( centerDistSq < (computedRadius*computedRadius) )
	{
		ASSERT( false, TXT("TODO: Collect collision geometry from circle obstacle shape!\n") );
	}
	return true;
}
Bool CObstacleShapeCircle::VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_center, m_radius, query.m_v1.AsVector2(), query.m_v2.AsVector2() );
}
Bool CObstacleShapeCircle::VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_center, m_radius + query.m_radius, query.m_v1.AsVector2(), query.m_v2.AsVector2() );
}
Bool CObstacleShapeCircle::VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPointOnLine;
	MathUtils::GeometryUtils::TestClosestPointOnLine2D( m_center, query.m_v1.AsVector2(), query.m_v2.AsVector2(), closestPointOnLine );
	Vector2 diff = closestPointOnLine - m_center;
	Float centerDistSq = diff.SquareMag();
	if ( centerDistSq < (m_radius*m_radius) )
	{
		query.m_closestGeometryPoint = closestPointOnLine;
		query.m_closestPointOnSegment = closestPointOnLine;
		query.m_closestDistSq = 0.f;
		query.m_obstacleHit = true;
		return true;
	}
	Float centerDist = sqrt( centerDistSq );
	Float dist = centerDist - m_radius;
	Float distSq = dist*dist;
	if( distSq < query.m_closestDistSq )
	{
		query.m_closestDistSq = distSq;
		query.m_closestPointOnSegment = closestPointOnLine;
		query.m_closestGeometryPoint = m_center + diff * (m_radius / centerDist);
		query.m_obstacleHit = true;
	}
	return false;
}

Bool CObstacleShapeCircle::VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const
{
	struct Fun
	{
		const CObstacleShapeCircle& m_me;

		Fun( const CObstacleShapeCircle* me )
			: m_me( *me ) {}

		RED_INLINE Bool operator()( const Vector2& v, Float ps ) const
		{
			Float radSq = ps+m_me.m_radius;
			radSq *= radSq;
			return (v - m_me.m_center).SquareMag() > radSq;
		}
		RED_INLINE Bool operator()( const Vector2& v1, const Vector2& v2, Float ps ) const
		{
			return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_me.m_center, m_me.m_radius + ps, v1, v2 );
		}
	} fun( this );

	if ( !fun( query.m_v1.AsVector2(), query.m_v2.AsVector2(), query.m_radius ) )
	{
		return query.OnIntersection( fun );
	}
	return true;
}

Bool CObstacleShapeCircle::VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( query.m_vMin, query.m_vMax, m_center, m_radius );
}
Bool CObstacleShapeCircle::VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionPolygonCircle2D( query.m_poly, m_center, m_radius );
}
Bool CObstacleShapeCircle::VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const
{
	// TODO
	return true;
}

void CObstacleShapeCircle::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	// point distance = (( personalSpace + radius )^2 * 2)^1/2
	Float pointDistance = (personalSpace + m_radius) * F_SQRT2 + 0.05f;

	auto funComputePoint =
		[ this, area, personalSpace, &outDetour ] ( const Vector2& v, const Vector2& delta ) -> Bool
	{
		Vector3 detourPoint;
		if ( this->ComputeDetourPoint( area, personalSpace, v, delta, detourPoint ) )
		{
			outDetour.PushBack( detourPoint );
			if ( outDetour.Size() == outDetour.Capacity() )
			{
				// out of capacity:(
				return false;
			}
		}
		return true;
	};

	Vector2 v[4];
	v[ 0 ].Set( pointDistance, 0.f );
	v[ 1 ].Set( 0.f, pointDistance );
	v[ 2 ].Set( -pointDistance, 0.f );
	v[ 3 ].Set( 0.f, -pointDistance );

	if ( !funComputePoint( m_center, v[ 0 ] ) )
	{
		return;
	}
	if ( !ComputeLineDetour( area, personalSpace, v[ 0 ], v[ 1 ], outDetour ) )
	{
		return;
	}
	if ( !funComputePoint( m_center, v[ 1 ] ) )
	{
		return;
	}
	if ( !ComputeLineDetour( area, personalSpace, v[ 1 ], v[ 2 ], outDetour ) )
	{
		return;
	}
	if ( !funComputePoint( m_center, v[ 2 ] ) )
	{
		return;
	}
	if ( !ComputeLineDetour( area, personalSpace, v[ 2 ], v[ 3 ], outDetour ) )
	{
		return;
	}
	if ( !funComputePoint( m_center, v[ 3 ] ) )
	{
		return;
	}
	if ( !ComputeLineDetour( area, personalSpace, v[ 3 ], v[ 0 ], outDetour ) )
	{
		return;
	}
}

CObstacleShapeCircle* CObstacleShapeCircle::AsCircle()
{
	return this;
}

////////////////////////////////////////////////////////////////////////////
// CObstacleShapeBox
Bool CObstacleShapeBox::VCompareShape( CObstacleShape* shape )
{
	return true;
}
Bool CObstacleShapeBox::VTestLocation( const Vector3& v ) const
{
	return false;
}
Bool CObstacleShapeBox::VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), query.m_circleCenter.AsVector2(), query.m_radius );
}
Bool CObstacleShapeBox::VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPoint;
	MathUtils::GeometryUtils::ClosestPointToRectangle2D( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), query.m_circleCenter.AsVector2(), closestPoint );
	Float distSq = (closestPoint - query.m_circleCenter.AsVector2()).SquareMag();
	if ( distSq < query.m_closestDistSq )
	{
		query.m_closestDistSq = distSq;
		query.m_pointOut = closestPoint;
		query.m_obstacleHit = true;
	}
	return true;
}
Bool CObstacleShapeBox::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestSpot;
	query.ClosestSpotRect( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), closestSpot );
	query.NoticeSpot( closestSpot );
	return true;
}


Bool CObstacleShapeBox::VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const
{
	if ( !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), query.m_circleCenter.AsVector2(), query.m_radius ) )
	{
		Vector v1( m_bbox[ 0 ].X, m_bbox[ 0 ].Y, query.m_basePos.Z );
		Vector v2( m_bbox[ 0 ].X, m_bbox[ 1 ].Y, query.m_basePos.Z );
		Vector v3( m_bbox[ 1 ].X, m_bbox[ 0 ].Y, query.m_basePos.Z );
		Vector v4( m_bbox[ 1 ].X, m_bbox[ 1 ].Y, query.m_basePos.Z );

		auto& o = query.m_output;
		Uint32 i = o.Size();
		o.Grow( 8 );

		o[ i+0 ] = v1;
		o[ i+1 ] = v2;
		o[ i+2 ] = v2;
		o[ i+3 ] = v4;
		o[ i+4 ] = v4;
		o[ i+5 ] = v3;
		o[ i+6 ] = v3;
		o[ i+7 ] = v1;
	}
	return true;
}
Bool CObstacleShapeBox::VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionLineRectangle2D( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), query.m_v1.AsVector2(), query.m_v2.AsVector2() );
}
Bool CObstacleShapeBox::VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const
{
	Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineRectangle2D( m_bbox[0].AsVector2(), m_bbox[1].AsVector2(), query.m_v1.AsVector2(), query.m_v2.AsVector2() );
	return distSq > query.m_radius*query.m_radius;
}
Bool CObstacleShapeBox::VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const
{
	Vector2 pointLineOut, pointRectangleOut;
	MathUtils::GeometryUtils::ClosestPointLineRectangle2D( query.m_v1.AsVector2(), query.m_v2.AsVector2(), bbox[0].AsVector2(), bbox[1].AsVector2(), pointLineOut, pointRectangleOut );
	Float distSq = (pointLineOut - pointRectangleOut).SquareMag();
	if ( distSq < query.m_closestDistSq )
	{
		query.m_closestGeometryPoint.AsVector2() = pointRectangleOut;
		query.m_closestPointOnSegment.AsVector2() = pointLineOut;
		query.m_closestGeometryPoint.Z = query.m_v1.Z;
		query.m_closestPointOnSegment.Z = query.m_v1.Z;
		query.m_closestDistSq = distSq;
		query.m_obstacleHit = true;
	}
	return true;
}

Bool CObstacleShapeBox::VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const
{
	struct Fun
	{
		const CObstacleShapeBox& m_me;

		Fun( const CObstacleShapeBox* me )
			: m_me( *me ) {}

		RED_INLINE Bool operator()( const Vector2& v, Float ps ) const
		{
			return !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_me.m_bbox[0].AsVector2(), m_me.m_bbox[1].AsVector2(), v, ps );
		}
		RED_INLINE Bool operator()( const Vector2& v1, const Vector2& v2, Float ps ) const
		{
			Float distSq = MathUtils::GeometryUtils::TestDistanceSqrLineRectangle2D( m_me.m_bbox[0].AsVector2(), m_me.m_bbox[1].AsVector2(), v1.AsVector2(), v2.AsVector2() );
			return distSq > ps*ps;
		}
	} fun( this );

	if ( !fun( query.m_v1.AsVector2(), query.m_v2.AsVector2(), query.m_radius ) )
	{
		return query.OnIntersection( fun );
	}
	return true;
}

Bool CObstacleShapeBox::VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const
{
	return
		!MathUtils::GeometryUtils::RangeOverlap1D( query.m_vMin.X, query.m_vMax.X, m_bbox[0].X, m_bbox[1].X ) ||
		!MathUtils::GeometryUtils::RangeOverlap1D( query.m_vMin.Y, query.m_vMax.Y, m_bbox[0].Y, m_bbox[1].Y );
}
Bool CObstacleShapeBox::VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const
{
	return !query.IntersectRect( bbox[ 0 ].AsVector2(), bbox[ 1 ].AsVector2() );
}
Bool CObstacleShapeBox::VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const
{
	// TODO
	return true;
}

void CObstacleShapeBox::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	// point distance = (( personalSpace + radius )^2 * 2)^1/2
	Float pointDistance = personalSpace * F_SQRT2 + 0.05f;

	auto funComputePoint =
		[ this, area, personalSpace, &outDetour ] ( const Vector2& v, const Vector2& delta ) -> Bool
	{
		Vector3 detourPoint;
		if ( this->ComputeDetourPoint( area, personalSpace, v, delta, detourPoint ) )
		{
			outDetour.PushBack( detourPoint );
			if ( outDetour.Size() == outDetour.Capacity() )
			{
				// out of capacity:(
				return false;
			}
		}
		return true;
	};
	if ( !funComputePoint( Vector2( m_bbox[ 0 ].X, m_bbox[ 0 ].Y ), Vector2( -pointDistance, -pointDistance ) ) )
	{
		return;
	}
	if ( !funComputePoint( Vector2( m_bbox[ 1 ].X, m_bbox[ 0 ].Y ), Vector2( pointDistance, -pointDistance ) ) )
	{
		return;
	}
	if ( !funComputePoint( Vector2( m_bbox[ 1 ].X, m_bbox[ 1 ].Y ), Vector2( pointDistance, pointDistance ) ) )
	{
		return;
	}
	if ( !funComputePoint( Vector2( m_bbox[ 0 ].X, m_bbox[ 1 ].Y ), Vector2( -pointDistance, pointDistance ) ) )
	{
		return;
	}
}

CObstacleShapeBox* CObstacleShapeBox::AsBox()
{
	return this;
}

////////////////////////////////////////////////////////////////////////////
// CObstacleShapePoly
Bool CObstacleShapePoly::VCompareShape( CObstacleShape* shape )
{
	CObstacleShapePoly* s = static_cast< CObstacleShapePoly* >( shape );
	if ( m_verts.Size() != s->m_verts.Size() )
	{
		return false;
	}
	for ( Uint32 i = 0, n = m_verts.Size(); i != n; ++i )
	{
		if ( (m_verts[ i ] - s->m_verts[ i ]).SquareMag() > SHAPE_COMPARISION_PRECISION_SQ )
		{
			return false;
		}
	}
	return true;
}
Bool CObstacleShapePoly::VTestLocation( const Vector3& v ) const
{
	return !MathUtils::GeometryUtils::IsPointInPolygon2D( m_verts, v.AsVector2() );
}
Bool CObstacleShapePoly::VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionPolygonCircle2D( m_verts, query.m_circleCenter.AsVector2(), query.m_radius );
}
Bool CObstacleShapePoly::VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPoint;
	Float distSq = MathUtils::GeometryUtils::ClosestPointPolygonPoint2D( m_verts, query.m_circleCenter.AsVector2(), sqrt( query.m_closestDistSq ), closestPoint );
	if ( distSq < query.m_closestDistSq )
	{
		query.m_closestDistSq = distSq;
		query.m_pointOut.AsVector2() = closestPoint;
		query.m_pointOut.Z = query.m_circleCenter.Z;
		query.m_obstacleHit = true;
	}
	return true;
}
Bool CObstacleShapePoly::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPoint;
	Float distSq = MathUtils::GeometryUtils::ClosestPointPolygonPoint2D( m_verts, query.m_circleCenter.AsVector2(), query.m_radius, closestPoint );
	if ( distSq < query.m_radius * query.m_radius )
	{
		query.OnIntersection( closestPoint );
	}

	return true;
}

Bool CObstacleShapePoly::VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const
{
	Vector2 bboxSegment[2];
	bboxSegment[ 0 ] = query.m_circleCenter.AsVector2() - Vector2( query.m_radius, query.m_radius );
	bboxSegment[ 1 ] = query.m_circleCenter.AsVector2() + Vector2( query.m_radius, query.m_radius );

	auto functor =
		[ &query ] ( const Vector2& v1, const Vector2& v2 ) -> Bool
	{
		if( MathUtils::GeometryUtils::TestIntersectionCircleLine2D( query.m_circleCenter.AsVector2(), query.m_radius, v1, v2 ) )
		{
			auto& o = query.m_output;
			o.Grow( 2 );
			o[ o.Size()-2 ] = Vector( v1.X, v1.Y, query.m_circleCenter.Z );
			o[ o.Size()-1 ] = Vector( v2.X, v2.Y, query.m_circleCenter.Z );
		}
		return false;
	};

	MathUtils::GeometryUtils::PolygonTest2D( m_verts, bboxSegment, functor );
	return true;
}
Bool CObstacleShapePoly::VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( m_verts, query.m_v1.AsVector2(), query.m_v2.AsVector2() );
}
Bool CObstacleShapePoly::VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( m_verts, query.m_v1.AsVector2(), query.m_v2.AsVector2(), query.m_radius );
}
Bool CObstacleShapePoly::VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const
{
	Vector2 closestPointPoly;
	Vector2 closestPointSegment;
	Float distSq = MathUtils::GeometryUtils::ClosestPointPolygonLine2D( m_verts, query.m_v1.AsVector2(), query.m_v2.AsVector2(), sqrt( query.m_closestDistSq ), closestPointPoly, closestPointSegment );
	if ( distSq < query.m_closestDistSq )
	{
		query.m_closestDistSq = distSq;
		query.m_closestGeometryPoint.AsVector2() = closestPointPoly;
		query.m_closestPointOnSegment.AsVector2() = closestPointSegment;
		query.m_closestGeometryPoint.Z = query.m_v1.Z;
		query.m_closestPointOnSegment.Z = query.m_v1.Z;
		query.m_obstacleHit = true;
	}
	return true;
}

Bool CObstacleShapePoly::VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const
{
	struct Fun
	{
		const CObstacleShapePoly& m_me;

		Fun( const CObstacleShapePoly* me )
			: m_me( *me ) {}

		RED_INLINE Bool operator()( const Vector2& v, Float ps ) const
		{
			return !MathUtils::GeometryUtils::TestIntersectionPolygonCircle2D( m_me.m_verts, v, ps );
		}
		RED_INLINE Bool operator()( const Vector2& v1, const Vector2& v2, Float ps ) const
		{
			return !MathUtils::GeometryUtils::TestIntersectionPolygonLine2D( m_me.m_verts, v1, v2, ps );
		}
	} fun( this );

	if ( !fun( query.m_v1.AsVector2(), query.m_v2.AsVector2(), query.m_radius ) )
	{
		return query.OnIntersection( fun );
	}
	return true;
}

Bool CObstacleShapePoly::VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const
{
	// TODO: optimize
	return !MathUtils::GeometryUtils::TestIntersectionPolygonRectangle2D( m_verts, query.m_vMin, query.m_vMax, NumericLimits< Float >::Epsilon() );
}
Bool CObstacleShapePoly::VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const
{
	return !MathUtils::GeometryUtils::IsPolygonsIntersecting2D( query.m_poly, m_verts );
}
Bool CObstacleShapePoly::VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const
{
	if ( MathUtils::GeometryUtils::IsPointInPolygon2D( m_verts, query.m_basePos.AsVector2() ) )
	{
		return false;
	}

	Vector2 bboxTest[2];
	bboxTest[ 0 ] = bbox[ 0 ].AsVector2();
	bboxTest[ 1 ] = bbox[ 1 ].AsVector2();

	auto functor = 
		[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
	{
		return query.m_customTester->IntersectLine( vertCurr, vertPrev );
	};

	return !MathUtils::GeometryUtils::PolygonTest2D( m_verts, bboxTest, functor );
}



void CObstacleShapePoly::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	Uint32 vertsCount = m_verts.Size();
	// TEMPORARY SHIT TO SUPPORT SHITTY DATA
	if ( vertsCount < 3 )
	{
		return;
	}
	Vector2 prevPoint = m_verts[ vertsCount-2 ];
	Vector2 currPoint = m_verts[ vertsCount-1 ];
	for ( Uint32 i = 0, n = m_verts.Size(); i < n; ++i )
	{
		if ( !ComputeLineDetour( area, personalSpace, prevPoint, currPoint, outDetour ) )
		{
			return;
		}

		Vector2 nextPoint = m_verts[ i ];

		// compute new node position
		Vector2 dirVec1 = currPoint - prevPoint;
		Vector2 dirVec2 = currPoint - nextPoint;
		dirVec1.Normalize();
		dirVec2.Normalize();
		// Calculate node distance based on angle between occluder walls
		Float cos2A = dirVec1.Dot( -dirVec2 );
		const Float r = personalSpace + 0.05f;
		if ( cos2A < -0.001f )
		{
			// special case when angle is greater then 90 degrees. In this case we put 2 nodes insteady of one
			Float cosA = sqrt( (cos2A + 1.f) / 2.f );
			Float sinA = sqrt( 1.f - cosA*cosA );

			Float x = (r * ( 1.f - cosA )) / sinA;
			// Now compute two locations
			Vector2 delta1 = MathUtils::GeometryUtils::PerpendicularR(dirVec1) * r + dirVec1 * x;
			Vector2 delta2 = MathUtils::GeometryUtils::PerpendicularL(dirVec2) * r + dirVec2 * x;

			Vector3 detourPoint1;
			Vector3 detourPoint2;

			if ( ComputeDetourPoint( area, personalSpace, currPoint, delta1, detourPoint1 ) )
			{
				outDetour.PushBack( detourPoint1 );
				if ( outDetour.Size() == outDetour.Capacity() )
				{
					// out of capacity:(
					return;
				}
			}
			if ( ComputeDetourPoint( area, personalSpace, currPoint, delta2, detourPoint2 ) )
			{
				outDetour.PushBack( detourPoint2 );
				if ( outDetour.Size() == outDetour.Capacity() )
				{
					// out of capacity:(
					return;
				}
			}
		}
		else
		{
			// spawn single node
			Float cosA = sqrt( (cos2A + 1.f) / 2.f );
			Float dist = r / cosA ;			// at most: 2^(1/2) * DEFAULT_PS
			//
			Vector2 dirVec = dirVec1+dirVec2;
			if ( !dirVec.IsZero() )
			{
				dirVec.Normalize();
				dirVec *= dist;
				Vector3 detourPoint;
				if ( ComputeDetourPoint( area, personalSpace, currPoint, dirVec, detourPoint ) )
				{
					outDetour.PushBack( detourPoint );
					if ( outDetour.Size() == outDetour.Capacity() )
					{
						// out of capacity:(
						return;
					}
				}
			}
		}

		prevPoint = currPoint;
		currPoint = nextPoint;
	}
}


CObstacleShapePoly* CObstacleShapePoly::AsPoly()
{
	return this;
}

////////////////////////////////////////////////////////////////////////////
// CObstacleShapeComposite
CObstacleShapeComposite::~CObstacleShapeComposite()
{
	ForEach( m_subShapes,
		[] ( CObstacleShape* subShape )
	{
		delete subShape;
	}
	);
}
Bool CObstacleShapeComposite::VCompareShape( CObstacleShape* shape )
{
	CObstacleShapeComposite* s = static_cast< CObstacleShapeComposite* >( shape );
	if ( m_subShapes.Size() != s->m_subShapes.Size() )
	{
		return false;
	}
	for ( Uint32 i = 0, n = m_subShapes.Size(); i != n; ++i )
	{
		if ( !m_subShapes[ i ]->CompareShape( s->m_subShapes[ i ] ) )
		{
			return false;
		}
	}
	return true;
}
void CObstacleShapeComposite::ComputeBBox()
{
	m_bbox[ 0 ].Set( FLT_MAX, FLT_MAX, FLT_MAX );
	m_bbox[ 1 ].Set(-FLT_MAX,-FLT_MAX,-FLT_MAX );
	for ( Uint32 i = 0, n = m_subShapes.Size(); i != n; ++i )
	{
		const auto& shape = *m_subShapes[ i ];
		const Vector3& shapeBoxMin = shape.GetBBoxMin();
		const Vector3& shapeBoxMax = shape.GetBBoxMax();
		m_bbox[ 0 ].Set(
			Min( m_bbox[ 0 ].X, shapeBoxMin.X ),
			Min( m_bbox[ 0 ].Y, shapeBoxMin.Y ),
			Min( m_bbox[ 0 ].Z, shapeBoxMin.Z )
			);

		m_bbox[ 1 ].Set(
			Max( m_bbox[ 1 ].X, shapeBoxMax.X ),
			Max( m_bbox[ 1 ].Y, shapeBoxMax.Y ),
			Max( m_bbox[ 1 ].Z, shapeBoxMax.Z )
			);
	}
}

template < class TQuery >
RED_INLINE Bool CObstacleShapeComposite::TSpatialQuery( TQuery& query, const Vector3* bbox ) const
{
	for ( auto it = m_subShapes.Begin(), end = m_subShapes.End(); it != end; ++it )
	{
		CObstacleShape* shape = *it;
		if ( shape->TestBoundings( bbox[ 0 ], bbox[ 1 ] ) )
		{
			if ( !shape->VSpatialQuery( query, bbox ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool CObstacleShapeComposite::VTestLocation( const Vector3& v ) const
{
	for ( auto it = m_subShapes.Begin(), end = m_subShapes.End(); it != end; ++it )
	{
		CObstacleShape* shape = *it;
		if ( shape->TestBoundings( v ) )
		{
			if ( !shape->VTestLocation( v ) )
			{
				return false;
			}
		}
	}
	return true;
}

Bool CObstacleShapeComposite::VSpatialQuery( CCircleQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CClosestObstacleCircleQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CCollectGeometryInCirceQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CLineQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CWideLineQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CClosestObstacleWideLineQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CClearWideLineInDirectionQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CRectangleQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CPolyTestQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}
Bool CObstacleShapeComposite::VSpatialQuery( CCustomTestQueryData& query, const Vector3* bbox ) const
{
	return TSpatialQuery( query, bbox );
}

void CObstacleShapeComposite::VComputeDetour( CAreaDescription* area, Float personalSpace, ObstacleDetour& outDetour )
{
	for ( auto it = m_subShapes.Begin(), end = m_subShapes.End(); it != end; ++it )
	{
		CObstacleShape* subShape = *it;
		subShape->VComputeDetour( area, personalSpace, outDetour );
		if ( outDetour.Size() == outDetour.Capacity() )
		{
			return;
		}
	}
}

CObstacleShapeComposite* CObstacleShapeComposite::AsComposite()
{
	return this;
}



};			// namespace PathLib

