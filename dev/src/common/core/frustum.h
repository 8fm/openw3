//////////////////////////////////////
//          Inferno Engine          //
// Copyright (C) 2002-2006 by Dexio //
//////////////////////////////////////

#pragma once

#include "math.h"

enum EFrustumResult
{
	FR_Outside,
	FR_Intersecting,
	FR_Inside,
};

enum EFrustumPlane
{
	FP_Near,
	FP_Right,
	FP_Left,
	FP_Bottom,
	FP_Top,
	FP_Far,
	FP_Max,
};

/// View frustum 
RED_ALIGNED_CLASS( CFrustum, 16 )
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_Engine, 16 );

protected:
	__m128		m_planes[ FP_Max ];		//!< Planes
	__m128		m_masks[ FP_Max ];		//!< Masks per plane for faster computations
	
public:
	//! Get minimum distance to planes
	Float GetPointMinDistance( const Vector &pos ) const;

public:
	CFrustum() { }

	CFrustum( const Matrix& worldToScreen )
	{
		InitFromCamera( worldToScreen );
	}

	//! Initialize from matrix and position
	void InitFromCamera( const Matrix& worldToScreen );

	//! Test visibility of given bounding box
	RED_FORCE_INLINE EFrustumResult TestBox( const Box &box ) const;
	RED_FORCE_INLINE EFrustumResult TestBox( const Vector& bbMin, const Vector& bbMax ) const;
	RED_FORCE_INLINE EFrustumResult TestBox( const __m128& bbMin, const __m128& bbMax ) const;
	RED_INLINE Bool IsPointInside( const Vector& v ) const;

private:
	RED_INLINE static Int32 PointOnPlaneSide( const Vector* __restrict p, const __m128& plane );
};

#if defined ( RED_PLATFORM_CONSOLE )
#define BLENDV_PS( v1, v2, mask ) _mm_blendv_ps( v1, v2, mask )
#define DOT_PRODUCT( dpResult, v1, v2 ) dpResult = _mm_dp_ps( v1, v2, 0xF1 )
#else
#define BLENDV_PS( v1, v2, mask ) _mm_or_ps( _mm_and_ps( mask, v2 ), _mm_andnot_ps( mask, v1 ) )
#define DOT_PRODUCT( dpResult, v1, v2 ) \
{ \
	__m128 r1 = _mm_mul_ps( v1, v2 ); \
	__m128 r2 = _mm_hadd_ps( r1, r1 ); \
	dpResult = _mm_hadd_ps( r2, r2 ); \
}
#endif

RED_FORCE_INLINE Bool BoxBehindPlane( const __m128& bbMin, const __m128& bbMax, const __m128& mask, const __m128& plane, EFrustumResult& result )
{
	__m128 vmin = BLENDV_PS( bbMin, bbMax, mask );
	__m128 vmax = BLENDV_PS( bbMax, bbMin, mask );
	__m128 dpmin;
	DOT_PRODUCT( dpmin, vmin, plane );
	__m128 dpmax;
	DOT_PRODUCT( dpmax, vmax, plane );

	// dpMin = dot product of min vector, dpmax - dot product of max vector
	const __m128 zero = _mm_setzero_ps();
	__m128 xorRes = _mm_xor_ps( _mm_cmpge_ss( dpmin, zero ), _mm_cmplt_ss( dpmax, zero ) );
	int xorResBit = _mm_comilt_ss( xorRes, zero );
	if ( xorResBit == 0 )
	{
		result = FR_Intersecting;
		return false;
	}
	return _mm_comilt_ss( dpmax, zero ) > 0;
}

RED_FORCE_INLINE EFrustumResult CFrustum::TestBox( const __m128& bbMin, const __m128& bbMax ) const
{
	EFrustumResult result = FR_Inside;
	if (	BoxBehindPlane( bbMin, bbMax, m_masks[ 0 ], m_planes[ 0 ], result ) ||
			BoxBehindPlane( bbMin, bbMax, m_masks[ 1 ], m_planes[ 1 ], result ) ||
			BoxBehindPlane( bbMin, bbMax, m_masks[ 2 ], m_planes[ 2 ], result ) ||
			BoxBehindPlane( bbMin, bbMax, m_masks[ 3 ], m_planes[ 3 ], result ) ||
			BoxBehindPlane( bbMin, bbMax, m_masks[ 4 ], m_planes[ 4 ], result ) ||
			BoxBehindPlane( bbMin, bbMax, m_masks[ 5 ], m_planes[ 5 ], result ) )
	{
		return FR_Outside;
	}

	return result;
}

RED_FORCE_INLINE EFrustumResult CFrustum::TestBox( const Vector& bbMin, const Vector& bbMax ) const
{
	return TestBox( *(__m128*)&bbMin, *(__m128*)&bbMax );
}

RED_FORCE_INLINE Int32 CFrustum::PointOnPlaneSide( const Vector* __restrict p, const __m128& plane )
{
	__m128 distToP;
	DOT_PRODUCT( distToP, *(__m128*)p, plane );
	return _mm_comilt_ss( distToP, _mm_setzero_ps() );
}

RED_FORCE_INLINE EFrustumResult CFrustum::TestBox( const Box &box ) const
{	
	return TestBox( *(__m128*)box.Min.A, *(__m128*)box.Max.A );
}

RED_INLINE Bool CFrustum::IsPointInside( const Vector& v ) const
{
	RED_FATAL_ASSERT( ( reinterpret_cast< Uint64 >( &v ) & 15 ) == 0, "Provide point is not aligned on 16 byte." );

	for ( Uint32 i = 0; i < FP_Max; ++i )
	{
		if ( PointOnPlaneSide( &v, m_planes[ i ] ) > 0 )
		{
			return false;
		}
	}
	return true; 
}
