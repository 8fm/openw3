/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItemCurveBase.h"
#include "fxSpawner.h"
#include "lightComponent.h"
#include "drawableComponent.h"

/// Dynamically spawned light
class CFXTrackItemDynamicLight : public CFXTrackItemCurveBase
{
	friend class CFXTrackItemDynamicLightPlayData;

	DECLARE_ENGINE_CLASS( CFXTrackItemDynamicLight, CFXTrackItemCurveBase, 0 );

private:	
	Color					m_color;					//!< Base light color
	SLightFlickering		m_lightFlickering;			//!< Light flickering
	Float					m_radius;					//!< Base light radius
	Float					m_brightness;				//!< Base brightness level
	Float					m_attenuation;				//!< Base attenuation level
	Float					m_specularScale;			//!< Light specular scale
	Float					m_autoHideDistance;
	Float					m_autoHideRange;
	Float					m_spotInnerAngle;
	Float					m_spotOuterAngle;
	IFXSpawner*				m_spawner;					//!< Object spawner
	EEnvColorGroup			m_colorGroup;				//!< Color group
	Uint8					m_lightChannels;			//!< Light channels
	Bool					m_isCastingShadows;			//!< Light is casting shadows
	Bool					m_isModulative;				//!< Modulate this light
	Bool					m_isSpotlight;


public:
	CFXTrackItemDynamicLight();

	//! Light shafts
	virtual String GetName() const { return TXT("Dynamic Light"); }

	//! Set the name
	virtual void SetName( const String &name ) {}

	//! Get name of the curve
	virtual String GetCurveName( Uint32 i = 0 ) const;

	//! Get number of curves
	virtual Uint32 GetCurvesCount() const { return 2; }

	//! Check if track item uses component with given name
	virtual Bool UsesComponent( const CName& componentName ) const;

public:
	//! Start track item effect, can spawn play data that will be ticked
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;
};

BEGIN_CLASS_RTTI( CFXTrackItemDynamicLight );
	PARENT_CLASS( CFXTrackItemCurveBase );
	PROPERTY_EDIT( m_color, TXT("Base light color") );
	PROPERTY_EDIT( m_radius, TXT("Base light radius") );
	PROPERTY_EDIT( m_brightness, TXT("Base brightness level") );
	PROPERTY_EDIT( m_attenuation, TXT("Base attenuation level") );
	PROPERTY_EDIT( m_specularScale, TXT("Light specular scale") );
	PROPERTY_BITFIELD_EDIT( m_lightChannels, ELightChannel, TXT("Light channels") );
	PROPERTY_EDIT( m_isCastingShadows, TXT("Light is casting shadows") );
	PROPERTY_EDIT( m_isModulative, TXT("Modulate this light") );
	PROPERTY_EDIT( m_lightFlickering, TXT("Light flickering") );
	PROPERTY_EDIT( m_autoHideDistance, TXT("Auto hide distance") );
	PROPERTY_EDIT( m_autoHideRange, TXT("Auto hide range") );
	PROPERTY_EDIT( m_colorGroup, TXT("Color group") );
	PROPERTY_EDIT( m_isSpotlight, TXT("Is spotlight?") );
	PROPERTY_EDIT( m_spotInnerAngle, TXT("Spot inner angle") );
	PROPERTY_EDIT( m_spotOuterAngle, TXT("Spot outer angle") );
	PROPERTY_INLINED( m_spawner, TXT("Object spawner") );
END_CLASS_RTTI();
