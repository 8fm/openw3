/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "evaluatorVector.h"
#include "baseEngine.h"

IMPLEMENT_ENGINE_CLASS( IEvaluatorVector );
IMPLEMENT_ENGINE_CLASS( CEvaluatorVectorConst );
IMPLEMENT_ENGINE_CLASS( CEvaluatorVectorRandomUniform );
IMPLEMENT_ENGINE_CLASS( CEvaluatorVectorStartEnd );
IMPLEMENT_ENGINE_CLASS( CEvaluatorVectorCurve );
IMPLEMENT_RTTI_ENUM( EFreeVectorAxes );

IEvaluatorVector::IEvaluatorVector()
	: m_freeAxes( FVA_Three )
	, m_spill( true )
{
}


// Default - all, four components are used
template< EFreeVectorAxes TFVAxes >
RED_INLINE Vector PopulateComponents( const Vector & v, Float fv )
{
	return v;
}

template<>
RED_INLINE Vector PopulateComponents< FVA_Three >( const Vector & v, Float fv )
{
	return Vector( v.A[ 0 ], v.A[ 1 ], v.A[ 2 ], fv );
}

template<>
RED_INLINE Vector PopulateComponents< FVA_Two >( const Vector & v, Float fv )
{
	return Vector( v.A[ 0 ], v.A[ 1 ], fv, fv );
}

template<>
RED_INLINE Vector PopulateComponents< FVA_One >( const Vector & v, Float fv )
{
	return Vector( v.A[ 0 ], fv, fv, fv );
}

void IEvaluatorVector::Evaluate( Float x, Vector & result ) const
{
	// This can be implemented somewhere else but this would interfere with current curve getter, so it is not changed now
	Vector v; EvaluateInternal( x, v );
	Float fv = m_spill ? v.A[ (Uint32) m_freeAxes - 1 ] : 0.f;

	switch( m_freeAxes )
	{
		case FVA_One:	result = PopulateComponents< FVA_One >	( v, fv ); break;
		case FVA_Two:	result = PopulateComponents< FVA_Two >	( v, fv ); break;
		case FVA_Three:	result = PopulateComponents< FVA_Three >( v, fv ); break;
		case FVA_Four:	result = PopulateComponents< FVA_Four >	( v, fv ); break;
		default: ASSERT( false );
	}

#if 0
	// Limit locked axes
	if ( m_spill )
	{
		switch ( m_freeAxes )
		{
			case FVA_One: result = Vector( internalValue.A[0], internalValue.A[0], internalValue.A[0], internalValue.A[0] ); break;
			case FVA_Two: result = Vector( internalValue.A[0], internalValue.A[1], internalValue.A[1], internalValue.A[1] ); break;
			case FVA_Three: result = Vector( internalValue.A[0], internalValue.A[1], internalValue.A[2], internalValue.A[2] ); break;
			case FVA_Four: result = internalValue; break;
		}
	}
	else
	{
		switch ( m_freeAxes )
		{
			case FVA_One: result = Vector( internalValue.A[0], 0.0f, 0.0f, 0.0f ); break;
			case FVA_Two: result = Vector( internalValue.A[0], internalValue.A[1], 0.0f, 0.0f ); break;
			case FVA_Three: result = Vector( internalValue.A[0], internalValue.A[1], internalValue.A[2], 0.0f ); break;
			case FVA_Four: result = internalValue; break;
		}
	}
#endif
}

void IEvaluatorVector::SpillVector( Vector& v ) const
{
	Float fv = m_spill ? v.A[ (Uint32) m_freeAxes - 1 ] : 0.f;

	switch( m_freeAxes )
	{
	case FVA_One:	v = PopulateComponents< FVA_One >( v, fv ); break;
	case FVA_Two:	v = PopulateComponents< FVA_Two >( v, fv ); break;
	case FVA_Three:	v = PopulateComponents< FVA_Three >( v, fv ); break;
	case FVA_Four:	v = PopulateComponents< FVA_Four > ( v, fv ); break;
	default: ASSERT( false );
	}
}


void CEvaluatorVectorConst::EvaluateInternal( Float x, Vector& result ) const
{
	result = m_value;
}

Bool CEvaluatorVectorConst::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();

	Vector approximated = m_value;
	SpillVector( approximated );

	samples.PushBack( approximated );
	samples.PushBack( approximated );

	return true;
}

void CEvaluatorVectorRandomUniform::EvaluateInternal( Float x, Vector& result ) const
{
	result = GRandomVector( m_min, m_max );
}

Bool CEvaluatorVectorRandomUniform::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();

	Vector approximatedMin = m_min;
	SpillVector( approximatedMin );
	samples.PushBack( approximatedMin );

	Vector approximatedMax = m_max;
	SpillVector( approximatedMax );
	samples.PushBack( approximatedMax );
	return true;
}

void CEvaluatorVectorStartEnd::EvaluateInternal( Float x, Vector& result ) const
{
	const Float clampedX = ::Clamp( x, 0.0f, 1.0f );
	Vector delta = ( m_end - m_start ) * clampedX;
	result = m_start + delta;
}

Bool CEvaluatorVectorStartEnd::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.Clear();

	Vector approximatedStart = m_start;
	SpillVector( approximatedStart );
	samples.PushBack( approximatedStart );

	Vector approximatedEnd = m_end;
	SpillVector( approximatedEnd );
	samples.PushBack( approximatedEnd );
	return true;
}

CEvaluatorVectorCurve::CEvaluatorVectorCurve()
{
#ifndef NO_EDITOR
	const CName curveName( TXT("Curves") );
	m_curves.CreateCurves( curveName, 4, this );
#endif
}

void CEvaluatorVectorCurve::EvaluateInternal( Float x, Vector & result ) const
{
	switch ( m_curves.GetCurveCount() )
	{
	case 4:
		result.A[3] = m_curves.GetCurveValue( 3, x );
	case 3:
		result.A[2] = m_curves.GetCurveValue( 2, x );
	case 2:
		result.A[1] = m_curves.GetCurveValue( 1, x );
	case 1:
		result.A[0] = m_curves.GetCurveValue( 0, x );
	}
}

void CEvaluatorVectorCurve::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// TODO: restore support for this
	/*
	// Change the number of curves in the group
	const Uint32 numAxis = m_freeAxes;
	CurveParameter& param = m_curves.GetCurveParameter(0);
	if ( param.GetCurveCount() != numAxis )
	{
		param.ChangeCurveCount( numAxis, this );
	}*/
}

// ** ******************************
//
void CEvaluatorVectorCurve::OnSerialize( IFile & file )
{
	TBaseClass::OnSerialize( file );

	file << m_curves;
}

#define NUM_CURVE_SAMPLES	16

Bool CEvaluatorVectorCurve::GetApproximationSamples( TDynArray< Vector >& samples ) const
{
	samples.ClearFast();

	Float t = 0.f;
	Float dt = 1.f / ( Float ) ( NUM_CURVE_SAMPLES - 1 );

	for ( Uint32 i=0; i<NUM_CURVE_SAMPLES; ++i, t += dt )
	{
		Vector val( 0.0f, 0.0f, 0.0f, 0.0f );
		switch( m_curves.GetCurveCount() )
		{
		case 4: 
			val.W = m_curves.GetCurveValue( 3, t );
		case 3: 
			val.Z = m_curves.GetCurveValue( 2, t );
		case 2: 
			val.Y = m_curves.GetCurveValue( 1, t );
		case 1: 
			val.X = m_curves.GetCurveValue( 0, t );
			break;
		default:
			ASSERT( false );
		}

		SpillVector( val );

		samples.PushBack( val );
	}
	return true;
}

Vector GRandomVector()
{
	Vector ret( GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >() , GEngine->GetRandomNumberGenerator().Get< Float >() );
	return ret;
}

Vector3 GRandomVector3()
{
	Vector3 ret( GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >(), GEngine->GetRandomNumberGenerator().Get< Float >() );
	return ret;
}
