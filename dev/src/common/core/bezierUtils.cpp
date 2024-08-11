// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "bezierUtils.h"
#include "math.h"

// =================================================================================================
namespace {
// =================================================================================================

/*
Computes P1 control points given rhs vector.

\param rhs Right hand side vector.
\param outP1ControlPoints (out) Solution vector. Must be the same size as rhs.

This code is based on http://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit.
*/
void ComputeP1ControlPoints( const TDynArray< Float >& rhs, TDynArray< Float >& outP1ControlPoints )
{
	const Uint32 n = rhs.Size();

	// temp workspace
	TDynArray< Float > tmp;
	tmp.ResizeFast( n );

	Float b = 2.0f;
	outP1ControlPoints[ 0 ] = rhs[ 0 ] / b;
	for( Uint32 i = 1; i < n; ++i ) // decomposition and forward substitution
	{
		tmp[ i ] = 1.0f / b;
		b = ( i < n - 1? 4.0f : 3.5f ) - tmp[ i ];
		outP1ControlPoints[ i ] = ( rhs[ i ] - outP1ControlPoints[ i - 1 ] ) / b;
	}
	for( Uint32 i = 1; i < n; ++i )
	{
		outP1ControlPoints[ n - i - 1 ] -= tmp[ n - i ] * outP1ControlPoints[ n - i ]; // back substitution
	}
}

// =================================================================================================
} // unnamed namespace
// =================================================================================================

/*
Computes control points of a cubic Bezier 2d spline that goes through specified points.

\param knots Points through which Bezier spline goes through. They are treated as P0 and P3 control points of each spline segment. There must be at least two points.
\param outP1ControlPoints (out) Storage for P1 control points of each spline segment. Must have (knots.Size() - 1) elements.
\param outP2ControlPoints (out) Storage for P2 control points of each spline semgent. Must have (knots.Size() - 1) elements

Each Bezier segment is defined by control points P0, P1, P2, P3:
P0 == knots[ i ]
P1 == outP1ControlPoints[ i ]
P2 == outP2ControlPoints[ i ]
P3 == knots[ i + 1]

This code is based on http://www.codeproject.com/Articles/31859/Draw-a-Smooth-Curve-through-a-Set-of-2D-Points-wit.
*/
void ComputeBezierSpline( const TDynArray< Vector2 >& knots, TDynArray< Vector2 >& outP1ControlPoints, TDynArray< Vector2 >& outP2ControlPoints )
{
	// at least two knot points are required
	ASSERT( knots.Size() >= 2 );

	Uint32 n = knots.Size() - 1;

	// handle special case - Bezier curve should be a straight line
	if( n == 1 )
	{
		// 3P1 = 2P0 + P3
		outP1ControlPoints[ 0 ].X = ( 2.0f * knots[ 0 ].X + knots[ 1 ].X ) / 3.0f;
		outP1ControlPoints[ 0 ].Y = ( 2.0f * knots[ 0 ].Y + knots[ 1 ].Y ) / 3.0f;

		// P2 = 2P1 - P0
		outP2ControlPoints[ 0 ].X = 2.0f * outP1ControlPoints[ 0 ].X - knots[ 0 ].X;
		outP2ControlPoints[ 0 ].Y = 2.0f * outP1ControlPoints[ 0 ].Y - knots[ 0 ].Y;

		return;
	}

	// calculate first Bezier control points
	// right hand side vector
	TDynArray< Float > rhs;
	rhs.ResizeFast( n );

	// set right hand side - X values
	for( Uint32 i = 1; i < n - 1; ++i )
	{
		rhs[ i ] = 4 * knots[ i ].X + 2 * knots[ i + 1 ].X;
	}
	rhs[ 0 ] = knots[ 0 ].X + 2.0f * knots[ 1 ].X;
	rhs[ n - 1 ] = ( 8.0f * knots[ n - 1 ].X + knots[ n ].X ) / 2.0f;

	// compute P1 control points - X values
	TDynArray< Float > x;
	x.ResizeFast( n );
	ComputeP1ControlPoints( rhs, x );

	// set right hand side - Y values
	for( Uint32 i = 1; i < n - 1; ++i )
	{
		rhs[ i ] = 4 * knots[ i ].Y + 2 * knots[ i + 1 ].Y;
	}
	rhs[ 0 ] = knots[ 0 ].Y + 2.0f * knots[ 1 ].Y;
	rhs[ n - 1 ] = ( 8.0f * knots[ n - 1 ].Y + knots[ n ].Y ) / 2.0f;

	// compute P1 control points - Y values
	TDynArray< Float > y;
	y.ResizeFast( n );
	ComputeP1ControlPoints( rhs, y );

	// fill output arrays
	for( Uint32 i = 0; i < n; ++i )
	{
		// P1 control point
		outP1ControlPoints[ i ].X = x[ i ];
		outP1ControlPoints[ i ].Y = y[ i ];

		// P2 control point
		if( i < n - 1 )
		{
			outP2ControlPoints[ i ].X = 2 * knots[ i + 1 ].X - x[ i + 1 ];
			outP2ControlPoints[ i ].Y = 2 * knots[ i + 1 ].Y - y[ i + 1 ];
		}
		else
		{
			outP2ControlPoints[ i ].X = ( knots[ n ].X + x[ n - 1 ] ) / 2.0f;
			outP2ControlPoints[ i ].Y = ( knots[ n ].Y + y[ n - 1 ] ) / 2.0f;
		}
	}
}
