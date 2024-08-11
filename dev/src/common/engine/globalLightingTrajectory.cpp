/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "globalLightingTrajectory.h"
#include "gameTime.h"

IMPLEMENT_ENGINE_CLASS( CGlobalLightingTrajectory );


CGlobalLightingTrajectory::CGlobalLightingTrajectory ()
{
	Reset();
}

void CGlobalLightingTrajectory::Reset()
{
	m_yawDegrees = 0;
	m_yawDegreesSunOffset = 0;
	m_yawDegreesMoonOffset = 0;
	m_sunSqueeze = 1;
	m_moonSqueeze = 1;
	m_sunCurveShiftFactor = 0;
	m_moonCurveShiftFactor = 0;

	m_sunHeight = SSimpleCurve ( SCT_Float );

	m_sunHeight.AddPoint(  0.5f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.4f)  );
	m_sunHeight.AddPoint(  0.15f, SSimpleCurvePoint::BuildValue(Color::BLACK, -0.08f)  );
	m_sunHeight.AddPoint(  0.85f, SSimpleCurvePoint::BuildValue(Color::BLACK, -0.08f)  );
	
	m_moonHeight = SSimpleCurve ( SCT_Float );
	m_moonHeight.AddPoint( 0.5f, SSimpleCurvePoint::BuildValue(Color::BLACK, -0.8f) );
	m_moonHeight.AddPoint(  0.15f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.08f)  );
	m_moonHeight.AddPoint(  0.85f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.08f)  );
	
	m_lightHeight = SSimpleCurve ( SCT_Float );
	m_lightHeight.AddPoint(  0.5f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.4f) );
	m_lightHeight.AddPoint(  0.15f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.01f) );
	m_lightHeight.AddPoint(  0.85f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.01f) );

	m_lightDirChoice = SSimpleCurve ( SCT_Float );
	m_lightDirChoice.AddPoint(  0.5f, SSimpleCurvePoint::BuildValue(Color::BLACK, 1.f) );

	m_skyDayAmount = SSimpleCurve ( SCT_Float );
	m_skyDayAmount.AddPoint(  0.25f, SSimpleCurvePoint::BuildValue(Color::BLACK, 1.0f) );
	m_skyDayAmount.AddPoint(  0.75f, SSimpleCurvePoint::BuildValue(Color::BLACK, 1.0f) );
	m_skyDayAmount.AddPoint(  0.15f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.0f) );
	m_skyDayAmount.AddPoint(  0.85f, SSimpleCurvePoint::BuildValue(Color::BLACK, 0.0f) );

	m_moonShaftsBeginHour = 0.0f;
	m_moonShaftsEndHour = 0.0f;
}

Bool CGlobalLightingTrajectory::AreMoonShaftsEnabled( const GameTime &gameTime ) const
{
	Int32 range  = 24 * 60 * 60;
	Float dayCycleProgress = static_cast<Float>( gameTime.GetSeconds() % range );

	Float beginTime = m_moonShaftsBeginHour * 60.0f * 60.0f;
	Float endTime = m_moonShaftsEndHour * 60.0f * 60.0f;

	if ( beginTime > endTime )
	{
		// eg. 23-5
		if ( dayCycleProgress > beginTime || dayCycleProgress < endTime )
		{
			return true;
		}
	}
	else
	{
		// eg. 1-5
		if ( (dayCycleProgress > beginTime) && (dayCycleProgress < endTime) )
		{
			return true;
		}
	}

	return false;
}

namespace
{
	inline Float CalcSqueezedProgress( Float progress, Float refProgress, Float squeeze )
	{
		progress -= MFloor( progress );
		refProgress -= MFloor( refProgress );
		Float t = progress;
		t = (t - refProgress) * squeeze + refProgress;
		t -= MFloor( t );
		return t;
	}

	inline Float CalcPitch( const SSimpleCurve &curve, Float progress )
	{
		progress -= MFloor( progress );
		Float curveValue = curve.GetFloatValue( progress );
		return 0.5f * M_PI * Clamp( curveValue, -1.f, 1.f );
	}

	inline Float CalcYaw( Float progress )
	{
		return 2.f * M_PI * progress;
	}

	inline Vector CalcDir( Float pitch, Float yaw )
	{
		Vector dir;
		dir = Vector ( cosf(pitch), 0.f, sinf(pitch) );
		dir = Vector ( dir.X * sinf(yaw), dir.X * cosf(yaw), dir.Z );
		return dir.Normalized3();
	}

	const Float DayCycleSeconds = 24 * 60 * 60;
}

Vector CGlobalLightingTrajectory::GetSunDirection( const GameTime &gameTime ) const
{	
	Float sunDayCycleProgress = gameTime.ToFloat() / DayCycleSeconds + m_sunCurveShiftFactor;
	Float sunPitchProgress = sunDayCycleProgress - m_sunCurveShiftFactor;
	Float sunYawProgress = CalcSqueezedProgress( sunDayCycleProgress, 0.5f, m_sunSqueeze )  + (m_yawDegrees + m_yawDegreesSunOffset) / 360;
	return CalcDir( CalcPitch( m_sunHeight, sunPitchProgress ), CalcYaw( sunYawProgress ) );
}

Vector CGlobalLightingTrajectory::GetMoonDirection( const GameTime &gameTime ) const
{
	Float moonDayCycleProgress = gameTime.ToFloat() / DayCycleSeconds + m_moonCurveShiftFactor;
	Float moonPitchProgress = moonDayCycleProgress - m_moonCurveShiftFactor;
	Float moonYawProgress = CalcSqueezedProgress( moonDayCycleProgress, 0.5f, m_moonSqueeze )  + (m_yawDegrees + m_yawDegreesMoonOffset) / 360;
	return CalcDir( CalcPitch( m_moonHeight, moonPitchProgress ), CalcYaw( moonYawProgress ) );
}

Vector CGlobalLightingTrajectory::GetLightDirection( const GameTime &gameTime ) const
{	
	Float sunDayCycleProgress = gameTime.ToFloat() / DayCycleSeconds + m_sunCurveShiftFactor;
	Float sunYawProgress = CalcSqueezedProgress( sunDayCycleProgress, 0.5f, m_sunSqueeze )  + (m_yawDegrees + m_yawDegreesSunOffset) / 360;
	Float moonDayCycleProgress = gameTime.ToFloat() / DayCycleSeconds + m_moonCurveShiftFactor;
	Float moonYawProgress = CalcSqueezedProgress( moonDayCycleProgress, 0.5f, m_moonSqueeze )  + (m_yawDegrees + m_yawDegreesMoonOffset) / 360;

	Float lightDayCycleProgress = gameTime.ToFloat() / DayCycleSeconds;
	Float lightPitchProgress = gameTime.ToFloat() / DayCycleSeconds;

	Float t = Clamp( m_lightDirChoice.GetFloatValue( lightDayCycleProgress ), 0.f, 1.f );
	moonYawProgress -= MFloor( moonYawProgress );
	sunYawProgress -= MFloor( sunYawProgress );
	
	Float lightYawProgress = 0.f;
	if ( sunYawProgress >= moonYawProgress )
	{
		lightYawProgress = sunYawProgress < moonYawProgress + 0.5f ? Lerp( t, moonYawProgress, sunYawProgress ) : Lerp( t, moonYawProgress, sunYawProgress - 1 );
	}
	else
	{
		lightYawProgress = moonYawProgress < sunYawProgress + 0.5f ? Lerp( t, moonYawProgress, sunYawProgress ) : Lerp( t, moonYawProgress, sunYawProgress + 1 );
	}
	lightYawProgress -= MFloor( lightYawProgress );
	
	return CalcDir( CalcPitch( m_lightHeight, lightPitchProgress ), CalcYaw( lightYawProgress ) );
}
