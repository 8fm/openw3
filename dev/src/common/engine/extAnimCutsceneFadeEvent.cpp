/*
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "extAnimCutsceneFadeEvent.h"
#include "cutsceneDebug.h"
#include "game.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneFadeEvent );

CExtAnimCutsceneFadeEvent::CExtAnimCutsceneFadeEvent()
	: m_in( true )
	, m_duration( 1.0f )
	, m_color( Color::BLACK )
{
	m_reportToScript = false;
}

CExtAnimCutsceneFadeEvent::CExtAnimCutsceneFadeEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_in( true )
	, m_duration( 1.0f )
	, m_color( Color::BLACK )
{
	m_reportToScript = false;
}

CExtAnimCutsceneFadeEvent::~CExtAnimCutsceneFadeEvent()
{
}

void CExtAnimCutsceneFadeEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( !component );

	if ( m_in )
	{
		GGame->ResetFadeLock( TXT( "CExtAnimCutsceneFadeEvent" ) );
		GGame->StartFade( true, TXT( "CExtAnimCutsceneFadeEvent" ), m_duration );
	}
	else
	{
		GGame->StartFade( false, TXT( "CExtAnimCutsceneFadeEvent" ), m_duration, m_color );
		GGame->SetFadeLock( TXT( "CExtAnimCutsceneFadeEvent" ) );
	}
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
