/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */
#pragma once

Plane::Plane( const Vector& normal, const Float& distance )
{
	NormalDistance.A[0] = normal.A[0];
	NormalDistance.A[1] = normal.A[1];
	NormalDistance.A[2] = normal.A[2];
	NormalDistance.A[3] = -distance;
}


void Plane::SetPlane( const Vector& normal, const Vector& point )
{
	RED_ASSERT( normal.Mag3() > 0.995f &&  normal.Mag3() < 1.005f );
	NormalDistance.A[0] = normal.A[0];
	NormalDistance.A[1] = normal.A[1];
	NormalDistance.A[2] = normal.A[2];
	NormalDistance.A[3] = -Vector::Dot3( point, normal );
}

Plane::Plane( const Vector& normal, const Vector& point )
{
	SetPlane( normal, point );
}

Plane::Plane( const Vector& p1, const Vector& p2, const Vector& p3 )
{
	SetPlane( p1, p2, p3 );
}

Float Plane::DistanceTo( const Vector& point ) const
{
	return Vector::Dot3( point, NormalDistance ) + NormalDistance.A[3];
}

Float Plane::DistanceTo( const Vector& plane, const Vector& point )
{
	return Vector::Dot3( point, plane ) + plane.A[3];
}

Plane::ESide Plane::GetSide( const Vector& point ) const
{
	Float distance = DistanceTo( point );

	if ( distance < 0.0f )
	{
		return Plane::PS_Back;
	}
	if ( distance > 0.0f )
	{
		return Plane::PS_Front;
	}
	return Plane::PS_None;
}

Plane::ESide Plane::GetSide( const Box& box ) const
{
	const Vector boxCenter = box.CalcCenter();
	const Vector boxExtents = box.CalcExtents();
	return GetSide( boxCenter, boxExtents );
}

Plane::ESide Plane::GetSide( const Vector& boxCenter, const Vector& boxExtents ) const
{
	Float distance = DistanceTo( boxCenter );

	Float maxDistance = 0.0f;
	maxDistance += Abs( NormalDistance.A[0] * boxExtents.A[0] );
	maxDistance += Abs( NormalDistance.A[1] * boxExtents.A[1] );
	maxDistance += Abs( NormalDistance.A[2] * boxExtents.A[2] );

	if ( distance < -maxDistance )
	{
		return Plane::PS_Back;
	}
	if ( distance > +maxDistance )
	{
		return Plane::PS_Front;
	}
	return Plane::PS_Both;
}

RED_INLINE Vector Plane::Project( const Vector& point ) const
{
	return point - NormalDistance * DistanceTo( point );
}

RED_INLINE Bool Plane::IntersectRay( const Vector& origin, const Vector& direction, Vector& intersectionPoint, Float &intersectionDistance ) const
{
	Float proj = -Vector::Dot3( NormalDistance, direction );
	if ( proj > 0.0f )
	{
		intersectionDistance = DistanceTo( origin ) / proj;
		intersectionPoint = origin + ( direction * intersectionDistance );
		return true;
	}

	return false;
}

RED_INLINE Plane Plane::operator-() const
{
	Plane ret;
	ret.NormalDistance = -NormalDistance;
	return ret;
}

RED_INLINE void Plane::SetPlane( const Vector& p1, const Vector& p2, const Vector& p3 )
{
	NormalDistance = Vector::Cross( p2-p1, p3-p1 );
	NormalDistance.Normalize3();
	NormalDistance.A[3] = -Vector::Dot3( p1, NormalDistance );
}

