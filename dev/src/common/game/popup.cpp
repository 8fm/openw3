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

#include "flashBindingAdapter.h"
#include "flashMovieAdapter.h"
#include "flashSpriteAdapter.h"
#include "guiManager.h"
#include "popupResource.h"
#include "popup.h"
#include "../engine/guiGlobals.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CPopup );

//////////////////////////////////////////////////////////////////////////
// CPopup
//////////////////////////////////////////////////////////////////////////
CPopup::CPopup()
	: m_popupResource( nullptr )
	, m_flashMovie( nullptr )
	, m_flashMovieAdapter( nullptr )
	, m_flashPopupAdapter( nullptr )
	, m_scriptedFlashObjectPool( nullptr )
	, m_scriptedFlashSprite( nullptr )
	, m_scriptedFlashValueStorage( nullptr )
{
}

void CPopup::OnFinalize()
{
	Cleanup();

	TBaseClass::OnFinalize();
}

void CPopup::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_popupResource )
		{
			file << m_popupResource;
		}
		if ( m_popupScriptData )
		{
			file << m_popupScriptData;
		}
	}

	TBaseClass::OnSerialize( file );
}

RED_DEFINE_STATIC_NAME( MessagePopup );

Bool CPopup::Init( const CName& popupName, CGuiManager* guiManager )
{
#if 0
	// A null parent is allowed, but then should load a SWF file
	if ( parentGuiObject )
	{
		// Use the parent's flash movie
	}
#endif

	ASSERT( ! m_popupName );
	if ( ! m_popupName )
	{
		m_popupName = popupName;
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
		GUI_ERROR( TXT("Popup Flash movie already loaded!") );
		return false;
	}

	ASSERT( m_popupResource );
	if ( ! m_popupResource )
	{
		GUI_ERROR( TXT("No popup resource!") );
		return false;
	}

	CPopupDef* popupDef = m_popupResource->GetPopupDef();
	if ( ! popupDef )
	{
		GUI_ERROR( TXT("No popup def!") );
		return false;
	}

	const TSoftHandle< CSwfResource >& popupFlashSwf = m_popupResource->GetPopupFlashSwf();

	Uint32 popupLayer = 0;
	if ( m_popupResource->GetLayer() > (Uint32)(NumericLimits< Int32 >::Max() - DEFAULT_POPUP_LAYER_START) )
	{
		GUI_WARN(TXT("Popup '%ls' with layer %u exceeds maximum"), popupName.AsString().AsChar(), popupLayer );
		popupLayer = NumericLimits< Int32 >::Max();
	}
	else
	{
		popupLayer = DEFAULT_POPUP_LAYER_START + m_popupResource->GetLayer();
	}

	SFlashMovieInitParams context;
	context.m_layer = popupLayer;

	//! If you need another popup to behave like this, please do a generic property in popupResource instead of hardcoding it.
	if (popupName == CNAME( MessagePopup ) )
	{
		context.m_renderGroup = eFlashMovieRenderGroup_Overlay;
	}

	// Flash movie starts with refcount 1.
	m_flashMovie = GGame->GetFlashPlayer()->CreateMovie( popupFlashSwf, context );
	ASSERT( m_flashMovie );
	if ( ! m_flashMovie )
	{
		GUI_ERROR( TXT("Could not create popup flash movie '%ls'"), popupFlashSwf.GetPath().AsChar() );
		return false;
	}

	m_flashMovieAdapter = new CFlashMovieAdapter( m_flashMovie );
	VERIFY( m_flashMovieAdapter->Init() );
	
	VERIFY( guiManager->RegisterForTick( this ) );

	m_guiManagerHandle = guiManager;

	m_scriptedFlashObjectPool = new CScriptedFlashObjectPool;

	return true;
}

void CPopup::Tick( Float timeDelta )
{
	RED_UNUSED( timeDelta );
}

Bool CPopup::InitWithFlashSprite( const CFlashValue& flashSprite )
{
	ASSERT( ! m_flashPopupAdapter );
	if ( m_flashPopupAdapter )
	{
		GUI_ERROR( TXT("Popup Flash already registered!") );
		return false;
	}

	m_flashPopupAdapter = new CFlashSpriteAdapter( m_flashMovie, flashSprite, this );
	ASSERT( m_flashPopupAdapter );
	if ( ! m_flashPopupAdapter->Init() )
	{
		GUI_ERROR( TXT("Failed to initialize Flash popup adapter") );
		return false;
	}

	m_scriptedFlashSprite = new CScriptedFlashSprite( *m_scriptedFlashObjectPool, flashSprite, TXT("[Popup]") );

	CFlashValueStorage* flashValueStorage = m_flashPopupAdapter->GetFlashValueStorage();
	ASSERT( flashValueStorage );
	m_scriptedFlashValueStorage = new CScriptedFlashValueStorage( *m_scriptedFlashObjectPool, flashValueStorage, m_flashMovie );

	return true;
}

Bool CPopup::RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
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

Bool CPopup::UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
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

Bool CPopup::RegisterRenderTarget( const String& targetName, Uint32 width, Uint32 height )
{
	Bool result = false;

	for ( CFlashRenderTarget* flashRenderTarget : m_flashRenderTargets )
	{
		if ( flashRenderTarget->GetTargetName() == targetName )
		{
			GUI_ERROR(TXT("Render target '%ls' already registered in popup '%ls'"), targetName.AsChar(), m_popupName.AsString().AsChar() );
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
		GUI_ERROR( TXT("Failed to register render scene '%ls' in popup '%ls'"), targetName.AsChar(), m_popupName.AsString().AsChar() );
	}

	if ( flashRenderTarget )
	{
		flashRenderTarget->Release();
		flashRenderTarget = nullptr;
	}

	return result;
}

Bool CPopup::UnregisterRenderTarget( const String& targetName )
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
		GUI_ERROR( TXT("Cannot find render target '%ls' to unregister in popup '%ls'"), targetName.AsChar(), m_popupName.AsString().AsChar() );
		return false;
	}

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	if ( ! guiManager || ! guiManager->UnregisterFlashRenderTarget( toUnregister ) )
	{
		toUnregister->Release();
		GUI_ERROR( TXT("Failed to unregister render target '%ls' to unregister in popup '%ls'"), targetName.AsChar(), m_popupName.AsString().AsChar() );

		return false;
	}

	toUnregister->Release();
	return true;
}

void CPopup::SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKeys( keysToSet );
	}
}

void CPopup::ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKeys( keysToClear );
	}
}

void CPopup::SetIgnoreKey( EInputKey keyToSet )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->SetIgnoreKey( keyToSet );
	}
}

void CPopup::ClearIgnoreKey( EInputKey keyToClear )
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearIgnoreKey( keyToClear );
	}
}

void CPopup::ClearAllIgnoreKeys()
{
	ASSERT( m_flashMovie );
	if ( m_flashMovie )
	{
		m_flashMovie->ClearAllIgnoreKeys();
	}
}

void CPopup::Cleanup()
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

	if ( m_flashPopupAdapter )
	{
		m_flashPopupAdapter->Release();
		m_flashPopupAdapter = nullptr;
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

	if ( m_popupResource )
	{
		// Don't call discard. Could be in the editor window.
		m_popupResource = nullptr;
	}
}

void CPopup::funcGetPopupFlash( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashSprite );
	if ( ! m_scriptedFlashSprite )
	{
		GUI_ERROR( TXT("Popup flash not available") );
	}

	RETURN_HANDLE( CScriptedFlashSprite, m_scriptedFlashSprite );
}

void CPopup::funcGetPopupFlashValueStorage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( m_scriptedFlashValueStorage );
	if ( ! m_scriptedFlashValueStorage )
	{
		GUI_ERROR( TXT("Popup Flash value storage not available") );
	}

	RETURN_HANDLE( CScriptedFlashValueStorage, m_scriptedFlashValueStorage );
}

void CPopup::funcGetPopupInitData( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( IScriptable, m_popupScriptData.Get() );
}

void CPopup::funcGetPopupName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_popupName );
}

void CPopup::funcClosePopup( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CGuiManager* guiManager = m_guiManagerHandle.Get();
	ASSERT( guiManager );
	if ( guiManager )
	{
		CGuiManager::SGuiEvent guiEvent;
		guiEvent.m_eventName = CNAME( OnClosePopup );
		guiEvent.m_eventEx = m_popupName;
		guiManager->CallGuiEvent( guiEvent );
	}
}

CPopup* CPopup::CreateObjectFromResource( CPopupResource* popupResource, CObject* parent )
{
	PC_SCOPE( CreateObjectFromResource );

	ASSERT( popupResource );
	if ( ! popupResource )
	{
		GUI_ERROR( TXT("Can't create simple popup from NULL GUI resource!") );
		return nullptr;
	}

	ASSERT( parent );
	if ( ! parent )
	{
		GUI_ERROR( TXT("No CObject parent passed to popup!") );
		return nullptr;
	}

	//FIXME!
	CClass* popupClass = SRTTI::GetInstance().FindClass( popupResource->GetPopupClass() );
	ASSERT( popupClass && popupClass->IsA< CPopup >() );
	if ( ! popupClass || ! popupClass->IsA< CPopup >() )
	{
		GUI_ERROR( TXT("GUI resource does not contain a popup template!" ) );
		return nullptr;
	}

	ASSERT( popupClass && ! popupClass->IsAbstract() &&
		! popupClass->IsAlwaysTransient() &&
		! popupClass->IsEditorOnly() );
	if ( ! popupClass )
	{
		GUI_ERROR( TXT("Popup base template class is NULL!") );
		return nullptr;
	}
	if ( popupClass->IsAbstract() )
	{
		GUI_ERROR( TXT("Popup base template class is abstract!") );
		return nullptr;
	}
	if ( popupClass->IsAlwaysTransient() )
	{
		GUI_ERROR( TXT("Popup base template class is always transient") );
		return nullptr;
	}
	if ( popupClass->IsEditorOnly() )
	{
		GUI_ERROR( TXT("Popup base template class is editor only") );
		return nullptr;
	}

	CPopup* newPopupObj = ::CreateObject< CPopup >( popupClass, parent, OF_Transient );
	ASSERT( newPopupObj );
	if ( ! newPopupObj )
	{
		GUI_ERROR( TXT("Failed to create popup object!") );
		return nullptr;
	}

	newPopupObj->m_popupResource = popupResource;

	return newPopupObj;
}
