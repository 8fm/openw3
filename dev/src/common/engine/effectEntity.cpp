/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "effectEntity.h"
#include "fxState.h"
#include "../core/gameSave.h"
#include "fxDefinition.h"

IMPLEMENT_ENGINE_CLASS( CEffectEntity );

RED_DEFINE_STATIC_NAME(effects);

void CEffectEntity::OnAttachFinished( CWorld* world )
{
	for ( Uint32 i=0; i<m_effects.Size(); ++i )
	{
		PlayEffect( m_effects[i] );
	}
	m_effects.Clear();
}

void CEffectEntity::OnSaveGameplayState( IGameSaver* saver )
{
	// Pass to base class
	TBaseClass::OnSaveGameplayState( saver );

	// Save effects
	{
		CGameSaverBlock block( saver, CNAME( effects ) );

		// Collect names of the effects that should be saved
		TDynArray< CName > effectsToSave;
		for ( Uint32 i=0; i<m_activeEffects.Size(); ++i )
		{
			if ( m_activeEffects[i]->IsAlive() && !m_activeEffects[i]->IsStopping() )
			{
				ASSERT( m_activeEffects[i]->GetDefinition() );
				effectsToSave.PushBackUnique( m_activeEffects[i]->GetDefinition()->GetName() );
			}
		}

		// Save those names
		saver->WriteValue( CNAME(numEffects), effectsToSave.Size() );
		for ( Uint32 i=0; i<effectsToSave.Size(); ++i )		
		{
			saver->WriteValue( CNAME(effectName), effectsToSave[i] );
		}
	}
}

void CEffectEntity::OnLoadGameplayState( IGameLoader* loader )
{
	// Pass to base class
	TBaseClass::OnLoadGameplayState( loader );

	// Restore effects
	{
		CGameSaverBlock block( loader, CNAME( effects ) );

		const Uint32 numEffects = loader->ReadValue< Uint32 >( CNAME(numEffects) ); 
		m_effects.Resize( numEffects );
		for ( Uint32 i = 0; i<numEffects; ++i )
		{
			m_effects[i] = loader->ReadValue< CName >( CNAME(effectName), CName::NONE );
		}
	}
}
