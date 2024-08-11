#pragma once
#include "storySceneEventCustomCamera.h"

class CStorySceneEventStartBlendToGameplayCamera : public CStorySceneEventCustomCamera
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventStartBlendToGameplayCamera, CStorySceneEventCustomCamera )

protected:
	Float					m_blendTime;
	Bool					m_changesCamera;

	Float					m_lightsBlendTime;
public:
	CStorySceneEventStartBlendToGameplayCamera();
	CStorySceneEventStartBlendToGameplayCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const StorySceneCameraDefinition& camDef, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventStartBlendToGameplayCamera* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventStartBlendToGameplayCamera )
	PARENT_CLASS( CStorySceneEventCustomCamera )
	PROPERTY_EDIT( m_blendTime, TXT( "Blend time (from scene to gameplay camera)" ) )
	PROPERTY_EDIT( m_changesCamera, TXT( "If set, changes camera" ) )
	PROPERTY_EDIT( m_lightsBlendTime, TXT( "" ) )
END_CLASS_RTTI()
