/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventAnimClip.h"

class CStorySceneEventEnterActor : public CStorySceneEventAnimClip
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventEnterActor, CStorySceneEventAnimClip )

protected:
	CName		m_behEvent;

public:
	CStorySceneEventEnterActor();
	CStorySceneEventEnterActor( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& behEvent, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventEnterActor* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	virtual const CName& GetAnimationName() const override { return m_behEvent; }

#ifndef  NO_EDITOR
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventEnterActor )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_CUSTOM_EDIT( m_behEvent, TXT( "Behaviour event name" ), TXT( "DialogEnterActor" ) )
END_CLASS_RTTI()
