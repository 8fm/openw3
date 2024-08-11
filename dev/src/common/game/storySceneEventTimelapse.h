#pragma once
#include "storySceneEvent.h"


class CStorySceneEventTimelapse : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventTimelapse, CStorySceneEvent )

	Bool	m_enable;
	Float	m_multiplier;

public:
	CStorySceneEventTimelapse()
	{}
	CStorySceneEventTimelapse( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	{}

	// compiler generated cctor is ok

	virtual CStorySceneEventTimelapse* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventTimelapse, TXT("Add misc event"), TXT("Timelapse"), TXT("IMG_DIALOG_MORPH") )				
};

BEGIN_CLASS_RTTI( CStorySceneEventTimelapse )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_enable, TXT("") )
	END_CLASS_RTTI()
