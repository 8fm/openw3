/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "actorInterface.h"
#include "extAnimDurationEvent.h"
#include "extAnimEvent.h"

class CExtAnimComboEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimComboEvent )

public:
	CExtAnimComboEvent();

	CExtAnimComboEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
};

BEGIN_CLASS_RTTI( CExtAnimComboEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimHitEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimHitEvent )

public:
	CExtAnimHitEvent();

	CExtAnimHitEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	Uint32 GetHitLevel() const;

protected:
	Uint32			m_hitLevel;
};

BEGIN_CLASS_RTTI( CExtAnimHitEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_hitLevel, TXT( "Hit level" ) );
END_CLASS_RTTI();
