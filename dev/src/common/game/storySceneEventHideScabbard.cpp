#include "build.h"
#include "storySceneEventHideScabbard.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventHideScabbard );
IMPLEMENT_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventHideScabbard );


RED_DEFINE_STATIC_NAME( steel_scabbards )
RED_DEFINE_STATIC_NAME( silver_scabbards )

CStorySceneEventHideScabbard* CStorySceneEventHideScabbard::Clone() const
{
	return new CStorySceneEventHideScabbard( *this );
}



void CStorySceneEventHideScabbard::ProcessEntry( CName itemName, CInventoryComponent* invComponent, CStorySceneEventsCollector& collector ) const
{
	SItemUniqueId item = invComponent->GetItemByCategory( itemName, true );
	if( !item )
	{
		invComponent->GetItemByCategory( itemName, false );
	}
	if ( item )
	{
		StorySceneEventsCollector::ActorItemVisibility evt( this, m_actorId );
		evt.m_item = item;
		evt.m_showHide = m_setVisible;
		collector.AddEvent( evt );
	}
}


void CStorySceneEventHideScabbard::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	if ( CActor* actor = Cast< CActor >( scenePlayer->GetSceneActorEntity( m_actorId ) ) )
	{
		CInventoryComponent* invComponent = actor->GetInventoryComponent();
	
		if( invComponent )
		{
			ProcessEntry( CNAME( steel_scabbards ), invComponent, collector );
			ProcessEntry( CNAME( silver_scabbards ), invComponent, collector );
			ProcessEntry( CNAME( steelsword ), invComponent, collector );
			ProcessEntry( CNAME( silversword ), invComponent, collector );
		}
	}	
}
