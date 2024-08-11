
#pragma once

#include "storySceneEvent.h"

class CStorySceneLine;

/*
Dialog line event.
*/
class CStorySceneEventDialogLine: public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventDialogLine, CStorySceneEvent )

protected:
	CStorySceneLine*	m_line;		// Dialog line to be played. May be background or "normal" line.

public:
	CStorySceneEventDialogLine();
	CStorySceneEventDialogLine( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName, CStorySceneLine* l );

	// compiler generated cctor is ok

	virtual CStorySceneEventDialogLine* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	String GetLineText() const;
};

BEGIN_CLASS_RTTI( CStorySceneEventDialogLine )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_line, TXT( "Dialog line" ), TXT( "CDialogLineSelector" ) )
END_CLASS_RTTI()
