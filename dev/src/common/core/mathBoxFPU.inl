/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "numericLimits.h"

Box::Box( const Box& rhs )
	: Min( rhs.Min )
	, Max( rhs.Max )
{
}

Box::Box( const Vector& min, const Vector& max )
	: Min( min )
	, Max( max )
{
	RED_ASSERT( min.X <= max.X, TXT( "Creating Box with min.X > max.X") );
	RED_ASSERT( min.Y <= max.Y, TXT( "Creating Box with min.Y > max.Y") );
	RED_ASSERT( min.Z <= max.Z, TXT( "Creating Box with min.Z > max.Z") );
}

Box::Box( const Vector& center, float radius )
	: Min( center - Vector( radius, radius, radius, 0 ) )
	, Max( center + Vector( radius, radius, radius, 0 ) )
{
}

Box::Box( EResetState )
	: Min( FLT_MAX, FLT_MAX, FLT_MAX )
	, Max( -FLT_MAX, -FLT_MAX, -FLT_MAX )
{

}

Box::Box( EMaximize )
	: Min( -FLT_MAX, -FLT_MAX, -FLT_MAX )
	, Max( FLT_MAX, FLT_MAX, FLT_MAX )
{

}

Bool Box::operator==( const Box& box ) const
{
	return Vector::Equal3( Min, box.Min ) && Vector::Equal3( Max, box.Max );
}

Bool Box::operator!=( const Box& box ) const
{
	return !( Vector::Equal3( Min, box.Min ) && Vector::Equal3( Max, box.Max ) );
}

Box Box::operator+( const Vector& dir ) const
{
	return Box( Min + dir, Max + dir );
}

Box Box::operator-( const Vector& dir ) const
{
	return Box( Min - dir, Max - dir );
}

Box Box::operator*( const Vector& scale ) const
{
	return Box( Min * scale, Max * scale );
}

Box& Box::operator=( const Box& rhs )
{
	Min.Set4( rhs.Min );
	Max.Set4( rhs.Max );
	return *this;
}

Box& Box::operator+=( const Vector& dir )
{
	Min += dir;
	Max += dir;
	return *this;
}

Box& Box::operator-=( const Vector& dir )
{
	Min -= dir;
	Max -= dir;
	return *this;
}

Box& Box::operator*=( const Vector& scale )
{
	Min *= scale;
	Max *= scale;
	return *this;
}

Box& Box::Clear()
{
	Min = Vector( FLT_MAX, FLT_MAX, FLT_MAX );
	Max = Vector( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	return *this;
}

Bool Box::IntersectSegment( const Segment& segment, Vector& enterPoint )
{
	Float t;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t );
	if ( ret && t * t <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t;
		return true;
	}
	return false;
}

Bool Box::IntersectSegment( const Segment& segment, Vector& enterPoint, Vector& exitPoint )
{
	Float t1, t2;
	Vector normal = segment.m_direction.Normalized3();
	Bool ret = IntersectRay( segment.m_origin, normal, t1, &t2 );
	if ( ret && t1 * t1 <= segment.m_direction.SquareMag3() )
	{
		enterPoint = segment.m_origin + normal * t1;
		exitPoint = segment.m_origin + normal * t2;
		return true;
	}
	return false;
}

Bool Box::IntersectRay( const Vector& origin, const Vector& direction, Float& enterDistFromOrigin, Float* exitDistFromOrigin ) const
{
	Float tmin = -NumericLimits<Float>::Infinity(), tmax = NumericLimits<Float>::Infinity();

	if ( direction.X != 0.0f ) {
		Float reverse = 1.0f / direction.X;
		Float tx1 = ( Min.X - origin.X ) * reverse;
		Float tx2 = ( Max.X - origin.X ) * reverse;

		tmin = Red::Math::NumericalUtils::Min( tx1, tx2 );
		tmax = Red::Math::NumericalUtils::Max( tx1, tx2 );
	} else {
		if ( origin.X <= Min.X || origin.X >= Max.X ) return false;
	}

	if ( direction.Y != 0.0f ) {
		Float reverse = 1.0f / direction.Y;
		Float tx1 = ( Min.Y - origin.Y ) * reverse;
		Float tx2 = ( Max.Y - origin.Y ) * reverse;

		tmin = Red::Math::NumericalUtils::Max( tmin, Red::Math::NumericalUtils::Min( tx1, tx2 ) );
		tmax = Red::Math::NumericalUtils::Min( tmax, Red::Math::NumericalUtils::Max( tx1, tx2 ) );
	} else {
		if ( origin.Y <= Min.Y || origin.Y >= Max.X ) return false;
	}

	if ( direction.Z != 0.0f ) {
		Float reverse = 1.0f / direction.Z;
		Float tx1 = ( Min.Z - origin.Z ) * reverse;
		Float tx2 = ( Max.Z - origin.Z ) * reverse;

		tmin = Red::Math::NumericalUtils::Max( tmin, Red::Math::NumericalUtils::Min( tx1, tx2 ) );
		tmax = Red::Math::NumericalUtils::Min( tmax, Red::Math::NumericalUtils::Max( tx1, tx2 ) );
	} else {
		if ( origin.Z <= Min.Z || origin.Z >= Max.Z ) return false;
	}
	if ( tmax < 0 ) return false;
	if ( tmin < 0 ) tmin = 0;

	enterDistFromOrigin		= tmin;
	if ( exitDistFromOrigin )
	{
		*exitDistFromOrigin		= tmax;
	}
	return tmax >= tmin;
}

Bool Box::IntersectRay( const Vector& origin, const Vector& direction, Vector& enterPoint ) const
{
	Float t;
	Bool ret = IntersectRay( origin, direction, t);
	enterPoint = origin + direction * t;
	return ret;
}

Bool Box::IntersectSphere( const Sphere& sphere ) const
{
	const Vector& sphereCenter = sphere.GetCenter();
	Float distanceMinSqr = 0;

	for ( Uint32 i = 0; i < 3; ++i )
	{
		if ( sphereCenter.A[i] < Min.A[i] )
		{
			const Float axisDistanceSqr = sphereCenter.A[i] - Min.A[i];
			distanceMinSqr += axisDistanceSqr * axisDistanceSqr;
		}
		else if ( sphereCenter.A[i] > Max.A[i] )
		{
			const Float axisDistanceSqr = sphereCenter.A[i] - Max.A[i];
			distanceMinSqr += axisDistanceSqr * axisDistanceSqr;
		}
	}

	return distanceMinSqr <= sphere.GetSquareRadius();
}

Bool Box::Contains( const Vector& point ) const
{
	return point.X >= Min.X && point.Y >= Min.Y && point.Z >= Min.Z &&
			point.X <= Max.X && point.Y <= Max.Y && point.Z <= Max.Z;
}

Bool Box::Contains2D( const Vector3& point ) const
{
	return point.X >= Min.X && point.Y >= Min.Y &&
		point.X <= Max.X && point.Y <= Max.Y;
}

Bool Box::Contains( const Vector3& point, Float zExt ) const
{
	const Float step = Max.Z - Min.Z;
	Vector v = point;

	for ( Float z = 0.f; z <= zExt; z += step )
	{
		v.Z += z;
		if ( Contains( v ) )
		{
			return true;
		}
	}

	v.Z = point.Z + zExt;
	return Contains( v );
}

Bool Box::Contains( const Box& box ) const
{
	return
		box.Min.X >= Min.X && box.Min.Y >= Min.Y && box.Min.Z >= Min.Z &&
		box.Max.X <= Max.X && box.Max.Y <= Max.Y && box.Max.Z <= Max.Z;
}

Bool Box::Contains2D( const Box& box ) const
{
	return
		box.Min.X >= Min.X && box.Min.Y >= Min.Y &&
		box.Max.X <= Max.X && box.Max.Y <= Max.Y;
}

Bool Box::ContainsExcludeEdges( const Vector& point ) const
{
	if ( point.X > Min.X && point.Y > Min.Y && point.Z > Min.Z &&
			point.X < Max.X && point.Y < Max.Y && point.Z < Max.Z )
	{
		return true;
	}

	return false;
}

Bool Box::Touches( const Box& box ) const
{
	return
		box.Max.X >= Min.X && box.Max.Y >= Min.Y && box.Max.Z >= Min.Z &&
		box.Min.X <= Max.X && box.Min.Y <= Max.Y && box.Min.Z <= Max.Z;
}

Bool Box::Touches( const Vector3& bMin, const Vector3& bMax ) const
{
	return
		bMax.X >= Min.X && bMax.Y >= Min.Y && bMax.Z >= Min.Z &&
		bMin.X <= Max.X && bMin.Y <= Max.Y && bMin.Z <= Max.Z;
}

Bool Box::Touches2D( const Box& box ) const
{
	return
		box.Max.X >= Min.X && box.Max.Y >= Min.Y &&
		box.Min.X <= Max.X && box.Min.Y <= Max.Y;
}

Box& Box::AddPoint( const Vector& point )
{
	Min = Vector::Min4( Min, point );
	Max = Vector::Max4( Max, point );
	return *this;
}

Box& Box::AddPoint( const Vector3& point )
{
	Min.X = ::Min( Min.X, point.X );
	Min.Y = ::Min( Min.Y, point.Y );
	Min.Z = ::Min( Min.Z, point.Z );

	Max.X = ::Max( Max.X, point.X );
	Max.Y = ::Max( Max.Y, point.Y );
	Max.Z = ::Max( Max.Z, point.Z );

	return *this;
}

Box& Box::AddBox( const Box& box )
{
	Min = Vector::Min4( Min, box.Min);
	Max = Vector::Max4( Max, box.Max );
	return *this;
}

Bool Box::IsEmpty() const
{
	return (Max.X <= Min.X) || (Max.Y <= Min.Y) || (Max.Z <= Min.Z);
}

void Box::CalcCorners( Vector* corners ) const
{
	corners[0] = Vector( Min.X, Min.Y, Min.Z );
	corners[1] = Vector( Max.X, Min.Y, Min.Z );
	corners[2] = Vector( Min.X, Max.Y, Min.Z );
	corners[3] = Vector( Max.X, Max.Y, Min.Z );
	corners[4] = Vector( Min.X, Min.Y, Max.Z );
	corners[5] = Vector( Max.X, Min.Y, Max.Z );
	corners[6] = Vector( Min.X, Max.Y, Max.Z );
	corners[7] = Vector( Max.X, Max.Y, Max.Z );
}

Vector Box::CalcCenter() const
{
	return ( Max + Min ) * 0.5f;
}

Vector Box::CalcExtents() const
{
	return ( Max - Min ) * 0.5f;
}

Vector Box::CalcSize() const
{
	return ( Max - Min );
}

Box& Box::Extrude( const Vector& dir )
{
	Max += dir;
	Min -= dir;
	return *this;
}

Box& Box::Extrude( const Float value )
{
	Max += value;
	Min -= value;
	return *this;
}

void Box::Crop( const Box& box )
{
	Min = Vector::Min4( Vector::Max4( Min, box.Min ), box.Max );
	Max = Vector::Min4( Vector::Max4( Max, box.Min ), box.Max );
}

Box& Box::Normalize( const Box& unitBox )
{
	RED_ASSERT( !unitBox.IsEmpty() );

	Vector size = Vector::ONES / unitBox.CalcSize();
	Min = ( Min - unitBox.Min ) * size;
	Max = ( Max - unitBox.Min ) * size;
	return *this;
}

Float Box::Distance( const Vector& pos ) const
{
	Float sqrsum = 0;
	if ( pos.X < Min.X ) sqrsum += (Min.X - pos.X)*(Min.X - pos.X);
	if ( pos.X > Max.X ) sqrsum += (Max.X - pos.X)*(Max.X - pos.X);
	if ( pos.Y < Min.Y ) sqrsum += (Min.Y - pos.Y)*(Min.Y - pos.Y);
	if ( pos.Y > Max.Y ) sqrsum += (Max.Y - pos.Y)*(Max.Y - pos.Y);
	if ( pos.Z < Min.Z ) sqrsum += (Min.Z - pos.Z)*(Min.Z - pos.Z);
	if ( pos.Z > Max.Z ) sqrsum += (Max.Z - pos.Z)*(Max.Z - pos.Z);
	return sqrt(sqrsum);
}

Float Box::SquaredDistance( const Vector& pos ) const
{
	Float sqrsum = 0.0f;
	if ( pos.X < Min.X ) sqrsum += (Min.X - pos.X)*(Min.X - pos.X);
	if ( pos.X > Max.X ) sqrsum += (Max.X - pos.X)*(Max.X - pos.X);
	if ( pos.Y < Min.Y ) sqrsum += (Min.Y - pos.Y)*(Min.Y - pos.Y);
	if ( pos.Y > Max.Y ) sqrsum += (Max.Y - pos.Y)*(Max.Y - pos.Y);
	if ( pos.Z < Min.Z ) sqrsum += (Min.Z - pos.Z)*(Min.Z - pos.Z);
	if ( pos.Z > Max.Z ) sqrsum += (Max.Z - pos.Z)*(Max.Z - pos.Z);
	return sqrsum;
}

Float Box::SquaredDistance2D( const Vector& pos ) const
{
	Float sqrsum = 0.0f;
	if ( pos.X < Min.X ) sqrsum += (Min.X - pos.X)*(Min.X - pos.X);
	if ( pos.X > Max.X ) sqrsum += (Max.X - pos.X)*(Max.X - pos.X);
	if ( pos.Y < Min.Y ) sqrsum += (Min.Y - pos.Y)*(Min.Y - pos.Y);
	if ( pos.Y > Max.Y ) sqrsum += (Max.Y - pos.Y)*(Max.Y - pos.Y);
	return sqrsum;
}


Float Box::SquaredDistance( const Box& box ) const
{
	Float accumulator = 0.f;			// accumulate squared root distance

	// for every dimmension
	for ( Uint32 i = 0; i < 3; ++i )
	{
		// check 1d distance boxes have at given dimmension
		if ( box.Max.A[ i ] < Min.A[ i ] )
		{
			Float d = Min.A[ i ] - box.Max.A[ i ];
			accumulator += d*d;
		}
		else if ( box.Min.A[ i ] > Max.A[ i ] )
		{
			Float d = box.Min.A[ i ]  - Max.A[ i ];
			accumulator += d*d;
		}
	}

	return accumulator;
}
