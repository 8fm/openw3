#pragma once
#include "storysceneevent.h"

class CStorySceneAddFactEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneAddFactEvent, CStorySceneEvent )

protected:
	String m_factId;
	Int32  m_expireTime;
	Int32  m_factValue;

public:
	CStorySceneAddFactEvent();
	CStorySceneAddFactEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneAddFactEvent* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneAddFactEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_factId, TXT( "" ) )
	PROPERTY_EDIT( m_expireTime, TXT( "" ) )
	PROPERTY_EDIT( m_factValue, TXT( "" ) )	
END_CLASS_RTTI()