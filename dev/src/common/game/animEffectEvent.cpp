
#include "build.h"
#include "animEffectEvent.h"

IMPLEMENT_RTTI_ENUM( EAnimEffectAction ); 
IMPLEMENT_ENGINE_CLASS( CExtAnimEffectEvent );

CExtAnimEffectEvent::CExtAnimEffectEvent()
	: CExtAnimEvent()
	, m_effectName( CNAME( anim_effect ) )
	, m_action( EA_Start )
{
	m_reportToScript = false;
}

CExtAnimEffectEvent::CExtAnimEffectEvent( const CName& eventName,
	const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_effectName( CNAME( anim_effect ) )
	, m_action( EA_Start )
{
	m_reportToScript = false;
}

CExtAnimEffectEvent::~CExtAnimEffectEvent()
{

}

void CExtAnimEffectEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );

	if ( component )
	{
		CEntity* entity = component->GetEntity();

		if ( entity )
		{
			if( m_action == EA_Start )
			{
				entity->PlayEffect( m_effectName );
			}
			else if( m_action == EA_Stop )
			{
				entity->StopEffect( m_effectName );
			}
		}
	}
}

IMPLEMENT_ENGINE_CLASS( CExtAnimEffectDurationEvent );

CExtAnimEffectDurationEvent::CExtAnimEffectDurationEvent()
: CExtAnimDurationEvent()
, m_effectName( CNAME( anim_effect ) )
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

CExtAnimEffectDurationEvent::CExtAnimEffectDurationEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName )
: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
, m_effectName( CNAME( anim_effect ) )
{
	m_reportToScript = false;
	m_alwaysFiresEnd = true;
}

CExtAnimEffectDurationEvent::~CExtAnimEffectDurationEvent()
{

}

void CExtAnimEffectDurationEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimDurationEvent::Start( info, component );
	ProcessEffect( info, component, EA_Start );
}

void CExtAnimEffectDurationEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	CExtAnimDurationEvent::Stop( info, component );
	ProcessEffect( info, component, EA_Stop );
}

void CExtAnimEffectDurationEvent::ProcessEffect( const CAnimationEventFired& info, CAnimatedComponent* component, EAnimEffectAction action ) const
{
	CExtAnimEvent::Process( info, component );
	ASSERT( component );

	if ( component )
	{
		CEntity* entity = component->GetEntity();

		if ( entity )
		{
			if( action == EA_Start )
			{
				entity->PlayEffect( m_effectName );
			}
			else if( action == EA_Stop )
			{
				entity->StopEffect( m_effectName );
			}
		}
	}
}


