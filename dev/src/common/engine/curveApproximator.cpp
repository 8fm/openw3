/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "curveApproximator.h"
#include "../core/curveBase.h"
#include "curve.h"

#if 0 // DEAD CODE!

// ** ************************************
//
CCurveApproximator::CCurveApproximator						( const CCurve * curve, Uint32 numSamples )
{
	GenerateApproximation( curve, numSamples );
}

// ** ************************************
//
CCurveApproximator::CCurveApproximator						()
{
	m_samples.PushBack( 0.f );
	m_samples.PushBack( 0.f );
}

// ** ************************************
//
CCurveApproximator::~CCurveApproximator						()
{
}

// ** ************************************
//
void	CCurveApproximator::GenerateApproximation			( const CCurve * curve, Uint32 numSamples )
{
	ASSERT( curve != NULL );
	
	if( numSamples < 2 )
		numSamples = 2;

	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( numSamples - 1 );

	m_samples.ClearFast();

	for( Uint32 i = 0; i < numSamples; ++i, t += dt )
	{
		m_samples.PushBack( curve->GetFloatValue( t ) );
	}
}

// ** ************************************
//
CCurveApproximatorParameter::CCurveApproximatorParameter	( const CurveParameter * curveParameter, Uint32 numSamples )
{
	GenerateApproximation( curveParameter, numSamples );
}

// ** ************************************
//
CCurveApproximatorParameter::CCurveApproximatorParameter	()
{
	m_count = 0;

	m_approximators[ 0 ] = NULL;
	m_approximators[ 1 ] = NULL;
	m_approximators[ 2 ] = NULL;
	m_approximators[ 3 ] = NULL;
}

// ** ************************************
//
CCurveApproximatorParameter::~CCurveApproximatorParameter	()
{
	DeleteApproximators();
}

// ** ************************************
//
void CCurveApproximatorParameter::GenerateApproximation		( const CurveParameter * curveParameter, Uint32 numSamples )
{
	ASSERT( curveParameter != NULL );

	DeleteApproximators();

	m_count = curveParameter->GetCurveCount();

	for( Uint32 i = 0; i < curveParameter->GetCurveCount(); ++i )
	{
		m_approximators[ i ] = new CCurveApproximator( curveParameter->GetCurve( i ), numSamples );
	}
}

// ** ************************************
//
void	CCurveApproximatorParameter::DeleteApproximators	()
{
	delete m_approximators[ 0 ];
	delete m_approximators[ 1 ];
	delete m_approximators[ 2 ];
	delete m_approximators[ 3 ];
}

// ** ************************************
//
CVecCurveApproximationParameter::CVecCurveApproximationParameter	( const CurveParameter * curveParameter, Uint32 numSamples )
{
	GenerateApproximation( curveParameter, numSamples );
}

// ** ************************************
//
CVecCurveApproximationParameter::CVecCurveApproximationParameter	()
{
}

// ** ************************************
//
CVecCurveApproximationParameter::~CVecCurveApproximationParameter	()
{
}

// ** ************************************
//
void	CVecCurveApproximationParameter::GenerateApproximation		( const CurveParameter * curveParameter, Uint32 numSamples )
{
	ASSERT( curveParameter != NULL );
	ASSERT( curveParameter->GetCurveCount() > 0 && curveParameter->GetCurveCount() <= 4 );

	m_samples.ClearFast();

	if( numSamples < 2 )
		numSamples = 2;

	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( numSamples - 1 );

	m_samples.ClearFast();

	for( Uint32 i = 0; i < numSamples; ++i, t += dt )
	{
		Vector val( 0.f, 0.f, 0.f, 0.f );

		// FIXME: zero fill this may not be the best way to initialize empty fields
		switch( curveParameter->GetCurveCount() )
		{
			case 4:
				val.A[ 3 ] = curveParameter->GetCurve( 3 )->GetFloatValue( t );
			case 3:
				val.A[ 2 ] = curveParameter->GetCurve( 2 )->GetFloatValue( t );
			case 2:
				val.A[ 1 ] = curveParameter->GetCurve( 1 )->GetFloatValue( t );
			case 1:
				val.A[ 0 ] = curveParameter->GetCurve( 0 )->GetFloatValue( t );
				break;
			default:
				ASSERT( false );
		}

		m_samples.PushBack( val );
	}
}

#endif