// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventDebugComment.h"
#include "storyScenePlayer.h"
#include "storySceneEventsCollector.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventDebugComment )

/*
Ctor.
*/
CStorySceneEventDebugComment::CStorySceneEventDebugComment()
{}

/*
Ctor.
*/
CStorySceneEventDebugComment::CStorySceneEventDebugComment( const String & eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
: CStorySceneEventDuration( eventName, sceneElement, startTime, 2.0f, trackName )
{}

/*
Dtor.
*/
CStorySceneEventDebugComment::~CStorySceneEventDebugComment()
{}

CStorySceneEventDebugComment* CStorySceneEventDebugComment::Clone() const
{
	return new CStorySceneEventDebugComment( *this );
}

void CStorySceneEventDebugComment::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::DisplayDebugComment evt( this );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_display = scenePlayer->GetSceneDisplay();
	evt.m_commentId = GetGUID();
	evt.m_comment = m_comment;

	collector.AddEvent( evt );
}

void CStorySceneEventDebugComment::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::HideDebugComment evt( this );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;
	evt.m_display = scenePlayer->GetSceneDisplay();
	evt.m_commentId = GetGUID();

	collector.AddEvent( evt );
}
