/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "math.h"

namespace MathUtils
{
	/**
	Solves equation with such a form: a*x^2 + b*x + c = 0. Returns true if solved. Result is stored in x1 and x2.
	*/
	RED_INLINE Bool SolveQuadraticEquation( const Float& a, const Float& b, const Float& c, Float& x1, Float& x2)
	{
		Float determinant = b * b - 4.0f * a * c;
		if ( determinant < 0.0f ) return false;
		if ( a == 0.0f )
		{
			if ( b == 0.0f )
			{
				if ( c == 0.0f )
				{
					x1 = x2 = 0.0f;
					return true;
				}
				else
				{
					return false;
				}
			}

			x1 = x2 = - c / b;
			return true;
		}
		determinant = MSqrt( determinant );
		Float t = 1.0f / ( 2.0f * a );
		x1 = ( - determinant - b ) * t;
		x2 = ( determinant - b ) * t;
		return true;
	}

	Float CyclicDistance( Float a, Float b, Float range = 1.f );

namespace VectorUtils
{
	Float GetAngleRadBetweenVectors( const Vector& from, const Vector& to );

	Float GetAngleDegBetweenVectors( const Vector& from, const Vector& to );

	Float GetAngleRadAroundAxis( const Vector& dirA, const Vector& dirB, const Vector& axis );

	Float GetAngleDegAroundAxis( const Vector& dirA, const Vector& dirB, const Vector& axis );
}

namespace GeometryUtils
{
	// triangle tests (2d & 3d)
	Bool IsPointInsideTriangle( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& point );
	Bool IsPointInsideTriangle_UV( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& point, Float& u, Float& v );
	Bool IsPointInsideTriangle2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point );
	Bool IsPointInsideTriangle2D_UV( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point, Float& u, Float& v );
	Vector GetTriangleNormal( const Vector& p1, const Vector& p2, const Vector& p3 );
	Float TriangleArea2D( const Vector2& p1, const Vector2& p2, const Vector2& p3 );

	// 3d triangle-ray
	Bool TestRayTriangleIntersection3D( const Vector3& point, const Vector3& dir, const Vector3& t0, const Vector& t1, const Vector& t2, Vector3& outPos );

	// 3d triangle-sphere
	Bool TestIntersectionTriAndSphere(	const Vector& start, const Vector& end, const Vector& p1, const Vector& p2, const Vector& p3, const Float radius, Float& distance );

	// 3d triangle-box
	Bool TestIntersectionTriAndBox( const Vector& v0, const Vector& v1, const Vector& v2, const Box& box );

	// 2d triangle-box
	Bool TestIntersectionTriAndRectangle2D( const Vector2& v0, const Vector2& v1, const Vector2& v2, const Vector2& rectMin, const Vector2& rectMax );

	// 3d line-line
	Bool TestIntersectionLineLine3D( const Vector& p1A, const Vector& p2A, const Vector& p1B, const Vector& p2B );
	// returns the shortest line segment between the two lines
	Bool DistanceLineLine3D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Vector& outP1, Vector& outP2 );

	// 3d line-point
	Float DistancePointToLine( const Vector& point, const Vector& lineA, const Vector& lineB, Vector& linePoint );
	Float DistanceSqrPointToLine( const Vector& point, const Vector& lineA, const Vector& lineB );
	Float DistanceSqrPointToLineSeg( const Vector& point, const Vector& lineA, const Vector& lineB, Vector& linePoint );

	// 2d line-point
	Float DistancePointToLine2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB, Vector2& linePoint );
	Float DistanceSqrPointToLine2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB );
	Float DistanceSqrPointToLineSeg2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB, Vector2& linePoint );

	Vector	ProjectPointOnLine( const Vector& point, const Vector& lineA, const Vector& lineB );
	Float	ProjectVecOnEdge( const Vector& vec, const Vector& a, const Vector& b );
	Float	ProjectVecOnEdgeUnclamped( const Vector& vec, const Vector& a, const Vector& b );
	void	GetPointFromEdge( Float p, const Vector& a, const Vector& b, Vector& point );

	// 3d sphere-line
	Bool TestIntersectionSphereLine( const Sphere& sphere, const Vector& pt0, const Vector& pt1, Int32& nbInter, Float& inter1, Float& inter2 );

	// 2d line-line
	Float TestDistanceSqrLineLine2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4);
	void ClosestPointsLineLine2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, Float& ratio1, Float& ratio2 );
	void ClosestPointsLineLine2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4, Vector2& point1Out, Vector2& point2Out );
	Bool TestIntersectionLineLine2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& p4 );
	Bool TestIntersectionLineLine2D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t );
	Bool TestIntersectionLineLine2D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t1, Float& t2 );
	Bool TestIntersectionLineLine2DClamped( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t );
	Bool TestIntersectionLineLine2DClamped( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t1, Float& t2 );

	// 2d line-ray
	Bool TestIntersectionRayLine2D( const Vector& rayDir, const Vector& rayOrigin, const Vector& p1, const Vector& p2, Int32 sX , Int32 sY, Float& t );
	void TestIntersectionRayLine2D( const Vector& rayDir, const Vector& rayOrigin, const Vector& p1, const Vector& p2, Int32 sX , Int32 sY, Float& t1, Float& t2 );

	// 2d line-circle(or point)
	Bool TestIntersectionCircleLine2D( const Vector2& c, Float r, const Vector2& p1, const Vector2& p2);
	void TestClosestPointOnLine2D( const Vector2& c, const Vector2& p1, const Vector2& p2, Vector2& outPoint );
	void TestClosestPointOnLine2D( const Vector2& c, const Vector2& p1, const Vector2& p2, Float& outRatio, Vector2& outPoint );

	struct Circle2D
	{
		Vector2 m_center;
		Float	m_radius;
	};

	// 2d circle-triangle intersection test
	Bool TestIntersectionCircleTriangle2D( const Circle2D& circle, const Vector2& v1, const Vector2& v2, const Vector2& v3 );

	Bool SweepCircleVsCircle2D( const Circle2D& sweptCircle, const Circle2D& otherCircle, const Vector2& sweepVector, Vector2& out );

	enum ENumberOfResults
	{
		RESNUM_Zero = 0,
		RESNUM_One,
		RESNUM_Two,
		RESNUM_Inifnite
	};

	ENumberOfResults FindIntersecionsOfTwo2DCircles( Circle2D firstCircle, Circle2D secondCircle, Vector2& point1, Vector2& point2 );
	//Bool _TryTheCvsCIntersectionTest( const Char* id, const Circle2D& oneCircle, const Circle2D& otherCircle, Vector2& p1, Vector2& p2 ); 
	//Bool _TrySomeOfTheCvsCIntersectionTests();
	//Bool _TrySomeOfTheCvsC2DSweepTests(); // "manual" unit test

	// 2d aabb rectangle
	void ClosestPointLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax, Vector2& pointLineOut, Vector2& pointRectangleOut );
	Float TestDistanceSqrLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax );
	Bool TestIntersectionLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax );
	void ClosestPointToRectangle2D( const Vector2& rectangleMin, const Vector2& rectangleMax, const Vector2& point, Vector2& outClosestPoint );
	Bool TestIntersectionCircleRectangle2D( const Vector2& rectangleMin, const Vector2& rectangleMax, const Vector2& circleCenter, Float radius );

	// 2d polygon
	template < class V >
	Bool IsPolygonConvex2D( const V* polyVertexes, Uint32 vertsCount );

	template < class V >
	Bool IsPointInPolygon2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& point );

	template < class V, class Functor >
	RED_INLINE Bool PolygonTest2D( const V* polyVertexes, Uint32 vertsCount, const Vector2* bboxTest, Functor& functor );

	template < class V >
	Bool TestIntersectionPolygonCircle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );
	template < class V >
	Bool TestEncapsulationPolygonCircle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );
	
	template < class V >
	Bool TestIntersectionPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2 );
	template < class V >
	Bool TestIntersectionPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float radius );

	template < class V >
	Bool TestIntersectionPolygonRectangle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax, Float distance );
	template < class V >
	Bool TestPolygonContainsRectangle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax );

	template < class V >
	Float ClosestPointPolygonPoint2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v, Float maxDist, Vector2& outClosestPointOnPolygon );
	template < class V >
	Float ClosestPointPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float maxDist, Vector2& outClosestPointOnPolygon, Vector2& outClosestPointOnLine );

	template < class V >
	Float GetClockwisePolygonArea2D( const V* polyVertexes, Uint32 vertsCount );

	template < class V, EMemoryClass mc >
	Bool IsPolygonsIntersecting2D( const TDynArray< V, mc >& polyVertexes1, const TDynArray< V, mc >& polyVertexes2 );
	void ComputeConvexHull2D( TDynArray< Vector2 > &input, TDynArray< Vector2 > &output );
	Bool ComputeConcaveHull2D( const TDynArray< Vector2 >& points, TDynArray< Vector2 >& hull, Uint32 k = 3 );
	
	template < class V, EMemoryClass mc >
	Bool IsPolygonConvex2D( const TDynArray< V, mc >& polyVertexes )																	{ return IsPolygonConvex2D( polyVertexes.TypedData(), polyVertexes.Size() ); }
	template < class V, EMemoryClass mc >
	Bool IsPointInPolygon2D( const TDynArray< V, mc >& polyVertexes, const Vector2& point )												{ return IsPointInPolygon2D( polyVertexes.TypedData(), polyVertexes.Size(), point ); }
	template < class V, EMemoryClass mc, class Functor >
	RED_INLINE Bool PolygonTest2D( const TDynArray< V, mc >& polyVertexes, const Vector2* bboxTest, Functor& functor ) 				{ return PolygonTest2D( polyVertexes.TypedData(), polyVertexes.Size(), bboxTest, functor ); }
	template < class V, EMemoryClass mc >
	Bool TestIntersectionPolygonCircle2D( const TDynArray< V, mc >& polyVertexes, const Vector2& circleCenter, Float circleRadius )		{ return TestIntersectionPolygonCircle2D( polyVertexes.TypedData(), polyVertexes.Size(), circleCenter, circleRadius ); }
	template < class V, EMemoryClass mc >
	Bool TestEncapsulationPolygonCircle2D( const TDynArray< V, mc >& polyVertexes, const Vector2& circleCenter, Float circleRadius )	{ return TestEncapsulationPolygonCircle2D( polyVertexes.TypedData(), polyVertexes.Size(), circleCenter, circleRadius ); }
	template < class V, EMemoryClass mc >
	Bool TestIntersectionPolygonLine2D( const TDynArray< V, mc >& polyVertexes, const Vector2& v1, const Vector2& v2 )					{ return TestIntersectionPolygonLine2D( polyVertexes.TypedData(), polyVertexes.Size(), v1, v2 ); }
	template < class V, EMemoryClass mc >
	Bool TestIntersectionPolygonLine2D( const TDynArray< V, mc >& polyVertexes, const Vector2& v1, const Vector2& v2, Float radius )	{ return TestIntersectionPolygonLine2D( polyVertexes.TypedData(), polyVertexes.Size(), v1, v2, radius ); }
	template < class V, EMemoryClass mc >
	Bool TestIntersectionPolygonRectangle2D( const TDynArray< V, mc >& polyVertexes, const Vector2& rectangleMin, const Vector2& rectangleMax, Float distance ) { return TestIntersectionPolygonRectangle2D( polyVertexes.TypedData(), polyVertexes.Size(), rectangleMin, rectangleMax, distance ); }
	template < class V, EMemoryClass mc >
	Bool TestPolygonContainsRectangle2D( const TDynArray< V, mc >& polyVertexes, const Vector2& rectangleMin, const Vector2& rectangleMax ) { return TestPolygonContainsRectangle2D( polyVertexes.TypedData(), polyVertexes.Size(), rectangleMin, rectangleMax );  }
	template < class V, EMemoryClass mc >
	Float ClosestPointPolygonPoint2D( const TDynArray< V, mc >& polyVertexes, const Vector2& v, Float maxDist, Vector2& outClosestPointOnPolygon ) { return ClosestPointPolygonPoint2D( polyVertexes.TypedData(), polyVertexes.Size(), v, maxDist, outClosestPointOnPolygon ); }
	template < class V, EMemoryClass mc >
	Float ClosestPointPolygonLine2D( const TDynArray< V, mc >& polyVertexes, const Vector2& v1, const Vector2& v2, Float maxDist, Vector2& outClosestPointOnPolygon, Vector2& outClosestPointOnLine ) { return ClosestPointPolygonLine2D( polyVertexes.TypedData(), polyVertexes.Size(), v1, v2, maxDist, outClosestPointOnPolygon, outClosestPointOnLine ); }
	template < class V, EMemoryClass mc >
	Float GetClockwisePolygonArea2D( const TDynArray< V, mc >& polyVertexes )															{ return GetClockwisePolygonArea2D( polyVertexes.TypedData(), polyVertexes.Size() ); }

	// 2d helpers
	//PointIsLeftOfSegment2D(): tests if a point is Left|On|Right of an infinite line.
	RED_INLINE Float PointIsLeftOfSegment2D( const Vector2& v1, const Vector2& v2, const Vector2& p ) { return (v2.X - v1.X)*(p.Y - v1.Y) - (p.X - v1.X)*(v2.Y - v1.Y); }

	RED_INLINE Vector2 Rotate2D(const Vector2& v, Float fSin, Float fCos )
	{
		return Vector2( v.X * fCos - v.Y * fSin, v.X * fSin + v.Y * fCos );
	}
	RED_INLINE Vector2 Rotate2D(const Vector2& v, Float radians )
	{
		Float fCos = cosf(radians);
		Float fSin = sinf(radians);
		return Rotate2D( v, fSin, fCos );
	}
	RED_INLINE Vector2 PerpendicularL(const Vector2& v) { return Vector2(-v.Y,v.X); }
	RED_INLINE Vector2 PerpendicularR(const Vector2& v) { return Vector2(v.Y,-v.X); }

	RED_INLINE Float ClampRadians( Float radians )
	{
		ASSERT( Abs( radians ) <= M_PI * 4.f );

		if ( radians > M_PI )
		{
			do { radians -= M_PI*2.f; }
			while( radians > M_PI );
		}
		else if ( radians < M_PI )
		{
			do { radians += M_PI*2.f; }
			while( radians < M_PI );
		}
		return radians;
	}

	RED_INLINE Float ClampDegrees( Float degrees )
	{
		ASSERT( Abs( degrees ) <= 720.f );

		if ( degrees > 180.f )
		{
			do { degrees -= 360.f; }
			while( degrees > 180.f );
		}
		else if ( degrees < -180.f )
		{
			do { degrees += 360.f; }
			while( degrees < -180.f );
		}
		return degrees;
	}

	RED_INLINE Bool RangeOverlap1D(Float a1, Float a2, Float b1, Float b2) { return a1 >= b1 ? a1 <= b2 : a2 >= b1; }
	RED_INLINE Float DistanceRanges1D(Float a1, Float a2, Float b1, Float b2) { return (a1 < b1) ? b1 - a2 : a1 - b2;  }

	// Oriented box
	// Matrix based obb is given by matrix and implicit [-1,-1,-1] - [1,1,1] box.
	Float OrientedBoxSquaredDistance( const Matrix &obbLocalToWorld, const Vector &point );
}

//////////////////////////////////////////////////////////////////////////

namespace InterpolationUtils
{
	Float Interpolate( Float from, Float to, Float t );

	Vector HermiteInterpolate( const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, Float t );
	Vector HermiteInterpolateWithTangents( const Vector & v1, const Vector & t1, const Vector & v2, const Vector & t2, Float t );

	RED_INLINE Float Hermite1D( const Float& t, const Float& p0, const Float& t0, const Float& t1, const Float& p1 )
	{
		ASSERT( t >= 0 && t <= 1.0f );
		Float h1 =  2.0f*t*t*t - 3.0f*t*t + 1.0f;
		Float h2 = -2.0f*t*t*t + 3.0f*t*t;
		Float h3 =  1.0f*t*t*t - 2.0f*t*t + t;
		Float h4 =  1.0f*t*t*t - 1.0f*t*t;
		return h1*p0 + h3*t0 + h4*t1 + h2*p1;
	}
	Vector CubicInterpolate( const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, Float t );

	void CatmullRomBuildTauMatrix( Matrix& outMat, Float tau = 0.5f );
	Vector CatmullRomInterpolate( const Vector &v1, const Vector &v2, const Vector &v3, const Vector &v4, Float t, const Matrix& tauMatrix );

	Float HermiteClosestPoint(  const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, const Vector & p, Vector &res, Float epsilon = 0.001f );
	Float CubicClosestPoint(  const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, const Vector & p, Vector &res, Float epsilon = 0.001f );

	void DistanceToEdge( const Vector &a, const Vector &b, const Vector &pt, Float &dist, Float &alpha );
	void InterpolatePoints( const TDynArray< Vector > &keyPoints, TDynArray< Vector > &outPoints, Float pointsDist, Bool closed );
}

//////////////////////////////////////////////////////////////////////////

template < class V, class Functor >
RED_INLINE Bool GeometryUtils::PolygonTest2D( const V* polyVertexes, Uint32 vertsCount, const Vector2* bboxTest, Functor& functor )
{
	Uint32 lastIt = vertsCount - 1;
	for ( Uint32 it = 0; it != vertsCount; it++ )
	{
		const Vector2& vertCurr = polyVertexes[it].AsVector2();
		const Vector2& vertPrev = polyVertexes[lastIt].AsVector2();
		lastIt = it;

		Vector2 bbox[2];
		bbox[0].X = Min(vertCurr.X,vertPrev.X);
		bbox[1].X = Max(vertCurr.X,vertPrev.X);
		bbox[0].Y = Min(vertCurr.Y,vertPrev.Y);
		bbox[1].Y = Max(vertCurr.Y,vertPrev.Y);
		if (RangeOverlap1D(bboxTest[0].X,bboxTest[1].X,bbox[0].X,bbox[1].X) &&
			RangeOverlap1D(bboxTest[0].Y,bboxTest[1].Y,bbox[0].Y,bbox[1].Y))
		{
			if ( functor( vertCurr, vertPrev ) )
			{
				return true;
			}
		}
	}
	return false;
}


} // Math Utils
