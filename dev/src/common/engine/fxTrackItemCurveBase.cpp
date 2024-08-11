/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fxTrackItemCurveBase.h"
#include "curve.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemCurveBase );

CFXTrackItemCurveBase::CFXTrackItemCurveBase( Uint32 numCurves, CName curvesName /* = CName::NONE */ )
	: m_curveParameter( NULL )
{
	// Create curves
	m_curveParameter = new CurveParameter();
#ifndef NO_EDITOR
	m_curveParameter->CreateCurves( curvesName, numCurves, this );

	// Initialize curves
	for ( Uint32 i=0; i<numCurves; i++ )
	{
		CCurve* curve = m_curveParameter->GetCurve( i );
		if ( !curve->IsEmpty() )
		{
			curve->SetValue( 0, 1.0f );
		}
	}
#endif
}

CFXTrackItemCurveBase::~CFXTrackItemCurveBase()
{
	if ( m_curveParameter )
	{
		delete m_curveParameter;
	}
}

void CFXTrackItemCurveBase::SetCurveValue( Uint32 curveIdx, Float value )
{
	CCurve* curve = m_curveParameter->GetCurve( curveIdx );
	for ( Uint32 i = 0; i < curve->GetNumPoints(); ++i )
	{
		curve->SetValue( i, value );
	}
}

Float CFXTrackItemCurveBase::GetCurveValue( Float time ) const
{
	// Get parameter value
	Float normalizedTime = ( time - GetTimeBegin() ) / GetTimeDuration();
	return m_curveParameter->GetCurveValue( 0, normalizedTime );
}

Float CFXTrackItemCurveBase::GetCurveValue( Uint32 curveIdx, Float time ) const
{
	// Get parameter value
	Float normalizedTime = ( time - GetTimeBegin() ) / GetTimeDuration();
	return m_curveParameter->GetCurveValue( curveIdx, normalizedTime );
}

Color CFXTrackItemCurveBase::GetColorFromCurve( Float time ) const
{
	// Normalize input time
	const Float normalizedTime = GetTimeDuration() ? (( time - GetTimeBegin() ) / GetTimeDuration()) : 0.0f;

	// Calculate color curves
	const Float r = m_curveParameter->GetCurveValue( 0, normalizedTime );
	const Float g = m_curveParameter->GetCurveValue( 1, normalizedTime );
	const Float b = m_curveParameter->GetCurveValue( 2, normalizedTime );
	const Float a = m_curveParameter->GetCurveValue( 3, normalizedTime );

	// Return clamped color
	Vector colorVector( r, g, b, a );
	return Color( colorVector );
}

Vector CFXTrackItemCurveBase::GetVectorFromCurve( Float time ) const
{
	// Normalize input time
	const Float normalizedTime = GetTimeDuration() ? (( time - GetTimeBegin() ) / GetTimeDuration()) : 0.0f;

	// Calculate color curves
	const Float r = m_curveParameter->GetCurveValue( 0, normalizedTime );
	const Float g = m_curveParameter->GetCurveValue( 1, normalizedTime );
	const Float b = m_curveParameter->GetCurveValue( 2, normalizedTime );
	//const Float a = m_curveParameter->GetCurveValue( 3, normalizedTime );

	// Return clamped color
	return Vector( r, g, b, 1.0f );
}

void CFXTrackItemCurveBase::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// (02.11.2011)DREY TODO: Remove all this code and leave 'file << *m_curveParameter' after resave 
	//( __LOC__ ": (02.11.2011) Wywaliæ kod po resave!" )

	// Save curves
	if ( file.IsWriter() )
	{
		file << *m_curveParameter;
	}
	else if ( file.IsReader() )
	{
		if ( file.GetVersion() < VER_APPROXIMATED_CURVES )
		{
			Uint32 curveCount = 0;
			CName curveName = CName::NONE;

			// Read params
			file << curveCount;
			file << curveName;
			m_curveParameter->SetName( curveName );
			m_curveParameter->SetCurveCount( curveCount );

			// Load curves
			for ( Uint32 i = 0; i < curveCount; ++i )
			{
				CCurve* curve = NULL;
				file << curve;
				m_curveParameter->SetCurve( i, curve );
			}
		}
		else
		{
			file << *m_curveParameter;
		}
	}
}

CCurve *CFXTrackItemCurveBase::GetCurve( Uint32 i ) 
{ 
	return m_curveParameter->GetCurve( i ); 
}

const CCurve *CFXTrackItemCurveBase::GetCurve( Uint32 i ) const 
{ 
	return m_curveParameter->GetCurve( i ); 
}