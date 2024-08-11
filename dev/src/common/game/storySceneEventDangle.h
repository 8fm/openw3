
#pragma once

#include "storySceneEvent.h"

class CStorySceneDisableDangleEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneDisableDangleEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	Float	m_weight;

public:
	CStorySceneDisableDangleEvent();
	CStorySceneDisableDangleEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneDisableDangleEvent* Clone() const override;

	Float GetWeight() const { return m_weight; }

	virtual CName GetSubject() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneDisableDangleEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_weight, TXT("Weight") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneResetClothAndDanglesEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneResetClothAndDanglesEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	Bool	m_forceRelaxedState;

public:
	CStorySceneResetClothAndDanglesEvent();
	CStorySceneResetClothAndDanglesEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneResetClothAndDanglesEvent* Clone() const override;

	virtual CName GetSubject() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneResetClothAndDanglesEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_forceRelaxedState, TXT("Force relax state - heavy, use with care") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneDanglesShakeEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneDanglesShakeEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	Float	m_factor;

public:
	CStorySceneDanglesShakeEvent();
	CStorySceneDanglesShakeEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneDanglesShakeEvent* Clone() const override;

	Float GetFactor() const { return m_factor; }

	virtual CName GetSubject() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneDanglesShakeEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT_RANGE( m_factor, TXT("Weight"), 0.f, 1.f );
END_CLASS_RTTI()
