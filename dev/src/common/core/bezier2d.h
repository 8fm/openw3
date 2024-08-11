
#pragma once

#include "classBuilder.h"
#include "math.h"

/*
Cubic Bezier 2d.
*/
struct Bezier2D
{
	DECLARE_RTTI_STRUCT( Bezier2D );

public:
	Vector m_points01;
	Vector m_points23;

public:
	RED_INLINE Bezier2D() : m_points01( 0.f, 0.f, 1.f/3.f, 0.f ), m_points23( 1.f-1.f/3.f, 1.f, 1.f, 1.f ) {}
	RED_INLINE Bezier2D( Float p0x, Float p0y, Float p1x, Float p1y, Float p2x, Float p2y, Float p3x, Float p3y ) : m_points01( p0x, p0y, p1x, p1y ), m_points23( p2x, p2y, p3x, p3y ) 
	{
		RED_ASSERT( p1x <= p3x );
		RED_ASSERT( p1x >= p0x );
		RED_ASSERT( p2x <= p3x );
		RED_ASSERT( p2x >= p0x );
		RED_ASSERT( p1y <= p3y );
		RED_ASSERT( p1y >= p0y );
		RED_ASSERT( p2y <= p3y );
		RED_ASSERT( p2y >= p0y );
	}
	RED_INLINE Bezier2D( const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3 ) : m_points01( p0.X, p0.Y, p1.X, p1.Y ), m_points23( p2.X, p2.Y, p3.X, p3.Y ) {}
	RED_INLINE Bezier2D( const Vector& p01, const Vector& p12 ) : m_points01( p01 ), m_points23( p12 ) {}

	/*
	Calculates value of y for specified x.

	\param xScaled Value of x scaled to range <0.0f, 1.0f> (should be <0.0f, 1.0f)).
	*/
	RED_INLINE Float Get( Float xScaled ) const
	{
		return Calc( FindTForX( xScaled ), m_points01.A[ 1 ], m_points01.A[ 3 ], m_points23.A[ 1 ], m_points23.A[ 3 ] );
	}

	RED_INLINE void SetPointsAndHandlers( Float p0x, Float p0y, Float h0x, Float h0y, Float p1x, Float p1y, Float h1x, Float h1y )
	{
		m_points01.A[ 0 ] = p0x;
		m_points01.A[ 1 ] = p0y;
		m_points23.A[ 2 ] = p1x;
		m_points23.A[ 3 ] = p1y;
		m_points01.A[ 2 ] = p0x+h0x;
		m_points01.A[ 3 ] = p0y+h0y;
		m_points23.A[ 0 ] = p1x+h1x;
		m_points23.A[ 1 ] = p1y+h1y;
	}

private:
	//RED_INLINE Float WA() const { return -m_handlers.A[0] + 3.f*m_handlers.A[1] - 3.f*m_handlers.A[2] + m_handlers.A[3]; }
	//RED_INLINE Float WB() const { return 3.f*m_handlers.A[0] - 6.f*m_handlers.A[1] + 3.f*m_handlers.A[2]; }
	//RED_INLINE Float WC() const { return -3.f*m_handlers.A[0] + 3.f*m_handlers.A[1]; }
	//RED_INLINE Float WD( Float t ) const { return m_handlers.A[0] - t; }

	//RED_INLINE Float Sqrt3( Float a ) const { const Float p = 1.f / 3.f; return powf( a, p ); }
	/*RED_INLINE Float GetTForX( Float t ) const
	{
		const Float a = WA();
		const Float b = WB();
		const Float c = WC();
		const Float d = WD( t );

		const Float delta = -4.f*c*c*c*a + c*c*b*b + 18.f*c*b*a*d - 27.f*d*d*a*a - 4.f*d*b*b*b;
		ASSERT( delta <= 0.f );

		const Float sqrt3 = Sqrt3( ( 9.f*c*b*a - 27.f*d*a*a - 2.f*b*b*b + 3*a*MSqrt( -3.f*delta ) ) / ( 2.f*a*a*a ) );

		const Float ret = ( 1.f / 3.f ) * ( sqrt3 ) - ( 3*c*a - b*b ) / ( 3.f*a*a* sqrt3 ) - ( b / ( 3.f*a ) );
		ASSERT( ret >= 0.f && ret <= 1.f );

		const Float part_1 = ( (-b*b*b) / (27.f*a*a*a) ) + ( (c*b) / (6.f*a*a) ) - ( (d) / (2.f*a) );
		const Float part_2_s_1 = ( (c) / (3.f*a) ) - ( (b*b) / (9.f*a*a) );
		const Float part_2 = part_1*part_1 + part_2_s_1*part_2_s_1*part_2_s_1;
		const Float partA = part_1 + MSqrt( part_2 );
		const Float partB = part_1 - MSqrt( part_2 );
		const Float ret2 = Sqrt3( partA ) + Sqrt3( partB ) - ( b / ( 3.f*a ) );

		ASSERT( ret2 >= 0.f && ret2 <= 1.f );

		return ret2;
	}*/

	RED_INLINE Float Calc( Float t, Float p0, Float p1, Float p2, Float p3 ) const { const Float t1=1.f-t; return t1*t1*t1*p0 + 3.f*t1*t1*t*p1 + 3.f*t1*t*t*p2 + t*t*t*p3; }
	RED_INLINE Float CalcSlope( Float t, Float p0, Float p1, Float p2, Float p3 ) const { const Float t1=1.f-t; return 3.f*t1*t1*(p1-p0) + 6.f*t1*t*(p2-p1) + 3.f*t*t*(p3-p2); }

	/*
	Finds value of t parameter such that B(t) is equal to specified x value.

	TODO: describe this more clearly

	\param x In range <0.0f, 1.0f> (should we use <0.0f, 1.0f) range instead?)
	\return Value of t parameter such that B(t) = P0x + xScaled * (P3x - P0x)
	*/
	RED_INLINE Float FindTForX( Float xScaled ) const
	{
		RED_ASSERT( xScaled >= 0.f && xScaled <= 1.f ); // shouldn't this be <0.0, 1.0f)?

		static Uint32 numIt = 20;

		// Get control points defining cubic Bezier 1d x(t).
		const Float p0 = m_points01.A[ 0 ];
		const Float p1 = m_points01.A[ 2 ];
		const Float p2 = m_points23.A[ 0 ];
		const Float p3 = m_points23.A[ 2 ];

		if ( p3 <= p0 )													// TODO: hm..
		{
			return xScaled;
		}

		Float guessT = xScaled;
		Float x = p0 + xScaled * ( p3 - p0 );

		for ( Uint32 i=0; i<numIt; ++i )
		{
			const Float slope = CalcSlope( guessT, p0, p1, p2, p3 );
			if ( slope == 0.f ) return guessT;							// TODO: hm...
			const Float currX = Calc( guessT, p0, p1, p2, p3 ) - x;
			guessT -= currX / slope;
		}

		RED_ASSERT( guessT >= 0.f && guessT <= 1.f );
		Clamp( guessT, 0.f , 1.f );

		return guessT;
	}
};

BEGIN_CLASS_RTTI( Bezier2D )
	PROPERTY( m_points01 );
	PROPERTY( m_points23 );
END_CLASS_RTTI();

class Bezier2DHandlerCalculator
{
public:
	static void CalcLinearFloat( Bezier2D& bezier, Float pointA, Float pointB, Float timeA, Float timeB, Float handlerWeight = 0.333f );
	static void CalcLinearAngle( Bezier2D& bezier, Float pointA, Float pointB, Float timeA, Float timeB, Float handlerWeight = 0.333f );

	static void SetLeft( Bezier2D& bezier, Float x, Float y );
	static void SetRight( Bezier2D& bezier, Float x, Float y );

	static void SetLeftWeight( Bezier2D& bezier, Float weight );
	static void SetRightWeight( Bezier2D& bezier, Float weight );
};
