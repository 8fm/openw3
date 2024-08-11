/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimEvent.h"

class CExtAnimCutsceneBodyPartEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneBodyPartEvent )

protected:
	CName m_appearance;

public:
	CExtAnimCutsceneBodyPartEvent();

	CExtAnimCutsceneBodyPartEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimCutsceneBodyPartEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	RED_INLINE CName GetAppearanceName() const { return m_appearance; }
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneBodyPartEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_appearance, TXT( "Appearance" ) );
END_CLASS_RTTI();
