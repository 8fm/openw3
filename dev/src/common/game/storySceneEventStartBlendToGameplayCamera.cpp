#include "build.h"
#include "storySceneEventStartBlendToGameplayCamera.h"
#include "storyScenePlayer.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneEventStartBlendToGameplayCamera );

CStorySceneEventStartBlendToGameplayCamera::CStorySceneEventStartBlendToGameplayCamera()
	: m_changesCamera( true )
	, m_blendTime( 4.0f )
	, m_lightsBlendTime( 5.f )
{}

CStorySceneEventStartBlendToGameplayCamera::CStorySceneEventStartBlendToGameplayCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const StorySceneCameraDefinition& camDef, const String& trackName )
	: CStorySceneEventCustomCamera( eventName, sceneElement, startTime, camDef, trackName )
	, m_changesCamera( true )
	, m_lightsBlendTime( 5.f )
	, m_blendTime( 4.0f )
{}

CStorySceneEventStartBlendToGameplayCamera* CStorySceneEventStartBlendToGameplayCamera::Clone() const
{
	return new CStorySceneEventStartBlendToGameplayCamera( *this );
}

void CStorySceneEventStartBlendToGameplayCamera::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	if ( m_changesCamera )
	{
		TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );
	}

	if ( scenePlayer->IsSceneInGame() && !scenePlayer->IsGameplayNow() )
	{
		StorySceneEventsCollector::CameraStartBlendToGameplay event( this );
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_blendTime = m_blendTime;
		event.m_lightsBlendTime = m_lightsBlendTime;

		collector.AddEvent( event );
	}
}