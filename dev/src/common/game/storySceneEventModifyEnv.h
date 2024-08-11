#pragma once
#include "storySceneEvent.h"

class CStorySceneEventModifyEnv : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventModifyEnv, CStorySceneEvent )

	THandle<CEnvironmentDefinition> m_environmentDefinition;
	Bool m_activate;
	Int32 m_priority;
	Float m_blendFactor;
	Float m_blendInTime;

public:
	CStorySceneEventModifyEnv() : m_activate(false), m_priority( 0 ), m_blendFactor( 0.f ), m_blendInTime( 0.f )
	{}
	CStorySceneEventModifyEnv( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName )
		: CStorySceneEvent( eventName, sceneElement, startTime, trackName ), m_activate(false), m_priority( 0 ), m_blendFactor( 0.f ), m_blendInTime( 0.f )
	{}

	virtual CStorySceneEventModifyEnv* Clone() const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	DECLARE_STORY_SCENE_EVENT_GENERIC_CREATION( CStorySceneEventModifyEnv, TXT("Add misc event"), TXT("Modify environment"), TXT("IMG_DIALOG_WEATHER") )				
};

BEGIN_CLASS_RTTI( CStorySceneEventModifyEnv )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_environmentDefinition, TXT("") )
	PROPERTY_EDIT( m_activate, TXT("") )
	PROPERTY_EDIT( m_priority, TXT("") )
	PROPERTY_EDIT( m_blendFactor, TXT("") )
	PROPERTY_EDIT( m_blendInTime, TXT("") )
END_CLASS_RTTI()
