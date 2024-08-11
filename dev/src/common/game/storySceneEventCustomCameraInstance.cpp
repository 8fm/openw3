
#include "build.h"
#include "storySceneEventCustomCameraInstance.h"
#include "storyScenePlayer.h"
#include "storyScene.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventCustomCameraInstance );

CStorySceneEventCustomCameraInstance::CStorySceneEventCustomCameraInstance()
	: CStorySceneEventCamera()
	, m_customCameraName()
	, m_enableCameraNoise( true )
{
}

CStorySceneEventCustomCameraInstance::CStorySceneEventCustomCameraInstance( const String& eventName, CStorySceneElement* sceneElement,
		   Float startTime, const String& trackName )
	: CStorySceneEventCamera( eventName, sceneElement, startTime, trackName )
	, m_customCameraName()
	, m_enableCameraNoise( true )
{
}

CStorySceneEventCustomCameraInstance* CStorySceneEventCustomCameraInstance::Clone() const
{
	return new CStorySceneEventCustomCameraInstance( *this );
}

void CStorySceneEventCustomCameraInstance::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	SCENE_ASSERT( scenePlayer );
	SCENE_ASSERT( scenePlayer->GetCurrentStoryScene() );

 	if ( !scenePlayer->IsInGameplay() )
	{
		// SCENE_TOMSIN_TODO - za duzo ->
		const StorySceneCameraDefinition* camera = scenePlayer->GetCurrentStoryScene()->GetCameraDefinition( m_customCameraName );
		if( camera )
		{
			StorySceneEventsCollector::CameraShot event( this );
			event.m_eventTimeAbs = timeInfo.m_timeAbs;
			event.m_eventTimeLocal = timeInfo.m_timeLocal;
			event.m_definition = *camera;
			event.m_enableCameraNoise = m_enableCameraNoise;

			collector.AddEvent( event );
		}
	}
}

const StorySceneCameraDefinition* CStorySceneEventCustomCameraInstance::GetCameraDefinition( CSceneEventFunctionSimpleArgs* ) const
{
	const CStoryScene* scene = m_sceneElement ? m_sceneElement->FindParent< CStoryScene >() : nullptr;
	return scene ? scene->GetCameraDefinition( m_customCameraName ) : nullptr;;
}

StorySceneCameraDefinition* CStorySceneEventCustomCameraInstance::GetCameraDefinition( CSceneEventFunctionSimpleArgs* )
{
	CStoryScene* scene = m_sceneElement ? m_sceneElement->FindParent< CStoryScene >() : nullptr;
	return scene ? scene->GetCameraDefinition( m_customCameraName ) : nullptr;;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
