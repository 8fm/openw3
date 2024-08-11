/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "soundAmbientEmitter.h"
#include "soundStartData.h"
#include "spriteComponent.h"
#include "world.h"
#include "soundEmitter.h"

IMPLEMENT_ENGINE_CLASS( CSoundAmbientEmitter );

CSoundAmbientEmitter::CSoundAmbientEmitter()
	: m_maxDistance( 30.0f ), m_occlusionEnabled( true )
{

}
	
void CSoundAmbientEmitter::OnAttached( CWorld* world )
{
	CComponent* component = FindComponent<CSpriteComponent>();
	if( component )
	{
		component->Destroy();
	}
	TBaseClass::OnAttached( world );

	if( m_soundEvents.Empty() ) return;
	
	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent();
	soundEmitterComponent->SetSoundLoop( m_soundEvents );
	soundEmitterComponent->SetMaxDistance( m_maxDistance );
	soundEmitterComponent->SetOcclusionEnable( m_occlusionEnabled );

	m_soundEvents.Clear();
}