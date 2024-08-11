/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Point::Point( Float x, Float y, Float z )
	: Vector( x, y, z )
{
}

Point::Point( const Float* f )
	: Vector( f )
{
}

Point::Point( const Point &a )
	: Vector( a )
{
}

Point::Point( const Vector &a )
: Vector( a )
{
}

Point Point::operator-() const
{
	return Vector::Negated();
}

Point Point::operator+( const Point& a ) const
{
	return Vector::Add4( *this, a );
}

Point Point::operator-( const Point& a ) const
{
	return Vector::Sub4( *this, a );
}

Point Point::operator*( const Point& a ) const
{
	return Vector::Mul4( *this, a );
}

Point Point::operator/( const Point& a ) const
{
	return Vector::Div4( *this, a );
}

Point Point::operator+( Float a ) const
{
	return Vector::Add4( *this, a );
}

Point Point::operator-( Float a ) const
{
	return Vector::Sub4( *this, a );
}

Point Point::operator*( Float a ) const
{
	return Vector::Mul4( *this, a );
}

Point Point::operator/( Float a ) const
{
	return Vector::Div4( *this, a );
}

Point& Point::operator+=( const Point& a )
{
	Vector::Add4( a );
	return *this;
}

Point& Point::operator-=( const Point& a )
{
	Vector::Sub4( a );
	return *this;
}

Point& Point::operator*=( const Point& a )
{
	Vector::Mul4( a );
	return *this;
}

Point& Point::operator/=( const Point& a )
{
	Vector::Div4( a );
	return *this;
}

Point& Point::operator+=( Float a )
{
	Vector::Add4( a );
	return *this;
}

Point& Point::operator-=( Float a )
{
	Vector::Sub4( a );
	return *this;
}

Point& Point::operator*=( Float a )
{
	Vector::Mul4( a );
	return *this;
}

Point& Point::operator/=( Float a )
{
	Vector::Div4( a );
	return *this;
}

Bool Point::operator==( const Point& a ) const
{
	return Vector::Equal4( *this, a );
}

Bool Point::operator!=( const Point& a ) const
{
	return !Vector::Equal4( *this, a );
}