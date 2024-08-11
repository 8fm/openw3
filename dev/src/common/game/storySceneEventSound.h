/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CStorySceneEventSound : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventSound, CStorySceneEvent )

protected:
	StringAnsi	m_soundEventName;
	CName		m_actor;
	CName		m_bone;
	Float		m_dbVolume;

public:
	CStorySceneEventSound();
	CStorySceneEventSound( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventSound* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventSound )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_soundEventName, TXT( "Sound event name" ), TXT( "AudioEventBrowser" ) )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_bone, TXT( "Bone on which sound is fired" ) )
	PROPERTY_EDIT( m_dbVolume, TXT( "Sound volume in dB" ) )
END_CLASS_RTTI()
