/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "previewWorld.h"
#include "../../common/engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CEdPreviewWorld );


CEdPreviewWorld::CEdPreviewWorld()
	: m_wind( 0, 0, 0, 0 )
{
}


CEdPreviewWorld::~CEdPreviewWorld()
{
}


Vector CEdPreviewWorld::GetWindAtPointForVisuals( const Vector& point, Bool withTurbulence, Bool withForcefield ) const
{
	//
	// This wind rippling was taken from the CWitcherWorld implementation.
	//
	Vector wind = GetWindAtPoint( point );

	if ( withTurbulence )
	{
		Float mul = wind.Mag3()*0.4f;

		Float time = ( Float ) EngineTime::GetNow();

		Float frequency = 1.0f;
		Float Amplitude = 1.0f;

		mul *= Amplitude;

		//high frequency sine can be treated like a noise
		Float Noisex = sinf( point.A[0]*1000.0f - (time*point.A[1]*frequency) ) * mul;
		Float Noisey = sinf( point.A[1]*1000.0f - (time*point.A[0]*frequency) ) * mul;
		Float Noisez = sinf( point.A[2]*1000.0f - (time*point.A[0]*frequency) ) * mul;

		Vector delta(Noisex, Noisey, Noisez);
		wind += delta;
	}

	return wind;
}

Vector CEdPreviewWorld::GetWindAtPoint( const Vector& point ) const
{
	return m_wind;
}

void CEdPreviewWorld::SetWind( const Vector& direction, Float magnitude )
{
	m_wind = direction.Normalized3() * magnitude;
}
