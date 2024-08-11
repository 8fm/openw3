/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "fxTrackItem.h"

/// Sound event
class CFXTrackItemSoundEvent : public CFXTrackItem
{
	DECLARE_ENGINE_CLASS( CFXTrackItemSoundEvent, CFXTrackItem, 0 );

private:
	StringAnsi	m_soundEventName;		//!< Event to raise on enter
	StringAnsi	m_latchEvent;			//!< event to trigger when the latch activates
	Float		m_maxDistance;			//!< Maximum distance event will be triggered
	CName		m_boneName;				//!< On which character bone sound should be played
	Bool		m_enabled;				//!< Is sound event enabled
	Bool		m_isAmbient;
	Float		m_stopFadeTime;
	Bool		m_useDistanceParameter; //!< Should update wwise with distance to listener
	Float		m_latchDistanceParameterBelow; //!< When distance is below this value, we no longer update it
	Bool		m_invertLatchDistance;	//!< latch works on distances above m_latchDistanceParameterBelow instead
	Float       m_speed;				//!< when the fx has a target to move to, use this to control how fast the sound moves
	Float		m_decelDist;			//!< when a speed is being used the speed will decrease linearly wrt dist when below this distance
 
public:
	CFXTrackItemSoundEvent();

	//! Get track name
	virtual String GetName() const { return ANSI_TO_UNICODE( m_soundEventName.AsChar() ); }

	//! Set track name
	virtual void SetName( const String& name ) {}

	//! Is this a zero-time track item
	virtual Bool IsTick() { return true; }

	//! Get sound event name
	RED_INLINE const StringAnsi& GetSoundEventName() const { return m_soundEventName; }

	//! Get if it is an ambient sound
	RED_INLINE Bool IsAmbient() const { return m_isAmbient; }

	//! Get max distance
	RED_INLINE Float GetMaxDistance() const { return m_maxDistance; }

	//! Get bone to fire effect on
	RED_INLINE const CName& GetBoneName() const { return m_boneName; }

	RED_INLINE Float GetStopFadeTime() const { return m_stopFadeTime; }

	RED_INLINE Bool GetUseDistanceParameter() const { return m_useDistanceParameter; }

	RED_INLINE Float GetDistanceLatchBelow() const { return m_latchDistanceParameterBelow; }

	RED_INLINE Bool ShouldInvertDistanceLatch() const { return m_invertLatchDistance; }

	RED_INLINE StringAnsi GetLatchEvent() const { return m_latchEvent; }

	RED_INLINE Float GetSpeed() const { return m_speed; }

	RED_INLINE Float GetDecelDistance() const { return m_decelDist; }

public:
	//! Spawn play data, called on 
	virtual IFXTrackItemPlayData* OnStart( CFXState& fxState ) const;

};

BEGIN_CLASS_RTTI( CFXTrackItemSoundEvent );
	PARENT_CLASS( CFXTrackItem );
	PROPERTY_CUSTOM_EDIT( m_soundEventName, TXT( "Sound event name" ), TXT( "AudioEventBrowser" ) );
	PROPERTY_EDIT( m_stopFadeTime, TXT( "Fade time durration of started event stop" ) );
	PROPERTY_EDIT( m_isAmbient, TXT( "Is this sound should act as ambient emitter" ) );
	PROPERTY_EDIT( m_enabled, TXT( "Is sound event enabled" ) );
	PROPERTY_EDIT( m_maxDistance, TXT( "Maximum distance event will be triggered" ) );
	PROPERTY_EDIT( m_useDistanceParameter, TXT("Update wwise with distance to listener"));
	PROPERTY_EDIT( m_latchDistanceParameterBelow, TXT("When distance is below this value, we no longer update it"));
	PROPERTY_EDIT( m_invertLatchDistance, TXT("latch works on distances above instead of below the latch value"));
	PROPERTY_CUSTOM_EDIT( m_latchEvent, TXT("Event to trigger when the latch activates"), TXT( "AudioEventBrowser"));
	PROPERTY_CUSTOM_EDIT( m_boneName, TXT( "Bone on which effect is fired" ), TXT( "EntityBoneList" ) );
	PROPERTY_EDIT( m_speed, TXT( "When the fx has a target to move to, use this to control how fast the sound moves" ));
	PROPERTY_EDIT( m_decelDist, TXT("when a speed is being used the speed will decrease linearly wrt dist when below this distance" ))
END_CLASS_RTTI();
