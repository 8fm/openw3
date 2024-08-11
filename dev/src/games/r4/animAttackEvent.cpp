
#include "build.h"
#include "animAttackEvent.h"


IMPLEMENT_ENGINE_CLASS( CExtAnimAttackEvent );

CExtAnimAttackEvent::CExtAnimAttackEvent()
	: CExtAnimEvent(), m_soundAttackType( CNAME( default ) )
{
}

CExtAnimAttackEvent::CExtAnimAttackEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName ), m_soundAttackType( CNAME( default ) )
{
}

CExtAnimAttackEvent::~CExtAnimAttackEvent()
{

}

RED_DEFINE_STATIC_NAME( OnAnimAttackEvent )