/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "envProbeParams.h"

IMPLEMENT_ENGINE_CLASS( SEnvProbeGenParams );

IEnvProbeDataSource::tLock& IEnvProbeDataSource::GetCommunicationLock()
{
	static IEnvProbeDataSource::tLock lock;
	return lock;
}

static Vector GetColorLinear( const Color &color, Float scale )
{
	const Float mul = 1.f / 255.f;
	const Float exp = 2.2f;
	return Vector ( scale * powf( color.R * mul, exp ), scale * powf( color.G * mul, exp ), scale * powf( color.B * mul, exp ), 1.f );
}

inline Float GetNormalizedWorldTime( Float worldTime )
{
	const Float scale = 1.f / (24.f * 60.f * 60.f);
	return worldTime * scale;
}

// ------------------
// SEnvProbeGenParams

SEnvProbeGenParams::SEnvProbeGenParams ()
	: m_useInInterior ( true )
	, m_useInExterior ( true )
	, m_isInteriorFallback ( false )
	, m_cullingDistance ( 125 )
	, m_ambientColor ( 255, 255, 255, 255 )
	, m_ambientIntensity ( 1.f )
	, m_dimmerFactor ( 1.f )
	, m_fadeInDuration ( -1 )
	, m_fadeOutDuration ( -1 )
	, m_lightScaleGlobal ( 1 )
	, m_lightScaleLocals ( 1 )
	, m_fogAmount ( 1 )
{
	m_daycycleAmbientIntensity.Reset( SCT_Float, 10.f, 0.f );
	m_daycycleLightScaleLocals.Reset( SCT_Float, 10.f, 0.f );
	m_daycycleEffectIntensity.Reset( SCT_Float, 10.f, 0.f );
}

Float SEnvProbeGenParams::GetLightScaleLocals( Float worldTime ) const
{
	const Float daycycleScale = m_daycycleLightScaleLocals.IsEmpty() ? 1.f : Max( 0.f, m_daycycleLightScaleLocals.GetFloatValue( GetNormalizedWorldTime( worldTime ) ) );
	return Max( 0.f, m_lightScaleLocals ) * daycycleScale;
}

Vector SEnvProbeGenParams::GetAmbientColorLinear( Float worldTime ) const
{
	const Float daycycleScale = m_daycycleAmbientIntensity.IsEmpty() ? 1.f : Max( 0.f, m_daycycleAmbientIntensity.GetFloatValue( GetNormalizedWorldTime( worldTime ) ) );
	return GetColorLinear( m_ambientColor, Max( 0.f, m_ambientIntensity * daycycleScale ) );
}

// ---------------
// SEnvProbeParams

SEnvProbeParams::SEnvProbeParams ()
	: m_debugId ( 0 )
	, m_nestingLevel ( 0 ) 
	, m_effectIntensity ( 1 )
	, m_probeGenOrigin ( Vector::ZEROS )
	, m_probeOrigin ( Vector::ZEROS )
	, m_areaLocalToWorld ( Matrix::IDENTITY )
	, m_parallaxLocalToWorld ( Matrix::IDENTITY )
	, m_contribution ( 1.f )
	, m_areaMarginFactor ( 0, 0, 0, 1 )
{}

SEnvProbeParams::SEnvProbeParams ( Uint32 debugId, Int32 nestingLevel, Float effectIntensity, const Vector &probeGenOrigin, const Vector &probeOrigin, const Matrix &areaLocalToWorld, const Matrix &parallaxLocalToWorld, Float contribution, const Vector &areaMarginFactor, const SEnvProbeGenParams &genParams )
	: m_debugId ( debugId )
	, m_nestingLevel ( nestingLevel )
	, m_effectIntensity ( effectIntensity )
	, m_probeGenOrigin ( probeGenOrigin )
	, m_probeOrigin ( probeOrigin )
	, m_areaLocalToWorld ( areaLocalToWorld )
	, m_parallaxLocalToWorld ( parallaxLocalToWorld )
	, m_contribution ( contribution )
	, m_areaMarginFactor ( areaMarginFactor )
	, m_genParams ( genParams )
{}

Float SEnvProbeParams::SquaredDistance( const Vector &point ) const
{
	return MathUtils::GeometryUtils::OrientedBoxSquaredDistance( m_areaLocalToWorld, point );
}

Bool SEnvProbeParams::IsGlobalProbe() const
{
	return -1 == m_contribution;
}

Float SEnvProbeParams::GetEffectIntensity( Float worldTime ) const
{
	const Float daycycleScale = m_genParams.m_daycycleEffectIntensity.IsEmpty() ? 1.f : Max( 0.f, m_genParams.m_daycycleEffectIntensity.GetFloatValue( GetNormalizedWorldTime( worldTime ) ) );
	return Max( 0.f, m_effectIntensity ) * daycycleScale;
}
