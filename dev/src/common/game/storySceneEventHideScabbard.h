#pragma once
#include "storySceneEvent.h"

class CStorySceneEventHideScabbard : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventHideScabbard, CStorySceneEvent )

	CName	m_actorId;
	Bool	m_setVisible;

public:
	CStorySceneEventHideScabbard() : m_actorId( CNAME( GERALT ) )
	{}
	CStorySceneEventHideScabbard( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_actorId( CNAME( GERALT ) )
	{}

	// compiler generated cctor is ok

	virtual CStorySceneEventHideScabbard* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
private:
	void ProcessEntry( CName itemName, CInventoryComponent* invComponent, CStorySceneEventsCollector& collector ) const;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventHideScabbard, TXT("Add actor event"), TXT("Actor hide scabbard"), TXT("IMG_DIALOG_VISIBILITY") )				
};

BEGIN_CLASS_RTTI( CStorySceneEventHideScabbard )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_setVisible, TXT("") )
	PROPERTY_EDIT( m_actorId, TXT("") )
END_CLASS_RTTI()
