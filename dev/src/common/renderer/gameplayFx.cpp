/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameplayFx.h"

#include "gameplayFXCatView.h"
#include "gameplayFxDrunk.h"
#include "gameplayFxFocusMode.h"
#include "gameplayFxOutline.h"
#include "gameplayFxSepia.h"
#include "gameplayFxSurface.h"

//////////////////////////////////////////////////////////////////////////
///
/// Main GameplayEffects Manager


CGameplayEffects::CGameplayEffects()
	: m_lastGameTime( 0.0f )
	, m_isInitialized( false )
{
	for( Uint32 i = 0; i < EPE_MAX; ++i )
	{
		m_effects[i] = nullptr;
	}
}

CGameplayEffects::~CGameplayEffects()
{
}

void CGameplayEffects::AddEffect( IGameplayEffect* effect )
{
	RED_ASSERT( effect, TXT("Trying to add NULL effect. Sad panda :'(") );
	RED_ASSERT( (Uint32)effect->GetType() < EPE_MAX , TXT("Unknown type of effect") );
	
	const Uint32 effectID = (Uint32)effect->GetType();

	RED_ASSERT( m_effects[ effectID ] == nullptr , TXT("Effect arleady exists") );

	effect->Init();
	m_effects[ effectID ] = effect;
}

void CGameplayEffects::Init()
{
	RED_ASSERT( ::SIsMainThread(), TXT("CGameplayEffects::Init() must be called from the main thread, so it can load resource files.") );

	// Focus effect
	AddEffect( new CSurfacePostFX( this ) );	

	// Focus effect
	AddEffect( new CFocusModeEffectPostFX( this ) );
	
	// Sepia effect
	AddEffect( new CSepiaEffectPostFX( this ) );

	//CatView effect
	AddEffect( new CCatViewEffectPostFX( this ) );

	//Drunk effect
	AddEffect( new CDrunkEffectPostFX( this ) );

	m_isInitialized = true;
}

void CGameplayEffects::Shutdown()
{
	RED_ASSERT( ::SIsMainThread(), TXT("CGameplayEffects::Shutdown() must be called from the main thread") );

	m_isInitialized = false;

	for ( Uint32 i = 0; i < EPE_MAX; ++i )
	{
		if( !m_effects[i] )
		{
			continue;
		}
		m_effects[i]->Shutdown();
		delete m_effects[i];
		m_effects[i] = nullptr;
	}

}

void CGameplayEffects::SetGameTime( Float time )
{
	if ( m_lastGameTime == 0.0f )
	{
		m_lastGameTime = time;
		return;
	}
	Float diff = Clamp< Float >( time - m_lastGameTime, 0.0f, 0.2f );

	m_lastGameTime = time;

	for( Uint32 i = 0; i < EPE_MAX; ++i )
	{
		if( m_effects[i] && m_effects[i]->IsEnabled() )
		{
			m_effects[i]->Tick( diff );
		}
	}

}

Vector	CGameplayEffects::GetParametersVector() const
{ 
	if( m_isInitialized )
	{
		return Vector( 
			1.0f - m_effects[ EPE_FOCUS_MODE ]->GetStrength() ,
			1.0f - m_effects[ EPE_SEPIA ]->GetStrength() ,
			1.0f - m_effects[ EPE_DRUNK ]->GetStrength() ,
			1.0f - m_effects[ EPE_CAT_VIEW ]->GetStrength()
			);
	}
	return Vector( 1.0f , 1.0f, 1.0f, 1.0f );
}


void CGameplayEffects::Apply( EPostFXOrder order, CRenderCollector &collector, const CRenderFrameInfo& frameInfo, ERenderTargetName& rtnColorSource , ERenderTargetName& rtnColorTarget )
{
	if( !frameInfo.IsShowFlagOn( SHOW_GameplayPostFx ) ) 
	{
		return;
	}

	for( Uint32 i = 0; i < EPE_MAX; ++i )
	{
		IGameplayEffect* effect = m_effects[i];

		if( effect && effect->IsEnabled() && effect->GetOrder() == order )
		{
			// If effect was applied correctly and it need swap buffers
			if( effect->Apply( collector , frameInfo , rtnColorSource , rtnColorTarget ) )
			{
				Swap( rtnColorSource, rtnColorTarget );
			}
		}
	}

}


void CGameplayEffects::DisableAll()
{
	for( Uint32 i = 0; i < EPE_MAX; ++i )
	{
		if( m_effects[i] )
		{
			m_effects[i]->SetEnabled( false );
			m_effects[i]->SetStrength( 0.0f );
			m_effects[i]->Reset();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// SCRIPTS

void ExportGameplayFxFunctions()
{
	extern void ExportCatViewFxNatives();
	ExportCatViewFxNatives();

	extern void ExportDrunkFxNatives();
	ExportDrunkFxNatives();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
