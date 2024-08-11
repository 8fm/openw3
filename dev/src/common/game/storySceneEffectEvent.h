
#pragma once

#include "storySceneEvent.h"
#include "storySceneEventDuration.h"

class CStorySceneActorEffectEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneActorEffectEvent, CStorySceneEvent )

protected:
	CName	m_actor;
	CName	m_effectName;
	Bool	m_startOrStop;
	Bool	m_persistAcrossSections;

public:
	CStorySceneActorEffectEvent();
	CStorySceneActorEffectEvent( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneActorEffectEvent* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneActorEffectEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_effectName, TXT("") );
	PROPERTY_EDIT( m_startOrStop, TXT("start=true, stop=false") );
	PROPERTY_EDIT( m_persistAcrossSections, TXT("") );
END_CLASS_RTTI()


class CStoryScenePropEffectEvent : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStoryScenePropEffectEvent, CStorySceneEvent )

protected:
	CName	m_propID;
	CName	m_effectName;
	Bool	m_startOrStop;

public:
	CStoryScenePropEffectEvent();
	CStoryScenePropEffectEvent( const String & eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName);

	// compiler generated cctor is ok

	virtual CStoryScenePropEffectEvent* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStoryScenePropEffectEvent )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_propID, TXT( "PropID" ), TXT( "DialogPropTag" )  )
	PROPERTY_EDIT( m_effectName, TXT("") );
	PROPERTY_EDIT( m_startOrStop, TXT("start=true, stop=false") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneActorEffectEventDuration : public CStorySceneEventDuration
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneActorEffectEventDuration, CStorySceneEventDuration )

protected:
	CName	m_actor;
	CName	m_effectName;

public:
	CStorySceneActorEffectEventDuration();
	CStorySceneActorEffectEventDuration( const String& eventName, CStorySceneElement* sceneElement, Float startTime, CName actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneActorEffectEventDuration* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo )	const override;
};

BEGIN_CLASS_RTTI( CStorySceneActorEffectEventDuration )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_effectName, TXT("") );
END_CLASS_RTTI()
