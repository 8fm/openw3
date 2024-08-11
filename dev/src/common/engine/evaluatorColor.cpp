/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "evaluatorColor.h"
#include "curve.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( IEvaluatorColor );
IMPLEMENT_ENGINE_CLASS( CEvaluatorColorConst );
IMPLEMENT_ENGINE_CLASS( CEvaluatorColorStartEnd );
IMPLEMENT_ENGINE_CLASS( CEvaluatorColorCurve );
IMPLEMENT_ENGINE_CLASS( CEvaluatorColorRandom );

CEvaluatorColorRandom::CEvaluatorColorRandom()
{
	Vector col
		(
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >()
		);
	m_value = Color( col );
};
CEvaluatorColorRandom::CEvaluatorColorRandom( CObject* parent, const Color& value )
	: m_value( value )
{
	SetParent( parent );
	Vector col
		(
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >(),
		GEngine->GetRandomNumberGenerator().Get< Float >()
		);
	m_value = Color( col );
}

void CEvaluatorColorConst::Evaluate( Float x, Vector& result ) const
{
	result = m_value.ToVector();
}

Bool CEvaluatorColorConst::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();
	Vector v = m_value.ToVector();
	Swap( v.X, v.Z );

	samples.PushBack( v );
	samples.PushBack( v );
	return true;
}

void CEvaluatorColorStartEnd::Evaluate( Float x, Vector& result ) const
{
	const Float clampedX = ::Clamp( x, 0.0f, 1.0f );
	const Vector a = m_start.ToVector();
	const Vector b = m_end.ToVector();
	result = ::Lerp( clampedX, a, b );
}

Bool CEvaluatorColorStartEnd::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();
	samples.PushBack( m_start.ToVector() );
	samples.PushBack( m_end.ToVector() );
	return true;
}

CEvaluatorColorCurve::CEvaluatorColorCurve()
{
#ifndef NO_EDITOR
	const CName curveName( TXT("Curves") );
	m_curves.CreateCurves( curveName, 4, this );
#endif
}

void CEvaluatorColorCurve::Evaluate( Float x, Vector & result ) const
{
	result.A[0] = ::Clamp( m_curves.GetCurveValue( 0, x ), 0.0f, 1.0f );
	result.A[1] = ::Clamp( m_curves.GetCurveValue( 1, x ), 0.0f, 1.0f );
	result.A[2] = ::Clamp( m_curves.GetCurveValue( 2, x ), 0.0f, 1.0f );
	result.A[3] = ::Clamp( m_curves.GetCurveValue( 3, x ), 0.0f, 1.0f );
}

// ** ******************************
//
void CEvaluatorColorCurve::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	file << m_curves;
}

#define NUM_CURVE_SAMPLES	16

Bool CEvaluatorColorCurve::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.ClearFast();

	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( NUM_CURVE_SAMPLES - 1 );

	Vector val( 0.0f, 0.0f, 0.0f, 0.0f );
	for ( Uint32 i=0; i<NUM_CURVE_SAMPLES; ++i, t += dt )
	{
		val.W = ::Clamp( m_curves.GetCurveValue( 3, t ), 0.0f, 1.0f );
		val.Z = ::Clamp( m_curves.GetCurveValue( 2, t ), 0.0f, 1.0f );
		val.Y = ::Clamp( m_curves.GetCurveValue( 1, t ), 0.0f, 1.0f );
		val.X = ::Clamp( m_curves.GetCurveValue( 0, t ), 0.0f, 1.0f );
		
		samples.PushBack( val );
	}
	return true;
}

void CEvaluatorColorRandom::Evaluate( Float x, Vector& result ) const
{
	result = m_value.ToVector();
}

Bool CEvaluatorColorRandom::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();
	samples.PushBack( m_value.ToVector() );
	samples.PushBack( m_value.ToVector() );
	return true;
}
