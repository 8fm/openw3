#include "build.h"
#include "storySceneEventCsCamera.h"


IMPLEMENT_ENGINE_CLASS( CStorySceneEventCsCamera );


CStorySceneEventCsCamera::CStorySceneEventCsCamera()
{

}

CStorySceneEventCsCamera::CStorySceneEventCsCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
{
}

CStorySceneEventCsCamera* CStorySceneEventCsCamera::Clone() const
{
	return new CStorySceneEventCsCamera( *this );
}

void CStorySceneEventCsCamera::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
}
