/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimRotationAdjustmentEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimRotationAdjustmentEvent )

public:
	CExtAnimRotationAdjustmentEvent();

	CExtAnimRotationAdjustmentEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimRotationAdjustmentEvent();

	void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;
	
protected:
	CName m_rotationAdjustmentVar;
};

BEGIN_CLASS_RTTI( CExtAnimRotationAdjustmentEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_rotationAdjustmentVar, TXT( "Movement Adjustment rotation direction variable (target yaw)" ) );
END_CLASS_RTTI();
