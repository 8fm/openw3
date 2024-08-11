#include "build.h"

#include "bezier2d.h"

IMPLEMENT_RTTI_CLASS( Bezier2D );

void Bezier2DHandlerCalculator::CalcLinearFloat( Bezier2D& bezier, Float pointA, Float pointB, Float timeA, Float timeB, Float handlerWeight )
{
	RED_ASSERT( handlerWeight >= 0.f && handlerWeight <= 1.f );

	Vector2 pA( timeA, pointA );
	Vector2 pB( timeB, pointB );

	Vector2 dir = pB - pA;
	dir.Normalize();
	dir *= handlerWeight;

	SetLeft( bezier, -dir.X, -dir.Y );
	SetRight( bezier, dir.X, dir.Y );
}

void Bezier2DHandlerCalculator::CalcLinearAngle( Bezier2D& bezier, Float pointA, Float pointB, Float timeA, Float timeB, Float handlerWeight )
{
	RED_ASSERT( handlerWeight >= 0.f && handlerWeight <= 1.f );

	Vector2 pA( timeA, pointA );
	Vector2 pB( timeB, pointB );

	Vector2 dir = pB - pA;
	dir.Y = EulerAngles::NormalizeAngle180( EulerAngles::AngleDistance( pointA, pointB ) );
	dir.Normalize();
	dir *= handlerWeight;

	SetLeft( bezier, -dir.X, -dir.Y );
	SetRight( bezier, dir.X, dir.Y );
}

void Bezier2DHandlerCalculator::SetLeft( Bezier2D& bezier, Float x, Float y )
{
	const Float p0x = bezier.m_points01.A[ 0 ];
	const Float p0y = bezier.m_points01.A[ 1 ];
	Float& p1x = bezier.m_points01.A[ 2 ];
	Float& p1y = bezier.m_points01.A[ 3 ];

	p1x = p0x + x;
	p1y = p0y + y;
}

void Bezier2DHandlerCalculator::SetRight( Bezier2D& bezier, Float x, Float y )
{
	const Float p3x = bezier.m_points23.A[ 2 ];
	const Float p3y = bezier.m_points23.A[ 3 ];
	Float& p2x = bezier.m_points23.A[ 0 ];
	Float& p2y = bezier.m_points23.A[ 1 ];

	p2x = p3x + x;
	p2y = p3y + y;
}

void Bezier2DHandlerCalculator::SetLeftWeight( Bezier2D& bezier, Float weight )
{
	RED_ASSERT( weight >= 0.f && weight <= 1.f );

	const Float p0x = bezier.m_points01.A[ 0 ];
	const Float p0y = bezier.m_points01.A[ 1 ];
	const Float p1x = bezier.m_points01.A[ 2 ];
	const Float p1y = bezier.m_points01.A[ 3 ];

	Vector2 dir( p1x - p0x, p1y - p0y );
	dir.Normalize();
	dir *= weight;

	SetLeft( bezier, p0x + dir.X, p0y + dir.Y );
}

void Bezier2DHandlerCalculator::SetRightWeight( Bezier2D& bezier, Float weight )
{
	RED_ASSERT( weight >= 0.f && weight <= 1.f );

	const Float p2x = bezier.m_points23.A[ 0 ];
	const Float p2y = bezier.m_points23.A[ 1 ];
	const Float p3x = bezier.m_points23.A[ 2 ];
	const Float p3y = bezier.m_points23.A[ 3 ];

	Vector2 dir( p2x - p3x, p2y - p3y );
	dir.Normalize();
	dir *= weight;

	SetRight( bezier, p3x + dir.X, p3y + dir.Y );
}
