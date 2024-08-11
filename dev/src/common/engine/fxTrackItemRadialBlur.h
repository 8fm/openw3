/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxTrackItemCurveBase.h"

/// Radial blur track item in the FX
class CFXTrackItemRadialBlur : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemRadialBlur, CFXTrackItemCurveBase, 0 );

private:
	Bool		m_trackComponentPosition;	// if true, than the center of radial blur will be set to the component position
	Float		m_distanceFromCamera;		// applicable only if 'm_trackComponentPosition' is false
	Float		m_centerMultiplier;

public:
	CFXTrackItemRadialBlur();

	virtual String GetName() const { return TXT("Radial Blur"); }

	virtual void SetName( const String &name ) {}

	virtual String GetCurveName( Uint32 i = 0 ) const;

	virtual Uint32 GetCurvesCount() const { return 4; }

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemRadialBlur );
	PARENT_CLASS( CFXTrackItemCurveBase );
	PROPERTY_EDIT( m_trackComponentPosition, TXT("Tracks component position") );
	PROPERTY_EDIT( m_distanceFromCamera, TXT("Distance from camera (if tracking component position is disabled).") );
	PROPERTY_EDIT( m_centerMultiplier, TXT("Center multiplier") );
END_CLASS_RTTI();
