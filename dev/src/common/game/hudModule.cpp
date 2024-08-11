/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashMovie.h"
#include "../../common/engine/flashFunction.h"
#include "../../common/engine/flashValueObject.h"

#include "flashSpriteAdapter.h"
#include "flashScriptSupport.h"
#include "hud.h"
#include "hudModule.h"
#include "hudResource.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CHudModule );

//////////////////////////////////////////////////////////////////////////
// CHudModule
//////////////////////////////////////////////////////////////////////////
CHudModule::CHudModule()
	: m_flashHudModuleAdapter( nullptr )
	, m_scriptedFlashObjectPool( nullptr )
	, m_scriptedFlashSprite( nullptr )
	, m_scriptedFlashValueStorage( nullptr )
{
}

Bool CHudModule::Init( CHud* hud, const SHudModuleInitParams& initParams )
{
	ASSERT( hud );
	if ( ! hud )
	{
		return false;
	}
	ASSERT( ! m_hudHandle.Get(), TXT("Already initialize with HUD handle!") );
	if ( m_hudHandle.Get() )
	{
		return false;
	}

	m_hudHandle = hud;
	m_moduleName = initParams.m_moduleName;

	m_scriptedFlashObjectPool = new CScriptedFlashObjectPool;

	return true;
}

Bool CHudModule::InitWithFlashSprite( const CFlashValue& flashSprite )
{
	ASSERT( ! m_flashHudModuleAdapter );
	if ( m_flashHudModuleAdapter )
	{
		GUI_ERROR( TXT("HUD module Flash already registered!") );
		return false;
	}

	CHud* hud = m_hudHandle.Get();
	ASSERT( hud );
	if ( ! hud )
	{
		GUI_ERROR( TXT("HUD module doesn't have a HUD parent!") );
		return false;
	}

	CFlashMovie* flashMovie = hud->GetFlashMovie();
	ASSERT( flashMovie );
	if ( ! flashMovie )
	{
		GUI_ERROR( TXT("HUD movie is NULL!") );
		return false;
	}

	m_flashHudModuleAdapter = new CFlashSpriteAdapter( flashMovie, flashSprite, this );
	ASSERT( m_flashHudModuleAdapter );
	if ( ! m_flashHudModuleAdapter->Init() )
	{
		GUI_ERROR( TXT("Failed to initialize Flash HUD module adapter") );
		return false;
	}

	m_scriptedFlashSprite = new CScriptedFlashSprite( *m_scriptedFlashObjectPool, flashSprite, String::Printf(TXT("[Hud module %ls]"), m_moduleName.AsChar() ) );

	CFlashValueStorage* flashValueStorage = m_flashHudModuleAdapter->GetFlashValueStorage();
	ASSERT( flashValueStorage );
	m_scriptedFlashValueStorage = new CScriptedFlashValueStorage( *m_scriptedFlashObjectPool, flashValueStorage, flashMovie );

	VERIFY( hud->CollectForTick( this ) );

	return true;
}

Bool CHudModule::IsInitWithFlashSprite() const
{
	return m_flashHudModuleAdapter != nullptr;
}

Bool CHudModule::RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
// 	ASSERT( ! m_isInTick );
// 	if ( m_isInTick )
// 	{
// 		return false;
// 	}

	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	CHud* hud = m_hudHandle.Get();
	ASSERT( hud );
	if ( hud )
	{
		return hud->RegisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

Bool CHudModule::UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
// 	ASSERT( ! m_isInTick );
// 	if ( m_isInTick )
// 	{
// 		return false;
// 	}

	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	CHud* hud = m_hudHandle.Get();
	ASSERT( hud );
	if ( hud )
	{
		return hud->UnregisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

void CHudModule::Tick( Float timeDelta )
{
	CallEvent( CNAME(OnTick), timeDelta );
}

void CHudModule::OnFinalize()
{
	Cleanup();

	TBaseClass::OnFinalize();
}

void CHudModule::Cleanup()
{
	if ( m_scriptedFlashObjectPool )
	{
		delete m_scriptedFlashObjectPool;
		m_scriptedFlashObjectPool = nullptr;
	}

	delete m_scriptedFlashObjectPool;
	m_scriptedFlashObjectPool = nullptr;	
	RED_FATAL_ASSERT( ! m_scriptedFlashValueStorage.Get(), "Handle should be null" );
	RED_FATAL_ASSERT( ! m_scriptedFlashSprite.Get(), "Handle should be null" );

	ASSERT( m_flashHudModuleAdapter );
	if ( m_flashHudModuleAdapter )
	{
		VERIFY( m_flashHudModuleAdapter->Release() == 0 );
		m_flashHudModuleAdapter = nullptr;
	}
}

void CHudModule::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
}

void CHudModule::funcGetModuleFlash( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashSprite );
	if ( ! m_scriptedFlashSprite )
	{
		GUI_ERROR( TXT("HUD flash not available") );
	}

	RETURN_HANDLE( CScriptedFlashSprite, m_scriptedFlashSprite );
}

void CHudModule::funcGetModuleFlashValueStorage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashValueStorage );
	if ( ! m_scriptedFlashValueStorage )
	{
		GUI_ERROR( TXT("HUD Flash value storage not available") );
	}

	RETURN_HANDLE( CScriptedFlashValueStorage, m_scriptedFlashValueStorage );
}
