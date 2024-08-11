
#include "build.h"
#include "storySceneEffectEvent.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneActorEffectEvent );

CStorySceneActorEffectEvent::CStorySceneActorEffectEvent() 
	: CStorySceneEvent()
	, m_startOrStop( true )
	, m_persistAcrossSections( false )
{}

CStorySceneActorEffectEvent::CStorySceneActorEffectEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
	, m_startOrStop( true )
	, m_persistAcrossSections( false )
{}


CStorySceneActorEffectEvent* CStorySceneActorEffectEvent::Clone() const
{
	return new CStorySceneActorEffectEvent( *this );
}

void CStorySceneActorEffectEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::PlayEffect evt( this, m_actor, m_startOrStop, m_persistAcrossSections );
	evt.m_effectName = m_effectName;
	collector.AddEvent( evt );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStoryScenePropEffectEvent );

void CStoryScenePropEffectEvent::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
	
	StorySceneEventsCollector::PlayEffect evt( this, m_propID, m_startOrStop );
	evt.m_effectName = m_effectName;
	collector.AddEvent( evt );
}

CStoryScenePropEffectEvent::CStoryScenePropEffectEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName id, const String& trackName ) 
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_startOrStop( true )
{
	if( CStorySceneSection* section = sceneElement->GetSection() )
	{
		if( CStoryScene* scene =  section->GetScene() )
		{
			if( scene->GetPropDefinition( id ) )
			{
				m_propID = id;
			}
		}
	}	
}

CStoryScenePropEffectEvent::CStoryScenePropEffectEvent() 
	: CStorySceneEvent()
	, m_startOrStop( true )
{

}

CStoryScenePropEffectEvent* CStoryScenePropEffectEvent::Clone() const
{
	return new CStoryScenePropEffectEvent( *this );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneActorEffectEventDuration );

CStorySceneActorEffectEventDuration::CStorySceneActorEffectEventDuration() 
	: CStorySceneEventDuration()
{}

CStorySceneActorEffectEventDuration::CStorySceneActorEffectEventDuration( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName)
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.f, trackName )
	, m_actor( actor )
{}

CStorySceneActorEffectEventDuration* CStorySceneActorEffectEventDuration::Clone() const
{
	return new CStorySceneActorEffectEventDuration( *this );
}

void CStorySceneActorEffectEventDuration::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::PlayEffect evt( this, m_actor, true );
	evt.m_effectName = m_effectName;
	collector.AddEvent( evt );
}

void CStorySceneActorEffectEventDuration::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	StorySceneEventsCollector::PlayEffect evt( this, m_actor, false );
	evt.m_effectName = m_effectName;
	collector.AddEvent( evt );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
