#include "build.h"
#include "storySceneEventReward.h"
#include "storyScenePlayer.h"
#include "storySceneVoicetagMapping.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventReward );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventReward );

RED_DEFINE_STATIC_NAME( AddItemOnNPC_S );

CStorySceneEventReward::CStorySceneEventReward( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName ) 
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_npcTag( CNAME( PLAYER ) ), m_quantity( 1 ), m_dontInformGui( false )
{
}

CStorySceneEventReward::CStorySceneEventReward()
	: m_npcTag( CNAME( PLAYER ) ), m_quantity( 1 ), m_dontInformGui( false )
{

}

CStorySceneEventReward* CStorySceneEventReward::Clone() const
{
	return new CStorySceneEventReward( *this );
}

void CStorySceneEventReward::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if( scenePlayer->IsSceneInGame() )
	{				
		Bool res = CallFunction( nullptr, CNAME(AddItemOnNPC_S), THandle<CStoryScenePlayer>( scenePlayer ), m_npcTag, m_itemName, m_quantity, m_dontInformGui );
		ASSERT( res );

		CWorld* world = scenePlayer->GetLayer()->GetWorld();		
		if ( GCommonGame && world )
		{
			CActor* target = nullptr;
			if ( m_npcTag == CNAME( PLAYER ) )
			{
				target = GCommonGame->GetPlayer();
			}
			else
			{
				target = Cast< CActor >( world->GetTagManager()->GetTaggedEntity( m_npcTag ) );
			}

			if( target )
			{
				GCommonGame->GiveRewardTo( m_rewardName, target );
			}
		}
	}		
}

IMPLEMENT_ENGINE_CLASS( CStorySceneEventSetupItemForSync );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventSetupItemForSync );


CStorySceneEventSetupItemForSync* CStorySceneEventSetupItemForSync::Clone() const
{
	return new CStorySceneEventSetupItemForSync( *this );
}


void CStorySceneEventSetupItemForSync::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{	
	StorySceneEventsCollector::SyncItemInfo evt;	
	evt.m_itemName = m_itemName;
	evt.m_actorTag = m_actorToSyncTo;
	evt.m_activate = m_activate;
	collector.AddEvent( evt );
}