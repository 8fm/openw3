/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimOnSlopeEvent.h"
#include "movingAgentComponent.h"
#include "movementAdjustor.h"
#include "..\engine\behaviorGraphStack.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimOnSlopeEvent );

RED_DEFINE_STATIC_NAME( OnSlope );

String CExtAnimOnSlopeEvent::GetDefaultTrackName()
{
	return CNAME( OnSlope ).AsString();
}

CExtAnimOnSlopeEvent::CExtAnimOnSlopeEvent()
	 : CExtAnimDurationEvent()
	 , m_slopeAngle( 0.0f )
{
	m_reportToScript = false;

	SetEventName( CNAME( OnSlope ));
}

CExtAnimOnSlopeEvent::CExtAnimOnSlopeEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( CNAME( OnSlope ), animationName, startTime, duration, trackName )
	, m_slopeAngle( 0.0f )
{
	m_reportToScript = false;
}

// this is called when names are the same and classes are the same too
Bool CExtAnimOnSlopeEvent::CanBeMergedWith( const CExtAnimEvent* _with ) const
{
	// it should be already checked if classes match, but there's nothing wrong with additional "assert"
	ASSERT( _with->GetClass()->IsA( CExtAnimOnSlopeEvent::GetStaticClass() ) );
	CExtAnimOnSlopeEvent* with = (CExtAnimOnSlopeEvent*)_with;
	return m_slopeAngle == with->m_slopeAngle;
}

#ifndef NO_EDITOR

void CExtAnimOnSlopeEvent::OnPropertyPostChanged( const CName& propertyName )
{
	// should always have "OnSlope" name
	SetEventName( CNAME( OnSlope ) );

	CExtAnimDurationEvent::OnPropertyPostChanged( propertyName );
}

#endif
