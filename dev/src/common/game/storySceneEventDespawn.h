/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEvent.h"

class CStorySceneEventDespawn : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventDespawn, CStorySceneEvent )

protected:
	CName		m_actor;

public:
	CStorySceneEventDespawn();
	CStorySceneEventDespawn( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventDespawn* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventDespawn )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventVisibility : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventVisibility, CStorySceneEvent )

protected:
	CName		m_actor;
	Bool		m_showHideFlag;

public:
	CStorySceneEventVisibility();
	CStorySceneEventVisibility( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventVisibility* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventVisibility )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_showHideFlag, TXT( "true = Show, false = Hide" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventApplyAppearance : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventApplyAppearance, CStorySceneEvent )

protected:
	CName		m_actor;
	CName		m_appearance;

public:
	CStorySceneEventApplyAppearance();
	CStorySceneEventApplyAppearance( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventApplyAppearance* Clone() const override;

public:
	CName GetAppearance() const { return m_actor; };
	CName GetActor() const { return m_appearance; };
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventApplyAppearance )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_appearance, TXT( "Appearance" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventPropVisibility : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventPropVisibility, CStorySceneEvent )

protected:
	CName		m_propID;
	Bool		m_showHideFlag;

public:
	CStorySceneEventPropVisibility();
	CStorySceneEventPropVisibility( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& prop, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventPropVisibility* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventPropVisibility )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_propID, TXT( "PropID" ), TXT( "DialogPropTag" )  )
	PROPERTY_EDIT( m_showHideFlag, TXT( "true = Show, false = Hide" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventWeatherChange : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventWeatherChange, CStorySceneEvent )

protected:
	CName	m_weatherName;
	Float	m_blendTime;

public:
	CStorySceneEventWeatherChange();
	CStorySceneEventWeatherChange( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventWeatherChange* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventWeatherChange )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_EDIT( m_weatherName, TXT( "" ) )
	PROPERTY_EDIT( m_blendTime, TXT( "" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventMimicLod : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventMimicLod, CStorySceneEvent )

protected:
	CName	m_actor;
	Bool	m_setMimicOn;

public:
	CStorySceneEventMimicLod();
	CStorySceneEventMimicLod( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventMimicLod* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventMimicLod )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_setMimicOn, TXT( "" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventUseHiresShadows : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventUseHiresShadows, CStorySceneEvent )

protected:
	CName	m_actor;
	Bool	m_useHiresShadows;

public:
	CStorySceneEventUseHiresShadows();
	CStorySceneEventUseHiresShadows( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventUseHiresShadows* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventUseHiresShadows )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_useHiresShadows, TXT( "" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
