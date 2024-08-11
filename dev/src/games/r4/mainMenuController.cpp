/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "mainMenuController.h"

#include "..\..\common\engine\gameResource.h"
#include "..\..\common\game\gameSaver.h"
#include "..\..\common\engine\localizationManager.h"
#include "..\..\games\r4\r4GuiManager.h"
#include "..\..\common\core\contentManager.h"

RED_DEFINE_STATIC_NAME( PopulateMenuQueueStartupOnce );
RED_DEFINE_STATIC_NAME( PopulateMenuQueueStartupAlways );
RED_DEFINE_STATIC_NAME( PopulateMenuQueueConfig );
RED_DEFINE_STATIC_NAME( PopulateMenuQueueMainOnce );
RED_DEFINE_STATIC_NAME( PopulateMenuQueueMainAlways );

RED_DEFINE_STATIC_NAME( Hidden );
RED_DEFINE_STATIC_NAME( HasSetup );

CMainMenuController::CMainMenuController()
	: m_gameState( GS_None )
	, m_signoutOccurred( false )
	, m_userProfileReady( false )
	, m_localizationChangePending( false )
{
}

CMainMenuController::~CMainMenuController()
{
}

RED_INLINE Bool CMainMenuController::HasSetupConfigVars() const
{
	RED_FATAL_ASSERT( GCommonGame, "This should always exist" );
	
	static const String expectedResult( TXT( "false" ) );
	return GCommonGame->GetInGameConfigWrapper().GetConfigValueConst( CNAME( Hidden ), CNAME( HasSetup ) ) == expectedResult;
}


void CMainMenuController::Tick( float timeDelta )
{
	if ( GGame->IsActive() )
	{
		if (m_gameState == GS_MainMenu)
		{
			CloseMenu();
		}
		m_gameState = GS_Playing;
	}

	switch( m_gameState )
	{
	case GS_None:
		ShowStartupMenus( true );
		break;

	case GS_Startup:
		TickStartupMenus();
		break;

	case GS_InitialConfig:
		TickInitialConfigMenus();
		break;

	case GS_MainMenu:
		TickMainMenu();
		break;

	case GS_Loading:
		break;

	case GS_Playing:
		if ( !GGame->IsActive() )
		{
			if (GGame->GetEndGameReason() == ENDGAMEREASON_UserSignOut)
			{
				ShowStartupMenus( false );
			}
			else
			{
				ShowMainMenu( false );
			}
		}
		break;
	}
}

void CMainMenuController::PopulateMenuQueue( const CName& name )
{
	if ( !CallFunctionRef( GGame, name, m_queuedMenus ) )
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( MainMenu ), TXT( "Cannot get '%ls' menu names" ), name.AsChar() );
	}
}

CMainMenuController::EQueuedMenuTickResult CMainMenuController::TickMenuQueue( Bool ignoreSignoutEvents )
{
	CR4GuiManager* manager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( !manager )
	{
		return EQueuedMenuTickResult::QMTR_NoManager;
	}

	if( !ignoreSignoutEvents && m_signoutOccurred )
	{
		if ( manager->IsAnyMenu() )
		{
			GCommonGame->CloseMenu( m_lastShownMenu );
		}

		return EQueuedMenuTickResult::QMTR_UserSignedOut;
	}

	if ( manager->IsAnyMenu() )
	{
		return EQueuedMenuTickResult::QMTR_MenuActive;
	}

	if( m_queuedMenus.Empty() )
	{
		return EQueuedMenuTickResult::QMTR_QueueEmpty;
	}

	TickLocalisationChange(); // No reason not to do this check between menu's as its always the ideal time

	CName currentMenu = m_queuedMenus[ 0 ];
	m_queuedMenus.RemoveAt( 0 );

	THandle< IScriptable > initData;
	GCommonGame->RequestMenu( currentMenu, initData );
	
	m_lastShownMenu = currentMenu;

	return EQueuedMenuTickResult::QMTR_MenuAdvanced;
}

void CMainMenuController::TickStartupMenus()
{
	EQueuedMenuTickResult result = TickMenuQueue( true );

	switch( result )
	{
	case EQueuedMenuTickResult::QMTR_QueueEmpty:

		TickLocalisationChange();

		if( m_signoutOccurred )
		{
			// This means a signout occurred just after someone clicked to progress past the start screen ¬_¬
			ShowStartupMenus( false );
		}
		else
		{
			if ( HasSetupConfigVars() )
			{
				ShowInitialConfigMenus();
			}
			else
			{
				ShowMainMenu( true );
			}
		}
		break;
	}

	if( m_signoutOccurred )
	{
		CloseMenu();
		ShowStartupMenus( false );
		m_signoutOccurred = false;
	}
}

void CMainMenuController::TickInitialConfigMenus()
{
	EQueuedMenuTickResult result = TickMenuQueue( false );

	switch( result )
	{
	case EQueuedMenuTickResult::QMTR_UserSignedOut:
		ShowStartupMenus( false );
		break;

	case EQueuedMenuTickResult::QMTR_QueueEmpty:
		ShowMainMenu( true );
		break;
	}
}

void CMainMenuController::TickMainMenu()
{
	TickLocalisationChange();

	if ( m_signoutOccurred )
	{
		// If we're responding to a signout event, but we still have an active user
		// that means we simply switched user and we should expect a signin event imminently
		if( GUserProfileManager->HasActiveUser() )
		{
			if( m_userProfileReady )
			{
				if( HasSetupConfigVars() )
				{
					CloseMenu();
					ShowInitialConfigMenus();
				}
				else
				{
					GCommonGame->GetGuiManager()->RefreshMenu();
					GCommonGame->CallEvent( CNAME( OnRefreshUIScaling ) );
				}

				m_signoutOccurred = false;
				m_userProfileReady = false;
			}
		}
		else
		{
			CloseMenu();
			ShowStartupMenus(false);

			m_signoutOccurred = false;
		}
	}
	else
	{
		TickMenuQueue( true );
	}
}

void CMainMenuController::ShowStartupMenus( bool firstStart )
{
	m_gameState = GS_Startup;

	m_queuedMenus.ClearFast();

	if( firstStart )
	{
		PopulateMenuQueue( CNAME( PopulateMenuQueueStartupOnce ) );
	}

	PopulateMenuQueue( CNAME( PopulateMenuQueueStartupAlways ) );
}

namespace Config
{
	TConfigVar< Float > cvUiVerticalFrameScale( "Hidden", "uiVerticalFrameScale", 1.0f, eConsoleVarFlag_Save );
	TConfigVar< Float > cvUiHorizontalFrameScale( "Hidden", "uiHorizontalFrameScale", 1.0f, eConsoleVarFlag_Save );
}

void CMainMenuController::ShowInitialConfigMenus()
{
	// Set the default values for the UI scale menu from whatever the system recommends
	Float x, y;
	GUserProfileManager->GetSafeArea( x, y );
	Config::cvUiVerticalFrameScale.Set( y );
	Config::cvUiHorizontalFrameScale.Set( x );

	m_gameState = GS_InitialConfig;

	m_queuedMenus.ClearFast();

	PopulateMenuQueue( CNAME( PopulateMenuQueueConfig ) );
}

void CMainMenuController::ShowMainMenu( Bool first )
{
	m_gameState = GS_MainMenu;

	//rich presence call
	GUserProfileManager->SetUserPresence( CName( TXT( "main_menu" ) ) );

	m_queuedMenus.ClearFast();

	if( first )
	{
		PopulateMenuQueue( CNAME( PopulateMenuQueueMainOnce ) );
	}

	PopulateMenuQueue( CNAME( PopulateMenuQueueMainAlways ) );

	LOG_GAME(TXT(">>> GContentManager->DumpProgressToLog() at MAIN MENU <<<"));
	GContentManager->DumpProgressToLog();
}

void CMainMenuController::CloseMenu()
{
	m_gameState = GS_Loading;

	THandle< IScriptable > initData;
	GCommonGame->CloseMenu( m_lastShownMenu );
}

void CMainMenuController::OnProfileSignedOut()
{
	m_signoutOccurred = true;
	m_userProfileReady = false;
}

void CMainMenuController::OnLoadingFailed()
{
	ShowMainMenu( false );
}

void CMainMenuController::OnUserProfileReady()
{
	m_userProfileReady = true;
	m_localizationChangePending = true;
}

void CMainMenuController::TickLocalisationChange()
{
	if( m_localizationChangePending )
	{
		SLocalizationManager::GetInstance().ReloadLanguageFromUserSettings();
		m_localizationChangePending = false;
	}
}
