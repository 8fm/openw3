/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimCutsceneBodyPartEvent.h"
#include "appearanceComponent.h"
#include "cutsceneDebug.h"
#include "animatedComponent.h"
#include "entity.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneBodyPartEvent );

CExtAnimCutsceneBodyPartEvent::CExtAnimCutsceneBodyPartEvent()
	: CExtAnimEvent()
{
	m_reportToScript = false;
}

CExtAnimCutsceneBodyPartEvent::CExtAnimCutsceneBodyPartEvent( const CName& eventName,
		 const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

CExtAnimCutsceneBodyPartEvent::~CExtAnimCutsceneBodyPartEvent()
{

}

void CExtAnimCutsceneBodyPartEvent::Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const
{
	ASSERT( component );

	if ( m_appearance )
	{
		if ( CAppearanceComponent* app = component->GetEntity()->FindComponent< CAppearanceComponent >() )
		{
			app->ApplyAppearance( m_appearance );
		}
	}
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
