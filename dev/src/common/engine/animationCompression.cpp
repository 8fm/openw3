/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "animationCompression.h"

IMPLEMENT_ENGINE_CLASS( IAnimationCompression );
IMPLEMENT_ENGINE_CLASS( CNoAnimationCompression );
IMPLEMENT_ENGINE_CLASS( CWaveletAnimationCompression );
IMPLEMENT_ENGINE_CLASS( CDeltaAnimationCompression );
IMPLEMENT_ENGINE_CLASS( CSplineAnimationCompression );
IMPLEMENT_ENGINE_CLASS( CAnimationFpsCompression );

//////////////////////////////////////////////////////////////////////////
// Wavelet compression

CWaveletAnimationCompression::CWaveletAnimationCompression()
	: m_quantizationBits( 8 )
	, m_positionTolerance( 0.01f )
	, m_rotationTolerance( 0.001f )
	, m_scaleTolerance( 0.01f )
{
}

//////////////////////////////////////////////////////////////////////////
// Delta compression

CDeltaAnimationCompression::CDeltaAnimationCompression()
	: m_quantizationBits( 8 )
	, m_positionTolerance( 0.01f )
	, m_rotationTolerance( 0.001f )
	, m_scaleTolerance( 0.01f )
{
}


//////////////////////////////////////////////////////////////////////////
// spline animation compression

CSplineAnimationCompression::CSplineAnimationCompression()
	: m_positionTolerance( 0.001f )
	, m_positionPolynomialDegree( 3 )
	, m_rotationTolerance( 0.001f )
	, m_rotationPolynomialDegree( 3 )
	, m_scaleTolerance( 0.001f )
	, m_scalePolynomialDegree( 3 )
	, m_floatTolerance( 0.001f )
	, m_floatPolynomialDegree( 3 )
{
}

void CSplineAnimationCompression::SetCompressionFactor( Float factor )
{
	ASSERT( factor <= 1.f && factor >= 0.f );

	static Float rotTolMin = 0.00001f;
	static Float rotTolMax = 1.f;

	m_rotationTolerance = rotTolMin + ( rotTolMax - rotTolMin ) * factor;
}

//////////////////////////////////////////////////////////////////////////
// fps animation compression

CAnimationFpsCompression::CAnimationFpsCompression()
	: m_fps( AF_30 )
{
}

void CAnimationFpsCompression::SetFps( EAnimationFps fps )
{
	m_fps = fps;
}

void CAnimationFpsCompression::SetCompressionFactor( Float factor )
{
	ASSERT( factor <= 1.f && factor >= 0.f );
	ASSERT( 0 );
}

namespace
{
	Int32 GetDivFactorFromEnum( EAnimationFps fps )
	{
		switch ( fps )
		{
		case AF_5:
			return 6;
		case AF_10:
			return 3;
		case AF_15:
			return 2;
		case  AF_30:
			return 1;
		default:
			ASSERT( 0 );
			return 1;
		};
	}
}
