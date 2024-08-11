/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimCutsceneQuestEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneQuestEvent )

public:
	CExtAnimCutsceneQuestEvent();

	CExtAnimCutsceneQuestEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName,
		const String& cutsceneName );

	virtual ~CExtAnimCutsceneQuestEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

private:
	String	m_cutsceneName;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneQuestEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY( m_cutsceneName )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneResetClothAndDangleEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneResetClothAndDangleEvent )

protected:
	Bool	m_forceRelaxedState;

public:
	CExtAnimCutsceneResetClothAndDangleEvent();

	CExtAnimCutsceneResetClothAndDangleEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneResetClothAndDangleEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneResetClothAndDangleEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_forceRelaxedState, TXT("Force relax state - heavy, use with care") );
END_CLASS_RTTI();


class CExtAnimCutsceneDisableClothEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneDisableClothEvent )

protected:
	Float	m_weight;
	Float	m_blendTime;

public:
	CExtAnimCutsceneDisableClothEvent();

	CExtAnimCutsceneDisableClothEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneDisableClothEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneDisableClothEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT_RANGE( m_weight, TXT("Weight"), 0.f, 1.f );
	PROPERTY_EDIT_RANGE( m_blendTime, TXT("Blend time"), 0.f, 1.f );
END_CLASS_RTTI();


class CExtAnimCutsceneDisableDangleEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneDisableDangleEvent )

	Float	m_weight;

public:
	CExtAnimCutsceneDisableDangleEvent() : m_weight( 0.f ) {}
	CExtAnimCutsceneDisableDangleEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
		: CExtAnimEvent( eventName, animationName, startTime, trackName ), m_weight( 0.f ) {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneDisableDangleEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_EDIT_RANGE( m_weight, TXT("Weight"), 0.f, 1.f );
END_CLASS_RTTI()
