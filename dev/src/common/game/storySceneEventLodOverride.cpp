// Copyright © 2016 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "storySceneEventLodOverride.h"
#include "storySceneEventsCollector.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventLodOverride );

/*
Ctor.
*/
CStorySceneEventLodOverride::CStorySceneEventLodOverride()
: m_actor( CName::NONE )
, m_forceHighestLod( false )
, m_disableAutoHide( false )
{}

/*
Ctor.
*/
CStorySceneEventLodOverride::CStorySceneEventLodOverride( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
, m_actor( actor )
, m_forceHighestLod( false )
, m_disableAutoHide( false )
{}

/*
Dtor.
*/
CStorySceneEventLodOverride::~CStorySceneEventLodOverride()
{}

CStorySceneEventLodOverride* CStorySceneEventLodOverride::Clone() const
{
	return new CStorySceneEventLodOverride( *this );
}

void CStorySceneEventLodOverride::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if( const Bool eventHasNoTarget = ( !m_actor && m_actorsByTag.Empty() ) )
	{
		return;
	}

	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::ActorLodOverride evt( this, m_actor );
	evt.m_eventTimeLocal = timeInfo.m_timeLocal;
	evt.m_eventTimeAbs = timeInfo.m_timeAbs;

	evt.m_actorsByTag = m_actorsByTag;
	evt.m_forceHighestLod = m_forceHighestLod;
	evt.m_disableAutoHide = m_disableAutoHide;

	collector.AddEvent( evt );
}
