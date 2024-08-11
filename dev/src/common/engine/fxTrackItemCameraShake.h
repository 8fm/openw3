/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"

/// Particle emitter spawner
class CFXTrackItemCameraShake : public CFXTrackItemCurveBase
{
	DECLARE_ENGINE_CLASS( CFXTrackItemCameraShake, CFXTrackItem, 0 );

private:
	Vector		m_maxShake;						//!< Maximum shake speen
	Float		m_shakeSpeed;					//!< Shape speed
	Float		m_effectFullForceRadius;		//!< Force size
	Float		m_effectMaxRadius;				//!< Maximum radius of the effect
	CName		m_shakeType;					//!< Type of shake

public:
	CFXTrackItemCameraShake();

	//! Change name of track item
	virtual void SetName( const String& name ){};

	//! Get name of track item
	virtual String GetName() const { return TXT("Camera shake"); };

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemCameraShake );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_effectFullForceRadius, TXT( "Radius in which shake force is applied with full effect" ) );
	PROPERTY_EDIT( m_effectMaxRadius, TXT( "Max shake radius (after which there is no camera shake)" ) );
	PROPERTY_CUSTOM_EDIT( m_shakeType, TXT( "Shake type" ), TXT("CameraShakeEnumList") );
END_CLASS_RTTI();


