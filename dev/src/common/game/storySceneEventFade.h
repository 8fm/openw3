/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "storySceneEventDuration.h"

class CStorySceneEventFade : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventFade, CStorySceneEventDuration )

protected:
	Bool	m_in;
	Color	m_color;

public:
	CStorySceneEventFade();
	CStorySceneEventFade( const String& eventName, CStorySceneElement* sceneElement, Float startTime, Bool in, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventFade* Clone() const override;

	RED_INLINE Bool IsFadeIn() const { return m_in; }

public:
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;	
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

private:
	void GenerateEvent( const CStorySceneInstanceBuffer& instanceBuffer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventFade )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_EDIT( m_in, TXT( "Fade in (true) or fade out (false)" ) )
	PROPERTY_EDIT( m_color, TXT( "Fade color" ) )
END_CLASS_RTTI()
