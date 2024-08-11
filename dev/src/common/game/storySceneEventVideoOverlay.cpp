// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventVideoOverlay.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventVideoOverlay );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventVideoOverlay );

CStorySceneEventVideoOverlay* CStorySceneEventVideoOverlay::Clone() const
{
	return new CStorySceneEventVideoOverlay( *this );
}

void CStorySceneEventVideoOverlay::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	StorySceneEventsCollector::VideoOverlay evt( m_fileName );
	collector.AddEvent( evt );
}
