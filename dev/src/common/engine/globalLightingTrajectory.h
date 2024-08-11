/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/curveSimple.h"

struct GameTime;

struct CGlobalLightingTrajectory
{
	DECLARE_RTTI_STRUCT( CGlobalLightingTrajectory );

protected:
	Float			m_yawDegrees;
	Float			m_yawDegreesSunOffset;
	Float			m_yawDegreesMoonOffset;
	Float			m_sunCurveShiftFactor;
	Float			m_moonCurveShiftFactor;
	Float			m_sunSqueeze;
	Float			m_moonSqueeze;
	SSimpleCurve	m_sunHeight;
	SSimpleCurve	m_moonHeight;
	SSimpleCurve	m_lightHeight;
	SSimpleCurve	m_lightDirChoice;
	SSimpleCurve	m_skyDayAmount;
	Float			m_moonShaftsBeginHour;
	Float			m_moonShaftsEndHour;

public:
	/// Class constructor
	CGlobalLightingTrajectory ();

	/// Reset to default
	void Reset();

public:
	/// Get yaw rotation degrees
	Float GetYawDegrees() const { return m_yawDegrees; }

	/// Get sun direction for given time
	Vector GetSunDirection( const GameTime &gameTime ) const;

	/// Get moon direction for given time
	Vector GetMoonDirection( const GameTime &gameTime ) const;

	/// Get common light direction (for instance for lighting) for given time
	Vector GetLightDirection( const GameTime &gameTime ) const;

	/// Get sky day amount curve
	const SSimpleCurve& GetSkyDayAmountCurve() const { return m_skyDayAmount; }

	/// Are moon shafts enabled instead of sun?
	Bool AreMoonShaftsEnabled(  const GameTime &gameTime ) const;

	/// Get yaw degrees sun  offset
	Float GetYawDegreesSunOffset() const { return m_yawDegreesSunOffset; }

	/// Get yaw degrees moon offset
	Float GetYawDegreesMoonOffset() const { return m_yawDegreesMoonOffset; }

	/// Get sun squeeze
	Float GetSunSqueeze() const { return m_sunSqueeze; }

	/// Get moon squeeze
	Float GetMoonSqueeze() const { return m_moonSqueeze; }
};

BEGIN_CLASS_RTTI( CGlobalLightingTrajectory )
	PROPERTY_EDIT( m_yawDegrees,			TXT("m_yawDegrees") );
	PROPERTY_EDIT( m_yawDegreesSunOffset,	TXT("m_yawDegreesSunOffset") );
	PROPERTY_EDIT( m_yawDegreesMoonOffset,	TXT("m_yawDegreesMoonOffset") );
	PROPERTY_EDIT( m_sunCurveShiftFactor,	TXT("m_sunCurveShiftFactor") );
	PROPERTY_EDIT( m_moonCurveShiftFactor,	TXT("m_moonCurveShiftFactor") );
	PROPERTY_EDIT( m_sunSqueeze,		TXT("m_sunSqueeze") );
	PROPERTY_EDIT( m_moonSqueeze,		TXT("m_moonSqueeze") );
	PROPERTY_EDIT( m_sunHeight,			TXT("m_sunHeight") );
	PROPERTY_EDIT( m_moonHeight,		TXT("m_moonHeight") );
	PROPERTY_EDIT( m_lightHeight,		TXT("m_lightHeight") );
	PROPERTY_EDIT( m_lightDirChoice,	TXT("m_lightDirChoice") );
	PROPERTY_EDIT( m_skyDayAmount,		TXT("m_skyDayAmount") );
	PROPERTY_EDIT( m_moonShaftsBeginHour,TXT("m_moonShaftsBeginHour") );
	PROPERTY_EDIT( m_moonShaftsEndHour,	TXT("m_moonShaftsEndHour") );
END_CLASS_RTTI()
