/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "extAnimSoundEvent.h"
#include "..\engine\Behavior\SharedHeaders\enumSide.h"

/// Animation event that plays footstep sound
class CExtAnimFootstepEvent : public CExtAnimSoundEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimFootstepEvent )

	Bool m_fx;
	CName m_customFxName;

public:
	CExtAnimFootstepEvent();

	CExtAnimFootstepEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimFootstepEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void ProcessPostponed( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	void ProcessFoliage( CEntity* entity ) const;
};

BEGIN_CLASS_RTTI( CExtAnimFootstepEvent )
	PARENT_CLASS( CExtAnimSoundEvent )
	PROPERTY_EDIT( m_fx, TXT( "" ) )
	PROPERTY_EDIT( m_customFxName, TXT( "" ) )
END_CLASS_RTTI()

/// Forces footstep event. Should be used in all cases when automatic footstep detection fails.
class CExtForcedLogicalFootstepAnimEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtForcedLogicalFootstepAnimEvent )

	ESide m_side;

public:
	CExtForcedLogicalFootstepAnimEvent();
	CExtForcedLogicalFootstepAnimEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );

	void Process( const CAnimationEventFired&, CAnimatedComponent* component ) const override;

	ESide GetSide() const { return m_side; }
};

BEGIN_CLASS_RTTI( CExtForcedLogicalFootstepAnimEvent )
	PARENT_CLASS( CExtAnimEvent )
	PROPERTY_EDIT( m_side, TXT( "Left or right leg?" ) )
END_CLASS_RTTI()