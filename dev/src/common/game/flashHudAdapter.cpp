/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../engine/flashValueObject.h"

#include "flashHudAdapter.h"
#include "hud.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// CHudFlashAdapter
//////////////////////////////////////////////////////////////////////////
CFlashHudAdapter::SFlashFuncImportDesc CFlashHudAdapter::sm_flashFunctionImportTable[] = {
	{ TXT("_HOOK_loadModule"), &CFlashHudAdapter::m_loadHudModuleHook },
	{ TXT("_HOOK_unloadModule"), &CFlashHudAdapter::m_unloadHudModuleHook },
};

CFlashHudAdapter::CFlashHudAdapter( CFlashMovie* flashMovie, const CFlashValue& flashHud, CHud* hud )
	: CFlashSpriteAdapter( flashMovie, flashHud, hud )
{
	// 	ASSERT( ! m_isInTick );
	// 	if ( m_isInTick )
	// 	{
	// 		return false;
	// 	}
	// 

	// 
	// 	if ( ! CreateAlwaysLoadedHudModules() )
	// 	{
	// 		GUI_ERROR( TXT("Couldn't create always loaded HUD modules") );
	// 		return false;
	// 	}
}

Bool CFlashHudAdapter::Init()
{
	if ( ! CFlashSpriteAdapter::Init() )
	{
		return false;
	}

	CFlashValue& flashHud = GetFlashSprite();
	ASSERT( flashHud.IsFlashDisplayObject() );
	if ( ! HookFlashFunctions( flashHud ) )
	{
		UnhookFlashFunctions();
		return false;
	}

	return true;
}

CFlashHudAdapter::~CFlashHudAdapter()
{
}

void CFlashHudAdapter::OnDestroy()
{
	CFlashSpriteAdapter::OnDestroy();

	UnhookFlashFunctions();
}

Bool CFlashHudAdapter::LoadModuleAsync( const String& moduleName, Int32 userData /*=-1*/ )
{
	if ( ! m_loadHudModuleHook.IsFlashClosure() )
	{
		GUI_ERROR( TXT("Tried to load a module before HUD flash ready") );
		return false;
	}

	ASSERT( GetFlashMovie() );
	CFlashString* flashString = GetFlashMovie()->CreateString( moduleName );
	if ( ! m_loadHudModuleHook.InvokeSelf( flashString->AsFlashValue(), CFlashValue( userData ) ) )
	{
		GUI_ERROR( TXT("Failed to invoke loadModuleHook for module '%ls' (possibly an exception was thrown in Flash)."), moduleName.AsChar() );

		flashString->Release();
		return false;
	}

	flashString->Release();
	flashString = nullptr;

	return true;
}

Bool CFlashHudAdapter::UnloadModuleAsync( const String& moduleName, Int32 userData /*=-1*/ )
{
	if ( ! m_unloadHudModuleHook.IsFlashClosure() )
	{
		GUI_ERROR( TXT("Tried to unload a module before HUD flash ready") );
		return false;
	}

	CFlashString* flashString = GetFlashMovie()->CreateString( moduleName );
	if ( ! m_unloadHudModuleHook.InvokeSelf( flashString->AsFlashValue(), CFlashValue( userData ) ) )
	{
		flashString->Release();
		GUI_ERROR( TXT("Failed to invoke unloadModuleHook for module '%ls' (possibly an exception was thrown in Flash)."), moduleName.AsChar() );
		return false;
	}
	flashString->Release();
	flashString = nullptr;

	return true;
}

Bool CFlashHudAdapter::HookFlashFunctions( CFlashValue& flashObject )
{
	PC_SCOPE( HookFlashFunctions );

	size_t len = sizeof(sm_flashFunctionImportTable)/sizeof(sm_flashFunctionImportTable[0]);
	for ( size_t i = 0; i < len; ++i )
	{
		if ( ! flashObject.GetMember( sm_flashFunctionImportTable[ i ].m_memberName, this->*sm_flashFunctionImportTable[ i ].m_flashFuncImport ) )
		{
			GUI_ERROR( TXT("Failed to import Flash function '%ls'"), sm_flashFunctionImportTable[ i ].m_memberName );
			return false;
		}
	}
	return true;
}

void CFlashHudAdapter::UnhookFlashFunctions()
{
	PC_SCOPE( UnhookFlashFunctions );

	size_t len = sizeof(sm_flashFunctionImportTable)/sizeof(sm_flashFunctionImportTable[0]);
	for ( size_t i = 0; i < len; ++i )
	{
		CFlashValue& flashHook = this->*sm_flashFunctionImportTable[ i ].m_flashFuncImport;
		flashHook.Clear();
	}
}

