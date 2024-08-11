#pragma once
#include "storySceneEvent.h"

class CStorySceneEventWorldEntityEffect : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventWorldEntityEffect, CStorySceneEvent )

	CName	m_entityTag;
	CName	m_effectName;
	Bool	m_startStop;

public:
	CStorySceneEventWorldEntityEffect() : m_startStop( true )
	{}
	CStorySceneEventWorldEntityEffect( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_startStop( true )
	{}

	// compiler generated cctor is ok

	virtual CStorySceneEventWorldEntityEffect* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventWorldEntityEffect, TXT("Add misc event"), TXT("World entity effect"), TXT("IMG_DIALOG_EFFECT") )				
};

BEGIN_CLASS_RTTI( CStorySceneEventWorldEntityEffect )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_entityTag, TXT("") );
	PROPERTY_EDIT( m_effectName, TXT("") );
	PROPERTY_EDIT( m_startStop, TXT("true = start - false = stop") );
END_CLASS_RTTI()
