#include "build.h"
#include "StorySceneEventWorldEntityEffect.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventWorldEntityEffect );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventWorldEntityEffect );

CStorySceneEventWorldEntityEffect* CStorySceneEventWorldEntityEffect::Clone() const
{
	return new CStorySceneEventWorldEntityEffect( *this );
}

void CStorySceneEventWorldEntityEffect::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if( CWorld* world = scenePlayer->GetLayer()->GetWorld() )
	{
		if( CEntity* ent = world->GetTagManager()->GetTaggedEntity( m_entityTag ) )
		{
			if( m_startStop )
			{
				ent->PlayEffect( m_effectName );
			}
			else
			{
				ent->StopEffect( m_effectName );
			}
		}
	}
}
