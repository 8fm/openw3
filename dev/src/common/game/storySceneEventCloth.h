#pragma once

#include "storySceneEvent.h"

class CStorySceneDisablePhysicsClothEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneDisablePhysicsClothEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	Float	m_weight;
	Float	m_blendTime;

public:
	CStorySceneDisablePhysicsClothEvent();
	CStorySceneDisablePhysicsClothEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneDisablePhysicsClothEvent* Clone() const override;

	Float GetWeight() const { return m_weight; }
	Float GetBlendTime() const { return m_blendTime; }

	virtual CName GetSubject() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneDisablePhysicsClothEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT_RANGE( m_weight, TXT("Weight"), 0.f, 1.f );
	PROPERTY_EDIT_RANGE( m_blendTime, TXT("Blend time"), 0.f, 1.f );
END_CLASS_RTTI()
