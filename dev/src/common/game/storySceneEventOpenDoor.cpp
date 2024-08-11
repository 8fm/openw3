#include "build.h"
#include "storyScenePlayer.h"
#include "storySceneEventOpenDoor.h"
#include "doorComponent.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventOpenDoor );



CStorySceneEventOpenDoor::CStorySceneEventOpenDoor() 
	: m_instant( false )
	, m_openClose( false )
	, m_flipDirection( false )
{

}

CStorySceneEventOpenDoor::CStorySceneEventOpenDoor( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_instant( false ), m_openClose( false )
{
}

CStorySceneEventOpenDoor* CStorySceneEventOpenDoor::Clone() const
{
	return new CStorySceneEventOpenDoor( *this );
}

void CStorySceneEventOpenDoor::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	StorySceneEventsCollector::DoorChangeState evt;
	evt.m_event = this;
	evt.m_doorTag = m_doorTag;
	evt.m_instant = m_instant;
	evt.m_openClose = m_openClose;
	evt.m_flipDirection = m_flipDirection;
	collector.AddEvent( evt );
}
