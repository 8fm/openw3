
#include "build.h"
#include "expEvents.h"

IMPLEMENT_ENGINE_CLASS( CExpSyncEvent );

CExpSyncEvent::CExpSyncEvent()
	: CExtAnimEvent()
	, m_translation( true )
	, m_rotation( true )
{
	m_reportToScript = false;
}


CExpSyncEvent::CExpSyncEvent( const CName& eventName,
	const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_translation( true )
	, m_rotation( true )
{
	m_reportToScript = false;
}

void CExpSyncEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExpSlideEvent );

CExpSlideEvent::CExpSlideEvent()
	: CExtAnimDurationEvent()
	, m_translation( true )
	, m_rotation( false )
	, m_toCollision( false )
{
	m_reportToScript = false;
}


CExpSlideEvent::CExpSlideEvent(	const CName& eventName, 
	const CName& animationName, 
	Float startTime, 
	Float duration, 
	const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, duration, trackName )
	, m_translation( true )
    , m_rotation( false )
	, m_toCollision( false )
{
	m_reportToScript = false;
}

void CExpSlideEvent::Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
}

void CExpSlideEvent::Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
}
