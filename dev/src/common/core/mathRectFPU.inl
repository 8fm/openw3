/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Rect::Rect( const Box& box )
: m_left	( Int32( box.Min.X ) )
, m_top		( Int32( box.Min.Y ) )
, m_right	( Int32( box.Max.X ) )
, m_bottom	( Int32( box.Max.Y ) )
{
}

Rect::Rect( EResetState )
{
	Clear();
}


Rect::Rect( Int32 left, Int32 right, Int32 top, Int32 bottom)
: m_left( left )
, m_top( top )
, m_right( right )
, m_bottom( bottom )
{}

void Rect::Clear()
{
	m_left		= NumericLimits< Int32 >::Max();
	m_right		= NumericLimits< Int32 >::Min();
	m_top		= NumericLimits< Int32 >::Max();
	m_bottom	= NumericLimits< Int32 >::Min();
}

Bool Rect::IsEmpty() const
{
	return m_left >= m_right || m_top >= m_bottom;
}

void Rect::Translate( Int32 x, Int32 y )
{
	m_left		= x + m_left;
	m_right		= x + m_right;
	m_top		= y + m_top;
	m_bottom	= y + m_bottom;
}

Rect Rect::GetTranslated( Int32 x, Int32 y ) const
{
	Rect ret = *this;
	ret.Translate( x, y );
	return ret;
}

void Rect::Trim( const Rect& trimmerRect )
{
	m_left		= Max( m_left, trimmerRect.m_left );
	m_right		= Min( m_right, trimmerRect.m_right );
	m_top		= Max( m_top, trimmerRect.m_top );
	m_bottom	= Min( m_bottom, trimmerRect.m_bottom );
}

Rect Rect::GetTrimmed( const Rect& trimmerRect ) const
{
	Rect ret = *this;
	ret.Trim( trimmerRect );
	return ret;
}

void Rect::Grow( Int32 sx, Int32 sy )
{
	m_left -= sx;
	m_right += sx;
	m_top -= sy;
	m_bottom += sy;
}

Rect Rect::GetGrown( Int32 sx, Int32 sy ) const
{
	Rect ret = *this;
	ret.Grow( sx, sy );
	return ret;
}

void Rect::Add( const Rect& addRect )
{
	m_left		= Min( m_left,		addRect.m_left );
	m_right		= Max( m_right,		addRect.m_right );
	m_top		= Min( m_top,		addRect.m_top );
	m_bottom	= Max( m_bottom,	addRect.m_bottom );
}

Bool Rect::Intersects( const Rect& other ) const
{
	return !( m_left > other.m_right || m_right < other.m_left || m_bottom < other.m_top || m_top > other.m_bottom );
}

Bool Rect::Contains( const Rect& other ) const
{
	return m_left <= other.m_left && m_right >= other.m_right && m_top <= other.m_top && m_bottom >= other.m_bottom;
}


Bool Rect::GetIntersection( const Rect& r1, const Rect& r2, Rect& out )
{
	if ( !r1.Intersects( r2 ) )
	{
		return false;
	}

	out.m_left		= Max( r1.m_left,	r2.m_left );
	out.m_right		= Min( r1.m_right,	r2.m_right );
	out.m_top		= Max( r1.m_top,	r2.m_top );
	out.m_bottom	= Min( r1.m_bottom, r2.m_bottom );
	return true;
}

///////////////////////////////////////////////////
//  RectF
///////////////////////////////////////////////////

RectF::RectF( const Box& box )
: m_left	( box.Min.X )
, m_top		( box.Max.Y )
, m_right	( box.Max.X )
, m_bottom	( box.Min.Y )
{
}

RectF::RectF( Float left, Float right, Float top, Float bottom)
: m_left( left )
, m_top( top )
, m_right( right )
, m_bottom( bottom )
{}

Bool RectF::Intersects( const RectF& other ) const
{
	return !( m_left > other.m_right || m_right < other.m_left || m_bottom < other.m_top || m_top > other.m_bottom );
}

Bool RectF::GetIntersection( const RectF& r1, const RectF& r2, RectF& out )
{
	if ( !r1.Intersects( r2 ) )
	{
		return false;
	}

	out.m_left		= Max<Float>( r1.m_left,	r2.m_left );
	out.m_right		= Min<Float>( r1.m_right,	r2.m_right );
	out.m_top		= Max<Float>( r1.m_top,		r2.m_top );
	out.m_bottom	= Min<Float>( r1.m_bottom,	r2.m_bottom );
	return true;
}
