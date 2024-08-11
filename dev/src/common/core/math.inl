/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "mathFPU.inl"

Float DistanceBetweenAngles( Float a, Float b )
{
	Float dif = fmodf( b - a, 360.f );

	if ( dif > 180.f )
		return dif - 360.f;

	if ( dif < -180.f )
		return dif + 360.f;

	return dif;
}

Float DistanceBetweenAnglesAbs( Float a, Float b )
{
	Float dif = fmodf( b - a, 360.f );

	if ( dif < 0 )
		dif = -dif;

	if ( dif > 180.f )
		return 360.f - dif;

	return dif;
}

Float PointToLineDistanceSquared2( const Vector &lineRoot, const Vector &lineDirection, const Vector &point, Float &projectionAlpha )
{
	Float ta = Vector::Dot2( lineDirection, lineRoot );
	Float p  = Vector::Dot2( lineDirection, point );

	projectionAlpha = p - ta;

	Vector projected = lineRoot + lineDirection * projectionAlpha;
	return ( projected - point ).SquareMag2();
}

Float PointToLineDistanceSquared3( const Vector &lineRoot, const Vector &lineDirection, const Vector &point, Float &projectionAlpha )
{
	Float ta = Vector::Dot3( lineDirection, lineRoot );
	Float p  = Vector::Dot3( lineDirection, point );

	projectionAlpha = p - ta;

	Vector projected = lineRoot + lineDirection * projectionAlpha;
	return ( projected - point ).SquareMag3();
}

Float Vector3::Dot( const Vector3& v ) const
{
	return X*v.X + Y*v.Y + Z*v.Z;
}

Vector3 Vector3::Cross( const Vector3& v) const
{
	return Vector3
	(
		Y * v.Z - Z * v.Y,
		Z * v.X - X * v.Z,
		X * v.Y - Y * v.X
	);
}

Vector3::Vector3( const Vector2& v )
{
	X = v.X;
	Y = v.Y;
	Z = 0.f;
}

Vector2& Vector3::AsVector2()
{
	return reinterpret_cast< Vector2& >( *this );
}

const Vector2& Vector3::AsVector2() const
{
	return reinterpret_cast< const Vector2& >( *this );
}

Float Vector2::Dot( const Vector2& v ) const
{
	return X*v.X + Y*v.Y;
}
