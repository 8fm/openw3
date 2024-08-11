/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimEvent.h"
#include "extAnimDurationEvent.h"
#include "extAnimScriptEvent.h"
#include "animatedComponent.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimScriptEvent );

RED_DEFINE_NAME( OnSlideToTargetAnimEvent );
RED_DEFINE_NAME( OnEnumAnimEvent );

CExtAnimScriptEvent::CExtAnimScriptEvent()
	: CExtAnimEvent()
{
	m_reportToScript = false;
}

CExtAnimScriptEvent::CExtAnimScriptEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

CExtAnimScriptEvent::~CExtAnimScriptEvent()
{

}

void CExtAnimScriptEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );

	SendEventToScript( component->GetEntity(), info.m_type, info.m_animInfo );
}

void CExtAnimScriptEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity );
	entity->CallEvent( CNAME( OnAnimScriptEvent ), GetEventName(), 0.f, type, animInfo );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SMultiValue );
IMPLEMENT_ENGINE_CLASS( CEASMultiValueSimpleEvent );

void CEASMultiValueSimpleEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity && m_callback );
	entity->CallEvent( m_callback, GetEventName(), m_properties, type, animInfo );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimScriptDurationEvent );

CExtAnimScriptDurationEvent::CExtAnimScriptDurationEvent()
	: CExtAnimDurationEvent()
{
	m_reportToScript = false;
}

CExtAnimScriptDurationEvent::CExtAnimScriptDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
{
	m_reportToScript = false;
}

CExtAnimScriptDurationEvent::~CExtAnimScriptDurationEvent()
{

}

void CExtAnimScriptDurationEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimDurationEvent::Process( info, component );

	SendEventToScript( component->GetEntity(), info.m_type, info.m_animInfo );
}

void CExtAnimScriptDurationEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity );
	entity->CallEvent( CNAME( OnAnimScriptEvent ), GetEventName(), m_duration, type, animInfo );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SSlideToTargetEventProps );
IMPLEMENT_ENGINE_CLASS( CEASSlideToTargetEvent );

void CEASSlideToTargetEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity );
	entity->CallEvent( CNAME( OnSlideToTargetAnimEvent ), GetEventName(), m_properties, type, m_duration, animInfo );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SEnumVariant );
IMPLEMENT_ENGINE_CLASS( CEASEnumEvent );

void CEASEnumEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity );
	entity->CallEvent( CNAME( OnEnumAnimEvent ), GetEventName(), m_enumVariant, type, m_duration, animInfo );
}

Bool CEASEnumEvent::CanBeMergedWith( const CExtAnimEvent* _with ) const
{
	// it should be already checked if classes match, but there's nothing wrong with additional "assert"
	ASSERT( _with->GetClass()->IsA( CEASEnumEvent::GetStaticClass() ) );
	CEASEnumEvent* with = (CEASEnumEvent*)_with;
	return m_enumVariant.m_enumType == with->m_enumVariant.m_enumType &&
		   m_enumVariant.m_enumValue == with->m_enumVariant.m_enumValue;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CEASMultiValueEvent );

void CEASMultiValueEvent::SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const
{
	ASSERT( entity && m_callback );
	entity->CallEvent( m_callback, GetEventName(), m_properties, type, m_duration, animInfo );
}