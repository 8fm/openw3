// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#pragma once

#include "storySceneEvent.h"

class CStorySceneEventVideoOverlay : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventVideoOverlay, CStorySceneEvent )

public:
	CStorySceneEventVideoOverlay();
	CStorySceneEventVideoOverlay( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	virtual CStorySceneEventVideoOverlay* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

private:
	String m_fileName;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventVideoOverlay, TXT( "Add misc event" ), TXT( "Video overlay" ), TXT( "IMG_DIALOG_MORPH" ) )
};

BEGIN_CLASS_RTTI( CStorySceneEventVideoOverlay )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_fileName, TXT("") )
END_CLASS_RTTI()

// =================================================================================================

RED_INLINE CStorySceneEventVideoOverlay::CStorySceneEventVideoOverlay()
{}

RED_INLINE CStorySceneEventVideoOverlay::CStorySceneEventVideoOverlay( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
{}
