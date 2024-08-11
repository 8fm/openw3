#pragma once
#include "storysceneevent.h"

class CStorySceneEventCamera : public CStorySceneEvent //Pure virtual abstract class
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCamera, CStorySceneEvent )

public:
	CStorySceneEventCamera()
	{}

	CStorySceneEventCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	{}

	// compiler generated cctor is ok

	virtual Bool IsVolatileEvent() const { return false; }
	virtual const StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL ) const = 0;
	virtual StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL ) = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneEventCamera )
	PARENT_CLASS( CStorySceneEvent )
END_CLASS_RTTI()
