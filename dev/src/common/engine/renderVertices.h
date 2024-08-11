/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../gpuApiUtils/gpuApiVertexFormats.h"

// Debug vertex, position only
struct DebugVertexBase : public GpuApi::SystemVertex_Pos
{
	RED_INLINE DebugVertexBase()
	{}

	RED_INLINE DebugVertexBase( const Vector& p )
		: GpuApi::SystemVertex_Pos( p.X, p.Y, p.Z )
	{
	}

	RED_INLINE void Set( const Vector& p )
	{
		x = p.X;
		y = p.Y;
		z = p.Z;		
	}

	RED_INLINE Vector GetPosVector() const
	{
		return Vector ( x, y, z );
	}
};

// Debug vertex, position and color only
struct DebugVertex : public GpuApi::SystemVertex_PosColor
{
	RED_INLINE DebugVertex()
	{}

	RED_INLINE DebugVertex( const Vector& p, const Color& inColor )
		: SystemVertex_PosColor( p.X, p.Y, p.Z, inColor.R | (inColor.G << 8) | (inColor.B << 16) | (inColor.A << 24) )
	{
	}

	RED_INLINE DebugVertex( const Vector& p, Uint32 inColor )
		: SystemVertex_PosColor( p.X, p.Y, p.Z, inColor )
	{
	}

	RED_INLINE void Set( const Vector& p, const Color& inColor )
	{
		x = p.X;
		y = p.Y;
		z = p.Z;
		color = inColor.ToUint32();
	}

	RED_INLINE void Set( const Vector& p, Uint32 inColor )
	{
		x = p.X;
		y = p.Y;
		z = p.Z;
		color = inColor;
	}

	RED_INLINE Bool operator==( const DebugVertex& v ) const
	{
		return x == v.x && y == v.y && z == v.z && color == v.color;
	}

	RED_FORCE_INLINE Uint32 CalcHash() const
	{
		const Uint32* _A = ( const Uint32* ) this;
		return _A[ 0 ] ^ _A[ 1 ] ^ _A[ 2 ] ^ _A[ 3 ];
	}
};

// Debug vertex with UV
struct DebugVertexUV : public GpuApi::SystemVertex_PosColorUV
{
	RED_INLINE DebugVertexUV()
	{}

	RED_INLINE DebugVertexUV( const Vector& p, const Color& inColor, Float inU, Float inV )
		: SystemVertex_PosColorUV( p.X, p.Y, p.Z, inColor.R | (inColor.G << 8) | (inColor.B << 16) | (inColor.A << 24), inU, inV )
	{
	}

	RED_INLINE void Set( const Vector& p, const Color& inColor, Float inU, Float inV )
	{
		x = p.X;
		y = p.Y;
		z = p.Z;
		color = inColor.ToUint32();
		u = inU;
		v = inV;
	}
};
