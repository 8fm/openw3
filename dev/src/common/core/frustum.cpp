/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "frustum.h"

void ComputeFrustumPlane( const EFrustumPlane plane, const Matrix& matrix, __m128& outPlane, __m128& outMask )
{
	Float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
	switch ( plane )
	{
	case FP_Near:
		{
			x = matrix.V[3].A[0] - matrix.V[2].A[0];
			y = matrix.V[3].A[1] - matrix.V[2].A[1];
			z = matrix.V[3].A[2] - matrix.V[2].A[2];
			w = matrix.V[3].A[3] - matrix.V[2].A[3];
			break;
		}
	case FP_Right:
		{
			x = matrix.V[3].A[0] - matrix.V[0].A[0];
			y = matrix.V[3].A[1] - matrix.V[0].A[1];
			z = matrix.V[3].A[2] - matrix.V[0].A[2];
			w = matrix.V[3].A[3] - matrix.V[0].A[3];
			break;
		}
	case FP_Left:
		{
			x = matrix.V[3].A[0] + matrix.V[0].A[0];
			y = matrix.V[3].A[1] + matrix.V[0].A[1];
			z = matrix.V[3].A[2] + matrix.V[0].A[2];
			w = matrix.V[3].A[3] + matrix.V[0].A[3];
			break;
		}
	case FP_Bottom:
		{
			x = matrix.V[3].A[0] + matrix.V[1].A[0];
			y = matrix.V[3].A[1] + matrix.V[1].A[1];
			z = matrix.V[3].A[2] + matrix.V[1].A[2];
			w = matrix.V[3].A[3] + matrix.V[1].A[3];
			break;
		}
	case FP_Top:
		{
			x = matrix.V[3].A[0] - matrix.V[1].A[0];
			y = matrix.V[3].A[1] - matrix.V[1].A[1];
			z = matrix.V[3].A[2] - matrix.V[1].A[2];
			w = matrix.V[3].A[3] - matrix.V[1].A[3];
			break;
		}
	case FP_Far:
		{
			x = matrix.V[3].A[0] + matrix.V[2].A[0];
			y = matrix.V[3].A[1] + matrix.V[2].A[1];
			z = matrix.V[3].A[2] + matrix.V[2].A[2];
			w = matrix.V[3].A[3] + matrix.V[2].A[3];
			break;
		}
	default:
		RED_FATAL_ASSERT( false, "Invalid FrustumPlane to calculate" );
	}

	// Normalize plane
	Float len = sqrt( x * x + y * y + z * z );
	outPlane = _mm_setr_ps( x / len, y / len, z / len, w / len );
	outMask = _mm_cmplt_ps( outPlane, _mm_setzero_ps() );
}

void CFrustum::InitFromCamera( const Matrix& matrix )
{
	Matrix w2sTransposed = matrix.Transposed();

	ComputeFrustumPlane( FP_Near,	w2sTransposed,	m_planes[ FP_Near ],	m_masks[ FP_Near ] );
	ComputeFrustumPlane( FP_Right,	w2sTransposed,	m_planes[ FP_Right ],	m_masks[ FP_Right ] );
	ComputeFrustumPlane( FP_Left,	w2sTransposed,	m_planes[ FP_Left ],	m_masks[ FP_Left ] );
	ComputeFrustumPlane( FP_Bottom,	w2sTransposed,	m_planes[ FP_Bottom ],	m_masks[ FP_Bottom ] );
	ComputeFrustumPlane( FP_Top,	w2sTransposed,	m_planes[ FP_Top ],		m_masks[ FP_Top ] );
	ComputeFrustumPlane( FP_Far,	w2sTransposed,	m_planes[ FP_Far ],		m_masks[ FP_Far ] );
}

Float CFrustum::GetPointMinDistance( const Vector &pos ) const
{
	__m128 testPos = _mm_set_ps( 1.0f, pos.Z, pos.Y, pos.X );
	__m128 tempDist;
	DOT_PRODUCT( tempDist, m_planes[0], testPos );
	Float minDist = _mm_cvtss_f32( tempDist );
	for ( Uint32 i = 1; i < FP_Max; ++i )
	{
		DOT_PRODUCT( tempDist, m_planes[ i ], testPos );
		minDist = Min( minDist, _mm_cvtss_f32( tempDist ) );
	}
	return minDist;
}
