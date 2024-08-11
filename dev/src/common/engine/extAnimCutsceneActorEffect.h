/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "extAnimDurationEvent.h"

class CExtAnimCutsceneActorEffect : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimCutsceneActorEffect )

public:
	CExtAnimCutsceneActorEffect();

	CExtAnimCutsceneActorEffect( const CName& eventName, const CName& animationName,
		Float startTime, Float duration, const CName& effectName, const String& trackName );
	virtual ~CExtAnimCutsceneActorEffect();

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	RED_INLINE const CName& GetEffectName() const
	{ return m_effectName; }

	RED_INLINE void SetEffectName( const CName& effectName )
	{ m_effectName = effectName; }

protected:
	CName	m_effectName;
};

BEGIN_CLASS_RTTI( CExtAnimCutsceneActorEffect )
	PARENT_CLASS( CExtAnimDurationEvent )
	PROPERTY_CUSTOM_EDIT( m_effectName, TXT( "Effect name" ), TXT( "CutsceneActorEffect" ) )
END_CLASS_RTTI()
