/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "extAnimEvent.h"
#include "extAnimDurationEvent.h"

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneDialogEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneDialogEvent )

public:
	CExtAnimCutsceneDialogEvent();

	CExtAnimCutsceneDialogEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneDialogEvent();
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneDialogEvent );
	PARENT_CLASS( CExtAnimEvent );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

class CExtAnimDisableDialogLookatEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimDisableDialogLookatEvent )

public:
	CExtAnimDisableDialogLookatEvent();

	CExtAnimDisableDialogLookatEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration,  const String& trackName );
	virtual ~CExtAnimDisableDialogLookatEvent();

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	//Bool	m_enableLookats;
	Float	m_speed;
};

BEGIN_CLASS_RTTI( CExtAnimDisableDialogLookatEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	//PROPERTY_EDIT( m_enableLookats, TXT("Enable or disable lookats") )
	PROPERTY_EDIT( m_speed, TXT("Speed at which lookats are to be disabled") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimCutsceneBreakEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneBreakEvent )

	Bool m_iAmHackDoNotUseMeInGame;

public:
	CExtAnimCutsceneBreakEvent();

	CExtAnimCutsceneBreakEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneBreakEvent();
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneBreakEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_RO( m_iAmHackDoNotUseMeInGame, TXT("I am hack. Do not use me in game!") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
