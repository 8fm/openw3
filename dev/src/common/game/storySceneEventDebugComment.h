// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "storySceneEventDuration.h"

#pragma once

/*
Debug comment.
*/
class CStorySceneEventDebugComment: public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventDebugComment, CStorySceneEventDuration )

public:
	CStorySceneEventDebugComment();
	CStorySceneEventDebugComment( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );
	virtual ~CStorySceneEventDebugComment() override;

	// compiler generated cctor is ok
	// compiler generated op= is ok

	virtual CStorySceneEventDebugComment* Clone() const override;

private:
	String m_comment;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventDebugComment )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_EDIT( m_comment, TXT("Debug comment") );
END_CLASS_RTTI()
