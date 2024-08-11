
#pragma once

#include "storySceneEventAnimClip.h"

class CStorySceneEventExitActor : public CStorySceneEventAnimClip
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventExitActor, CStorySceneEventAnimClip )

protected:
	CName		m_behEvent;

public:
	CStorySceneEventExitActor();
	CStorySceneEventExitActor( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& behEvent, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventExitActor* Clone() const override;

protected:
	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const override;
public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;	
public:
	virtual const CName& GetAnimationName() const override { return m_behEvent; }
#ifndef  NO_EDITOR
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;	
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventExitActor )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_CUSTOM_EDIT( m_behEvent, TXT( "Behaviour event name" ), TXT( "DialogExitActor" ) )
END_CLASS_RTTI()
