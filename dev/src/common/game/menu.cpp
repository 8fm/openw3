/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/core/numericLimits.h"
#include "../../common/engine/flashMovie.h"
#include "../../common/engine/flashFunction.h"
#include "../../common/engine/flashValueObject.h"
#include "../../common/engine/flashRenderTarget.h"

#include "../../common/game/flashScriptSupport.h"
#include "../../common/game/flashScriptFunctionCalling.h"

#include "../../common/core/fileSystemProfiler.h"

#include "flashBindingAdapter.h"
#include "flashMovieAdapter.h"
#include "flashSpriteAdapter.h"
#include "guiManager.h"
#include "menuResource.h"
#include "menu.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CMenu );

//////////////////////////////////////////////////////////////////////////
// CMenu
//////////////////////////////////////////////////////////////////////////
CMenu::CMenu()
	: m_menuResource( nullptr )
	, m_flashMovie( nullptr )
	, m_flashMovieAdapter( nullptr )
	, m_flashMenuAdapter( nullptr )
	, m_scriptedFlashObjectPool( nullptr )
	, m_scriptedFlashSprite( nullptr )
	, m_scriptedFlashValueStorage( nullptr )
{
}

void CMenu::OnFinalize()
{
	Cleanup();

	TBaseClass::OnFinalize();
}

void CMenu::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_menuResource )
		{
			file << m_menuResource;
		}
		if ( m_menuScriptData )
		{
			file << m_menuScriptData;
		}
	}

	TBaseClass::OnSerialize( file );
}

Bool CMenu::Init( const CName& menuName, CGuiManager* guiManager )
{
#if 0
	// A null parent is allowed, but then should load a SWF file
	if ( parentGuiObject )
	{
		// Use the parent's flash movie
	}
#endif

	GFileManagerProfiler.Flush();

	ASSERT( ! m_menuName );
	if ( ! m_menuName )
	{
		m_menuName = menuName;
	}

	ASSERT( guiManager );
	if ( ! guiManager )
	{
		return false;
	}

	ASSERT( GGame && GGame->GetFlashPlayer() );
	if ( ! GGame || ! GGame->GetFlashPlayer() )
	{
		return false;
	}

	ASSERT( ! m_guiManagerHandle.Get() );
	if ( m_guiManagerHandle.Get() )
	{
		GUI_ERROR( TXT("Already initialized with a guiManager!") );
		return false;
	}

	ASSERT( ! m_flashMovie );
	if ( m_flashMovie )
	{
		GUI_ERROR( TXT("Menu Flash movie already loaded!") );
		return false;
	}

	ASSERT( m_menuResource );
	if ( ! m_menuResource )
	{
		GUI_ERROR( TXT("No menu resource!") );
		return false;
	}

	CMenuDef* menuDef = m_menuResource->GetMenuDef();
	if ( ! menuDef )
	{
		GUI_ERROR( TXT("No menu def!") );
		return false;
	}

	const TSoftHandle< CSwfResource >& menuFlashSwf = m_menuResource->GetMenuFlashSwf();

	Uint32 menuLayer = 0;
	if ( m_menuResource->GetLayer() > (Uint32)(NumericLimits< Int32 >::Max() - DEFAULT_MENU_LAYER_START) )
	{
		GUI_WARN(TXT("Menu '%ls' with layer %u exceeds maximum"), menuName.AsString().AsChar(), menuLayer );
		menuLayer = NumericLimits< Int32 >::Max();
	}
	else
	{
		menuLayer = DEFAULT_MENU_LAYER_START + m_menuResource->GetLayer();
	}

	SFlashMovieInitParams context;
	context.m_layer = menuLayer;
	// Flash movie starts with refcount 1.
	m_flashMovie = GGame->GetFlashPlayer()->CreateMovie( menuFlashSwf, context );
	ASSERT( m_flashMovie );
	if ( ! m_flashMovie )
	{
		GUI_ERROR( TXT("Could not create menu flash movie '%ls'"), menuFlashSwf.GetPath().AsChar() );
		return false;
	}

	m_flashMovieAdapter = new CFlashMovieAdapter( m_flashMovie );
	VERIFY( m_flashMovieAdapter->Init() );
	
	VERIFY( guiManager->RegisterForTick( this ) );

	m_guiManagerHandle = guiManager;

	m_scriptedFlashObjectPool = new CScriptedFlashObjectPool;

	return true;
}

void CMenu::Tick( Float timeDelta )
{
	RED_UNUSED( timeDelta );
}

Bool CMenu::InitWithFlashSprite( const CFlashValue& flashSprite )
{
	ASSERT( ! m_flashMenuAdapter );
	if ( m_flashMenuAdapter )
	{
		GUI_ERROR( TXT("Menu Flash already registered!") );
		return false;
	}

	m_flashMenuAdapter = new CFlashSpriteAdapter( m_flashMovie, flashSprite, this );
	ASSERT( m_flashMenuAdapter );
	if ( ! m_flashMenuAdapter->Init() )
	{
		GUI_ERROR( TXT("Failed to initialize Flash menu adapter") );
		return false;
	}

	m_scriptedFlashSprite = new CScriptedFlashSprite( *m_scriptedFlashObjectPool, flashSprite, TXT("[Menu]") );

	CFlashValueStorage* flashValueStorage = m_flashMenuAdapter->GetFlashValueStorage();
	ASSERT( flashValueStorage );
	m_scriptedFlashValueStorage = new CScriptedFlashValueStorage( *m_scriptedFlashObjectPool, flashValueStorage, m_flashMovie );

	return true;
}

Bool CMenu::RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
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

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		return guiManager->RegisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

Bool CMenu::UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
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

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		return guiManager->UnregisterFlashValueStorage( flashValueStorage );
	}

	return false;
}

Bool CMenu::RegisterRenderTarget( const String& targetName, Uint32 width, Uint32 height )
{
	Bool result = false;

	for ( CFlashRenderTarget* flashRenderTarget : m_flashRenderTargets )
	{
		if ( flashRenderTarget->GetTargetName() == targetName )
		{
			GUI_ERROR(TXT("Render target '%ls' already registered in menu '%ls'"), targetName.AsChar(), m_menuName.AsString().AsChar() );
			return false;
		}
	}

	CFlashRenderTarget* flashRenderTarget = m_flashMovie->CreateRenderTarget( targetName, width, height );
	CGuiManager* guiManager = m_guiManagerHandle.Get();
	if ( flashRenderTarget && guiManager )
	{
		guiManager->RegisterFlashRenderTarget( flashRenderTarget );
		m_flashRenderTargets.PushBack( flashRenderTarget );
		result = true;
	}
	else
	{
		GUI_ERROR( TXT("Failed to register render scene '%ls' in menu '%ls'"), targetName.AsChar(), m_menuName.AsString().AsChar() );
	}

	if ( flashRenderTarget )
	{
		flashRenderTarget->Release();
		flashRenderTarget = nullptr;
	}

	return result;
}

Bool CMenu::UnregisterRenderTarget( const String& targetName )
{
	CFlashRenderTarget* toUnregister = nullptr;
	for ( Int32 j = m_flashRenderTargets.SizeInt() - 1; j >= 0; --j )
	{
		CFlashRenderTarget* flashRenderTarget = m_flashRenderTargets[ j ];
		if ( flashRenderTarget->GetTargetName() == targetName )
		{
			toUnregister = flashRenderTarget;
			m_flashRenderTargets.RemoveAt( j );
			break;
		}
	}

	if ( ! toUnregister )
	{
		GUI_ERROR( TXT("Cannot find render target '%ls' to unregister in menu '%ls'"), targetName.AsChar(), m_menuName.AsString().AsChar() );
		return false;
	}

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	if ( ! guiManager || ! guiManager->UnregisterFlashRenderTarget( toUnregister ) )
	{
		toUnregister->Release();
		GUI_ERROR( TXT("Failed to unregister render target '%ls' to unregister in menu '%ls'"), targetName.AsChar(), m_menuName.AsString().AsChar() );

		return false;
	}

	toUnregister->Release();
	return true;
}

void CMenu::SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKeys( keysToSet );
	}
}

void CMenu::ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKeys( keysToClear );
	}
}

void CMenu::SetIgnoreKey( EInputKey keyToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKey( keyToSet );
	}
}

void CMenu::ClearIgnoreKey( EInputKey keyToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKey( keyToClear );
	}
}

void CMenu::ClearAllIgnoreKeys()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearAllIgnoreKeys();
	}
}

void CMenu::Cleanup()
{
	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );

	for ( CFlashRenderTarget* flashRenderTarget : m_flashRenderTargets )
	{
		VERIFY( guiManager && guiManager->UnregisterFlashRenderTarget( flashRenderTarget ) );
	}
	m_flashRenderTargets.ClearFast();
		
	delete m_scriptedFlashObjectPool;
	m_scriptedFlashObjectPool = nullptr;	
	RED_FATAL_ASSERT( ! m_scriptedFlashValueStorage.Get(), "Handle should be null" );
	RED_FATAL_ASSERT( ! m_scriptedFlashSprite.Get(), "Handle should be null" );

	if ( m_flashMenuAdapter )
	{
		m_flashMenuAdapter->Release();
		m_flashMenuAdapter = nullptr;
	}

	if ( m_flashMovieAdapter )
	{
		VERIFY( m_flashMovieAdapter->Release() == 0 );
		m_flashMovieAdapter = nullptr;
	}

	if ( m_flashMovie )
	{
		VERIFY( m_flashMovie->Release() == 0 );
		m_flashMovie = nullptr;
	}

	VERIFY( guiManager && guiManager->UnregisterForTick( this ) );

	if ( m_menuResource )
	{
		// Don't call discard. Could be in the editor window.
		m_menuResource = nullptr;
	}
}

void CMenu::funcGetMenuFlash( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashSprite );
	if ( ! m_scriptedFlashSprite )
	{
		GUI_ERROR( TXT("Menu flash not available") );
	}

	RETURN_HANDLE( CScriptedFlashSprite, m_scriptedFlashSprite );
}

void CMenu::funcGetMenuFlashValueStorage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashValueStorage );
	if ( ! m_scriptedFlashValueStorage )
	{
		GUI_ERROR( TXT("Menu Flash value storage not available") );
	}

	RETURN_HANDLE( CScriptedFlashValueStorage, m_scriptedFlashValueStorage );
}

void CMenu::funcGetMenuInitData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( IScriptable, m_menuScriptData.Get() );
}

void CMenu::funcGetMenuName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_menuName );
}

void CMenu::funcRequestSubMenu( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, menuName, CName::NONE );
	GET_PARAMETER_OPT( THandle< IScriptable >, initData, nullptr );
	FINISH_PARAMETERS;

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	if ( ! guiManager )
	{
		GUI_ERROR( TXT("CMenu::funcRequestSubMenu - no CGuiManager") );
		return;
	}
	CGuiManager::SGuiEvent guiEvent;
	guiEvent.m_eventName = CNAME( OnRequestMenu );
	guiEvent.m_eventEx = menuName;
	guiEvent.m_parent = this;
	if ( initData.Get() )
	{
		guiEvent.m_args.PushBack( SGuiEventArg( initData ) );
	}

	guiManager->CallGuiEvent( guiEvent );
}

void CMenu::funcCloseMenu( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnCloseMenu );
		guiEvent.m_eventEx = m_menuName;
		guiManager->CallGuiEvent( guiEvent );
	}
}

CMenu* CMenu::CreateObjectFromResource( CMenuResource* menuResource, CObject* parent )
{
	PC_SCOPE( CreateObjectFromResource );

	ASSERT( menuResource );
	if ( ! menuResource )
	{
		GUI_ERROR( TXT("Can't create simple menu from NULL GUI resource!") );
		return nullptr;
	}

	ASSERT( parent );
	if ( ! parent )
	{
		GUI_ERROR( TXT("No CObject parent passed to menu!") );
		return nullptr;
	}

	//FIXME!
	CClass* menuClass = SRTTI::GetInstance().FindClass( menuResource->GetMenuClass() );
	ASSERT( menuClass && menuClass->IsA< CMenu >() );
	if ( ! menuClass || ! menuClass->IsA< CMenu >() )
	{
		GUI_ERROR( TXT("GUI resource does not contain a menu template!" ) );
		return nullptr;
	}

	ASSERT( menuClass && ! menuClass->IsAbstract() &&
		! menuClass->IsAlwaysTransient() &&
		! menuClass->IsEditorOnly() );
	if ( ! menuClass )
	{
		GUI_ERROR( TXT("Menu base template class is NULL!") );
		return nullptr;
	}
	if ( menuClass->IsAbstract() )
	{
		GUI_ERROR( TXT("Menu menu base template class is abstract!") );
		return nullptr;
	}
	if ( menuClass->IsAlwaysTransient() )
	{
		GUI_ERROR( TXT("Menu menu base template class is always transient") );
		return nullptr;
	}
	if ( menuClass->IsEditorOnly() )
	{
		GUI_ERROR( TXT("Menu base template class is editor only") );
		return nullptr;
	}

	CMenu* newMenuObj = ::CreateObject< CMenu >( menuClass, parent, OF_Transient );
	ASSERT( newMenuObj );
	if ( ! newMenuObj )
	{
		GUI_ERROR( TXT("Failed to create menu object!") );
		return nullptr;
	}

	newMenuObj->m_menuResource = menuResource;

	return newMenuObj;
}
