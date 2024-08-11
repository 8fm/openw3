/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idEventSound.h"
#include "../../common/engine/tagManager.h"


IMPLEMENT_ENGINE_CLASS( CIdEventSound )


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIdEventSound::Activate( CIDTopicInstance* topicInstance )
{
	// No sounds on loading? Maybe we want to have a dialog while loading in the future
	if( GGame->IsLoadingScreenShown() ) return;

	// Get the entity
	CEntity* entity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( m_data.m_entityTag );
	if( ! entity )
	{
		return;
	}

	// Get the sound emitter
	CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
	if( !soundEmitterComponent ) return;

	// Is it a stop request?
	if( m_data.m_stop )
	{
		soundEmitterComponent->SoundStop();
	}
	else
	{
		// GetThe location if there is a bone
		Int32 bone	= -1;
		if( m_data.m_boneName )
		{
			CAnimatedComponent* component = entity->GetRootAnimatedComponent();
			if( component )
			{
				Int32 bone = component->FindBoneByName( m_data.m_boneName );
			}
		}
	
		// Call for the sound
		if( bone > -1 )
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_data.m_sound.AsChar() ), bone );
		}
		else
		{
			soundEmitterComponent->SoundEvent( UNICODE_TO_ANSI( m_data.m_sound.AsChar() ) );
		}
	}
}

