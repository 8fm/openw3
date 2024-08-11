/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimRotationAdjustmentLocationBasedEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimRotationAdjustmentLocationBasedEvent )

public:
	CExtAnimRotationAdjustmentLocationBasedEvent();

	CExtAnimRotationAdjustmentLocationBasedEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimRotationAdjustmentLocationBasedEvent();

	void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const override;
	
protected:
	CName m_locationAdjustmentVar;
	CName m_targetLocationVar;
	CName m_adjustmentActiveVar;
};

BEGIN_CLASS_RTTI( CExtAnimRotationAdjustmentLocationBasedEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_locationAdjustmentVar, TXT( "Movement Adjustment location variable" ) );
	PROPERTY_EDIT( m_targetLocationVar, TXT( "Target location variable" ) );
	PROPERTY_EDIT( m_adjustmentActiveVar, TXT( "Is Movement Adjustment active" ) );
END_CLASS_RTTI();
