#pragma once
#include "storySceneEvent.h"


class CStorySceneEventOpenDoor : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventOpenDoor, CStorySceneEvent )

protected:
	CName m_doorTag;
	Bool  m_instant;
	Bool  m_openClose;
	Bool  m_flipDirection;

public:
	CStorySceneEventOpenDoor();
	CStorySceneEventOpenDoor( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventOpenDoor* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventOpenDoor )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_doorTag, TXT( "" ) )
	PROPERTY_EDIT( m_instant, TXT( "" ) )
	PROPERTY_EDIT( m_openClose, TXT( "true - open / false - close" ) )	
	PROPERTY_EDIT( m_flipDirection, TXT( "" ) )
END_CLASS_RTTI()