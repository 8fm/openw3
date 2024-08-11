
#include "build.h"
#include "storySceneEventEquipItem.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"
#include "extAnimItemEvents.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventEquipItem );

IMPLEMENT_RTTI_ENUM( ESceneItemEventMode )

CStorySceneEventEquipItem::CStorySceneEventEquipItem() 
	: CStorySceneEvent()
	, m_leftItem( CNAME(Any) )
	, m_rightItem( CNAME(Any) )
	, m_instant( true )
	, m_internalMode( SIEM_Default )
{

}

CStorySceneEventEquipItem::CStorySceneEventEquipItem( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor(actor)
	, m_leftItem( CNAME(Any) )
	, m_rightItem( CNAME(Any) )
	, m_instant( true )
	, m_internalMode( SIEM_Default )
{

}

CStorySceneEventEquipItem* CStorySceneEventEquipItem::Clone() const
{
	return new CStorySceneEventEquipItem( *this );
}

void CStorySceneEventEquipItem::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorItem evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		
		evt.m_leftItem = m_leftItem;
		evt.m_rightItem = m_rightItem;
		evt.m_instant = m_instant;
		evt.m_useMountInstead = m_internalMode == SIEM_Mount;
		evt.m_useUnmountInstead = m_internalMode == SIEM_Unmount;

		collector.AddEvent( evt );
	}
}

void CStorySceneEventEquipItem::PreprocessItems( CStoryScenePlayer* player ) const
{
	CEntity* ent = player->GetSceneActorEntity( m_actor );
	if( CGameplayEntity* gpEnt = Cast< CGameplayEntity >( ent ) )
	{
		auto func = [ player, gpEnt ]( CName item, CName actor, CName ignoreItemsWithTag )
		{
			SItemUniqueId spawnedItem;
			if( item != CNAME( Any ) && item != CName::NONE )
			{
				CExtAnimItemEvent::PreprocessItem( gpEnt, spawnedItem, item, ignoreItemsWithTag );
			}	
			if ( spawnedItem )
			{
				player->RegisterSpawnedItem( MakePair( actor, spawnedItem ) );
			}
		};

		func( m_leftItem, m_actor, m_ignoreItemsWithTag );
		func( m_rightItem, m_actor, m_ignoreItemsWithTag );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneMorphEvent );

CStorySceneMorphEvent::CStorySceneMorphEvent() 
	: CStorySceneEvent()
	, m_weight( 0.f )
{

}

CStorySceneMorphEvent::CStorySceneMorphEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor(actor)
	, m_weight( 0.f )
{

}

CStorySceneMorphEvent* CStorySceneMorphEvent::Clone() const
{
	return new CStorySceneMorphEvent( *this );
}

void CStorySceneMorphEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor && m_morphComponentId )
	{
		StorySceneEventsCollector::ActorMorph evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_weight = m_weight;
		evt.m_morphComponentId = m_morphComponentId;

		collector.AddEvent( evt );
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
