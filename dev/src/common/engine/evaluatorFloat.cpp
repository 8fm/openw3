/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "evaluatorFloat.h"
#include "../core/dataError.h"
#include "game.h"
#include "baseEngine.h"
#include "utils.h"

IMPLEMENT_ENGINE_CLASS( IEvaluatorFloat );
IMPLEMENT_ENGINE_CLASS( CEvaluatorFloatConst );
IMPLEMENT_ENGINE_CLASS( CEvaluatorFloatRandomUniform );
IMPLEMENT_ENGINE_CLASS( CEvaluatorFloatRainStrength );
IMPLEMENT_ENGINE_CLASS( CEvaluatorFloatStartEnd );
IMPLEMENT_ENGINE_CLASS( CEvaluatorFloatCurve );

void CEvaluatorFloatConst::Evaluate( Float x, Float& result ) const
{
	result = m_value;
}


void CEvaluatorFloatRainStrength::Evaluate( Float x, Float& result ) const
{
	THandle< CWorld > world = GGame ? GGame->GetActiveWorld() : nullptr;
	if ( world )
	{
		result = m_valueMultiplier * 0.0f;
		return;
	}

	result = 0.0f;
}


Bool CEvaluatorFloatConst::GetApproximationSamples( TDynArray< Float >& samples ) const
{
	samples.Clear();
	samples.PushBack( m_value );
	samples.PushBack( m_value );
	return true;
}

void CEvaluatorFloatRandomUniform::Evaluate( Float x, Float& result ) const
{
	result = GEngine->GetRandomNumberGenerator().Get< Float >( m_min , m_max );
}

Bool CEvaluatorFloatRandomUniform::GetApproximationSamples( TDynArray< Float >& samples ) const
{
	samples.Clear();
	samples.PushBack( m_min );
	samples.PushBack( m_max );
	return true;
}

void CEvaluatorFloatRandomUniform::OnPropertyPostChange( IProperty* property )
{
	IEvaluatorFloat::OnPropertyPostChange( property );

	if( m_min > m_max )
	{
		Swap( m_min, m_max );
	}

	if ( m_max == m_min )
	{
		DATA_HALT( DES_Major, GetResource(), TXT( "Random Numbers" ), TXT( "Min must be smaller than max, %f < %f" ), m_min, m_max );
	}
}

void CEvaluatorFloatRandomUniform::OnPostLoad()
{
	IEvaluatorFloat::OnPostLoad();

	if ( m_max < m_min )
	{
		DATA_HALT( DES_Major, GetResource(), TXT( "Random Numbers" ), TXT( "Min cannot be larger than max %f < %f" ), m_min, m_max );
	}

	if ( m_max == m_min )
	{
		DATA_HALT( DES_Major, GetResource(), TXT( "Random Numbers" ), TXT( "Min must be smaller than max, %f < %f" ), m_min, m_max );
	}
}

const CResource* CEvaluatorFloatRandomUniform::GetResource() const
{
	return CResourceObtainer::GetResource( GetParent() );
}

void CEvaluatorFloatStartEnd::Evaluate( Float x, Float& result ) const
{
	const Float clampedX = ::Clamp( x, 0.0f, 1.0f );
	result = m_start + ( clampedX * ( m_end - m_start ) );
}

Bool CEvaluatorFloatStartEnd::GetApproximationSamples( TDynArray< Float >& samples ) const
{
	samples.ClearFast();
	samples.PushBack( m_start );
	samples.PushBack( m_end );
	return true;
}

CEvaluatorFloatCurve::CEvaluatorFloatCurve()
{
#ifndef NO_EDITOR
	CName curveName( TXT("Curve") );
	m_curves.CreateCurves( curveName, 1, this );
#endif
}

void CEvaluatorFloatCurve::Evaluate( Float x, Float& result ) const
{
	result = m_curves.GetCurveValue( 0, x );
}

// ** ******************************
//
void CEvaluatorFloatCurve::OnSerialize( IFile & file )
{
	TBaseClass::OnSerialize( file );

	file << m_curves;
}

#define NUM_CURVE_SAMPLES	16

Bool CEvaluatorFloatCurve::GetApproximationSamples( TDynArray< Float >& samples ) const
{
	samples.ClearFast();

	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( NUM_CURVE_SAMPLES - 1 );

	for( Uint32 i = 0; i < NUM_CURVE_SAMPLES; ++i, t += dt )
	{
		samples.PushBack( m_curves.GetCurveValue( 0, t ) );
	}
	return true;
}
