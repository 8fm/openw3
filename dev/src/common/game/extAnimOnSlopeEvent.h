/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimOnSlopeEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimOnSlopeEvent )

public:
	CExtAnimOnSlopeEvent();

	CExtAnimOnSlopeEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	
	virtual Bool CanBeMergedWith( const CExtAnimEvent* _with ) const;

#ifndef NO_EDITOR
	virtual void OnPropertyPostChanged( const CName& propertyName ) override;
#endif

	Float GetSlopeAngle() const { return m_slopeAngle; }

	static String GetDefaultTrackName();

protected:
	Float m_slopeAngle; // Positive - going up, negative - going down. In degrees.
};

BEGIN_CLASS_RTTI( CExtAnimOnSlopeEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_slopeAngle, TXT( "Positive - going up, negative - going down. In degrees." ) );
END_CLASS_RTTI();
