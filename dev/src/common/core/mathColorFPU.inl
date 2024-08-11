/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

Color::Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a/*=255*/ )
	: R( r )
	, G( g )
	, B( b )
	, A( a )
{
}

Color::Color( const Vector& x )
{
	R = (Uint8) ::Clamp( x.A[0] * 255.0f, 0.0f, 255.0f );
	G = (Uint8) ::Clamp( x.A[1] * 255.0f, 0.0f, 255.0f );
	B = (Uint8) ::Clamp( x.A[2] * 255.0f, 0.0f, 255.0f );
	A = (Uint8) ::Clamp( x.A[3] * 255.0f, 0.0f, 255.0f );
}

Color::Color( Uint32 x )
{
	R = (Uint8)(   x         & 0xff );
	G = (Uint8)( ( x >> 8  ) & 0xff );
	B = (Uint8)( ( x >> 16 ) & 0xff );
	A = (Uint8)( ( x >> 24 ) & 0xff );
}


Vector Color::ToVector() const
{
	return Vector( 
		R / 255.0f,
		G / 255.0f,
		B / 255.0f,
		A / 255.0f
	);
}

Vector Color::ToVectorLinear() const
{
	return Vector( 
		powf( R / 255.0f, 2.2f ),
		powf( G / 255.0f, 2.2f ),
		powf( B / 255.0f, 2.2f ),
		A / 255.0f
		);
}

Color Color::Mul3( const Color& a, Float b )
{
	RED_ASSERT( b >= 0.0f && b <= 1.0f );
	Color out;
	out.R = (Uint8) ::Clamp< Float >( a.R*b, 0.0f, 255.0f );
	out.G = (Uint8) ::Clamp< Float >( a.G*b, 0.0f, 255.0f );
	out.B = (Uint8) ::Clamp< Float >( a.B*b, 0.0f, 255.0f );
	out.A = a.A;
	return out;
}

Color Color::Mul4( const Color& a, Float b )
{
	RED_ASSERT( b >= 0.0f && b <= 1.0f );
	Color out;
	out.R = (Uint8) ::Clamp< Float >( a.R*b, 0.0f, 255.0f );
	out.G = (Uint8) ::Clamp< Float >( a.G*b, 0.0f, 255.0f );
	out.B = (Uint8) ::Clamp< Float >( a.B*b, 0.0f, 255.0f );
	out.A = (Uint8) ::Clamp< Float >( a.A*b, 0.0f, 255.0f );
	return out;
}

void Color::Mul3( Float b )
{
	RED_ASSERT( b >= 0.0f && b <= 1.0f );
	R = (Uint8) ::Clamp< Float >( R*b, 0.0f, 255.0f );
	G = (Uint8) ::Clamp< Float >( G*b, 0.0f, 255.0f );
	B = (Uint8) ::Clamp< Float >( B*b, 0.0f, 255.0f );
}

void Color::Mul4( Float b )
{
	RED_ASSERT( b >=0.0f && b <= 1.0f );
	R = (Uint8) ::Clamp< Float >( R*b, 0.0f, 255.0f );
	G = (Uint8) ::Clamp< Float >( G*b, 0.0f, 255.0f );
	B = (Uint8) ::Clamp< Float >( B*b, 0.0f, 255.0f );
	A = (Uint8) ::Clamp< Float >( A*b, 0.0f, 255.0f );
}

Color Color::Lerp( Float coef , const Color&a , const Color&b )
{
	RED_ASSERT( coef >=0.0f && coef <= 1.0f );
	Float one_minut_coef = 1.0f - coef;
	Color out;
	out.R = (Uint8) ::Clamp< Float >( a.R * one_minut_coef + b.R * coef, 0.0f, 255.0f );
	out.G = (Uint8) ::Clamp< Float >( a.G * one_minut_coef + b.G * coef, 0.0f, 255.0f );
	out.B = (Uint8) ::Clamp< Float >( a.B * one_minut_coef + b.B * coef, 0.0f, 255.0f );
	out.A = (Uint8) ::Clamp< Float >( a.A * one_minut_coef + b.A * coef, 0.0f, 255.0f );
	return out;
}
