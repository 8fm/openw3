
#include "build.h"
#include "storySceneEventCloth.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneDisablePhysicsClothEvent );

CStorySceneDisablePhysicsClothEvent::CStorySceneDisablePhysicsClothEvent() 
	: CStorySceneEvent()
	, m_weight( 0.f )
	, m_blendTime( 1.f )
{

}

CStorySceneDisablePhysicsClothEvent::CStorySceneDisablePhysicsClothEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor(actor)
	, m_weight( 0.f )
	, m_blendTime( 1.f )
{

}

CStorySceneDisablePhysicsClothEvent* CStorySceneDisablePhysicsClothEvent::Clone() const
{
	return new CStorySceneDisablePhysicsClothEvent( *this );
}

CName CStorySceneDisablePhysicsClothEvent::GetSubject() const
{
	return m_actor;
}

void CStorySceneDisablePhysicsClothEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorDisablePhysicsCloth evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		
		evt.m_blendTime = m_blendTime;
		evt.m_weight = m_weight;

		collector.AddEvent( evt );
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
