/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimLocationAdjustmentEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimLocationAdjustmentEvent )

public:
	CExtAnimLocationAdjustmentEvent();

	CExtAnimLocationAdjustmentEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimLocationAdjustmentEvent();

	void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;
	
protected:
	CName m_locationAdjustmentVar;
	CName m_adjustmentActiveVar;
};

BEGIN_CLASS_RTTI( CExtAnimLocationAdjustmentEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_locationAdjustmentVar, TXT( "Movement Adjustment location variable" ) );
	PROPERTY_EDIT( m_adjustmentActiveVar, TXT( "Is Movement Adjustment active" ) );
END_CLASS_RTTI();
