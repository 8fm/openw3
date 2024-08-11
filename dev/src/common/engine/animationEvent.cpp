/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "animationEvent.h"
#include "extAnimEvent.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

IMPLEMENT_RTTI_ENUM( EAnimationEventType );
IMPLEMENT_ENGINE_CLASS( SAnimationEventAnimInfo );

CAnimationEventFired::CAnimationEventFired()
	: m_type( AET_Tick )
	, m_alpha( 1.0f )
	, m_extEvent( nullptr )
{

}

Bool CAnimationEventFired::CanMergeWith( const CAnimationEventFired& event ) const
{
	ASSERT( m_extEvent != NULL );
	ASSERT( event.m_extEvent != NULL );

	return (	m_extEvent->GetEventName() == event.m_extEvent->GetEventName() &&
				m_extEvent->GetClass() == event.m_extEvent->GetClass() &&
				m_type == event.m_type &&
				m_extEvent->CanBeMergedWith( event.m_extEvent ));
}

void CAnimationEventFired::MergeWith( const CAnimationEventFired& event, Float alpha )
{
	m_alpha = Clamp< Float >( alpha * event.m_alpha + m_alpha, 0.f, 1.f );
}

const CName& CAnimationEventFired::GetEventName() const
{
	ASSERT( m_extEvent );
	return m_extEvent->GetEventName();
}

Float CAnimationEventFired::GetProgress() const
{
	const Float d = m_extEvent->VGetDuration();
	if ( d > 0.f )
	{
		return Clamp( ( m_animInfo.m_localTime - m_extEvent->GetStartTime() ) / d, 0.f, 1.f );
	}
	else
	{
		return 1.f;
	}
}

SAnimationEventAnimInfo::SAnimationEventAnimInfo()
	: m_animation( nullptr )
	, m_localTime( 0.0f )
	, m_eventEndsAtTime( 0.0f )
	, m_eventDuration( 0.0f )
{

}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
