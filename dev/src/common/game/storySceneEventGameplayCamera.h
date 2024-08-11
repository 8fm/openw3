#pragma once
#include "storysceneeventcamera.h"
#include "storySceneCameraDefinition.h"

class CStorySceneEventGameplayCamera : public CStorySceneEventCamera
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventGameplayCamera, CStorySceneEventCamera )

protected:
	 TInstanceVar< StorySceneCameraDefinition > i_cameraDefinition;
	 TInstanceVar< Bool >						i_cameraResetScheduled;

public:
	CStorySceneEventGameplayCamera();
	CStorySceneEventGameplayCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );
	CStorySceneEventGameplayCamera( const CStorySceneEventGameplayCamera& other );

	virtual CStorySceneEventGameplayCamera* Clone() const override;

	virtual Bool IsVolatileEvent() const override;
	virtual const StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args ) const override;
	virtual StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args ) override;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CStorySceneInstanceBuffer& instance ) const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

private:
	void UpdateCameraDefinition( CSceneEventFunctionSimpleArgs* args ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventGameplayCamera )
	PARENT_CLASS( CStorySceneEventCamera )
END_CLASS_RTTI()
