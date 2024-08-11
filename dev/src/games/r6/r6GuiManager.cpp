/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashPlayer.h"
#include "../../common/game/commonGameResource.h"
#include "../../common/game/guiConfigResource.h"
#include "../../common/game/hudResource.h"
#include "../../common/game/menuResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"

#include "r6Hud.h"
#include "r6Menu.h"
#include "r6GuiManager.h"
#include "../../common/engine/guiGlobals.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/viewport.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CR6GuiManager );

//////////////////////////////////////////////////////////////////////////
// Gathered Resources
//////////////////////////////////////////////////////////////////////////
CGatheredResource resDefaultGuiConfigR6( TXT("gameplay\\gui\\guirsrc\\r6default.guiconfig"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////
// CR6GuiManager
//////////////////////////////////////////////////////////////////////////
CR6GuiManager::CR6GuiManager()
	: m_hud( nullptr )
	, m_menu( nullptr )
	, m_pausedGame( false )
	, m_cursorShown( false )
{
}

Bool CR6GuiManager::Initialize()
{
	if ( TBaseClass::Initialize() )
	{
		return true;
	}

	return false;
}

void CR6GuiManager::OnFinalize()
{
	//TODO: move into game state manager
	if ( m_pausedGame && GGame )
	{
		GGame->Unpause( TXT("CR6GuiManager") );
		m_pausedGame = false;
	}
	if ( m_cursorShown && GGame )
	{
		IViewport* vp = GGame->GetViewport();
		if ( vp )
		{
			//vp->CaptureInput( ICM_Full );
		}
		m_cursorShown = false;
	}

	TBaseClass::OnFinalize();
}

void CR6GuiManager::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_hud )
		{
			file << m_hud;
		}

		if ( m_menu )
		{
			file << m_menu;
		}
	}

	TBaseClass::OnSerialize( file );
}

void CR6GuiManager::OnGameStart( const CGameInfo& gameInfo )
{
	TBaseClass::OnGameStart( gameInfo );

	m_cachedHudDepotMap.ClearFast();
	m_cachedMenuDepotMap.ClearFast();

// Hardcoded for now, as need to put in the gamedef

	CGuiConfigResource* guiConfigResource = resDefaultGuiConfigR6.LoadAndGet< CGuiConfigResource >();
	ASSERT( guiConfigResource );
	if ( ! guiConfigResource )
	{
		return;
	}

// 	CGuiConfigResource* guiConfigResource = GetGuiConfigOverride();
// 
// 	if ( ! guiConfigResource )
// 	{
// 		return;
// 	}

#ifndef NO_EDITOR
	//if ( ! guiConfigResource )
	//{
	//		GUI_WARN( TXT("No game resource for GUI manager at game start. Will try to use defaults.") );
	//}

#endif

	const TDynArray< SHudDescription >& hudDescList = guiConfigResource->GetHudDescriptions();
	const TDynArray< SMenuDescription >& menuDescList = guiConfigResource->GetMenuDescriptions();

	for ( TDynArray< SHudDescription >::const_iterator it = hudDescList.Begin(); it != hudDescList.End(); ++it )
	{
		const SHudDescription& hudDesc = *it;
		const CName& hudName = hudDesc.m_hudName;
		const String& resourceDepotPath = hudDesc.m_hudResource.GetPath();
		
		if ( ! hudName )
		{
			GUI_WARN( TXT("HUD name is not defined in guiconfig. Skipping.") );
			continue;
		}
		if ( resourceDepotPath.Empty() )
		{
			GUI_WARN( TXT("HUD '%ls' from guiconfig doesn't have a resource file. Skipping."), hudName.AsString().AsChar() );
			continue;
		}

		if ( ! m_cachedHudDepotMap.Insert( hudName, resourceDepotPath ) )
		{
			GUI_WARN( TXT("HUD '%ls' with resource path '%ls' already defined. Skipping."), hudName.AsString().AsChar(), resourceDepotPath.AsChar() );
		}
	}

	for ( TDynArray< SMenuDescription >::const_iterator it = menuDescList.Begin(); it != menuDescList.End(); ++it )
	{
		const SMenuDescription& menuDesc = *it;
		const CName& menuName = menuDesc.m_menuName;
		const String& resourceDepotPath = menuDesc.m_menuResource.GetPath();

		if ( ! menuName )
		{
			GUI_WARN( TXT("Menu name is not defined in guiconfig. Skipping.") );
			continue;
		}
		if ( resourceDepotPath.Empty() )
		{
			GUI_WARN( TXT("Menu '%ls' from guiconfig doesn't have a resource file. Skipping."), menuName.AsString().AsChar() );
			continue;
		}

		if ( ! m_cachedMenuDepotMap.Insert( menuName, resourceDepotPath ) )
		{
			GUI_WARN( TXT("Menu '%ls' with resource path '%ls' already defined. Skipping."), menuName.AsString().AsChar(), resourceDepotPath.AsChar() );
		}
	}
}

void CR6GuiManager::OnGameEnd( const CGameInfo& gameInfo )
{
	TBaseClass::OnGameEnd( gameInfo );

	m_cachedHudDepotMap.Clear();
	m_cachedMenuDepotMap.Clear();
}

void CR6GuiManager::OnWorldStart( const CGameInfo& gameInfo )
{
	TBaseClass::OnWorldStart( gameInfo );

	m_hud = CreateHud();
	ASSERT( m_hud );
	if ( m_hud )
	{
		VERIFY( m_hud->Init( this ) );
	}
}

CHud* CR6GuiManager::GetHud() const
{ 
	return m_hud;
}

Bool CR6GuiManager::RequestMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData )
{
	//TBD: if already open... transition properly
	if ( m_menu )
	{
		m_menu->Discard();
		m_menu = nullptr;
	}

	m_menu = CreateMenu( menuName );
	ASSERT( m_menu );

	if ( m_menu )
	{
		//TBD: menu parents
		VERIFY( m_menu->Init( menuName, this ) );
		m_menu->SetMenuScriptData( scriptInitData );
	}

	return true;
}

Bool CR6GuiManager::CloseMenu( const CName& menuName )
{
	// Unused for now; might want to check against the menu name to verify, or close a specific menu
	RED_UNUSED( menuName );

	if ( m_menu )
	{
		m_menu->Discard();
		m_menu = nullptr;
		return true;
	}

	return false;
}

Bool CR6GuiManager::RequestPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData )
{
	return false;
}

Bool CR6GuiManager::ClosePopup( const CName& popuName )
{
	return false;
}

Bool CR6GuiManager::RegisterFlashHud( const String& hudName, const CFlashValue& flashHud )
{
	ASSERT( m_hud );
	if (! m_hud )
	{
		GUI_ERROR( TXT("No HUD expected") );
		return false;
	}

	if ( ! m_hud->InitWithFlashSprite( flashHud ) )
	{
		GUI_ERROR( TXT("Failed to register the HUD '%ls'"), hudName.AsChar() );
		return false;
	}

	return true;
}

Bool CR6GuiManager::RegisterFlashMenu( const String& menuName, const CFlashValue& flashMenu )
{
	//TBD: Have loading token to make sure the correct menu is being used

	if ( ! m_menu )
	{
		GUI_ERROR( TXT("No menu expected") );
		return false;
	}

	//TODO: Verify against menu name
	if ( ! m_menu->InitWithFlashSprite( flashMenu ) )
	{
		GUI_ERROR( TXT("Failed to register menu '%ls'."), menuName.AsChar() );
		return false;
	}

	return true;
}

Bool CR6GuiManager::RegisterFlashPopup( const String& popupName, const CFlashValue& flashPopup )
{
	return false;
}

CR6Hud* CR6GuiManager::CreateHud()
{
	ASSERT( ! m_hud, TXT("HUD already exists") );

	CHudResource* hudResource = nullptr;

	String hudDepotPath;
	
	if ( ! m_cachedHudDepotMap.Find( CNAME(DefaultHud), hudDepotPath ) )
	{
		GUI_ERROR( TXT("Default HUD not found") );
		return nullptr;
	}

	hudResource = Cast< CHudResource >( GDepot->LoadResource( hudDepotPath ) );
	ASSERT( hudResource );
	if ( ! hudResource )
	{
		GUI_ERROR( TXT("HUD resource '%ls' from gamedef not loaded."), hudDepotPath.AsChar() );
		return nullptr;
	}

	CR6Hud* hud = SafeCast< CR6Hud >( CHud::CreateObjectFromResource( Cast< CHudResource >( hudResource ), this ) );
	ASSERT( hud );
	if ( ! hud )
	{
		GUI_ERROR( TXT("Failed to create HUD from resource") );
		return nullptr;
	}

	return hud;
}

void CR6GuiManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	TBaseClass::OnWorldEnd( gameInfo );

	m_debugSubtitle.m_valid = false;

	if ( m_hud )
	{
		m_hud->Discard();
		m_hud = nullptr;
	}

	if ( m_menu )
	{
		m_menu->Discard();
		m_menu = nullptr;
	}
}

Bool CR6GuiManager::NeedsInputCapture() const 
{
	//TODO: 
	return m_menu != nullptr;
}

void CR6GuiManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD

	if ( m_debugSubtitle.m_valid )
	{
		CEntity* actor = m_debugSubtitle.m_actor.Get();
		String actorName = actor ? actor->GetDisplayName() : TXT("<actor?>");
		const String displayText = String::Printf( TXT("%s: %s"), actorName.AsChar(), m_debugSubtitle.m_text.AsChar() );
		frame->AddDebugScreenText( 20, frame->GetFrameOverlayInfo().m_height - 100, displayText, Color::WHITE );
	}

#endif // RED_FINAL_BUILD
}

void CR6GuiManager::Tick( Float deltaTime )
{
	PC_SCOPE( CR6GuiManager_TickAll );

	TBaseClass::Tick( deltaTime );

	//TODO: Have a way of potentially locking the gui event queue if needs be.

	ProcessGameState();
}

void CR6GuiManager::ProcessGameState()
{
	//TODO: Not so hardcoded. 
	// Just don't have time right now and who knows what R6's real needs will be atm.

	ProcessHudVisibility();

	if ( m_pausedGame && ! m_menu && GGame )
	{
		GGame->Unpause( TXT("CR6GuiManager") );
		m_pausedGame = false;
	}
	else if ( ! m_pausedGame && m_menu && GGame )
	{
		GGame->Pause( TXT("CR6GuiManager") );
		m_pausedGame = true;
	}

	if ( m_cursorShown && ! m_menu )
	{
		IViewport* vp = GGame ? GGame->GetViewport() : nullptr;
		if ( vp )
		{
			//vp->CaptureInput( ICM_Full );
		}
		m_cursorShown = false;
	}
	else if ( ! m_cursorShown && m_menu )
	{
		IViewport* vp = GGame ? GGame->GetViewport() : nullptr;
		if ( vp )
		{
			//vp->CaptureInput( ICM_Background );
		}
		m_cursorShown = true;
	}
}

//TODO: not so hardcoded
void CR6GuiManager::ProcessHudVisibility() const
{
	const Bool gameActive = GGame && ! GGame->IsBlackscreen() && ! GGame->IsLoadingScreenShown() && GGame->IsActive();
	
	//TBD: have menus that don't hide the HUD
	const Bool onlyElement = ! m_menu;

	const Bool showHud = gameActive && onlyElement;
	if ( m_hud )
	{
		if ( showHud )
		{
			m_hud->Attach();
		}
		else
		{
			m_hud->Detach();
		}
	}
}

CR6Menu* CR6GuiManager::CreateMenu( const CName& menuName )
{
	String menuDepothPath;
	if ( ! m_cachedMenuDepotMap.Find( menuName, menuDepothPath ) )
	{
		GUI_ERROR( TXT("Menu '%ls' not found"), menuName.AsString().AsChar() );
		return nullptr;
	}

	CMenuResource* menuResource = Cast< CMenuResource >( GDepot->LoadResource( menuDepothPath ) );
	ASSERT( menuResource );
	if ( ! menuResource )
	{
		GUI_ERROR( TXT("Failed to open menu '%ls' with resource '%ls'"), menuName.AsString().AsChar(), menuDepothPath.AsChar() );
		return nullptr;
	}

	CR6Menu* menu = SafeCast< CR6Menu >( CMenu::CreateObjectFromResource( Cast< CMenuResource >( menuResource ), this ) );
	ASSERT( menu );
	if ( ! menu )
	{
		GUI_ERROR( TXT("Failed to create menu '%ls' from resource '%ls'"), menuName.AsString().AsChar(), menuDepothPath.AsChar() );
		return nullptr;
	}

	return menu;
}
