#include "build.h"
#include "storySceneEventModifyEnv.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventModifyEnv );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventModifyEnv );

CStorySceneEventModifyEnv* CStorySceneEventModifyEnv::Clone() const
{
	return new CStorySceneEventModifyEnv( *this );
}

void CStorySceneEventModifyEnv::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if( const CEnvironmentDefinition* envDef = m_environmentDefinition.Get() )
	{
		StorySceneEventsCollector::EnvChange evt;

		evt.m_environmentDefinition = envDef;
		evt.m_activate = m_activate;
		evt.m_priority = m_priority;
		evt.m_blendFactor = m_blendFactor;
		evt.m_blendInTime = m_blendInTime;
		evt.m_event = this;

		collector.AddEvent( evt );
	}
}
