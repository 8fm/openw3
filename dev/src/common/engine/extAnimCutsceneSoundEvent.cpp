/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "extAnimCutsceneSoundEvent.h"
#include "soundStartData.h"
#include "cutsceneDebug.h"
#include "soundSystem.h"
#include "entity.h"

#ifdef DEBUG_CUTSCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CExtAnimCutsceneSoundEvent );

CExtAnimCutsceneSoundEvent::CExtAnimCutsceneSoundEvent()
	: m_soundEventName()
	, m_bone( CNAME( Trajectory ) )
	, m_useMaterialInfo( false )
{
	m_reportToScript = false;
}

CExtAnimCutsceneSoundEvent::CExtAnimCutsceneSoundEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
	, m_soundEventName()
	, m_bone( CNAME( Trajectory ) )
	, m_useMaterialInfo( false )
{
	m_reportToScript = false;
}

CExtAnimCutsceneSoundEvent::~CExtAnimCutsceneSoundEvent()
{

}

Bool CExtAnimCutsceneSoundEvent::PlaySound( CComponent* animatedComponent, Int32 bone ) const
{

	if( animatedComponent == NULL )
	{
		GSoundSystem->SoundEvent( GetSoundEventName().AsChar() );
		return true;
	}

	CEntity *entity = Cast< CEntity >( animatedComponent->GetParent() );
	ASSERT( entity );

	const StringAnsi& eventName = GetSoundEventName();

	CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
	if( !soundEmitterComponent ) return false;

	soundEmitterComponent->SoundEvent( eventName.AsChar(), bone );

#ifdef SOUND_EDITOR_STUFF
	if( GetUseMaterialInfo() )
	{
//		soundEmitterComponent->SoundSwitch( CSoundSystem::PARAM_MATERIAL, value->AsChar(), bone );
	}
#endif
	
	return true;
}

#ifdef DEBUG_CUTSCENES
#pragma optimize("",on)
#endif
