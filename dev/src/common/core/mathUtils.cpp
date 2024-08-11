/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mathUtils.h"

namespace MathUtils
{
	Float CyclicDistance( Float a, Float b, Float range /*= 1.f */ )
	{
		const Float delta = b - a;

		// Get shortest distance
		if ( delta < -range )
		{
			return delta + range * 2.f;
		}
		else if ( delta > range )
		{
			return delta - range * 2.f;
		}
		else
		{
			return delta;
		}
	}

	namespace VectorUtils
	{
		Float GetAngleRadBetweenVectors( const Vector& from, const Vector& to )
		{
			return acosf( Clamp( Vector::Dot3( from.Normalized3(), to.Normalized3() ), -1.f, 1.f ) );
		}

		Float GetAngleDegBetweenVectors( const Vector& from, const Vector& to )
		{
			return RAD2DEG( GetAngleRadBetweenVectors( from, to ) );
		}

		Float GetAngleRadAroundAxis( const Vector& dirA, const Vector& dirB, const Vector& axis ) 
		{
			Vector _dirA = dirA - Vector::Project( dirA, axis );
			Vector _dirB = dirB - Vector::Project( dirB, axis );

			Float angle = GetAngleRadBetweenVectors( _dirA, _dirB );

			return angle * ( Vector::Dot3( axis, Vector::Cross( _dirA, _dirB ) ) < 0 ? -1 : 1 );
		}

		Float GetAngleDegAroundAxis( const Vector& dirA, const Vector& dirB, const Vector& axis ) 
		{
			return RAD2DEG( GetAngleRadAroundAxis( dirA, dirB, axis ) );
		}
	}

	namespace GeometryUtils
	{

		Bool IsPointInsideTriangle( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& point )
		{
			const Vector u = p2 - p1;
			const Vector v = p3 - p1;
			const Vector w = point - p1;

			const Float uu = u.Dot3( u );
			const Float uv = u.Dot3( v );
			const Float vv = v.Dot3( v );
			const Float wu = w.Dot3( u );
			const Float wv = w.Dot3( v );
			const Float d = uv * uv - uu * vv;

			const Float invD = 1.f / d;
			const Float s = ( uv * wv - vv * wu ) * invD;

			if ( s < 0.f || s > 1.f )
			{
				return false;
			}

			const Float t = ( uv * wu - uu * wv ) * invD;

			if ( t < 0.f || ( s + t ) > 1.f )
			{
				return false;
			}

			return true;
		}

		Bool IsPointInsideTriangle_UV( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& point, Float& s, Float& t )
		{
			const Vector u = p2 - p1;
			const Vector v = p3 - p1;
			const Vector w = point - p1;

			const Float uu = u.Dot3( u );
			const Float uv = u.Dot3( v );
			const Float vv = v.Dot3( v );
			const Float wu = w.Dot3( u );
			const Float wv = w.Dot3( v );
			const Float d = uv * uv - uu * vv;

			Float invD = 1.f / d;

			s = ( uv * wv - vv * wu ) * invD;
			t = ( uv * wu - uu * wv ) * invD;

			if ( s < 0.f || s > 1.f || t < 0.f || ( s + t ) > 1.f )
			{
				return false;
			}

			return true;
		}

		Bool IsPointInsideTriangle2D_UV( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point, Float& s, Float& t )
		{
			const Vector2 u = p2 - p1;
			const Vector2 v = p3 - p1;
			const Vector2 w = point - p1;

			const Float uu = u.Dot( u );
			const Float uv = u.Dot( v );
			const Float vv = v.Dot( v );
			const Float wu = w.Dot( u );
			const Float wv = w.Dot( v );
			const Float d = uv * uv - uu * vv;

			Float invD = 1.f / d;

			s = ( uv * wv - vv * wu ) * invD;
			t = ( uv * wu - uu * wv ) * invD;

			if ( s < 0.f || s > 1.f || t < 0.f || ( s + t ) > 1.f )
			{
				return false;
			}

			return true;
		}

		Bool IsPointInsideTriangle2D( const Vector2& p1, const Vector2& p2, const Vector2& p3, const Vector2& point )
		{
			Float d0 = (p2.X-p1.X) * (point.Y-p1.Y) - (p2.Y-p1.Y) * (point.X-p1.X);
			if (d0 >= 0.f)
			{
				Float d1 = (p3.X-p2.X) * (point.Y-p2.Y) - (p3.Y-p2.Y) * (point.X-p2.X);
				if (d1 >= 0.f)
				{
					Float d2 = (p1.X-p3.X) * (point.Y-p3.Y) - (p1.Y-p3.Y) * (point.X-p3.X);
					return d2 >= 0.f;
				}
			}
			else
			{
				Float d1 = (p3.X-p2.X) * (point.Y-p2.Y) - (p3.Y-p2.Y) * (point.X-p2.X);
				if (d1 <= 0.f)
				{
					Float d2 = (p1.X-p3.X) * (point.Y-p3.Y) - (p1.Y-p3.Y) * (point.X-p3.X);
					return d2 <= 0.f;
				}
			}
			return false;
		}

		Float TriangleArea2D( const Vector2& p1, const Vector2& p2, const Vector2& p3 )
		{
			Vector2 edge1 = p2 - p1;
			Vector2 edge2 = p3 - p1;
			return 0.5f * edge1.CrossZ( edge2 );
		}

		Vector GetTriangleNormal( const Vector& p1, const Vector& p2, const Vector& p3 )
		{
			Vector v1 = p2 - p1;
			Vector v2 = p3 - p1;

			Vector n = Vector::Cross( v1, v2 );

			return n.Normalized3();
		}
		void ClosestPointsLineLine2D( const Vector2& a1, const Vector2& a2, const Vector2& b1, const Vector2& b2, Float& ratio1, Float& ratio2 )
		{
			Vector2 u = a2 - a1;
			Vector2 v = b2 - b1;
			Vector2 w = a1 - b1;
			Float a = u.SquareMag(); // u.Dot( u )
			Float b = u.Dot(v);
			Float c = v.SquareMag(); // v.Dot( v )
			Float D = a*c - b*b; // always >= 0

			// compute the line parameters of the two closest points
			if ( D < NumericLimits< Float >::Epsilon() )		 // the lines are almost parallel
			{
				if ( c < NumericLimits< Float >::Epsilon() )
				{
					if ( a < NumericLimits< Float >::Epsilon() )
					{
						ratio1 = 1.f;
						ratio2 = 1.f;
						return;
					}
					// actually thats viable case. one of lines is almost nullified
					Vector2 diff = b2 - a1;
					Float t = diff.Dot( u ) / a;

					ratio1 = Clamp( t, 0.f, 1.f );
					ratio2 = 1.f;
					return;
				}
				else if ( a < NumericLimits< Float >::Epsilon() )
				{
					Vector2 diff = a2 - b1;
					Float t = diff.Dot( v ) / c;

					ratio1 = 1.f;
					ratio2 = Clamp( t, 0.f, 1.f );
					return;
				}

				Vector2 q = a2 - b1;

				Float dotA1B = w.Dot( v );
				Float dotA2B = q.Dot( v );

				Bool bNeg = dotA1B > dotA2B;

				Float dotMin = bNeg ? dotA2B : dotA1B;
				Float dotMax = bNeg ? dotA1B : dotA2B;

				// range overlap if-ladder
				if ( dotMax <= 0.f )			// no-overlap
				{
					ratio1 = bNeg ? 0.f : 1.f;
					ratio2 = 0.f;
				}
				else if ( dotMin >= c )			// no overlap
				{
					ratio1 = bNeg ? 1.f : 0.f;
					ratio2 = 1.f;
				}
				else
				{
					// there is an overlap
					if ( dotMin >= 0.f )
					{
						ratio1 = bNeg ? 1.f : 0.f;
						ratio2 = dotMin / c;
					}
					else
					{
						ratio1 = dotMin / (dotMin - dotMax);
						if ( bNeg )
						{
							ratio1 = 1.f - ratio1;
						}
						ratio2 = 0.f;
					}
				}

				return;
			}

			Float d = u.Dot(w);
			Float e = v.Dot(w);
			Float sN = b*e - c*d;
			Float sD = D; // sc = sN / sD, default sD = D >= 0
			Float tN = a*e - b*d;
			Float tD = D; // tc = tN / tD, default tD = D >= 0

			if (sN < 0.0) { // sc < 0 => the s=0 edge is visible
				sN = 0.0;
				tN = e;
				tD = c;
			}
			else if (sN > sD) { // sc > 1 => the s=1 edge is visible
				sN = sD;
				tN = e + b;
				tD = c;
			}

			if (tN < 0.0) { // tc < 0 => the t=0 edge is visible
				tN = 0.0;
				// recompute sc for this edge
				if (-d < 0.0)
					sN = 0.0;
				else if (-d > a)
					sN = sD;
				else {
					sN = -d;
					sD = a;
				}
			}
			else if (tN > tD) { // tc > 1 => the t=1 edge is visible
				tN = tD;
				// recompute sc for this edge
				if ((-d + b) < 0.0)
					sN = 0;
				else if ((-d + b) > a)
					sN = sD;
				else {
					sN = (-d + b);
					sD = a;
				}
			}
			// finally do the division to get sc and tc
			ratio1 = (abs(sN) < NumericLimits< Float >::Epsilon() ? 0.0f : sN / sD);
			ratio2 = (abs(tN) < NumericLimits< Float >::Epsilon() ? 0.0f : tN / tD);
		}
		Float TestDistanceSqrLineLine2D( const Vector2& a1, const Vector2& a2, const Vector2& b1, const Vector2& b2 )
		{
			Vector2 u = a2 - a1;
			Vector2 v = b2 - b1;
			Vector2 w = a1 - b1;
			Float a = u.SquareMag(); // u.Dot( u )
			Float b = u.Dot(v);
			Float c = v.SquareMag(); // v.Dot( v )
			Float D = a*c - b*b; // always >= 0

			// compute the line parameters of the two closest points
			if ( D < NumericLimits< Float >::Epsilon() )		 // the lines are almost parallel
			{
				if ( c < NumericLimits< Float >::Epsilon() )
				{
					if ( a < NumericLimits< Float >::Epsilon() )
					{
						return ( b2 - a2 ).SquareMag();
					}
					// actually thats viable case. one of lines is almost nullified
					Vector2 diff = b2 - a1;
					Float t = diff.Dot( u ) / a;

					t = Clamp( t, 0.1f, 1.f );
					Vector2 dist = b2 - a1 - u * t;
					return (diff - u * t).SquareMag();
				}
				else if ( a < NumericLimits< Float >::Epsilon() )
				{
					Vector2 diff = a2 - b1;
					Float t = diff.Dot( v ) / c;
					
					t = Clamp( t, 0.1f, 1.f );
					return (diff - v * t).SquareMag();
				}

				Vector2 q = a2 - b1;

				Float dotA1B = w.Dot( v );
				Float dotA2B = q.Dot( v );

				Bool bNeg = dotA1B > dotA2B;

				Float dotMin = bNeg ? dotA2B : dotA1B;
				Float dotMax = bNeg ? dotA1B : dotA2B;

				// range overlap if-ladder
				if ( dotMax <= 0.f )			// no-overlap
				{
					return
						( b1
						- ( bNeg ? a1 : a2 )
						).SquareMag();
				}
				else if ( dotMin >= c )			// no overlap
				{
					return
						( b2
						- ( bNeg ? a2 : a1 )
						).SquareMag();
				}
				else							// overlap
				{
					Vector2 perpedicular = PerpendicularL( u );
					Float dot = perpedicular.Dot( q );
					return dot * dot / a;
				}
			}

			Float d = u.Dot(w);
			Float e = v.Dot(w);
			Float sN = b*e - c*d;
			Float sD = D; // sc = sN / sD, default sD = D >= 0
			Float tN = a*e - b*d;
			Float tD = D; // tc = tN / tD, default tD = D >= 0

			if (sN < 0.0) { // sc < 0 => the s=0 edge is visible
				sN = 0.0;
				tN = e;
				tD = c;
			}
			else if (sN > sD) { // sc > 1 => the s=1 edge is visible
				sN = sD;
				tN = e + b;
				tD = c;
			}

			if (tN < 0.0) { // tc < 0 => the t=0 edge is visible
				tN = 0.0;
				// recompute sc for this edge
				if (-d < 0.0)
					sN = 0.0;
				else if (-d > a)
					sN = sD;
				else {
					sN = -d;
					sD = a;
				}
			}
			else if (tN > tD) { // tc > 1 => the t=1 edge is visible
				tN = tD;
				// recompute sc for this edge
				if ((-d + b) < 0.0)
					sN = 0;
				else if ((-d + b) > a)
					sN = sD;
				else {
					sN = (-d + b);
					sD = a;
				}
			}
			// finally do the division to get sc and tc
			Float sc = (abs(sN) < NumericLimits< Float >::Epsilon() ? 0.0f : sN / sD);
			Float tc = (abs(tN) < NumericLimits< Float >::Epsilon() ? 0.0f : tN / tD);

			// get the difference of the two closest points
			Vector2 dP = w + (u * sc) - (v * tc); // = S1(sc) - S2(tc)

			return dP.SquareMag(); // return the closest distance
		}

		void ClosestPointsLineLine2D( const Vector2& a1, const Vector2& a2, const Vector2& b1, const Vector2& b2, Vector2& aOut, Vector2& bOut )
		{
			Vector2 u = a2 - a1;
			Vector2 v = b2 - b1;
			Vector2 w = a1 - b1;
			Float a = u.SquareMag(); // u.Dot( u )
			Float b = u.Dot(v);
			Float c = v.SquareMag(); // v.Dot( v )
			Float D = a*c - b*b; // always >= 0

			// compute the line parameters of the two closest points
			if ( D < NumericLimits< Float >::Epsilon() )		 // the lines are almost parallel
			{
				if ( c < NumericLimits< Float >::Epsilon() )
				{
					if ( a < NumericLimits< Float >::Epsilon() )
					{
						aOut = a2;
						bOut = b2;
						return;
					}
					// actually thats viable case. one of lines is almost nullified
					Vector2 diff = b2 - a1;
					Float t = diff.Dot( u ) / a;
					t = Clamp( t, 0.f, 1.f );
					aOut = a1 + u * t;
					bOut = b2;
					return;
				}
				else if ( a < NumericLimits< Float >::Epsilon() )
				{
					Vector2 diff = a2 - b1;
					Float t = diff.Dot( v ) / c;
					t = Clamp( t, 0.f, 1.f );
					aOut = a2;
					bOut = b1 + v * t;
					return;
				}

				Vector2 q = a2 - b1;

				Float dotA1B = w.Dot( v );
				Float dotA2B = q.Dot( v );

				Bool bNeg = dotA1B > dotA2B;

				Float dotMin = bNeg ? dotA2B : dotA1B;
				Float dotMax = bNeg ? dotA1B : dotA2B;

				Float ratio1, ratio2;

				// range overlap if-ladder
				if ( dotMax <= 0.f )			// no-overlap
				{
					ratio1 = bNeg ? 0.f : 1.f;
					ratio2 = 0.f;
				}
				else if ( dotMin >= c )			// no overlap
				{
					ratio1 = bNeg ? 1.f : 0.f;
					ratio2 = 1.f;
				}
				else
				{
					// there is an overlap
					if ( dotMin >= 0.f )
					{
						ratio1 = bNeg ? 1.f : 0.f;
						ratio2 = dotMin / c;
					}
					else
					{
						ratio1 = dotMin / (dotMin - dotMax);
						if ( bNeg )
						{
							ratio1 = 1.f - ratio1;
						}
						ratio2 = 0.f;
					}
				}

				aOut = a1 + (u * ratio1);
				bOut = b1 + (v * ratio2);

				return;
			}

			Float d = u.Dot(w);
			Float e = v.Dot(w);
			Float sN = b*e - c*d;
			Float sD = D; // sc = sN / sD, default sD = D >= 0
			Float tN = a*e - b*d;
			Float tD = D; // tc = tN / tD, default tD = D >= 0

			if (sN < 0.0) { // sc < 0 => the s=0 edge is visible
				sN = 0.0;
				tN = e;
				tD = c;
			}
			else if (sN > sD) { // sc > 1 => the s=1 edge is visible
				sN = sD;
				tN = e + b;
				tD = c;
			}

			if (tN < 0.0) { // tc < 0 => the t=0 edge is visible
				tN = 0.0;
				// recompute sc for this edge
				if (-d < 0.0)
					sN = 0.0;
				else if (-d > a)
					sN = sD;
				else {
					sN = -d;
					sD = a;
				}
			}
			else if (tN > tD) { // tc > 1 => the t=1 edge is visible
				tN = tD;
				// recompute sc for this edge
				if ((-d + b) < 0.0)
					sN = 0;
				else if ((-d + b) > a)
					sN = sD;
				else {
					sN = (-d + b);
					sD = a;
				}
			}
			// finally do the division to get sc and tc
			Float ratio1 = (abs(sN) < NumericLimits< Float >::Epsilon() ? 0.0f : sN / sD);
			Float ratio2 = (abs(tN) < NumericLimits< Float >::Epsilon() ? 0.0f : tN / tD);

			aOut = a1 + (u * ratio1);
			bOut = b1 + (v * ratio2);
		}
		
		Bool TestIntersectionLineLine2D( const Vector2& a1, const Vector2& a2, const Vector2& b1, const Vector2& b2 )
		{
			Float a1yb1y, a1xb1x, a2xa1x, a2ya1y, b2xb1x, b2yb1y;
			Float crossa, crossb, denominator;

			//----------------------------------------------------------------------

			a1yb1y = a1.Y-b1.Y;
			a1xb1x = a1.X-b1.X;
			a2xa1x = a2.X-a1.X;
			a2ya1y = a2.Y-a1.Y;
			b2xb1x = b2.X-b1.X;
			b2yb1y = b2.Y-b1.Y;

			//----------------------------------------------------------------------

			crossa = a1yb1y * b2xb1x - a1xb1x * b2yb1y;
			denominator = a2xa1x * b2yb1y - a2ya1y * b2xb1x;

			//----------------------------------------------------------------------

			if ( denominator == 0 )
			{
				return false;			// Parallel lines
			}
			else if ( fabs( crossa ) > fabs( denominator ) || crossa * denominator < 0.0f )
			{
				return false;
			}
			else
			{
				crossb = a1yb1y * a2xa1x - a1xb1x * a2ya1y;

				if ( fabs( crossb ) > fabs( denominator ) || crossb * denominator< 0.0f )
				{
					return false;
				}
			}

			//----------------------------------------------------------------------
			// Optional - intersection point calculation
			//Float fRatio = crossa/denominator;

			//intersectionX = a1.x + fRatio * (a2.x - a1.x);
			//intersectionY = a1.y + fRatio * (a2.y - a1.y);

			return true;
		}
		Bool TestIntersectionLineLine2D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t )
		{
			Vector d1 = p2 - p1;
			Vector d2 = p3 - p4;

			Float dn = d2.A[ sY ] * d1.A[ sX ] - d2.A[ sX ] * d1.A[ sY ];
			if ( !dn )
			{
				return false;
			}

			t = d2.A[ sX ] * ( p1.A[ sY ] - p3.A[ sY ] ) - d2.A[ sY ] * ( p1.A[ sX ] - p3.A[ sX ] );
			t /= dn;

			return true;
		}

		Bool TestIntersectionLineLine2D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t1, Float& t2 )
		{
			Vector d1 = p2 - p1;
			Vector d2 = p3 - p4;

			Float dn = d2.A[ sY ] * d1.A[ sX ] - d2.A[ sX ] * d1.A[ sY ];
			if ( !dn )
			{
				return false;
			}

			t1 = d2.A[ sX ] * ( p1.A[ sY ] - p3.A[ sY ] ) - d2.A[ sY ] * ( p1.A[ sX ] - p3.A[ sX ] );
			t1 /= dn;

			t2 = d1.A[ sX ] * ( p2.A[ sY ] - p4.A[ sY ] ) - d1.A[ sY ] * ( p2.A[ sX ] - p4.A[ sX ] );
			t2 /= dn;

			return true;
		}

		// inspired by voronoi regions. Basically we want to simplify this test to line-line based on what voronoi region p1 is in.
		//      I   I
		//    --rrrrr--
		//      r   r
		//      r   r
		//    --rrrrr--
		//      I   I
		void ClosestPointLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax, Vector2& pointLineOut, Vector2& pointRectangleOut )
		{
			auto funCornerTest =
				[ &p1, &p2, &pointLineOut, &pointRectangleOut ] ( const Vector2& cornerPoint, const Vector2& leftPoint, const Vector2& rightPoint )
			{
				Vector2 diff = p2 - p1;
				Vector2 p1ToCorner = cornerPoint - p1;
				Float dot = diff.Dot( p1ToCorner );
				if ( dot < 0.f )
				{

					pointRectangleOut = cornerPoint;
					TestClosestPointOnLine2D( cornerPoint, p1, p2, pointLineOut );
				}
				else
				{
					Float crossZ = diff.CrossZ( p1ToCorner );
					const Vector2& otherPoint = crossZ < 0.f ? rightPoint : leftPoint;
					ClosestPointsLineLine2D( p1, p2, cornerPoint, otherPoint, pointLineOut, pointRectangleOut );
				}
			};
			// if-ladder
			if ( p1.Y > rectangleMax.Y )
			{
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					funCornerTest( rectangleMax, Vector2( rectangleMin.X, rectangleMax.Y ), Vector2( rectangleMax.X, rectangleMin.Y ) );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					funCornerTest( Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin, rectangleMax );
				}
				else
				{
					// y-top edge
					ClosestPointsLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMax, pointLineOut, pointRectangleOut );
				}
			}
			else if ( p1.Y < rectangleMin.Y )
			{
				// symetric to above test
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					funCornerTest( Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax, rectangleMin );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					funCornerTest( rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ), Vector2( rectangleMin.X, rectangleMax.Y ) );
				}
				else
				{
					// y-bottom edge
					ClosestPointsLineLine2D( p1, p2, rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ), pointLineOut, pointRectangleOut );
				}
			}
			else if ( p1.X > rectangleMax.X )
			{
				ClosestPointsLineLine2D( p1, p2, Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax, pointLineOut, pointRectangleOut );
			}
			else if ( p1.X < rectangleMin.X )
			{
				ClosestPointsLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin, pointLineOut, pointRectangleOut );
			}
			else
			{
				pointLineOut = p1;
				pointRectangleOut = p1;
			}
		}

		Float TestDistanceSqrLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax )
		{
			auto funCornerTest =
				[ &p1, &p2 ]( const Vector2& cornerPoint, const Vector2& leftPoint, const Vector2& rightPoint ) -> Float
			{
				Vector2 diff = p2 - p1;
				Vector2 p1ToCorner = cornerPoint - p1;
				Float dot = diff.Dot( p1ToCorner );
				if ( dot < 0.f )
				{
					return p1ToCorner.SquareMag();
				}
				Float crossZ = diff.CrossZ( p1ToCorner );
				const Vector2& otherPoint = crossZ < 0.f ? rightPoint : leftPoint;
				return TestDistanceSqrLineLine2D( p1, p2, cornerPoint, otherPoint );
			};
			// if-ladder
			if ( p1.Y > rectangleMax.Y )
			{
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					return funCornerTest( rectangleMax, Vector2( rectangleMin.X, rectangleMax.Y ), Vector2( rectangleMax.X, rectangleMin.Y ) );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					return funCornerTest( Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin, rectangleMax );
				}
				else
				{
					// y-top edge
					return TestDistanceSqrLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMax );
				}
			}
			else if ( p1.Y < rectangleMin.Y )
			{
				// symetric to above test
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					return funCornerTest( Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax, rectangleMin );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					return funCornerTest( rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ), Vector2( rectangleMin.X, rectangleMax.Y ) );
				}
				else
				{
					// y-bottom edge
					return TestDistanceSqrLineLine2D( p1, p2, rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ) );
				}
			}
			else if ( p1.X > rectangleMax.X )
			{
				return TestDistanceSqrLineLine2D( p1, p2, Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax );
			}
			else if ( p1.X < rectangleMin.X )
			{
				return TestDistanceSqrLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin );
			}

			// p1 is inside rectangle
			return 0.f;
		}

		// simple implementation - far from optimal
		Bool TestIntersectionLineRectangle2D( const Vector2& p1, const Vector2& p2, const Vector2& rectangleMin, const Vector2& rectangleMax )
		{
			auto funCornerTest =
				[ &p1, &p2 ]( const Vector2& cornerPoint, const Vector2& leftPoint, const Vector2& rightPoint ) -> Bool
			{
				Vector2 diff = p2 - p1;
				Vector2 p1ToCorner = cornerPoint - p1;
				Float dot = diff.Dot( p1ToCorner );
				if ( dot < 0.f )
				{
					return false;
				}
				Float crossZ = diff.CrossZ( p1ToCorner );
				const Vector2& otherPoint = crossZ < 0.f ? rightPoint : leftPoint;
				return TestIntersectionLineLine2D( p1, p2, cornerPoint, otherPoint );
			};
			// if-ladder
			if ( p1.Y > rectangleMax.Y )
			{
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					return funCornerTest( rectangleMax, Vector2( rectangleMin.X, rectangleMax.Y ), Vector2( rectangleMax.X, rectangleMin.Y ) );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					return funCornerTest( Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin, rectangleMax );
				}
				else
				{
					if ( p2.Y > rectangleMax.Y )
					{
						return false;
					}
					// y-top edge
					return TestIntersectionLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMax );
				}
			}
			else if ( p1.Y < rectangleMin.Y )
			{
				// symetric to above test
				if ( p1.X > rectangleMax.X )
				{
					// corner area
					return funCornerTest( Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax, rectangleMin );
				}
				else if ( p1.X < rectangleMin.X )
				{
					// corner area
					return funCornerTest( rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ), Vector2( rectangleMin.X, rectangleMax.Y ) );
				}
				else
				{
					if ( p2.Y < rectangleMin.Y )
					{
						return false;
					}
					// y-bottom edge
					return TestIntersectionLineLine2D( p1, p2, rectangleMin, Vector2( rectangleMax.X, rectangleMin.Y ) );
				}
			}
			else if ( p1.X > rectangleMax.X )
			{
				if ( p2.X > rectangleMax.X )
				{
					return false;
				}
				return TestIntersectionLineLine2D( p1, p2, Vector2( rectangleMax.X, rectangleMin.Y ), rectangleMax );
			}
			else if ( p1.X < rectangleMin.X )
			{
				if ( p2.X < rectangleMin.X )
				{
					return false;
				}
				return TestIntersectionLineLine2D( p1, p2, Vector2( rectangleMin.X, rectangleMax.Y ), rectangleMin );
			}

			// p1 is inside rectangle
			return true;
		}

		void ClosestPointToRectangle2D( const Vector2& rectangleMin, const Vector2& rectangleMax, const Vector2& point, Vector2& outClosestPoint )
		{
			// if-ladder that determines voronoi region. In each region problem is trivial.
			if ( point.Y > rectangleMax.Y )
			{
				// point is above rectangle
				if ( point.X > rectangleMax.X )
				{
					outClosestPoint = rectangleMax;
				}
				else if ( point.X < rectangleMin.X )
				{
					outClosestPoint = Vector2( rectangleMin.X, rectangleMax.Y );
				}
				else
				{
					outClosestPoint = Vector2( point.X, rectangleMax.Y );
				}
			}
			else if ( point.Y < rectangleMin.Y )
			{
				// symetric to above test
				if ( point.X > rectangleMax.X )
				{
					outClosestPoint = Vector2(rectangleMax.X,rectangleMin.Y);
				}
				else if ( point.X < rectangleMin.X )
				{
					outClosestPoint = rectangleMin;
				}
				else
				{
					outClosestPoint = Vector2( point.X, rectangleMin.Y );
				}
			}
			else if ( point.X > rectangleMax.X )
			{
				outClosestPoint = Vector2( rectangleMax.X, point.Y );
			}
			else if ( point.X < rectangleMin.X )
			{
				outClosestPoint = Vector2( rectangleMin.X, point.Y );
			}
			else
			{
				// point inside rectangle
				outClosestPoint = point;
			}
		}

		// inspired by sat theory and voronoi regions
		Bool TestIntersectionCircleRectangle2D( const Vector2& rectangleMin, const Vector2& rectangleMax, const Vector2& circleCenter, Float radius )
		{
			// if-ladder
			if ( circleCenter.Y > rectangleMax.Y )
			{
				// circle is above rectangle
				if ( circleCenter.Y > rectangleMax.Y + radius )
				{
					return false;
				}
				if ( circleCenter.X > rectangleMax.X )
				{
					// point distance test
					Float squareDist = (rectangleMax - circleCenter).SquareMag();
					return squareDist < radius*radius;
				}
				else if ( circleCenter.X < rectangleMin.X )
				{
					// point distance test
					Float squareDist = (Vector2(rectangleMin.X,rectangleMax.Y) - circleCenter).SquareMag();
					return squareDist < radius*radius;
				}
				else
				{
					// intersection
					return true;
				}
			}
			else if ( circleCenter.Y < rectangleMin.Y )
			{
				// symetric to above test
				// circle is above rectangle
				if ( circleCenter.Y < rectangleMin.Y - radius )
				{
					return false;
				}
				if ( circleCenter.X > rectangleMax.X )
				{
					// point distance test
					Float squareDist = (Vector2(rectangleMax.X,rectangleMin.Y) - circleCenter).SquareMag();
					return squareDist < radius*radius;
				}
				else if ( circleCenter.X < rectangleMin.X )
				{
					// point distance test
					Float squareDist = (rectangleMin - circleCenter).SquareMag();
					return squareDist < radius*radius;
				}
				else
				{
					// intersection
					return true;
				}
			}
			else if ( circleCenter.X > rectangleMax.X )
			{
				if ( circleCenter.X > rectangleMax.X + radius )
				{
					return false;
				}
				// intersection
				return true;
			}
			else if ( circleCenter.X < rectangleMin.X )
			{
				if ( circleCenter.X < rectangleMin.X - radius )
				{
					return false;
				}
				return true;
			}

			// circle inside rectangle
			return true;
		}

		Bool TestIntersectionLineLine2DClamped( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t )
		{
			TestIntersectionLineLine2D( p1, p2, p3, p4, sX, sY, t );
			return t <= 1.f && t >= 0.f ? true : false;
		}

		Bool TestIntersectionLineLine2DClamped( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Int32 sX , Int32 sY, Float& t1, Float& t2 )
		{
			TestIntersectionLineLine2D( p1, p2, p3, p4, sX, sY, t1, t2 );
			return t1 <= 1.f && t1 >= 0.f && t2 <= 1.f && t2 >= 0.f ? true : false;
		}

		Vector ProjectPointOnLine( const Vector& point, const Vector& lineA, const Vector& lineB )
		{
			Vector u = lineB - lineA;
			Vector v = point - lineA;

			u.Normalize3();
			return lineA + u * v.Dot3(u);
		}

		Float ProjectVecOnEdge( const Vector& vec, const Vector& a, const Vector& b )
		{
			Vector d = b - a;
			const Float len = d.Normalize3();

			const Vector v( vec.X - a.X, vec.Y - a.Y, vec.Z - a.Z );

			Float proj = Clamp( Vector::Dot3( v, d ) / len, 0.f, 1.f );
			return proj;
		}

		Float ProjectVecOnEdgeUnclamped( const Vector& vec, const Vector& a, const Vector& b )
		{
			Vector d = b - a;
			const Float len = d.Normalize3();

			const Vector v( vec.X - a.X, vec.Y - a.Y, vec.Z - a.Z );

			Float proj = Vector::Dot3( v, d ) / len;
			return proj;
		}

		void GetPointFromEdge( Float p, const Vector& a, const Vector& b, Vector& point )
		{
			point = b * p + a * ( 1 - p );
		}

		Float DistancePointToLine( const Vector& point, const Vector& lineA, const Vector& lineB, Vector& linePoint )
		{
			Vector v = point - lineA;
			Vector s = lineB - lineA;

			Float lenSq = s.SquareMag3();
			Float dot = v.Dot3( s ) / lenSq;
			Vector disp = s * dot;

			linePoint = lineA + disp;
			v -= disp;

			return v.Mag3();
		}

		Float DistanceSqrPointToLine( const Vector& point, const Vector& lineA, const Vector& lineB )
		{
			Vector v = point - lineA;
			Vector s = lineB - lineA;

			Float lenSq = s.SquareMag3();
			Float dot = v.Dot3( s ) / lenSq;
			Vector disp = s * dot;

			v -= disp;

			return v.SquareMag3();
		}

		Float DistanceSqrPointToLineSeg( const Vector& point, const Vector& lineA, const Vector& lineB, Vector& linePoint )
		{
			const Vector v = point - lineA;
			const Vector s = lineB - lineA;

			const Float dot = Clamp( v.Dot3( s ) / s.Dot3( s ), 0.f, 1.f );

			linePoint = Vector::Interpolate( lineA, lineB, dot );

			return linePoint.DistanceSquaredTo( point );
		}

		Float DistancePointToLine2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB, Vector2& linePoint )
		{
			Vector2 v = point - lineA;
			const Vector2 s = lineB - lineA;

			const Float lenSq = s.SquareMag();
			const Float dot = v.Dot( s ) / lenSq;
			const Vector2 disp = s * dot;

			linePoint = lineA + disp;
			v -= disp;

			return v.Mag();
		}

		Float DistanceSqrPointToLine2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB )
		{
			Vector2 v = point - lineA;
			const Vector2 s = lineB - lineA;

			const Float lenSq = s.SquareMag();
			const Float dot = v.Dot( s ) / lenSq;
			const Vector2 disp = s * dot;

			v -= disp;

			return v.SquareMag();
		}

		Float DistanceSqrPointToLineSeg2D( const Vector2& point, const Vector2& lineA, const Vector2& lineB, Vector2& lineSegPoint )
		{
			Vector2 v = point - lineA;
			const Vector2 s = lineB - lineA;

			const Float lenSq = s.SquareMag();
			const Float dot = v.Dot( s ) / lenSq;

			if ( dot <= 0.f )
			{
				// closest point is lineA
				lineSegPoint = lineA;
			}
			else if ( dot >= 1.f )
			{
				// closest point is lineB
				lineSegPoint = lineB;
				v = point - lineB;
			}
			else
			{
				const Vector2 disp = s * dot;
				lineSegPoint = lineA + disp;
				v -= disp;
			}

			return v.SquareMag();
		}

		Bool TestIntersectionSphereLine( const Sphere& sphere, const Vector& pt0, const Vector& pt1, Int32& nbInter, Float& inter1, Float& inter2 )
		{
			Float a, b, c, i;

			const Vector v = pt0 - pt1;
			const Vector center = sphere.GetCenter();
			const Float radius = sphere.GetRadius();

			a = v.SquareMag3();

			b =  2 * ( v.X * ( pt0.X - center.X )
				+ v.Y * ( pt0.Y - center.Y )
				+ v.Z * ( pt0.Z - center.Z ) ) ;

			c = center.X * center.X + center.Y * center.Y + center.Z * center.Z +
				pt0.SquareMag3() -
				2 * ( center.X * pt0.X + center.Y * pt0.Y + center.Z * pt0.Z ) - radius * radius;

			i =  b * b - 4 * a * c;

			if ( i < 0 )
			{
				return false;
			}

			if ( i == 0 ) 
			{
				nbInter = 1;
				inter1 = -b / (2 * a);
			}
			else 
			{
				nbInter = 2;
				inter1 = ( -b + MSqrt( b*b - 4*a*c ) ) / (2 * a);
				inter2 = ( -b - MSqrt( b*b - 4*a*c ) ) / (2 * a);
			}

			return true;
		}

		Bool TestIntersectionCircleLine2D( const Vector2& c, Float r, const Vector2& p1, const Vector2& p2)
		{
			Vector2 dir = p2 - p1;
			Vector2 diff = c - p1;
			Float t = diff.Dot(dir) / dir.SquareMag();
			Float distsqr;
			if (t < 0.0f)
			{
				distsqr = (p1 - c).SquareMag();
			}
			else if (t > 1.0f)
			{
				distsqr = (p2 - c).SquareMag();
			}
			else
			{
				Vector2 closest = p1 + (dir * t);
				Vector2 d = c - closest;
				distsqr = d.SquareMag();
			}
			return distsqr <= r*r;
		}

		RED_INLINE Bool _SCvsC2D_Verify( const Float x, const Float y, const Vector2& dir, Vector2& out )
		{
			const Bool ok = MAbs( x ) <= MAbs( dir.X ) && MAbs( y ) <= MAbs( dir.Y ) && MSign( x ) == MSign( dir.X ) && MSign( y ) == MSign( y );
			out = ok ? Vector2( x, y ) : dir;
			return ok;
		}

		Bool SweepCircleVsCircle2D( const Circle2D& sweptCircle, const Circle2D& otherCircle, const Vector2& dir, Vector2& out )
		{
			// test initial overlap
			const Vector2 diff = otherCircle.m_center - sweptCircle.m_center;
			const Float R = sweptCircle.m_radius + otherCircle.m_radius;

			if ( diff.SquareMag() > R * R )
			{
				// no initial overlap
				const Float dirMagSq = dir.SquareMag();

				// zero sweep?
				if ( dirMagSq == 0.f )
				{
					out = Vector2( 0.f, 0.f );
					return false;
				}

				const Float a = otherCircle.m_center.X - sweptCircle.m_center.X;
				const Float b = otherCircle.m_center.Y - sweptCircle.m_center.Y;

				if ( dir.X == 0.f )
				{
					// special case
					const Float B = ( -2.f * b );
					const Float C = ( a * a ) + ( b * b ) - ( R * R );
					const Float delta = ( B * B ) - ( 4.f * C );

					if ( delta < 0.f )
					{
						// no hits
						out = dir;
						return false;
					}
					else if ( delta == 0.f )
					{
						return _SCvsC2D_Verify( 0.f, ( -B ) / ( 2.f ), dir, out );
					}
					else
					{
						// two hits ( intersection )
						const Float deltaSqrt = MSqrt( delta );
						const Float y1 = ( -B - deltaSqrt ) / 2.f;
						const Float y2 = ( -B + deltaSqrt ) / 2.f;

						if ( MAbs( y1 ) > MAbs( y2 ) )
						{
							if ( _SCvsC2D_Verify( 0.f, y2, dir, out ) )
							{
								return true;
							}
							else
							{
								return _SCvsC2D_Verify( 0.f, y1, dir, out );
							}
						}
						else
						{
							if ( _SCvsC2D_Verify( 0.f, y1, dir, out ) )
							{
								return true;
							}
							else
							{
								return _SCvsC2D_Verify( 0.f, y2, dir, out );
							}
						}
					}

				}
				else
				{
					const Float p = dir.Y / dir.X;
					const Float A = p * p + 1;
					const Float B = a * ( -2.f ) - ( 2.f * p * b );
					const Float C = a * a + b * b - ( R * R );
					const Float delta = B * B - ( 4.f * A * C );
					if ( delta < 0.f )
					{
						// no hits
						out = dir;
						return false;
					}
					else if ( delta == 0.f )
					{
						// one hit ( touch )
						const Float x = ( -B ) / ( 2.f * A );
						const Float y = p * x;
						return _SCvsC2D_Verify( x, y, dir, out );
					}
					else
					{
						// two hits ( intersection )
						const Float deltaSqrt = MSqrt( delta );

						Float x = ( -B - deltaSqrt ) / ( 2.f * A );
						Float y = p * x;
						Vector2 v1( x, y );

						x = ( -B + deltaSqrt ) / ( 2.f * A );
						y = p * x;
						Vector2 v2( x, y );

						if ( v1.SquareMag() > v2.SquareMag() )
						{
							if ( _SCvsC2D_Verify( v2.X, v2.Y, dir, out ) )
							{
								return true;
							}
							else
							{
								return _SCvsC2D_Verify( v1.X, v1.Y, dir, out );
							}
						}
						else
						{
							if ( _SCvsC2D_Verify( v1.X, v1.Y, dir, out ) )
							{
								return true;
							}
							else
							{
								return _SCvsC2D_Verify( v2.X, v2.Y, dir, out );
							}
						}
					}
				}
			}
			else
			{
				// initial overlap, return the same point
				out = Vector2( 0.f, 0.f );
				return true;
			}
		};

		Bool _TryTheCvsC2DSweepTest( const Char* id, const Circle2D& sweptCircle, const Circle2D& otherCircle, const Vector2& dir, Vector2& out ) 
		{
			LOG_CORE( TXT("Trying %s: swept circle is (%.1f, %.1f) r=%.1f, test circle is (%.1f, %.1f) r=%.1f, dir is (%.1f, %.1f)"), id, 
				sweptCircle.m_center.X, sweptCircle.m_center.Y, sweptCircle.m_radius,
				otherCircle.m_center.X, otherCircle.m_center.Y, otherCircle.m_radius,
				dir.X, dir.Y );

			const Bool result = SweepCircleVsCircle2D( sweptCircle, otherCircle, dir, out );

			LOG_CORE( TXT("Result is: %s, moved by (%.1f, %.1f)"), result ? TXT("hit ") : TXT("miss"), out.X, out.Y );

			RED_UNUSED( id );

			return result;
		}

#if 0 // "manual" unit test - debug code - please leave it here
		Bool _TrySomeOfTheCvsC2DSweepTests()
		{
			Circle2D swept, tested;
			Vector2 moveVec, out;
			Bool success( true );

			swept.m_radius = 1.f;

			// A
			tested.m_center = Vector2( 7.f, 7.f );
			tested.m_radius = 2.f;

			swept.m_center = Vector2( 5.f, 11.f );
			moveVec = Vector2( 5.f, -5.f );
			success &= _TryTheCvsC2DSweepTest( TXT("av1"), swept, tested, moveVec, out );

			swept.m_center = Vector2( 7.f, 8.f );
			moveVec = Vector2( 0.f, -2.f );
			success &= _TryTheCvsC2DSweepTest( TXT("av2"), swept, tested, moveVec, out );

			swept.m_center = Vector2( 6.f, 4.f );
			moveVec = Vector2( 8.f, 0.f );
			success &= _TryTheCvsC2DSweepTest( TXT("av3"), swept, tested, moveVec, out );


			// B
			tested.m_center = Vector2( 0.5f, -3.5f );
			tested.m_radius = 2.5f;

			swept.m_center = Vector2( 0.f, -3.f );
			moveVec = Vector2( 0.f, 12.f );
			success &= _TryTheCvsC2DSweepTest( TXT("bv1"), swept, tested, moveVec, out );

			swept.m_center = Vector2( 0.f, 3.f );
			moveVec = Vector2( 0.f, -6.f );
			success &= _TryTheCvsC2DSweepTest( TXT("bv2"), swept, tested, moveVec, out );

			swept.m_center = Vector2( -5.f, 4.f );
			moveVec = Vector2( 13.f, -12.f );
			success &= _TryTheCvsC2DSweepTest( TXT("bv3"), swept, tested, moveVec, out );

			swept.m_center = Vector2( 3.f, 1.f );
			moveVec = Vector2( -8.f, -8.f );
			success &= _TryTheCvsC2DSweepTest( TXT("bv4"), swept, tested, moveVec, out );

			swept.m_center = Vector2( -3.f, -3.f );
			moveVec = Vector2( -3.f, -3.f );
			success &= _TryTheCvsC2DSweepTest( TXT("bv5"), swept, tested, moveVec, out );


			// C
			tested.m_center = Vector2( -14.f, 0.f );
			tested.m_radius = 4.f;

			swept.m_center = Vector2( -6.f, 0.f );
			moveVec = Vector2( -9.f, 0.f );
			success &= _TryTheCvsC2DSweepTest( TXT("cv1"), swept, tested, moveVec, out );

			swept.m_center = Vector2( -6.f, 8.f );
			moveVec = Vector2( -4.f, -6.f );
			success &= _TryTheCvsC2DSweepTest( TXT("cv2"), swept, tested, moveVec, out );

			swept.m_center = Vector2( -14.f, 7.f );
			moveVec = Vector2( 0.f, -15.f );
			success &= _TryTheCvsC2DSweepTest( TXT("cv3"), swept, tested, moveVec, out );

			swept.m_center = Vector2( 14.f, -9.f );
			moveVec = Vector2( 0.f, 5.f );
			success &= _TryTheCvsC2DSweepTest( TXT("cv4"), swept, tested, moveVec, out );

			swept.m_center = Vector2( -9.f, -2.f );
			moveVec = Vector2( 2.f, 0.f );
			success &= _TryTheCvsC2DSweepTest( TXT("cv5"), swept, tested, moveVec, out );

			return success;
		}
#endif // if 0

		ENumberOfResults FindIntersecionsOfTwo2DCircles( Circle2D firstCircle, Circle2D secondCircle, Vector2& point1, Vector2& point2 )
		{
			// same center
			if ( firstCircle.m_center == secondCircle.m_center )
			{
				return ( firstCircle.m_radius == secondCircle.m_radius ) ? RESNUM_Inifnite : RESNUM_Zero; 
			}

			Float r, R, vc12sqMag, vc12mag;
			Vector2 vc1, vc2, vc12;

			if ( firstCircle.m_radius != secondCircle.m_radius )
			{
				const Bool firstIsSmaller = firstCircle.m_radius < secondCircle.m_radius;

				r = Red::Math::NumericalUtils::Min( firstCircle.m_radius, secondCircle.m_radius );
				R = Red::Math::NumericalUtils::Max( firstCircle.m_radius, secondCircle.m_radius );

				vc1 = firstIsSmaller ? firstCircle.m_center : secondCircle.m_center;
				vc2 = firstIsSmaller ? secondCircle.m_center : firstCircle.m_center;
				vc12 = vc1 - vc2;

				vc12sqMag = vc12.SquareMag();

				if ( vc12.SquareMag() < ( R - r ) * ( R - r ) )
				{
					return RESNUM_Zero;
				}

			}
			else
			{
				r = R = firstCircle.m_radius;
				vc1 = firstCircle.m_center;
				vc2 = secondCircle.m_center;
				vc12 = vc1 - vc2;
				vc12sqMag = vc12.SquareMag();
			}

			// no intersections?
			const Float d = ( r + R ) * ( r + R );
			if ( vc12sqMag > d )
			{
				return RESNUM_Zero;
			}

			vc12mag = MSqrt( vc12sqMag );

			// touch?
			if ( vc12sqMag == d )
			{
				point1 = vc2 + ( vc12 * ( R / vc12mag ) );
				return RESNUM_One;
			}
			 
			// two intersections, for sure
			const Float cosa = ( vc12sqMag + ( r * r ) - ( R * R ) ) / ( 2.f * vc12mag * r );
			const Float sina = MSqrt( 1.f - ( cosa * cosa ) );

			const Vector2 vp = ( vc2 - vc1 ) * ( r / vc12mag );

			point1.X = ( vp.X * cosa ) - ( vp.Y * sina ) + vc1.X;
			point1.Y = ( vp.X * sina ) + ( vp.Y * cosa ) + vc1.Y;
			point2.X = ( vp.X * cosa ) + ( vp.Y * sina ) + vc1.X;
			point2.Y = ( vp.Y * cosa ) - ( vp.X * sina ) + vc1.Y;

			return RESNUM_Two;			
		} 

#if 0 // "manual" unit test - debug code - please leave it here
		Bool _TryTheCvsCIntersectionTest( const Char* id, const Circle2D& oneCircle, const Circle2D& otherCircle, Vector2& p1, Vector2& p2 ) 
		{
			LOG_CORE( TXT("Trying %s: one circle is (%.1f, %.1f) r=%.1f, other circle is (%.1f, %.1f) r=%.1f"), id, 
				oneCircle.m_center.X, oneCircle.m_center.Y, oneCircle.m_radius,
				otherCircle.m_center.X, otherCircle.m_center.Y, otherCircle.m_radius );

			const ENumberOfResults res = FindIntersecionsOfTwo2DCircles( oneCircle, otherCircle, p1, p2 );

			const Char* results[] = { TXT("zero"), TXT("one"), TXT("two"), TXT("infinite number") };

			LOG_CORE( TXT("Results: %s, (%.1f, %.1f), (%.1f, %.1f)"), results[ res ], p1.X, p1.Y, p2.X, p2.Y );

			return true;
		}

		Bool _TrySomeOfTheCvsCIntersectionTests()
		{
			Circle2D A, B, C, a, b, c, d, e;
			Vector2 p1, p2;
			Bool success( true );

			A.m_center = Vector2( 7.f, 7.f );
			A.m_radius = 2.f;
			B.m_center = Vector2( 0.5f, -3.5f );
			B.m_radius = 2.5f;
			C.m_center = Vector2( -14.f, 0.f );
			C.m_radius = 4.f;

			a.m_radius = b.m_radius = c.m_radius = d.m_radius = e.m_radius = 5.f;
			a.m_center = Vector2( -14.f, -3.f );
			b.m_center = Vector2( -11.f, 0.f );
			c.m_center = Vector2( 2.f, 11.f );
			d.m_center = Vector2( 14.f, 7.f );
			e.m_center = Vector2( 5.f, -0.5f );
			
			_TryTheCvsCIntersectionTest( TXT("Ac"), A, c, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Ad"), A, d, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Ae"), A, e, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Be"), B, e, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Bc"), B, c, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Ba"), B, a, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Ca"), C, a, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Cb"), C, b, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("Ce"), C, e, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("AB"), A, B, p1, p2 );
			_TryTheCvsCIntersectionTest( TXT("AA"), A, A, p1, p2 );

			return true;
		}
#endif // if 0

		void TestClosestPointOnLine2D( const Vector2& c, const Vector2& p1, const Vector2& p2, Vector2& outPoint )
		{
			Vector2 dir = p2 - p1;
			Vector2 diff = c - p1;
			Float t = diff.Dot(dir) / dir.SquareMag();
			if (t < 0.0f)
			{
				outPoint = p1;
			}
			else if (t > 1.0f)
			{
				outPoint = p2;
			}
			else
			{
				outPoint = p1 + (dir * t);
			}
		}

		void TestClosestPointOnLine2D( const Vector2& c, const Vector2& p1, const Vector2& p2, Float& outRatio, Vector2& outPoint )
		{
			Vector2 dir = p2 - p1;
			Vector2 diff = c - p1;
			Float t = diff.Dot(dir) / dir.SquareMag();
			if (t < 0.0f)
			{
				outPoint = p1;
				outRatio = 0.f;
			}
			else if (t > 1.0f)
			{
				outPoint = p2;
				outRatio = 1.f;
			}
			else
			{
				outPoint = p1 + (dir * t);
				outRatio = t;
			}
		}

		Bool TestIntersectionCircleTriangle2D( const Circle2D& circle, const Vector2& v1, const Vector2& v2, const Vector2& v3 )
		{
			const Vector2 c1 = circle.m_center - v1;
			const Vector2 c2 = circle.m_center - v2;
			const Vector2 c3 = circle.m_center - v3;

			const Float sqRadius = circle.m_radius * circle.m_radius;

			// Check if any of the triangle's vertex is inside the circle
			if( c1.SquareMag() <= sqRadius || c2.SquareMag() <= sqRadius || c3.SquareMag() <= sqRadius )
			{
				return true;
			}

			const Vector2 e1 = v2 - v1;
			const Vector2 e2 = v3 - v2;
			const Vector2 e3 = v1 - v3;

			// Check if circle center is inside the triangle
			if ( e1.X * c1.Y - e1.Y * c1.X >= 0.f )
			{
				// Counterclockwise triangle
				if ( e2.X * c2.Y - e2.Y * c2.X >= 0.f && e3.X * c3.Y - e3.Y * c3.X >= 0.f )
				{
					return true;
				}
			} // Clockwise triangle
			else if ( e2.X * c2.Y - e2.Y * c2.X <= 0.f && e3.X * c3.Y - e3.Y * c3.X <= 0.f )
			{
				return true;
			}

			// Finally check if any edge intersects the circle

			//        C - circle center
			//       /|
			//     p/ |d
			//     /  |
			//  V1/___|_____V2
			//      k
			//      
			// c1 dot e1 = |c1| * |e1| * cos(angle)
			// c1 dot e1 = |c1| * |e1| * (k/p)
			// c1 dot e1 = ( p * |e1| * k ) / p    => since |c1| = p
			// k = ( c1 dot e1 ) / |e1|
			
			Float k = c1.Dot( e1 );

			// if dot < 0 then it means that the perpendicular line is beyond the edge
			if( k > 0.f )
			{
				const Float sqLen = e1.SquareMag();
				k = k*k / sqLen;

				// k > |e1| means that we are beyond the edge aswell
				if( k < sqLen )
				{
					// from picture above => p*p = k*k + d*d => d*d = p*p - k*k
					if( c1.SquareMag() - k <= sqRadius )
					{
						return true;
					}
				}
			}

			// Second edge
			k = c2.Dot( e2 );
			if( k > 0.f )
			{
				const Float sqLen = e2.SquareMag();
				k = k*k / sqLen;

				if( k < sqLen && c2.SquareMag() - k <= sqRadius )
				{
					return true;
				}
			}

			// Third edge
			k = c3.Dot( e3 );
			if( k > 0.f )
			{
				const Float sqLen = e3.SquareMag();
				k = k*k / sqLen;

				if( k < sqLen && c3.SquareMag() - k <= sqRadius )
				{
					return true;
				}
			}

			return false;
		}

		Bool TestIntersectionTriAndSphere(	const Vector& start, const Vector& end,
			const Vector& p1, const Vector& p2, const Vector& p3,
			const Float radius,
			Float& distance )
		{
			// Based on:
			// http://www.flipcode.com/archives/Moving_Sphere_VS_Triangle_Collision.shtml

			Sphere sphere( start, radius );

			Vector triNormal = GetTriangleNormal( p1, p2, p3 );

			Vector lineDir = end - start;
			Float traceDist = lineDir.Normalize3();

			if ( Vector::Dot3( triNormal, lineDir ) > -0.001f )
			{
				return false;
			}

			Vector points[] = { p1, p2, p3 };
			const Vector center = sphere.GetCenter();

			Int32 col = -1;
			Float dist = FLT_MAX;

			// Pass1: sphere VS plane
			{
				Plane plane( triNormal, p1 );

				Float h = plane.DistanceTo( center );
				if ( h < -radius )
				{
					return false;
				}

				if ( h > radius )
				{
					h -= radius;

					Float dot = Vector::Dot3( triNormal, lineDir );
					if ( dot != 0.f )
					{
						Float t = -h / dot;
						Vector onPlane = center + lineDir * t;

						if ( IsPointInsideTriangle( p1, p2, p3, onPlane ) )
						{
							if ( t < dist )
							{
								dist = t;
								//_reaction = triNormal;
								col = 0;
							}
						}
					}
				}
			}

			// Pass2: sphere VS triangle vertices
			{
				for ( Int32 i=0; i < 3; i++) 
				{
					Vector pt1 = points[ i ];
					Vector pt2 = pt1 - lineDir;

					Int32 nbInter;
					Float inter1, inter2;

					//Bool res = sphere.IntersectEdge( pt1, pt2, inter1, inter2 ) != 0;
					Bool res = TestIntersectionSphereLine( sphere, pt1, pt2, nbInter, inter1, inter2 );
					if ( res == false )
					{
						continue;
					}

					//Float t1 = inter1.DistanceTo( center );
					//Float t2 = inter2.DistanceTo( center );
					Float t = Min( inter1, inter2 );

					if ( t < 0 )
					{
						continue;
					}

					if ( t < dist ) 
					{
						dist = t;
						//Vector onSphere = pt1 + v * t;
						//_reaction = center - onSphere;
						col = 1;
					}
				}
			}

			// Pass3: sphere VS triangle edges
			{
				Float srr = radius * radius;

				for ( Int32 i = 0; i < 3; i++) 
				{
					Vector edge0 = points[ i ];

					Int32 j = i + 1;
					if ( j == 3 )
					{
						j = 0;
					}

					Vector edge1 = points[ j ];

					Plane plane( edge0, edge1, edge1 - lineDir );

					float d = plane.DistanceTo( center );
					if ( d > radius || d < -radius )
					{
						continue;
					}

					Float r = MSqrt( srr - d*d );

					Vector pt0 = plane.Project( center ); // center of the sphere slice (a circle)

					Vector onLine;
					DistancePointToLine( pt0, edge0, edge1, onLine );
					Vector v = ( onLine - pt0 ).Normalized3();

					Vector pt1 = v * r + pt0; // point on the sphere that will maybe collide with the edge

					Int32 a0 = 0, a1 = 1;
					Float pl_x = MAbs( plane.NormalDistance.X );
					Float pl_y = MAbs( plane.NormalDistance.Y );
					Float pl_z = MAbs( plane.NormalDistance.Z );

					if ( pl_x > pl_y && pl_x > pl_z ) 
					{
						a0 = 1;
						a1 = 2;
					}
					else if (pl_y > pl_z) 
					{
						a0 = 0;
						a1 = 2;
					}

					Vector vv = pt1 + lineDir;

					Float t;
					Bool res = TestIntersectionLineLine2D(	Vector( pt1.A[ a0 ], pt1.A[ a1 ], 0.f ),
						Vector( vv.A[ a0 ], vv.A[ a1 ], 0.f ),
						Vector( edge0.A[ a0 ], edge0.A[ a1 ], 0.f ),
						Vector( edge1.A[ a0 ], edge1.A[ a1 ], 0.f ),
						0, 1, t );
					if ( !res || t < 0 )
					{
						continue;
					}

					Vector inter = pt1 + lineDir * t;

					Vector r1 = edge0 - inter;
					Vector r2 = edge1 - inter;
					if ( Vector::Dot3( r1, r2 ) > 0 )
					{
						continue;
					}

					if ( t > dist )
					{
						continue;
					}

					dist = t;
					//_reaction = center - pt1;
					col = 2;
				}
			}

			//if ( col != -1 )
			//{
			//	_reaction.Normalize3();
			//}

			if ( col == -1 )
			{
				return false;
			}
			else
			{
				distance = dist / traceDist;

				if ( distance >= 0.f && distance <= 1.f )
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		}


		Bool TestIntersectionTriAndBox( const Vector& tri0, const Vector& tri1, const Vector& tri2, const Box& box )
		{
			struct Local
			{
				static RED_INLINE Bool AxisTest( const Vector& v0, const Vector& v1, Float a, Float b, Float rad )
				{
					Float p0 = a*v0.Y - b*v0.Z;
					Float p1 = a*v1.Y - b*v1.Z;
					Float min, max;

					if( p0<p1 )
					{
						min = p0;
						max = p1;
					}
					else
					{
						min = p1;
						max = p0;
					}

					return ( min<=rad && max>=-rad );
				}
			
			};

			//    use separating axis theorem to test overlap between triangle and box
			//    need to test for overlap in these directions:
			//
			//    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle
			//       we do not even need to test these)
			//
			//    2) normal of the triangle 
			//
			//    3) crossproduct(edge from tri, {x,y,z}-directin)
			//       this gives 3x3=9 more tests


			// move everything so that the boxcenter is in (0,0,0)
			Vector boxSize = box.Max - box.Min;
			Vector boxHalfSize = boxSize * 0.5f;
			Vector boxCenter = box.Min + boxHalfSize;

			Vector v0 = tri0 - boxCenter;
			Vector v1 = tri1 - boxCenter;
			Vector v2 = tri2 - boxCenter;


			// Test 1:

			//  first test overlap in the {x,y,z}-directions
			//  find min, max of the triangle each direction, and test for overlap in
			//  that direction -- this is equivalent to testing a minimal AABB around
			//  the triangle against the AABB


			for ( Uint32 i = 0; i < 3; ++i )
			{
				Float min = Min( v0.A[ i ], v1.A[ i ], v2.A[ i ] );
				Float max = Max( v0.A[ i ], v1.A[ i ], v2.A[ i ] );

				if ( min > boxHalfSize.A[ i ] || max < -boxHalfSize.A[ i ] )
				{
					return false;
				}
			}

			// compute triangle edges

			Vector e0 = v1 - v0;		// tri edge 0
			Vector e1 = v2 - v1;		// tri edge 1
			Vector e2 = v0 - v2;		// tri edge 2


			// Test 2:

			//  test the 9 tests first (this was faster)
			Float fex,fey,fez;

			fex = fabsf(e0.X);
			fey = fabsf(e0.Y);
			fez = fabsf(e0.Z);

			if (
				!Local::AxisTest( v0, v2, e0.Z, e0.Y, fez * boxHalfSize.Y + fey * boxHalfSize.Z ) ||
				!Local::AxisTest( v0, v2,-e0.Z, e0.X, fez * boxHalfSize.X + fex * boxHalfSize.Z ) ||
				!Local::AxisTest( v1, v2, e0.Y, e0.X, fey * boxHalfSize.X + fex * boxHalfSize.Y ) )
			{
				return false;
			}
			
			fex = fabsf(e1.X);
			fey = fabsf(e1.Y);
			fez = fabsf(e1.Z);

			if ( 
				!Local::AxisTest( v0, v2, e1.Z, e1.Y, fez * boxHalfSize.Y + fey * boxHalfSize.Z ) ||
				!Local::AxisTest( v0, v2,-e1.Z, e1.X, fez * boxHalfSize.X + fex * boxHalfSize.Z ) ||
				!Local::AxisTest( v0, v1, e1.Y, e1.X, fey * boxHalfSize.X + fex * boxHalfSize.Y ) )
			{
				return false;
			}

			fex = fabsf(e2.X);
			fey = fabsf(e2.Y);
			fez = fabsf(e2.Z);

			if ( 
				!Local::AxisTest( v0, v1, e2.Z, e2.Y, fez * boxHalfSize.Y + fey * boxHalfSize.Z ) ||
				!Local::AxisTest( v0, v1,-e2.Z, e2.X, fez * boxHalfSize.X + fex * boxHalfSize.Z ) ||
				!Local::AxisTest( v1, v2, e2.Y, e2.X, fey * boxHalfSize.X + fex * boxHalfSize.Y ) )
			{
				return false;
			}


			// Test 3:

			//  test if the box intersects the plane of the triangle
			//  compute plane equation of triangle: normal*x+d=0 

			Vector normal = Vector::Cross( e0, e1 );


			Vector vmin, vmax;
			for( Uint32 dim = 0; dim <= 3; ++dim )
			{

				Float v=v0.A[ dim ];

				if( normal.A[dim] > 0.0f )
				{
					vmin.A[ dim ] =-boxHalfSize.A[ dim ] - v;
					vmax.A[ dim ] = boxHalfSize.A[ dim ] - v;
				}
				else

				{
					vmin.A[ dim ] = boxHalfSize.A[ dim ] - v;
					vmax.A[ dim ] =-boxHalfSize.A[ dim ] - v;
				}
			}

			if( normal.Dot3( vmin ) > 0.f || normal.Dot3( vmax ) < 0.f )
			{
				// plane and box don't overlap
				return false;
			}

			return true;
		}

		// SAT theory based 2d query
		Bool TestIntersectionTriAndRectangle2D( const Vector2& v0, const Vector2& v1, const Vector2& v2, const Vector2& rectMin, const Vector2& rectMax )
		{
			// 1D bounds quick tests - with basically is SAT rectangle tests
			if (
				!MathUtils::GeometryUtils::RangeOverlap1D( Min( v0.X, v1.X, v2.X ), Max( v0.X, v1.X, v2.X ), rectMin.X, rectMax.X ) ||
				!MathUtils::GeometryUtils::RangeOverlap1D( Min( v0.Y, v1.Y, v2.Y ), Max( v0.Y, v1.Y, v2.Y ), rectMin.Y, rectMax.Y ) )
			{
				return false;
			}

			auto funHandleEdge = [ v0, v1, v2, rectMin, rectMax ] ( const Vector2& vecEdge ) -> Bool
			{
				if ( vecEdge.IsAlmostZero() )
				{
					return true;
				}

				Vector2 vecAxis(-vecEdge.Y, vecEdge.X);
				
				// NOTE: everyone is doing axis normalization. But we are using it only for dot products that just scale lineary with axis length (min and max will be scaled)

				// project triangle on axis
				Float dotTri0 = v0.Dot( vecAxis );
				Float dotTri1 = v1.Dot( vecAxis );
				Float dotTri2 = v2.Dot( vecAxis );

				// project rectangle on axis
				Float dotRect0 = Vector2( rectMin.X, rectMin.Y ).Dot( vecAxis );
				Float dotRect1 = Vector2( rectMin.X, rectMax.Y ).Dot( vecAxis );
				Float dotRect2 = Vector2( rectMax.X, rectMin.Y ).Dot( vecAxis );
				Float dotRect3 = Vector2( rectMax.X, rectMax.Y ).Dot( vecAxis );

				Float minTriVal = Min( dotTri0, dotTri1, dotTri2 );
				Float maxTriVal = Max( dotTri0, dotTri1, dotTri2 );
				Float minRectVal = Min( dotRect0, dotRect1, Min( dotRect2, dotRect3 ) );
				Float maxRectVal = Max( dotRect0, dotRect1, Max( dotRect2, dotRect3 ) );

				if( !RangeOverlap1D( minTriVal, maxTriVal, minRectVal, maxRectVal ) )
				{ 
					return false;
				}
				return true;
			};

			// iterate over triangle edges
			if ( !funHandleEdge( v1 - v0 ) )
			{
				return false;
			}
			if ( !funHandleEdge( v2 - v1 ) )
			{
				return false;
			}
			if ( !funHandleEdge( v0 - v2 ) )
			{
				return false;
			}

			return true; 
		} 

		Bool TestIntersectionLineLine3D( const Vector& p1A, const Vector& p2A, const Vector& p1B, const Vector& p2B )
		{
			const Vector t1 = p1B - p1A;
			const Vector m1 = Vector::Cross( p1A, p1B );
			const Vector t2 = p2B - p2A;
			const Vector m2 = Vector::Cross( p2A, p2B );

			if( t1.Dot3( m2 ) + t2.Dot3( m1 ) < 0.01 )
			{
				return true;
			}

			return false;
		}

		Bool DistanceLineLine3D( const Vector& p1, const Vector& p2, const Vector& p3, const Vector& p4, Vector& outP1, Vector& outP2 )
		{
			/*	Pa = P1 + mua(P2 - P1)	-> 'Pa' point on line P1P2
			 *	Pb = P3 + mub(P4 - P3)	-> 'Pb' point on line P3P4
			 *	Pb - Pa					-> shortest line segment between the two lines
			 *	
			 *	Substituting 'Pb - Pa' gives:
			 *	P1 - P3 + mua(P2 - P1) - mub(P4 - P3)
			 *	
			 *	The shortest line segment between the two lines will be perpendicular to the two lines
			 *	(Pa - Pb) dot (P2 - P1) = 0
			 *	(Pa - Pb) dot (P4 - P3) = 0
			 *	
			 *	Expanding these given the equation of the lines
			 *
			 *	( P1 - P3 + mua(P2 - P1) - mub(P4 - P3) ) dot (P2 - P1) = 0
			 *	( P1 - P3 + mua(P2 - P1) - mub(P4 - P3) ) dot (P4 - P3) = 0
			 *	
			 *	Expanding further gives
			 *	
			 *	(P1-P3)dot(P2-P1) + mua(P2-P1)dot(P2-P1) - mub(P4-P3)dot(P2-P1) = 0
			 *	(P1-P3)dot(P4-P3) + mua(P2-P1)dot(P4-P3) - mub(P4-P3)dot(P4-P3) = 0
			 *	
			 *	Finally, solving for mua gives:
			 *	
			 *	mua = ( d1343 d4321 - d1321 d4343 ) / ( d2121 d4343 - d4321 d4321 )
			 */

			static const Float epsilon = 0.001f;

			const Vector v13 = p1 - p3;
			const Vector v21 = p2 - p1;
			const Vector v43 = p4 - p3;

			if( v43.SquareMag3() < epsilon || v21.SquareMag3() < epsilon )
			{
				return false;
			}

			const Float d1343 = v13.Dot3( v43 );
			const Float d4321 = v43.Dot3( v21 );
			const Float d1321 = v13.Dot3( v21 );
			const Float d4343 = v43.SquareMag3();
			const Float d2121 = v21.SquareMag3();

			const Float denom = d2121 * d4343 - d4321 * d4321;
			if( MAbs( denom ) < epsilon )
			{
				return false;
			}

			const Float mua = (d1343 * d4321 - d1321 * d4343) / denom;
			const Float mub = (d1343 + d4321 * mua) / d4343;

			outP1 = p1 + v21 * mua;
			outP2 = p3 + v43 * mub;

			return true;
		}

		Bool TestIntersectionRayLine2D( const Vector& rayDir, const Vector& rayOrigin, const Vector& a, const Vector& b, Int32 sX , Int32 sY, Float& t )
		{
			Float t0, t1;

			TestIntersectionRayLine2D( rayDir, rayOrigin, a, b, sX, sY, t0, t1 );

			t = t1;

			return t0 > 0.f && t <= 1.f && t >= 0.f;
		}

		void TestIntersectionRayLine2D( const Vector& rayDir, const Vector& rayOrigin, const Vector& a, const Vector& b, Int32 sX , Int32 sY, Float& t0, Float& t1 )
		{
			const Float d1x = rayDir.A[ sX ];
			const Float d1y = rayDir.A[ sY ];

			const Float d0x = b.A[ sX ] - a.A[ sX ];
			const Float d0y = b.A[ sY ] - a.A[ sY ];

			const Float p1x = rayOrigin.A[ sX ];
			const Float p1y = rayOrigin.A[ sY ];

			const Float p0x = a.A[ sX ];
			const Float p0y = a.A[ sY ];

			const Float qx = p1x - p0x;
			const Float qy = p1y - p0y;

			const Float m = d0x * d1y - d0y * d1x;

			if ( MAbs( m ) < NumericLimits< Float >::Epsilon() )
			{
				t0 = 0.f;
				t1 = 0.f;
				return;
			}

			const Float l0 = qx * d0y - qy * d0x;
			const Float l1 = qx * d1y - qy * d1x;

			t0 = l0 / m;
			t1 = l1 / m;
		}

		template < class V >
		Bool IsPolygonConvex2D( const V* polyVertexes, Uint32 vertsCount )
		{
			if ( vertsCount <= 3 )
			{
				return true;
			}

			Int32 sgn = 0;

			Vector2 lastDiff = polyVertexes[ vertsCount-1 ] - polyVertexes[vertsCount - 2];

			for ( Uint32 j = 0, i = vertsCount-1; j < vertsCount; i = j++ )
			{
				Vector2 newDiff = polyVertexes[ j ] - polyVertexes[ i ];

				Float crossZ = newDiff.CrossZ( lastDiff );

				if ( Abs( crossZ ) < NumericLimits< Float >::Epsilon() )
				{
					// ignore segment
					continue;
				}

				if ( sgn == 0 )
				{
					sgn = Sgn( crossZ );
				}
				else
				{
					Int32 testSgn = Sgn( crossZ );
					if ( testSgn != sgn )
					{
						return false;
					}
				}
				lastDiff = newDiff;
			}

			return true;
		}
		template
		Bool IsPolygonConvex2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount );
		template
		Bool IsPolygonConvex2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount );

		// http://www.gamedev.net/topic/371458-2d-polygon---polygon-intersection-optimization-solved/
		template < class V >
		Bool IsPointInPolygon2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& point )
		{
			Bool inside = false;
			for ( Uint32 j = 0, i = vertsCount-1; j < vertsCount; i = j++ )
			{
				Vector2 v1 = polyVertexes[i].AsVector2();
				Vector2 v2 = polyVertexes[j].AsVector2();
				if (v1.X >= point.X || v2.X >= point.X)
				{ // At least one endpoint >= px
					if (v1.Y < point.Y)
					{ // y1 below ray
						if (v2.Y >= point.Y)
						{ // y2 on or above ray, edge going 'up'
							if ((point.Y-v1.Y)*(v2.X-v1.X) >= (point.X-v1.X)*(v2.Y-v1.Y))
							{
								inside = !inside;
							}
						}
					}
					else if (v2.Y < point.Y)
					{ // y1 on or above ray, y2 below ray, edge going 'down'
						if ((point.Y-v1.Y)*(v2.X-v1.X) <= (point.X-v1.X)*(v2.Y-v1.Y))
						{
							inside = !inside;
						}
					}
				}
			}
			return inside;
		}

		template
		Bool IsPointInPolygon2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& point );
		template
		Bool IsPointInPolygon2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& point );

		template < class V >
		Bool TestIntersectionPolygonCircle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, circleCenter ) )
			{
				return true;
			}

			Vector2 bboxTest[2];
			bboxTest[0] = circleCenter - Vector2( circleRadius, circleRadius );
			bboxTest[1] = circleCenter + Vector2( circleRadius, circleRadius );

			auto functor = 
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					return TestIntersectionCircleLine2D( circleCenter,circleRadius, vertCurr, vertPrev );
				};

			return PolygonTest2D< V >( polyVertexes, vertsCount, bboxTest, functor );
		}

		template
		Bool TestIntersectionPolygonCircle2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );
		template
		Bool TestIntersectionPolygonCircle2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );

		template < class V >
		Bool TestEncapsulationPolygonCircle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius )
		{
			if ( !IsPointInPolygon2D( polyVertexes, vertsCount, circleCenter ) )
			{
				return false;
			}

			Vector2 bboxTest[2];
			bboxTest[0] = circleCenter - Vector2( circleRadius, circleRadius );
			bboxTest[1] = circleCenter + Vector2( circleRadius, circleRadius );

			auto functor = 
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
			{
				return TestIntersectionCircleLine2D( circleCenter,circleRadius, vertCurr, vertPrev );
			};

			return !PolygonTest2D< V >( polyVertexes, vertsCount, bboxTest, functor );
		}

		template
		Bool TestEncapsulationPolygonCircle2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );

		template
		Bool TestEncapsulationPolygonCircle2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& circleCenter, Float circleRadius );

		template < class V >
		Bool TestIntersectionPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2 )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, v2 ) )
			{
				return true;
			}
			Vector2 bboxSegment[2];
			bboxSegment[0].X = Min(v1.X,v2.X);
			bboxSegment[1].X = Max(v1.X,v2.X);
			bboxSegment[0].Y = Min(v1.Y,v2.Y);
			bboxSegment[1].Y = Max(v1.Y,v2.Y);

			auto functor =
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					return TestIntersectionLineLine2D( v1, v2, vertCurr, vertPrev );
				};

			return PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );
		}

		template
		Bool TestIntersectionPolygonLine2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2 );
		template
		Bool TestIntersectionPolygonLine2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2 );

		template < class V >
		Bool TestIntersectionPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float radius )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, v2 ) )
			{
				return true;
			}

			Float radiusSq = radius*radius;
			Vector2 bboxSegment[2];
			bboxSegment[0].X = Min(v1.X,v2.X) - radius;
			bboxSegment[1].X = Max(v1.X,v2.X) + radius;
			bboxSegment[0].Y = Min(v1.Y,v2.Y) - radius;
			bboxSegment[1].Y = Max(v1.Y,v2.Y) + radius;

			auto functor =
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					return TestDistanceSqrLineLine2D( v1, v2, vertCurr, vertPrev ) <= radiusSq;
				};

			return PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );
		};

		template
		Bool TestIntersectionPolygonLine2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float radius );
		template
		Bool TestIntersectionPolygonLine2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float radius );

		template < class V >
		Bool TestIntersectionPolygonRectangle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax, Float distance )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, (rectangleMin + rectangleMax) / 2.f ) )
			{
				return true;
			}
			Vector2 bboxSegment[2];
			bboxSegment[ 0 ] = rectangleMin - Vector2( distance, distance );
			bboxSegment[ 1 ] = rectangleMax + Vector2( distance, distance );
			Float distanceSq = distance*distance;

			auto functor =
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					return TestDistanceSqrLineRectangle2D( vertCurr, vertPrev, rectangleMin, rectangleMax ) <= distanceSq;
				};

			return PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );
		}

		template
		Bool TestIntersectionPolygonRectangle2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax, Float distance );
		template
		Bool TestIntersectionPolygonRectangle2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax, Float distance );

		template < class V >
		Bool TestPolygonContainsRectangle2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax )
		{
			if ( !IsPointInPolygon2D( polyVertexes, vertsCount, (rectangleMin + rectangleMax) / 2.f ) )
			{
				return false;
			}
			Vector2 bboxSegment[2];
			bboxSegment[ 0 ] = rectangleMin;
			bboxSegment[ 1 ] = rectangleMax;

			auto functor =
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
			{
				return TestIntersectionLineRectangle2D( vertCurr, vertPrev, rectangleMin, rectangleMax );
			};

			return !PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );
		}

		template
		Bool TestPolygonContainsRectangle2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& rectangleMin, const Vector2& rectangleMax );

		// Closest point test. Returns square distance of point found or FLT_MAX if there are no intersection in given maxDist
		template < class V >
		Float ClosestPointPolygonPoint2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v, Float maxDist, Vector2& outClosestPointOnPolygon )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, v ) )
			{
				outClosestPointOnPolygon = v;
				return 0.f;
			}

			Vector2 bboxSegment[2];
			bboxSegment[ 0 ] = v - Vector2( maxDist, maxDist );
			bboxSegment[ 1 ] = v + Vector2( maxDist, maxDist );

			Float closestDistSq = maxDist*maxDist;
			Bool foundIntersection = false;
			auto functor =
				[ &v, &outClosestPointOnPolygon, &closestDistSq, &foundIntersection ] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					Vector2 foundPoint;
					TestClosestPointOnLine2D( v, vertCurr, vertPrev, foundPoint );
					Float distSq = (v - foundPoint).SquareMag();
					if ( distSq < closestDistSq )
					{
						outClosestPointOnPolygon = foundPoint;
						closestDistSq = distSq;
						foundIntersection = true;
					}
					return false;
				};

			PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );

			return foundIntersection ? closestDistSq : FLT_MAX;
		}

		template
		Float ClosestPointPolygonPoint2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& v, Float maxDist, Vector2& outClosestPointOnPolygon );
		template
		Float ClosestPointPolygonPoint2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& v, Float maxDist, Vector2& outClosestPointOnPolygon );

		// Closest point on line and on poly. Returns square distance of points found or FLT_MAX if there are no intersections in maxDist
		template < class V >
		Float ClosestPointPolygonLine2D( const V* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float maxDist, Vector2& outClosestPointOnPolygon, Vector2& outClosestPointOnLine )
		{
			if ( IsPointInPolygon2D( polyVertexes, vertsCount, v1 ) )
			{
				outClosestPointOnPolygon = v1;
				outClosestPointOnLine = v1;
				return 0.f;
			}

			Vector2 bboxSegment[2];
			bboxSegment[0].X = Min(v1.X,v2.X) - maxDist;
			bboxSegment[1].X = Max(v1.X,v2.X) + maxDist;
			bboxSegment[0].Y = Min(v1.Y,v2.Y) - maxDist;
			bboxSegment[1].Y = Max(v1.Y,v2.Y) + maxDist;

			Float closestDistSq = maxDist*maxDist;
			Bool foundIntersection = false;
			auto functor =
				[&] ( const Vector2& vertCurr, const Vector2& vertPrev ) -> Bool
				{
					Vector2 polyPoint;
					Vector2 segmentPoint;
					ClosestPointsLineLine2D( vertCurr, vertPrev, v1, v2, polyPoint, segmentPoint );
					Float distSq = (segmentPoint - polyPoint).SquareMag();
					if ( distSq < closestDistSq )
					{
						outClosestPointOnPolygon = polyPoint;
						outClosestPointOnLine = segmentPoint;
						closestDistSq = distSq;
						foundIntersection = true;
					}
					return false;
				};

			PolygonTest2D< V >( polyVertexes, vertsCount, bboxSegment, functor );

			return foundIntersection ? closestDistSq : FLT_MAX;
		}

		template
		Float ClosestPointPolygonLine2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float maxDist, Vector2& outClosestPointOnPolygon, Vector2& outClosestPointOnLine );
		template
		Float ClosestPointPolygonLine2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount, const Vector2& v1, const Vector2& v2, Float maxDist, Vector2& outClosestPointOnPolygon, Vector2& outClosestPointOnLine );

		template < class V >
		Float GetClockwisePolygonArea2D( const V* polyVertexes, Uint32 vertsCount )
		{
			Float area = 0;				// accumulated area

			for ( Uint32 i=0, j = vertsCount-1; i<vertsCount; i++ )
			{
				area += (polyVertexes[j].X + polyVertexes[i].X) * (polyVertexes[j].Y - polyVertexes[i].Y);
				j = i;
			}

			return area/2;
		}

		template
		Float GetClockwisePolygonArea2D< Vector >( const Vector* polyVertexes, Uint32 vertsCount );
		template
		Float GetClockwisePolygonArea2D< Vector2 >( const Vector2* polyVertexes, Uint32 vertsCount );

		// Code based on SAT (separate axis theorem)
		// Quite readable, but it has performance cost of (n1*n2) - couldn't that work a little better?
		// source: http://forums.gentoo.org/viewtopic-t-872347-start-0.html
		// Almost exact solutions (just rewritten) are available all over net.
		template < class V, EMemoryClass mc >
		Bool IsPolygonsIntersecting2D( const TDynArray< V, mc >& polyVertexes1, const TDynArray< V, mc >& polyVertexes2 )
		{
			struct Local
			{
				RED_INLINE static void ProjectPolygonOnAxis(const Vector2 &vecAxis, const TDynArray< V, mc  >&polyVertexes, Float &valMin, Float &valMax) 
				{ 
					Float dotValue = polyVertexes[0].AsVector2().Dot(vecAxis); 
					valMin = valMax = dotValue; 
					for(Uint32 i = 1, n = polyVertexes.Size(); i < n; ++i) 
					{ 
						dotValue = polyVertexes[i].AsVector2().Dot(vecAxis); 
						if(dotValue < valMin) valMin = dotValue; 
						else if(dotValue > valMax) valMax = dotValue; 
					} 
				}
				RED_INLINE static Bool HandleEdge( Vector2& vecEdge, const TDynArray< V, mc >& polyVertexes1, const TDynArray< V, mc >& polyVertexes2 )
				{
					if ( vecEdge.IsAlmostZero() )
						return true;
					Vector2 vecAxis(-vecEdge.Y, vecEdge.X);
					// NOTE: everyone is doing axis normalization. But we are using it only for dot products that just scale lineary with axis length (min and max will be scaled) - so what's that big deal?

					Float valMinA, valMaxA, valMinB, valMaxB;

					ProjectPolygonOnAxis( vecAxis, polyVertexes1, valMinA, valMaxA );
					ProjectPolygonOnAxis( vecAxis, polyVertexes2, valMinB, valMaxB );

					if( !RangeOverlap1D(valMinA, valMaxA, valMinB, valMaxB) )		// >= ?
					{ 
						return false;
					}
					return true;
				}
			};
			Uint32 pointsCount1 = polyVertexes1.Size();
			Uint32 pointsCount2 = polyVertexes2.Size(); 

			for( Uint32 i = 0, j = pointsCount1-1; i != pointsCount1; j = i++ ) 
			{ 
				// Get the current edge
				Vector2 vecEdge = polyVertexes1[ i ].AsVector2() - polyVertexes1[ j ].AsVector2();
				if ( !Local::HandleEdge( vecEdge, polyVertexes1, polyVertexes2 ) )
					return false;
			}
			for( Uint32 i = 0, j = pointsCount2-1; i != pointsCount2; j = i++ ) 
			{ 
				// Get the current edge
				Vector2 vecEdge = polyVertexes2[ i ].AsVector2() - polyVertexes2[ j ].AsVector2();
				if ( !Local::HandleEdge( vecEdge, polyVertexes1, polyVertexes2 ) )
					return false;
			}


			return true; 
		}

		template
		Bool IsPolygonsIntersecting2D< Vector2, MC_DynArray >( const TDynArray< Vector2, MC_DynArray >& polyVertexes1, const TDynArray< Vector2, MC_DynArray >& polyVertexes2 );
		template
		Bool IsPolygonsIntersecting2D< Vector, MC_DynArray >( const TDynArray< Vector, MC_DynArray >& polyVertexes1, const TDynArray< Vector, MC_DynArray >& polyVertexes2 );
		template
		Bool IsPolygonsIntersecting2D< Vector, MC_AreaShapes >( const TDynArray< Vector, MC_AreaShapes >& polyVertexes1, const TDynArray< Vector, MC_AreaShapes>& polyVertexes2 );

		// Copyright 2001, softSurfer (www.softsurfer.com)
		// This code may be freely used and modified for any purpose
		// providing that this copyright notice is included with it.
		// SoftSurfer makes no warranty for this code, and cannot be held
		// liable for any real or imagined damage resulting from its use.
		// Users of this code must verify correctness for their application.
		//===================================================================
		// chainHull_2D(): Andrew's monotone chain 2D convex hull algorithm
		//     Input:  P[] = an array of 2D points
		//                   presorted by increasing x- and y-coordinates
		//             n = the number of points in P[]
		//     Output: H[] = an array of the convex hull vertices (max is n)
		//     Return: the number of points in H[]
		void ComputeConvexHull2D( TDynArray< Vector2 > &P, TDynArray< Vector2 > &H )
		{
			struct OrderX
			{
				RED_INLINE Bool operator()( const Vector2& v1, const Vector2& v2 )  const { return v1.X < v2.X || ( v1.X == v2.X && v1.Y < v2.Y ); }
			};
			Sort( P.Begin(), P.End(), OrderX() );
			// remove non-unique points
			for ( Int32 i = P.Size()-2; i >= 0; --i )
			{
				for ( Int32 j = i+1; j < Int32( P.Size() ); )
				{
					if ( P[ j ].X - P[ i ].X > NumericLimits< Float >::Epsilon() )
					{
						break;
					}
					if ( (P[ j ] - P[ i ]).IsAlmostZero( NumericLimits< Float >::Epsilon() ) )
					{
						P.RemoveAt( j );
					}
					else
					{
						++j;
					}
				}
			}
			Int32 n = P.Size();
			H.Resize( n + 1 );

			// the output array H[] will be used as the stack
			Int32    bot=0, top=(-1);  // indices for bottom and top of the stack
			Int32    i;                // array scan index

			// Get the indices of points with min x-coord and min|max y-coord
			Int32 minmin = 0, minmax;
			Float xmin = P[0].X;
			for (i=1; i<n; i++)
				if (P[i].X != xmin) break;
			minmax = i-1;
			if (minmax == n-1) {       // degenerate case: all x-coords == xmin
				H[++top] = P[minmin];
				if (P[minmax].Y != P[minmin].Y) // a nontrivial segment
					H[++top] = P[minmax];
				H[++top] = P[minmin];           // add polygon endpoint

				H.Resize( top );
				return;
			}

			// Get the indices of points with max x-coord and min|max y-coord
			Int32 maxmin, maxmax = n-1;
			Float xmax = P[n-1].X;
			for (i=n-2; i>=0; i--)
				if (P[i].X != xmax) break;
			maxmin = i+1;

			// Compute the lower hull on the stack H
			H[++top] = P[minmin];      // push minmin point onto stack
			i = minmax;
			while (++i <= maxmin)
			{
				// the lower line joins P[minmin] with P[maxmin]
				if (PointIsLeftOfSegment2D( P[minmin], P[maxmin], P[i]) >= 0 && i < maxmin)
					continue;          // ignore P[i] above or on the lower line

				while (top > 0)        // there are at least 2 points on the stack
				{
					// test if P[i] is left of the line at the stack top
					if (PointIsLeftOfSegment2D( H[top-1], H[top], P[i]) > 0)
						break;         // P[i] is a new hull vertex
					else
						top--;         // pop top point off stack
				}
				H[++top] = P[i];       // push P[i] onto stack
			}

			// Next, compute the upper hull on the stack H above the bottom hull
			if (maxmax != maxmin)      // if distinct xmax points
				H[++top] = P[maxmax];  // push maxmax point onto stack
			bot = top;                 // the bottom point of the upper hull stack
			i = maxmin;
			while (--i >= minmax)
			{
				// the upper line joins P[maxmax] with P[minmax]
				if (PointIsLeftOfSegment2D( P[maxmax], P[minmax], P[i]) >= 0 && i > minmax)
					continue;          // ignore P[i] below or on the upper line

				while (top > bot)    // at least 2 points on the upper stack
				{
					// test if P[i] is left of the line at the stack top
					if (PointIsLeftOfSegment2D( H[top-1], H[top], P[i]) > 0)
						break;         // P[i] is a new hull vertex
					else
						top--;         // pop top point off stack
				}
				H[++top] = P[i];       // push P[i] onto stack
			}
			if (minmax != minmin)
				H[++top] = P[minmin];  // push joining endpoint onto stack

			H.Resize( top );
		}

		// Based on the paper "Concave hull: A k-nearest neighbours approach" by Adriano Moreira and Maribel Yasmina Santos.
		Bool ComputeConcaveHull2D( const TDynArray< Vector2 >& points, TDynArray< Vector2 >& hull, Uint32 k )
		{
			hull.Clear( );

			// Can't even build a triangle, abort.
			if( points.Size( ) < 3 )
				return false;

			// When initial neighbours is set to zero, all points are considered from the beginning,
			// resulting in the computation of the convex hull of the set.
			if( k == 0 )
				k = points.Size( );

			Uint32 initPointIdx = 0;

			// First point of the boundary is the one with lowest Y.
			for( Uint32 i = 0; i < points.Size( ); ++i )
			{
				if( points[ i ].Y < points[ initPointIdx ].Y )
					initPointIdx = i;
			}

			// Build a mask table for used points.
			TDynArray< Bool > usedPoints( points.Size( ) );
			for( Bool& b : usedPoints ) b = false;

			// Add the first point to the boundary.
			hull.PushBack( points[ initPointIdx ] );
			usedPoints[ initPointIdx ] = true;

			struct SNearPoint
			{
				Uint32	m_idx;
				Float	m_dist;
				SNearPoint( ) : m_idx( 0 ), m_dist( 0 ) { }
				SNearPoint( Uint32 idx, Float dist )
					: m_idx( idx ), m_dist( dist ) { }
			};

			struct SNearPointCompareFunc
			{
				static RED_INLINE Bool Less( const SNearPoint& a, const SNearPoint& b )
				{ return a.m_dist < b.m_dist; }
			};

			TSortedArray< SNearPoint, SNearPointCompareFunc > nearestPointsIdxs;

			Uint32 currPointIdx = initPointIdx;

			// Iterate until the initial point is selected again (the polygon gets closed).
			while( hull.Size( ) <= points.Size( ) )
			{
				// Enable the initial point after a triangle has been built.
				if( hull.Size( ) == 3 )
					usedPoints[ initPointIdx ] = false;

				nearestPointsIdxs.Clear( );

				// Sort available points by distance, and compute their angle with previous edge.
				for( Uint32 pointIdx = 0; pointIdx < points.Size( ); ++pointIdx )
				{
					if( usedPoints[ pointIdx ] )
						continue;
					nearestPointsIdxs.Insert( SNearPoint( pointIdx, ( points[ pointIdx ] - hull[ hull.Size( ) - 1 ] ).SquareMag( ) ) );
				}

				// Previous edge.
				Vector prevEdge( -1, 0, 0 );
				if( hull.Size( ) > 1 )
					prevEdge = hull[ hull.Size( ) - 2 ] - hull[ hull.Size( ) - 1 ];

				Int32 bestIdx = -1;
				Float bestAngle = -1.0f;

				// Select the point with a wider turn among the k-nearest ones.
				for( Uint32 pointIdx = 0; pointIdx < k && pointIdx < nearestPointsIdxs.Size( ); ++pointIdx )
				{
					// Compute the angle between the previous edge and the current one.
					const Vector currEdge = points[ nearestPointsIdxs[ pointIdx ].m_idx ] - hull[ hull.Size( ) - 1 ];
					Float angle = atan2f( currEdge.Y, -currEdge.X ) - atan2f( prevEdge.Y, -prevEdge.X );
					if( angle < 0.0f ) angle += M_PI * 2.0f;

					if( angle > bestAngle )
					{
						Bool intersects = false;

						// Before selecting this candidate, intersections with current hull must be checked.
						if( nearestPointsIdxs[ pointIdx ].m_idx != initPointIdx )
						{
							const Vector& newSrc = hull[ hull.Size( ) - 1 ];
							const Vector& newDst = points[ nearestPointsIdxs[ pointIdx ].m_idx ];

							for( Uint32 i = 1; i < hull.Size( ) - 1; ++i )
							{
								Float t1 = -1.0f, t2 = -1.0f;
								if( TestIntersectionLineLine2DClamped( hull[ i-1 ], hull[ i ], newSrc, newDst, 0, 1, t1, t2 ) )
								{
									intersects = true;
									break;
								}
							}
						}

						// No intersections found, this is a good candidate.
						if( !intersects )
						{
							bestAngle = angle;
							bestIdx = pointIdx;
						}
					}
				}

				#define RESTART_NEXT_K( ) \
				{ \
					hull.Clear( ); \
					if( ++k > points.Size( ) ) \
						break; \
					for( Bool& b : usedPoints ) b = false; \
					hull.PushBack( points[ initPointIdx ] ); \
					usedPoints[ initPointIdx ] = true; \
					currPointIdx = initPointIdx; \
					continue; \
				}

				// No good candidate could be found due to intersections.
				// Increase the number of neighbours (k) and restart the process.
				if( bestIdx < 0 )
				{
					RESTART_NEXT_K( );
				}

				currPointIdx = nearestPointsIdxs[ bestIdx ].m_idx;

				// The polygon has been closed.
				if( currPointIdx == initPointIdx )
				{
					Bool allPointsIncluded = true;
					usedPoints[ initPointIdx ] = true;

					// Test that all points are inside the polygon (except the ones used in the boundaries).
					for( Uint32 i = 0; i < points.Size( ); ++i )
					{
						if( !usedPoints[ i ] && !IsPointInPolygon2D( reinterpret_cast< Vector2* >( hull.Data( ) ), hull.Size( ), points[ i ] ) )
						{
							allPointsIncluded = false;
							break;
						}
					}

					// At least one point was left out of the polygon.
					// Increase the number of neighbours considered (k) and restart the process.
					if( !allPointsIncluded )
					{
						RESTART_NEXT_K( );
					}

					return true;
				}

				// Add the point to the hull and move onto the next one.
				hull.PushBack( points[ currPointIdx ] );
				usedPoints[ currPointIdx ] = true;
			}

			hull.Clear( );
			return false;
		}


		// http://www.gamedev.net/topic/307870-problems-with-moller-trumbore-ray-triangle-intersection-code/
		Bool TestRayTriangleIntersection3D( const Vector3& point, const Vector3& dir, const Vector3& t0, const Vector& t1, const Vector& t2, Vector3& outPos )
		{
			Vector3 edge1 = t1 - t0;
			Vector3 edge2 = t2 - t0;

			Vector3 pvec = dir.Cross( edge2 );

			Float det = edge1.Dot( pvec );
			
			if ( det < NumericLimits< Float >::Epsilon() ) // fabs(det) ?
				return false;

			Float inv_det = 1.f / det;

			Vector3 tvec = point - t0;

			Float u = tvec.Dot( pvec ) * inv_det;
			if ( u < 0.0 || u > 1.0)
				return false;

			Vector3 qvec = tvec.Cross( edge1 );

			Float v = dir.Dot( qvec ) * inv_det;
			if ( v < 0.0 || u + v > 1.0)
				return false;

			/* calculate t, ray intersects triangle */
			//Float t = edge2.Dot( qvec ) * inv_det;
			//if ( t < 0.f )
			//	return false;

			outPos = t0 + edge1 * u + edge2 * v;
			return true;
		}

		Float OrientedBoxSquaredDistance( const Matrix &obbLocalToWorld, const Vector &point )
		{
			const Vector scale = Vector::Max4( Vector::ONES * 0.0001f, obbLocalToWorld.GetScale33() );
			const Matrix mat = Matrix(obbLocalToWorld).SetScale33( Vector::ONES / scale ).Inverted();
			const Vector tformPoint = mat.TransformPoint( point );
			return Box( -scale, scale ).SquaredDistance( tformPoint );
		}

	}
	

	//////////////////////////////////////////////////////////////////////////

	namespace InterpolationUtils
	{
		Float Interpolate( Float from, Float to, Float t )
		{
			return from + ( to - from ) * t;
		}

		Vector HermiteInterpolate( const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, Float t )
		{
			const Vector tangent2 = ( v3 - v1 ).Normalized3();
			const Vector tangent3 = ( v4 - v2 ).Normalized3();

			return HermiteInterpolateWithTangents(v2, tangent2, v3, tangent3, t);
		}

		Vector HermiteInterpolateWithTangents( const Vector & v1, const Vector & t1, const Vector & v2, const Vector & t2, Float t )
		{
			const Float t_Sqr = t * t;
			const Float t_Cub = t * t_Sqr;

			const Float h01 = 3*t_Sqr - 2*t_Cub;
			const Float h00 = 1 - h01;
			const Float h10 = t_Cub - 2*t_Sqr + t;
			const Float h11 = t_Cub - t_Sqr;

			const Float h = ( v2 - v1 ).Mag3();

			return v1 * h00 + t1 * ( h10 * h ) + v2 * h01 + t2 * ( h11 * h );
		}

		Vector CubicInterpolate( const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, Float t )
		{
			Float  t_Sqr = t * t;

			Vector a0 = v4 - v3 - v1 + v2;
			Vector a1 = v1 - v2 - a0;
			Vector a2 = v3 - v1;
			Vector a3 = v2;

			return a0*t*t_Sqr + a1*t_Sqr + a2*t + a3;
		}

		void CatmullRomBuildTauMatrix( Matrix& outMat, Float tau )
		{
			outMat.V[0].X = 0.0f;
			outMat.V[0].Y = 1.0f;
			outMat.V[0].Z = 0.0f;
			outMat.V[0].W = 0.0f;

			outMat.V[1].X = -tau;
			outMat.V[1].Y = 0.0f;
			outMat.V[1].Z = tau;
			outMat.V[1].W = 0.0f;


			outMat.V[2].X = 2.0f * tau;
			outMat.V[2].Y = tau - 3.0f;
			outMat.V[2].Z = 3.0f - 2.0f*tau;
			outMat.V[2].W = -tau;

			outMat.V[3].X = -tau;
			outMat.V[3].Y = 2.0f - tau;
			outMat.V[3].Z = tau - 2.0f;
			outMat.V[3].W = tau;
		}

		Vector CatmullRomInterpolate( const Vector &v1, const Vector &v2, const Vector &v3, const Vector &v4, Float t, const Matrix& tauMatrix ) 
		{
			Float t2 = t*t;
			Vector tVec( 1.0f, t, t2, t2*t );
			Matrix pointsMatrix( v1, v2, v3, v4 );
			Vector vec = tauMatrix.TransformVectorWithW( tVec );
			vec = pointsMatrix.TransformVectorWithW( vec );
			return vec;
		}

		void DistanceToEdge( const Vector &a, const Vector &b, const Vector &pt, Float &dist, Float &alpha )
		{
			Vector edge = (b - a).Normalized3();
			Float ta = Vector::Dot3( edge, a );
			Float tb = Vector::Dot3( edge, b );
			Float p = Vector::Dot3( edge, pt );
			if ( p >= ta && p <= tb )
			{
				alpha = p - ta;
				Vector projected = a + edge * alpha;
				alpha /= tb - ta;
				dist  = pt.DistanceTo( projected );
			}
			else if ( p < ta )
			{
				alpha = 0.f;
				dist  = pt.DistanceTo( a );
			}
			else
			{
				alpha = 1.f;
				dist  = pt.DistanceTo( b );
			}
		}

		Float HermiteClosestPoint(  const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, const Vector & p, Vector &res, Float epsilon )
		{
			Float distAlpha, distBest;
			DistanceToEdge( v2, v3, p, distBest, distAlpha );

			Float deltaAlpha  = 0.25f;

			Float h = ( v3 - v2 ).Mag3() * 2.f;
			Vector tangent2times2 = ( v3 - v1 ).Normalized3();
			Vector tangent3times2 = ( v4 - v2 ).Normalized3();

			Uint32 iterations = 0;
			for ( ;; )
			{
				++iterations;

				Float  t = distAlpha;

				Float t_Sqr = t * t;
				Float t_Cub = t * t_Sqr;
				Float h01 = -2*t_Cub + 3*t_Sqr;
				Float h00 = 1 - h01;
				Float h10 = t_Cub - 2*t_Sqr + t;
				Float h11 = t_Cub - t_Sqr;
				Vector pLine0 = v2 * h00 + tangent2times2 * ( h10 * 0.5f * h ) + v3 * h01 + tangent3times2 * ( h11 * 0.5f * h );

				t = distAlpha + 0.01f;

				t_Sqr = t * t;
				t_Cub = t * t_Sqr;
				h01 = -2*t_Cub + 3*t_Sqr;
				h00 = 1 - h01;
				h10 = t_Cub - 2*t_Sqr + t;
				h11 = t_Cub - t_Sqr;
				Vector pLine1 = v2 * h00 + tangent2times2 * ( h10 * 0.5f * h ) + v3 * h01 + tangent3times2 * ( h11 * 0.5f * h );

				Vector vecToLine  = p - pLine0;
				Vector vecTangent = pLine1 - pLine0;

				Float cosAngle = Vector::Dot3( vecToLine, vecTangent );

				// 90degs?
				if ( MAbs(cosAngle) < epsilon )
				{
					//LOG_CORE( TXT("Dist to Path iterations: %d (SUCCESS) error = %f"), iterations, cosAngle );
					res = pLine0;
					return distAlpha;
				}

				// Move towards 
				if ( cosAngle < 0 )
				{
					if ( distAlpha <= 0.f )
					{
						//LOG_CORE( TXT("Dist to Path iterations: %d (Less than 0.f) error = %f"), iterations, cosAngle );
						res = v2;
						return distAlpha;
					}
					distAlpha -= Min( deltaAlpha, distAlpha );
				}
				else
				{
					if ( distAlpha >= 1.f )
					{
						//LOG_CORE( TXT("Dist to Path iterations: %d (More than 1.f) error = %f"), iterations, cosAngle );
						res = v3;
						return distAlpha;
					}
					distAlpha += Min( deltaAlpha, 1.f - distAlpha );
				}
				// Make steps smaller and smaller
				deltaAlpha *= 0.5f;

				// Limit total no of iterations
				if ( iterations > 10 )
				{
					//LOG_CORE( TXT("Dist to Path iterations: %d (Max iterations) error = %f"), iterations, cosAngle );
					res = pLine0;
					return distAlpha;
				}
			}
		}

		Float CubicClosestPoint(  const Vector & v1, const Vector & v2, const Vector & v3, const Vector & v4, const Vector & p, Vector &res, Float epsilon )
		{
			Vector a0 = v4 - v3 - v1 + v2;
			Vector a1 = v1 - v2 - a0;
			Vector a2 = v3 - v1;
			Vector a3 = v2;

			Float distAlpha, distBest;
			DistanceToEdge( v2, v3, p, distBest, distAlpha );

			Float deltaAlpha  = 0.25f;

			Uint32 iterations = 0;
			for ( ;; )
			{
				++iterations;

				Float  t     = distAlpha;
				Float  t_Sqr = t * t;
				Vector pLine0 = a0*t*t_Sqr + a1*t_Sqr + a2*t + a3;

				t     = distAlpha + 0.01f;
				t_Sqr = t * t;
				Vector pLine1 = a0*t*t_Sqr + a1*t_Sqr + a2*t + a3;

				Vector vecToLine  = p - pLine0;
				Vector vecTangent = pLine1 - pLine0;

				Float cosAngle = Vector::Dot3( vecToLine, vecTangent );

				// 90degs?
				if ( MAbs(cosAngle) < epsilon )
				{
					//LOG_CORE( TXT("Dist to Path iterations: %d (SUCCESS) error = %f"), iterations, cosAngle );
					res = pLine0;
					return distAlpha;
				}

				// Move towards 
				if ( cosAngle < 0 )
				{
					if ( distAlpha <= 0.f )
					{
						//LOG_CORE( TXT("Dist to Path iterations: %d (Less than 0.f) error = %f"), iterations, cosAngle );
						res = v2;
						return distAlpha;
					}
					distAlpha -= Min( deltaAlpha, distAlpha );
				}
				else
				{
					if ( distAlpha >= 1.f )
					{
						//LOG_CORE( TXT("Dist to Path iterations: %d (More than 1.f) error = %f"), iterations, cosAngle );
						res = v3;
						return distAlpha;
					}
					distAlpha += Min( deltaAlpha, 1.f - distAlpha );
				}
				// Make steps smaller and smaller
				deltaAlpha *= 0.5f;

				// Limit total no of iterations
				if ( iterations > 10 )
				{
					//LOG_CORE( TXT("Dist to Path iterations: %d (Max iterations) error = %f"), iterations, cosAngle );
					res = pLine0;
					return distAlpha;
				}
			}
		}

		void InterpolatePoints( const TDynArray< Vector > &keyPoints, TDynArray< Vector > &outPoints, Float pointsDist, Bool closed )
		{
			ASSERT( pointsDist > 0.f );
			ASSERT( keyPoints.Size() > 0 );

			Uint32 nVertices = keyPoints.Size();
			closed         = closed && nVertices > 2;
			Uint32 nEdges    = closed ? nVertices : nVertices - 1;

			for ( Uint32 i = 0; i < nEdges; ++i )
			{
				const Vector &p1 = i > 0 
					? keyPoints[ i - 1 ]
				: closed ? keyPoints[ nVertices-1 ] : keyPoints[ 0 ];
				const Vector &p2 = keyPoints[ i                   ];
				const Vector &p3 = keyPoints[ (i + 1) % nVertices ];
				const Vector &p4 = closed
					? keyPoints[ (i + 2) % nVertices ]
				: keyPoints[ Min<Int32>( nVertices-1, i+2 )];

				Float dist   = p2.DistanceTo( p3 );
				Uint32  numPts = Min<Uint32>( 15, Max<Uint32>( 8, static_cast<Uint32>( MCeil( dist / pointsDist ) ) ) );
				Float step   = 1.f / ( numPts + 1 );
				Float alpha  = step;

				outPoints.PushBack( p2 );

				for ( Uint32 i = 0; i < numPts; ++i )
				{
					outPoints.PushBack( HermiteInterpolate( p1,p2,p3,p4, alpha ) );
					alpha += step;
				}
			}

			outPoints.PushBack( closed ? keyPoints[ 0 ] : keyPoints[ nVertices-1 ] );
		}
	} // Interpolation Utils

} // Math Utils
