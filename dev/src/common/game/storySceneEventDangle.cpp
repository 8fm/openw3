
#include "build.h"
#include "storySceneEventDangle.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneDisableDangleEvent );

CStorySceneDisableDangleEvent::CStorySceneDisableDangleEvent() 
	: CStorySceneEvent()
	, m_weight( 0.f )
{

}

CStorySceneDisableDangleEvent::CStorySceneDisableDangleEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor(actor)
	, m_weight( 0.f )
{

}

CStorySceneDisableDangleEvent* CStorySceneDisableDangleEvent::Clone() const
{
	return new CStorySceneDisableDangleEvent( *this );
}

CName CStorySceneDisableDangleEvent::GetSubject() const
{
	return m_actor;
}

void CStorySceneDisableDangleEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorDisableDangle evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_weight = m_weight;

		collector.AddEvent( evt );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneResetClothAndDanglesEvent );

CStorySceneResetClothAndDanglesEvent::CStorySceneResetClothAndDanglesEvent() 
	: CStorySceneEvent()
	, m_forceRelaxedState( false )
{

}

CStorySceneResetClothAndDanglesEvent::CStorySceneResetClothAndDanglesEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor(actor)
	, m_forceRelaxedState( false )
{

}

CStorySceneResetClothAndDanglesEvent* CStorySceneResetClothAndDanglesEvent::Clone() const
{
	return new CStorySceneResetClothAndDanglesEvent( *this );
}

CName CStorySceneResetClothAndDanglesEvent::GetSubject() const
{
	return m_actor;
}

void CStorySceneResetClothAndDanglesEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorResetClothAndDangles evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_forceRelaxedState = m_forceRelaxedState;

		collector.AddEvent( evt );
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneDanglesShakeEvent );

CStorySceneDanglesShakeEvent::CStorySceneDanglesShakeEvent() 
	: CStorySceneEvent()
	, m_factor( 1.f )
{

}

CStorySceneDanglesShakeEvent::CStorySceneDanglesShakeEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
	, m_factor( 1.f )
{

}

CStorySceneDanglesShakeEvent* CStorySceneDanglesShakeEvent::Clone() const
{
	return new CStorySceneDanglesShakeEvent( *this );
}

CName CStorySceneDanglesShakeEvent::GetSubject() const
{
	return m_actor;
}

void CStorySceneDanglesShakeEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorDanglesShake evt( this, m_actor );
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;

		evt.m_factor = m_factor;

		collector.AddEvent( evt );
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
