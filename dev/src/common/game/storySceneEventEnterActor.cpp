/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEventEnterActor.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector_events.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventEnterActor );

CStorySceneEventEnterActor::CStorySceneEventEnterActor()
   : CStorySceneEventAnimClip()
{
	m_blendIn = 0.f;
}

CStorySceneEventEnterActor::CStorySceneEventEnterActor( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& behEvent, const String& trackName )
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
{
	m_blendIn = 0.f;
}


CStorySceneEventEnterActor* CStorySceneEventEnterActor::Clone() const
{
	return new CStorySceneEventEnterActor( *this );
}

void CStorySceneEventEnterActor::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorVisibility evt( this, m_actor );
		evt.m_eventTimeAbs = timeInfo.m_timeAbs;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;

		evt.m_showHide = true;

		collector.AddEvent( evt );
	}
}

void CStorySceneEventEnterActor::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	if ( m_actor )
	{
		StorySceneEventsCollector::ActorVisibility evt( this, m_actor );
		evt.m_eventTimeAbs = timeInfo.m_timeLocal;
		evt.m_eventTimeLocal = timeInfo.m_timeLocal;

		evt.m_showHide = true;

		collector.AddEvent( evt );
	}
}

#ifndef NO_EDITOR
void CStorySceneEventEnterActor::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	m_eventName = m_behEvent.AsString();
}
#endif

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
