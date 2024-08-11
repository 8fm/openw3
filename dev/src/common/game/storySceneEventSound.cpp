/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventSound.h"
#include "..\..\common\engine\soundStartData.h"
#include "storyScenePlayer.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventSound );

CStorySceneEventSound::CStorySceneEventSound()
	: CStorySceneEvent()
	, m_soundEventName()
	, m_actor()
	, m_bone()
	, m_dbVolume( 0.0f )
{

}

CStorySceneEventSound::CStorySceneEventSound( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_soundEventName()
	, m_actor( actor )
	, m_bone()
	, m_dbVolume( 0.0f )
{

}

CStorySceneEventSound* CStorySceneEventSound::Clone() const
{
	return new CStorySceneEventSound( *this );
}

void CStorySceneEventSound::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const 
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	CActor* actor = scenePlayer->GetMappedActor( m_actor );
	if ( actor )
	{
		CSoundEmitterComponent* soundEmitterComponent = actor->GetSoundEmitterComponent();
		if ( soundEmitterComponent && scenePlayer->ShouldPlaySounds() )
		{
			// SCENE_TOMSIN_TODO - FindBoneByName by name za kazdym razem - lame....
			const Int32 bone = actor->GetRootAnimatedComponent() && m_bone ? actor->GetRootAnimatedComponent()->FindBoneByName( m_bone ) : -1;

			soundEmitterComponent->SoundEvent( m_soundEventName.AsChar(), bone );
		}
	}
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
