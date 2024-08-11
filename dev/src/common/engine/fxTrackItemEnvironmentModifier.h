/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "fxTrackItemCurveBase.h"

//! E3 DEMOHACK storm lightning
class CFXTrackItemEnvironmentModifier : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemEnvironmentModifier, CFXTrackItem, 0 );

private:
	Vector				m_lightDirection;				//!< Direction of sun light
	Color				m_sunLightDiffuse;				//!< Sun light diffuse
	Float				m_sunLightBrightness;			//!< Sun light scaler
	//Bool				m_hdrAdaptationDisabled;		//!< HDR disabled
	Color				m_ambientOverride;				//!< Ambient color override
	Float				m_ambientOverrideBrightness;	//!< Ambient color scaler override

	Bool				m_overrideBalancing;
	Color				m_parametricBalanceLow;
	Float				m_parametricBalanceLowScale;
	Color				m_parametricBalanceMid;
	Float				m_parametricBalanceMidScale;
	Color				m_parametricBalanceHigh;
	Float				m_parametricBalanceHighScale;

public:
	CFXTrackItemEnvironmentModifier();

	//! Get the name of the track item
	virtual String GetName() const { return TXT("Environment modifier"); }

	//! Change name
	virtual void SetName( const String &name ) {}

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemEnvironmentModifier );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_EDIT( m_lightDirection, TXT("light direction") );
	PROPERTY_EDIT( m_sunLightDiffuse, TXT("sunlight color") );		
	PROPERTY_EDIT( m_sunLightBrightness, TXT("sun light brithtness") );
	//PROPERTY_EDIT( m_hdrAdaptationDisabled, TXT("disable hdr adaptation") );
	PROPERTY_EDIT( m_ambientOverride, TXT("ambient color override") );
	PROPERTY_EDIT( m_ambientOverrideBrightness, TXT("ambient brightness") );
	PROPERTY_EDIT( m_overrideBalancing, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceLow, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceLowScale, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceMid, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceMidScale, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceHigh, TXT("") );
	PROPERTY_EDIT( m_parametricBalanceHighScale, TXT("") );
END_CLASS_RTTI();
// -