/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventFade.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventFade );

CStorySceneEventFade::CStorySceneEventFade()
   : CStorySceneEventDuration()
   , m_in( true )
   , m_color( Color::BLACK )
{}

CStorySceneEventFade::CStorySceneEventFade( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, Bool in, const String& trackName)
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.0f, trackName )
    , m_in( in )
	, m_color( Color::BLACK )
{}

CStorySceneEventFade* CStorySceneEventFade::Clone() const
{
	return new CStorySceneEventFade( *this );
}

void CStorySceneEventFade::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
	if ( !scenePlayer->IsGameplayNow() )
	{
		GenerateEvent( data, collector, timeInfo );
	}
}

void CStorySceneEventFade::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( !scenePlayer->IsGameplayNow() )
	{
		GenerateEvent( data, collector, timeInfo );
	}
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );
}

void CStorySceneEventFade::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );
	if ( !scenePlayer->IsGameplayNow() )
	{
		GenerateEvent( data, collector, timeInfo );
	}
}

void CStorySceneEventFade::GenerateEvent( const CStorySceneInstanceBuffer& instanceBuffer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	collector.AddEvent( StorySceneEventsCollector::Fade::Fade( this, m_in, timeInfo.m_timeLocal, GetInstanceDuration( instanceBuffer ), m_color ) );
}
