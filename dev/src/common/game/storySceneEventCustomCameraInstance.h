
#pragma once

#include "storySceneEvent.h"
#include "storySceneEventCamera.h"

class CStorySceneEventCustomCameraInstance : public CStorySceneEventCamera
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCustomCameraInstance, CStorySceneEventCamera )

private:
	CName	m_customCameraName;
	Bool	m_enableCameraNoise;

public:
	CStorySceneEventCustomCameraInstance();
	CStorySceneEventCustomCameraInstance( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventCustomCameraInstance* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual const StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL ) const override;
	virtual StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL ) override;
public:
	RED_INLINE const CName& GetCustomCameraName() const		{ return m_customCameraName; }
	RED_INLINE Bool  GetEnableCameraNoise() const				{ return m_enableCameraNoise; }
	RED_INLINE void SetCustomCameraName( const CName& name )	{ m_customCameraName = name; }
};

BEGIN_CLASS_RTTI( CStorySceneEventCustomCameraInstance )
	PARENT_CLASS( CStorySceneEventCamera )
	PROPERTY_CUSTOM_EDIT( m_customCameraName, TXT( "Name of custom camera" ), TXT( "CustomCameraSelector") )
	PROPERTY_EDIT( m_enableCameraNoise, TXT( "Should noise movement be enabled" ) );
END_CLASS_RTTI()
