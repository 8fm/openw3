#include "build.h"
#include "storySceneEventTimelapse.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventTimelapse );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventTimelapse );

CStorySceneEventTimelapse* CStorySceneEventTimelapse::Clone() const
{
	return new CStorySceneEventTimelapse( *this );
}

void CStorySceneEventTimelapse::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	StorySceneEventsCollector::TimeMultiplier evt;
	evt.m_enable = m_enable;
	evt.m_multiplier = m_multiplier;

	collector.AddEvent( evt );
}
