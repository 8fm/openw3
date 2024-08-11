/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimSoundEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimSoundEvent )

public:
	CExtAnimSoundEvent();

	CExtAnimSoundEvent( const CName& eventName,	const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimSoundEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	RED_INLINE const StringAnsi& GetSoundEventName() const { return m_soundEventName; }

protected:
	StringAnsi				m_soundEventName;
	Float					m_maxDistance;
	Float					m_filterCooldown;
	CName					m_bone;
	TDynArray< StringAnsi >	m_switchesToUpdate;
	TDynArray< StringAnsi >	m_parametersToUpdate;
	Bool					m_filter;
	Bool					m_useDistanceParameter;
	Float					m_speed;
	Float					m_decelDist;			//!< when a speed is being used the speed will decrease linearly wrt dist when below this distance

};

BEGIN_CLASS_RTTI( CExtAnimSoundEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_CUSTOM_EDIT( m_soundEventName, TXT( "Sound event name" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_EDIT( m_maxDistance, TXT( "Maximum distance event is heard from" ) )
	PROPERTY_CUSTOM_EDIT( m_bone, TXT( "Bone on which sound is fired" ), TXT( "EntityBoneList" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_switchesToUpdate, TXT( "Select which switches should be updated just before launching event" ), TXT( "AudioSwitchBrowser" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_parametersToUpdate, TXT( "Select which parameters should be updated just before launching event" ), TXT( "AudioSwitchBrowser" ) )
	PROPERTY_EDIT( m_filter, TXT( "Is this event filterable" ) )
	PROPERTY_EDIT( m_filterCooldown, TXT( "The minimum time between multiple triggering of the same event on an individual bone" ) )
	PROPERTY_EDIT( m_useDistanceParameter, TXT( "Sets the parameter 'distance' (between source and listener) when playing a sound" ));
	PROPERTY_EDIT( m_speed, TXT( "Time for the position of the bone to transition to move for the next teleport"))
	PROPERTY_EDIT( m_decelDist, TXT("when a speed is being used the speed will decrease linearly wrt dist when below this distance" ))
END_CLASS_RTTI()
