#pragma once
#include "entity.h"

struct SLightInfo
{
	Vector pos;
	Float brightness;
	Bool wasEnabled;
	SLightInfo(){}
};

class CDaycycleGraphicsEntity : public CEntity
{
	DECLARE_ENGINE_CLASS( CDaycycleGraphicsEntity, CEntity, 0 );

protected:
	// Properties
	Float				m_startEmissiveHour;
	Float				m_startEmissiveFadeTime;
	Float				m_endEmissiveHour;
	Float				m_endEmissiveFadeTime;
	Float				m_randomRange;

	Float				m_lightBrightnessWhenOnMin;
	Float				m_lightBrightnessWhenOnMax;

	Float				m_lightRandomOffsetX;
	Float				m_lightRandomOffsetY;
	Float				m_lightRandomOffsetZ;

	Float				m_engineValueXWhenOnMin;
	Float				m_engineValueXWhenOnMax;
	Float				m_engineValueXWhenOff;
	Float				m_engineValueYWhenOn;
	Float				m_engineValueYWhenOff;
	Float				m_engineValueZWhenOn;
	Float				m_engineValueZWhenOff;
	Float				m_engineValueWWhenOn;
	Float				m_engineValueWWhenOff;
	Color				m_engineValueColorWhenOn;
	Color				m_engineValueColorWhenOff;

	Float				m_particleAlphaWhenOnMin;
	Float				m_particleAlphaWhenOnMax;
	Float				m_particleAlphaWhenOff;

	Float				m_flickeringPeriod;

	Float				m_lightRadius;
	Float				m_lightAutoHideDistance;
	Float				m_lightAutoHideRange;
	Bool				m_overrideRadius;

	Bool				m_startStopLightsAndEngineValues;
	Bool				m_startStopEffects;

	// Vars used
	Float				m_randomizedStartHour;
	Float				m_randomizedStartHourFadeEnd;
	Float				m_randomizedEndHour;
	Float				m_randomizedEndHourFadeEnd;
	Float				m_lastVal;
	Uint32				m_tickCounter;

	Float				m_lastFlickerRandomizedTime;
	Float				m_lastFlickerRandomizedValue;
	Float				m_nextFlickerRandomizedValue;

	Vector				m_lastRandomOffset;
	Vector				m_nextRandomOffset;
	Bool				m_animatePosition;

	Bool				m_flicker;
	Float				m_localTimer;

	Bool				m_wasChangingParams;

	THashMap<String,SLightInfo> m_lightInfo;


private:
	void RecalcAndSendEffectParameter( Float time );
	void SendParameters( Float lerpFactor );
	void RestoreAll();


public:
	CDaycycleGraphicsEntity();

	// Entity was attached to world
	virtual void OnAttached( CWorld* world );

	// Entity was detached from world
	virtual void OnDetached( CWorld* world );

	// On tick
	virtual void OnTick( Float time );

	// On pre save
	virtual void OnPreDependencyMap( IFile& mapper );
};

BEGIN_CLASS_RTTI( CDaycycleGraphicsEntity );
PARENT_CLASS( CEntity );
PROPERTY_EDIT( m_startStopEffects,			TXT("Start/stop effects at proper hour") );
PROPERTY_EDIT( m_startStopLightsAndEngineValues, TXT("Start/stop lights and send engine values at proper hours") );
PROPERTY_EDIT( m_startEmissiveHour,			TXT("When emissive starts") );
PROPERTY_EDIT( m_startEmissiveFadeTime,		TXT("How quickly emissive fades in") );
PROPERTY_EDIT( m_endEmissiveHour,			TXT("When emissive ends") );
PROPERTY_EDIT( m_endEmissiveFadeTime,		TXT("How quickly emissive fades out") );
PROPERTY_EDIT( m_randomRange,				TXT("Randomized range") );
PROPERTY_EDIT( m_flickeringPeriod,			TXT("Flickering period") );
PROPERTY_EDIT( m_lightBrightnessWhenOnMin,	TXT("Light brightness when on") );
PROPERTY_EDIT( m_lightBrightnessWhenOnMax,	TXT("Light brightness when on") );
PROPERTY_EDIT( m_lightRandomOffsetX,		TXT("Light random offset X") );
PROPERTY_EDIT( m_lightRandomOffsetY,		TXT("Light random offset Y") );
PROPERTY_EDIT( m_lightRandomOffsetZ,		TXT("Light random offset Z") );
PROPERTY_EDIT( m_lightRadius,				TXT("Light radius when on") );
PROPERTY_EDIT( m_lightAutoHideDistance,		TXT("Light autohide distance") );
PROPERTY_EDIT( m_lightAutoHideRange,		TXT("Light autohide range") );
PROPERTY_EDIT( m_overrideRadius,			TXT("Override radius?") );
PROPERTY_EDIT( m_particleAlphaWhenOnMin,	TXT("Particle alpha when on") );
PROPERTY_EDIT( m_particleAlphaWhenOnMax,	TXT("Particle alpha when on") );
PROPERTY_EDIT( m_particleAlphaWhenOff,		TXT("Particle alpha when off") );
PROPERTY_EDIT( m_engineValueXWhenOnMin,		TXT("Engine value X when on") );
PROPERTY_EDIT( m_engineValueXWhenOnMax,		TXT("Engine value X when on") );
PROPERTY_EDIT( m_engineValueXWhenOff,		TXT("Engine value X when off") );
PROPERTY_EDIT( m_engineValueYWhenOn,		TXT("Engine value Y when on") );
PROPERTY_EDIT( m_engineValueYWhenOff,		TXT("Engine value Y when off") );
PROPERTY_EDIT( m_engineValueZWhenOn,		TXT("Engine value Z when on") );
PROPERTY_EDIT( m_engineValueZWhenOff,		TXT("Engine value Z when off") );
PROPERTY_EDIT( m_engineValueWWhenOn,		TXT("Engine value W when on") );
PROPERTY_EDIT( m_engineValueWWhenOff,		TXT("Engine value W when off") );
PROPERTY_EDIT( m_engineValueColorWhenOn,	TXT("Engine value color when on") );
PROPERTY_EDIT( m_engineValueColorWhenOff,	TXT("Engine value color when off") );
END_CLASS_RTTI();
