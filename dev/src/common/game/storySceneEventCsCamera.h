#pragma once
#include "storysceneevent.h"

class CStorySceneEventCsCamera : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCsCamera, CStorySceneEvent )

public:
	CStorySceneEventCsCamera();
	CStorySceneEventCsCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventCsCamera* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventCsCamera )
	PARENT_CLASS( CStorySceneEvent )
END_CLASS_RTTI()
