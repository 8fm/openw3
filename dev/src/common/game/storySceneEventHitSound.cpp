
#include "build.h"
#include "storySceneEventHitSound.h"
#include "storyScenePlayer.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventHitSound );

CStorySceneEventHitSound::CStorySceneEventHitSound()
	: CStorySceneEvent()
{

}

CStorySceneEventHitSound::CStorySceneEventHitSound( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
{

}

CStorySceneEventHitSound* CStorySceneEventHitSound::Clone() const
{
	return new CStorySceneEventHitSound( *this );
}

void CStorySceneEventHitSound::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	CActor* actor = scenePlayer->GetMappedActor( m_actor );
	if ( !actor ) return;

	CSoundEmitterComponent* soundComponent = actor->GetSoundEmitterComponent();
	if( soundComponent )
	{
		soundComponent->SoundSwitch( "opponent_attack_type", UNICODE_TO_ANSI( m_soundAttackType.AsChar() ) );
		soundComponent->SoundSwitch( "opponent_weapon_type", UNICODE_TO_ANSI( m_actorAttackerWeaponSlot.AsChar() ) );
		soundComponent->SoundSwitch( "opponent_weapon_size", UNICODE_TO_ANSI( m_actorAttackerWeaponName.AsChar() ) );
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
