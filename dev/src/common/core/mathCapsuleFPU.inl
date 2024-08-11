/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

FixedCapsule::FixedCapsule( const Vector& point, Float radius, Float height )
{
	Set( point, radius, height );	
}

void FixedCapsule::Set( const Vector& point, Float radius, Float height )
{
	PointRadius = point;

	PointRadius.W = radius;

	Height = height;
}

const Vector& FixedCapsule::GetPosition() const
{
	return PointRadius;
}

Vector FixedCapsule::CalcPointA() const
{
	Vector vec = PointRadius;
	vec.Z += GetRadius();
	return vec;
}

Vector FixedCapsule::CalcPointB() const
{
	Vector vec = PointRadius;
	vec.Z += GetHeight() - GetRadius();
	return vec;
}

Float FixedCapsule::GetRadius() const
{
	return PointRadius.W;
}

Float FixedCapsule::GetHeight() const
{
	return Height;
}

Bool FixedCapsule::Contains( const Vector& point ) const
{
	Float r2 =  GetRadius() *  GetRadius();

	// TODO: optimalization
	Float distSqr = 0.f;
	{
		Vector pointA = CalcPointA();
		Vector pointB = CalcPointB();

		Vector v = point - pointA;

		Vector s = pointB - pointA;

		Float lenSq = s.SquareMag3();
		Float dot = v.Dot3( s ) / lenSq;

		Vector disp = s * dot;

		if ( ( dot > 1.f || dot < 0.f ) && point.DistanceSquaredTo( pointA ) > r2 && point.DistanceSquaredTo( pointB ) > r2 )
		{
			return false;
		}

		v -= disp;

		distSqr = v.SquareMag3();
	}

	return distSqr <= r2;
}

Bool FixedCapsule::Contains( const Sphere& sphere ) const
{
	Float r = sphere.GetRadius() + GetRadius();
	Float r2 = r * r;

	// TODO: optimalization
	Float distSqr = 0.f;
	{
		Vector pointA = CalcPointA();
		Vector pointB = CalcPointB();

		Vector v = sphere.GetCenter() - pointA;

		Vector s = pointB - pointA;

		Float lenSq = s.SquareMag3();
		Float dot = v.Dot3( s ) / lenSq;
		
		Vector disp = s * dot;

		if ( ( dot > 1.f || dot < 0.f ) && sphere.GetCenter().DistanceSquaredTo( pointA ) > r2 && sphere.GetCenter().DistanceSquaredTo( pointB ) > r2 )
		{
			return false;
		}

		v -= disp;

		distSqr = v.SquareMag3();
	}

	return distSqr <= r2;
}

FixedCapsule FixedCapsule::operator+( const Vector& dir ) const
{
	return FixedCapsule( PointRadius + dir, GetRadius(), GetHeight() );
}

FixedCapsule FixedCapsule::operator-( const Vector& dir ) const
{
	return FixedCapsule( PointRadius - dir, GetRadius(), GetHeight() );
}

FixedCapsule& FixedCapsule::operator+=( const Vector& dir )
{
	PointRadius.A[0] += dir.A[0];
	PointRadius.A[1] += dir.A[1];
	PointRadius.A[2] += dir.A[2];
	return *this;
}

FixedCapsule& FixedCapsule::operator-=( const Vector& dir )
{
	PointRadius.A[0] -= dir.A[0];
	PointRadius.A[1] -= dir.A[1];
	PointRadius.A[2] -= dir.A[2];
	return *this;
}
