/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/flashRenderTarget.h"
#include "../../common/engine/flashRenderSceneProvider.h"
#include "../../common/engine/flashPlayer.h"
#include "../../common/game/commonGameResource.h"
#include "../../common/game/guiConfigResource.h"
#include "../../common/game/menu.h"
#include "../../common/game/popup.h"

#include "guiManager.h"
#include "../engine/guiGlobals.h"
#include "../engine/renderGameplayRenderTargetInterface.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CGuiManager );

RED_DEFINE_STATIC_NAME( OnFailedCreateMenu );
RED_DEFINE_NAME( DefaultHud );
RED_DEFINE_NAME( OnRequestMenu );
RED_DEFINE_NAME( OnCloseMenu );
RED_DEFINE_NAME( OnRequestPopup );
RED_DEFINE_NAME( OnClosePopup );

//////////////////////////////////////////////////////////////////////////
// SGuiEventArg
//////////////////////////////////////////////////////////////////////////
SGuiEventArg::SGuiEventArg()
	: m_argType( Type_Undefined )
{
}

SGuiEventArg::SGuiEventArg( const THandle< IScriptable >& value )
	: m_argType( Type_Object )
{
	m_objectValue = value;
}

SGuiEventArg::SGuiEventArg( const String& value )
	: m_argType( Type_String )
{
	m_stringValue = value;
}

SGuiEventArg::SGuiEventArg( const CName& value )
	: m_argType( Type_Name )
{
	u.m_uintValue = static_cast< Uint32 >( value.GetIndex() );
}

SGuiEventArg::SGuiEventArg( const SItemUniqueId& value )
	: m_argType( Type_Item )
{
	u.m_uintValue = static_cast< Uint32 >( value.GetValue() );
}

SGuiEventArg::SGuiEventArg( Bool value )
	: m_argType( Type_Bool )
{
	u.m_boolValue = value;
}

SGuiEventArg::SGuiEventArg( Int32 value )
	: m_argType( Type_Int32 )
{
	u.m_intValue = value;
}

SGuiEventArg::SGuiEventArg( Uint32 value )
	: m_argType( Type_Uint32 )
{
	u.m_uintValue = value;
}

SGuiEventArg::SGuiEventArg( Float value )
	: m_argType( Type_Float )
{
	u.m_floatValue = value;
}

//////////////////////////////////////////////////////////////////////////
// CGuiManager
//////////////////////////////////////////////////////////////////////////
CGuiManager::SExternalInterfaceFuncDesc CGuiManager::sm_externalInterfaceTable[] = {
	{ TXT("registerHud"), &CGuiManager::ExternalInterfaceRegisterHud },
	{ TXT("registerMenu"), &CGuiManager::ExternalInterfaceRegisterMenu },
	{ TXT("registerPopup"), &CGuiManager::ExternalInterfaceRegisterPopup },
	{ TXT("isUsingPad"), &CGuiManager::ExternalInterfaceIsUsingPad },
	{ TXT("isPadConnected"), &CGuiManager::ExternalInterfaceIsPadConnected },
};

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !! PLEASE THINK LONG AND HARD BEFORE JUST ADDING EVENTS INTO THIS TABLE. THERE'S A REASON
// !! YOU CAN'T ADD THEM IN SCRIPTS. UNLESS YOU HATE PERFORMANCE, THAT IS.
// !! E.g., would it be better if a HUD module instead checked itself instead of spamming this
// !! up with events, which aren't free as the air. E.g., bad candidates are events
// !! that are firing frequently (e.g., updating some coordinates or whatever).
CGuiManager::SGuiEventDesc CGuiManager::sm_guiEventTable[] = {
	{ CNAME( OnRequestMenu ), &CGuiManager::GuiEventOnRequestMenu },
	{ CNAME( OnCloseMenu ), &CGuiManager::GuiEventOnCloseMenu },
	{ CNAME( OnRequestPopup ), &CGuiManager::GuiEventOnRequestPopup },
	{ CNAME( OnClosePopup ), &CGuiManager::GuiEventOnClosePopup },
};

CGuiManager::CGuiManager()
	: m_guiConfigResource( nullptr )
	, m_guiEventsProcessed( false )
{
}

Bool CGuiManager::Initialize()
{
	CFlashPlayer* flashPlayer = GGame ? GGame->GetFlashPlayer() : nullptr;
	ASSERT( flashPlayer );
	if ( ! flashPlayer )
	{
		return false;
	}

	VERIFY( flashPlayer->RegisterStatusListener( this ) );

	// External interface
	VERIFY( RegisterExternalInterfaces() );

	return true;
}

void CGuiManager::OnFinalize()
{
	CFlashPlayer* flashPlayer = GGame ? GGame->GetFlashPlayer() : nullptr;
	ASSERT( flashPlayer );
	if ( flashPlayer )
	{
		VERIFY( flashPlayer->UnregisterStatusListener( this ) );
	}

	// Don't discard. Could still be used in editor.
	m_guiConfigResource = nullptr;

	TBaseClass::OnFinalize();
}

void CGuiManager::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_guiConfigResource )
		{
			file << m_guiConfigResource;
		}

		for ( TGuiObjectHandleList::const_iterator it = m_guiObjectTickList.Begin(); it != m_guiObjectTickList.End(); ++it )
		{
			THandle< CGuiObject > guiObjectHandle = *it;
			CGuiObject* guiObject = guiObjectHandle.Get();
			ASSERT( guiObject );
			if ( guiObject )
			{
				file << guiObject;
			}
		}
	}

	TBaseClass::OnSerialize( file );
}

void CGuiManager::OnGameStart( const CGameInfo& gameInfo )
{
	CCommonGameResource* gameResource = Cast< CCommonGameResource >( GGame->GetGameResource() );

	//ASSERT( gameResource );
	if ( ! gameResource )
	{
		GUI_WARN( TXT("No game resource for GUI manager at game start.") );
		return;
	}

	const TSoftHandle< CGuiConfigResource >& guiConfigHandle = gameResource->GetGuiConfigOverride();
	CGuiConfigResource* guiConfigResource = guiConfigHandle.Get();
	if ( ! guiConfigResource )
	{
		//... TBD until part of gamedef
	}
}

void CGuiManager::OnGameEnd( const CGameInfo& gameInfo )
{
	m_guiEventQueue.ClearFast();
}

void CGuiManager::OnWorldStart( const CGameInfo& gameInfo )
{

}

void CGuiManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	m_guiEventQueue.Clear();
}

Bool CGuiManager::RegisterForTick( CGuiObject* guiObject )
{
	ASSERT( guiObject );
	if ( ! guiObject )
	{
		return false;
	}

	return m_guiObjectTickList.PushBackUnique( guiObject );
}

Bool CGuiManager::UnregisterForTick( CGuiObject* guiObject )
{
	ASSERT( guiObject );
	if ( ! guiObject )
	{
		return false;
	}

	return m_guiObjectTickList.Remove( guiObject );
}

Bool CGuiManager::RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	if ( m_flashValueStorageList.PushBackUnique( flashValueStorage ) )
	{
		flashValueStorage->AddRef();
		return true;
	}

	return false;
}

Bool CGuiManager::UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage )
{
	ASSERT( flashValueStorage );
	if ( ! flashValueStorage )
	{
		return false;
	}

	if ( m_flashValueStorageList.Remove( flashValueStorage ) )
	{
		flashValueStorage->Release();
		return true;
	}

	return false;
}

Bool CGuiManager::RegisterFlashRenderTarget( CFlashRenderTarget* flashRenderTarget )
{
	RED_FATAL_ASSERT( flashRenderTarget, "No target set");
	if ( ! flashRenderTarget )
	{
		return false;
	}

	if ( m_flashRenderTargetList.PushBackUnique( flashRenderTarget ) )
	{
		flashRenderTarget->AddRef();
		return true;
	}

	return false;
}

Bool CGuiManager::UnregisterFlashRenderTarget( CFlashRenderTarget* flashRenderTarget )
{
	RED_FATAL_ASSERT( flashRenderTarget, "No target set");
	if ( ! flashRenderTarget )
	{
		return false;
	}

	if ( m_flashRenderTargetList.Remove( flashRenderTarget ) )
	{
		flashRenderTarget->Release();
		return true;
	}

	return false;
}

void CGuiManager::SetRenderSceneProvider( IFlashRenderSceneProvider* renderSceneProvider )
{
	m_flashRenderSceneProvider = renderSceneProvider;
}

void CGuiManager::ClearRenderSceneProvider()
{
	m_flashRenderSceneProvider = nullptr;
}

void CGuiManager::Tick( Float timeDelta )
{
	{
	PC_SCOPE( ProcessGuiEventsQueue );
	ProcessGuiEventQueue();
	}

	if( IsAnyMenu() )
	{
		PC_SCOPE( UpdateScripts );
		CallFunction( this, CNAME( Update ), timeDelta );
	}

	{
	PC_SCOPE( GuiObjects_Tick );
	for ( TGuiObjectHandleList::const_iterator it = m_guiObjectTickList.Begin(); it != m_guiObjectTickList.End(); ++it )
	{
		const THandle< CGuiObject > guiObjectHandle = *it;
		CGuiObject* guiObject = guiObjectHandle.Get();;

		ASSERT( guiObject );
		if ( guiObject )
		{
			guiObject->Tick( timeDelta );
		}
	}
	}

	{
	PC_SCOPE( InvalidatedKeys );
	m_invalidatedKeysMap.ClearFast();
	for ( TFlashValueStorageList::const_iterator it = m_flashValueStorageList.Begin(); it != m_flashValueStorageList.End(); ++it )
	{
		CFlashValueStorage* flashValueStorage = *it;
		ASSERT( flashValueStorage );
		flashValueStorage->CollectInvalidatedKeys( m_invalidatedKeysMap );
	}
	for ( TFlashValueStorageList::const_iterator it = m_flashValueStorageList.Begin(); it != m_flashValueStorageList.End(); ++it )
	{
		CFlashValueStorage* flashValueStorage = *it;
		ASSERT( flashValueStorage );
		flashValueStorage->CollectHandlers( m_invalidatedKeysMap );
	}

	for ( TFlashValueStorageList::const_iterator it = m_flashValueStorageList.Begin(); it != m_flashValueStorageList.End(); ++it )
	{
		CFlashValueStorage* flashValueStorage = *it;
		ASSERT( flashValueStorage );
		flashValueStorage->Process( m_invalidatedKeysMap );
	}
	}

	{
	PC_SCOPE( FlashRenderScene_Tick_Render );
	if ( m_flashRenderSceneProvider && !m_flashRenderTargetList.Empty() )
	{
		m_flashRenderSceneProvider->Tick( timeDelta );

		ForEach( m_flashRenderTargetList, [&](CFlashRenderTarget* flashRenderTarget) {
			flashRenderTarget->RenderScene( m_flashRenderSceneProvider );
		} );
	}
	}
}

void CGuiManager::CallGuiEvent( const SGuiEvent& event  )
{
	m_guiEventQueue.PushBack( event );
}

void CGuiManager::ProcessGuiEventQueue()
{
	m_guiEventsProcessed = true;

	//TODO: Mark delayed events for "last one wins". E.g., if trying to open multiple menus.
	for ( Uint32 i = 0; i < m_guiEventQueue.Size(); ++i )
	{
		Bool handled = false;

		const SGuiEvent& event = m_guiEventQueue[ i ];
		size_t len = sizeof(sm_guiEventTable)/sizeof(sm_guiEventTable[0]);
		for ( size_t j = 0; j < len; ++j )
		{
			SGuiEventDesc& desc = sm_guiEventTable[j];
			if ( event.m_eventName == desc.m_eventName )
			{
				TGuiEventFunc func = desc.m_func;

				if ( ! (this->*func)( event ) )
				{
					GUI_ERROR( TXT("Call to gui event '%ls' ('%ls' extra) failed"), event.m_eventName.AsString().AsChar(), event.m_eventEx.AsString().AsChar() );
				}
				handled = true;
				break;
			}

		}
		if ( !handled )
		{
			GUI_WARN( TXT("Unhandled gui event '%ls'"), event.m_eventName.AsString().AsChar() );
		}
	}

	m_guiEventsProcessed = false;

	m_guiEventQueue.ClearFast();
}

Bool CGuiManager::RegisterExternalInterfaces()
{
	CFlashPlayer* flashPlayer = GGame ? GGame->GetFlashPlayer() : nullptr;
	ASSERT( flashPlayer );
	if ( ! flashPlayer )
	{
		return false;
	}

	size_t len = sizeof(sm_externalInterfaceTable)/sizeof(sm_externalInterfaceTable[0]);
	for ( size_t i = 0; i < len; ++i )
	{
		const String& methodName = sm_externalInterfaceTable[i].m_methodName;
		if ( ! flashPlayer->RegisterExternalInterface( methodName, this ) )
		{
			GUI_ERROR( TXT("Could not register as external interface for '%ls'"), methodName.AsChar() );
			return false;
		}
	}

	return true;
}

void CGuiManager::OnFlashExternalInterface( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args, CFlashValue& outRetval )
{
	size_t len = sizeof(sm_externalInterfaceTable)/sizeof(sm_externalInterfaceTable[0]);
	for ( size_t i = 0; i < len; ++i )
	{
		if ( methodName == sm_externalInterfaceTable[i].m_methodName )
		{
			TExternalInterfaceFunc func = sm_externalInterfaceTable[i].m_func;
			const Bool retval = (this->*func)( methodName, flashMovie, args );
			outRetval.SetFlashBool( retval );
			return;
		}
	}

	GUI_WARN( TXT("CGuiManager::OnFlashExternalInterface - Unhandled external interface '%ls'"), methodName.AsChar() );
	outRetval.SetFlashBool( false );
}

Bool CGuiManager::ExternalInterfaceRegisterHud( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args )
{
	const Uint32 numExpectedArgs = 2;
	if ( args.Size() != numExpectedArgs )
	{
		GUI_ERROR( TXT("Expected '%u' args, but received '%u' for external interface '%ls'"), numExpectedArgs, args.Size(), methodName.AsChar() );
		return false;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("Expected Flash string arg[0] for external interface '%ls'") );
		return false;
	}
	String hudName = args[0].GetFlashString();

	CFlashValue flashHud = args[1];
	if ( ! flashHud.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject arg[1] for external interface '%ls'") );
		return false;
	}

	if ( ! flashHud.IsFlashDisplayObjectOnStage() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject argument for external interface '%ls'. Use Flash Event.ADDED_TO_STAGE or CLIK's configUI()"), methodName.AsChar() );
		return false;
	}

	return RegisterFlashHud( hudName, flashHud );
}

Bool CGuiManager::ExternalInterfaceRegisterMenu( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args )
{
	const Uint32 numExpectedArgs = 2;
	if ( args.Size() != numExpectedArgs )
	{
		GUI_ERROR( TXT("Expected '%u' args, but received '%u' for external interface '%ls'"), numExpectedArgs, args.Size(), methodName.AsChar() );
		return false;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("Expected Flash String arg[0] for external interface '%ls'"), methodName.AsChar() );
		return false;
	}
	String menuName = args[0].GetFlashString();

	CFlashValue flashMenu = args[1];
	if ( ! flashMenu.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject arg[1] for external interface '%ls'"), methodName.AsChar() );
		return false;
	}

	if ( ! flashMenu.IsFlashDisplayObjectOnStage() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject argument for external interface '%ls'. Use Flash Event.ADDED_TO_STAGE or CLIK's configUI()"), methodName.AsChar() );
		return false;
	}

	return RegisterFlashMenu( menuName, flashMenu );
}

Bool CGuiManager::ExternalInterfaceRegisterPopup( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args )
{
	const Uint32 numExpectedArgs = 2;
	if ( args.Size() != numExpectedArgs )
	{
		GUI_ERROR( TXT("Expected '%u' args, but received '%u' for external interface '%ls'"), numExpectedArgs, args.Size(), methodName.AsChar() );
		return false;
	}

	if ( ! args[0].IsFlashString() )
	{
		GUI_ERROR( TXT("Expected Flash String arg[0] for external interface '%ls'"), methodName.AsChar() );
		return false;
	}
	String popupName = args[0].GetFlashString();

	CFlashValue flashPopup = args[1];
	if ( ! flashPopup.IsFlashDisplayObject() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject arg[1] for external interface '%ls'"), methodName.AsChar() );
		return false;
	}

	if ( ! flashPopup.IsFlashDisplayObjectOnStage() )
	{
		GUI_ERROR( TXT("Expected Flash DisplayObject argument for external interface '%ls'. Use Flash Event.ADDED_TO_STAGE or CLIK's configUI()"), methodName.AsChar() );
		return false;
	}

	return RegisterFlashPopup( popupName, flashPopup );
}

Bool CGuiManager::ExternalInterfaceIsUsingPad( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args )
{
	const Uint32 numExpectedArgs = 0;
	if ( args.Size() != numExpectedArgs )
	{
		GUI_ERROR( TXT("Expected '%u' args, but received '%u' for external interface '%ls'"), numExpectedArgs, args.Size(), methodName.AsChar() );
		return false;
	}

	return GCommonGame->IsUsingPad();
}

Bool CGuiManager::ExternalInterfaceIsPadConnected( const String& methodName, CFlashMovie* flashMovie, const TDynArray< CFlashValue >& args )
{
	const Uint32 numExpectedArgs = 0;
	if ( args.Size() != numExpectedArgs )
	{
		GUI_ERROR( TXT("Expected '%u' args, but received '%u' for external interface '%ls'"), numExpectedArgs, args.Size(), methodName.AsChar() );
		return false;
	}

	return GCommonGame->IsPadConnected();
}

Bool CGuiManager::GuiEventOnRequestMenu( const SGuiEvent& event )
{
	ASSERT( event.m_eventName == CNAME( OnRequestMenu ) );

	Bool retVal = false;

	const CName& menuName = event.m_eventEx;
	if ( ! menuName )
	{
		GUI_ERROR( TXT("GuiEventOnRequestMenu - expected menu name") );
		return false;
	}

	THandle< IScriptable > scriptInitData = nullptr;
	if ( ! event.m_args.Empty() )
	{
		const SGuiEventArg& eventArg = event.m_args[0];
		ASSERT( eventArg.GetArgType() == SGuiEventArg::Type_Object );
		if ( eventArg.GetArgType() == SGuiEventArg::Type_Object )
		{
			scriptInitData = eventArg.GetObject();
		}
	}

	CGuiObject* parentObject = event.m_parent.Get();
	if ( ! parentObject )
	{
		// Request a root menu
		retVal = RequestMenu( menuName, scriptInitData );

		if (!retVal)
		{
			CallEvent( CNAME(OnFailedCreateMenu) );
		}
	}
	else
	{
		CMenu* parentMenu = Cast< CMenu >( parentObject );
		if ( parentMenu )
		{
			// Request a child menu
			retVal = parentMenu->RequestSubMenu( menuName, scriptInitData );

			if (!retVal)
			{
				parentMenu->CallEvent( CNAME(OnFailedCreateMenu) );
			}
		}
	}

	GUI_ERROR( TXT("GuiEventOnRequestMenu - unexpected error") );
	return retVal;
}

Bool CGuiManager::GuiEventOnCloseMenu( const SGuiEvent& event )
{
	ASSERT( event.m_eventName == CNAME( OnCloseMenu ) );

	const CName& menuName = event.m_eventEx;
	if ( ! menuName )
	{
		GUI_ERROR( TXT("GuiEventOnRequestMenu - expected menu name") );
		return false;
	}

	return CloseMenu( menuName );
}

Bool CGuiManager::GuiEventOnRequestPopup( const SGuiEvent& event )
{
	ASSERT( event.m_eventName == CNAME( OnRequestPopup ) );

	const CName& popupName = event.m_eventEx;
	if ( ! popupName )
	{
		GUI_ERROR( TXT("GuiEventOnRequestPopup - expected popup name") );
		return false;
	}

	THandle< IScriptable > scriptInitData = nullptr;
	if ( ! event.m_args.Empty() )
	{
		const SGuiEventArg& eventArg = event.m_args[0];
		ASSERT( eventArg.GetArgType() == SGuiEventArg::Type_Object );
		if ( eventArg.GetArgType() == SGuiEventArg::Type_Object )
		{
			scriptInitData = eventArg.GetObject();
		}
	}

	// Request a root menu
	return RequestPopup( popupName, scriptInitData );
}

Bool CGuiManager::GuiEventOnClosePopup( const SGuiEvent& event )
{
	ASSERT( event.m_eventName == CNAME( OnClosePopup ) );

	const CName& popupName = event.m_eventEx;
	if ( ! popupName )
	{
		GUI_ERROR( TXT("GuiEventOnRequestPopup - expected popup name") );
		return false;
	}

	return ClosePopup( popupName );
}
