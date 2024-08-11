/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

enum EGameState
{
	GS_None,
	GS_Startup,
	GS_InitialConfig,
	GS_MainMenu,
	GS_Loading,
	GS_Playing,
};


////////////////////////////////////////////////////////////////////////////////////////////
//
// CMainMenuController
//
class CMainMenuController
{

private:
	TDynArray< CName >	m_queuedMenus;
	CName				m_lastShownMenu;
	EGameState			m_gameState;
	Bool				m_signoutOccurred;
	Bool				m_userProfileReady;
	Bool				m_localizationChangePending;

public:
	CMainMenuController();
	~CMainMenuController();

public:
	void		Tick( Float timeDelta );
	void		CloseMenu();

	void		OnUserProfileReady();
	void		OnProfileSignedOut();
	void		OnLoadingFailed();

	EGameState  GetCurrentMenuState() { return m_gameState; }

	void		RequireLocalizationChange() { m_localizationChangePending = true; }

private:
	enum class EQueuedMenuTickResult
	{
		QMTR_NoManager,
		QMTR_UserSignedOut,
		QMTR_MenuActive,
		QMTR_MenuAdvanced,
		QMTR_QueueEmpty
	};

	EQueuedMenuTickResult	TickMenuQueue( Bool ignoreSignoutEvents );
	void					TickStartupMenus();
	void					TickInitialConfigMenus();
	void					TickMainMenu();
	void					TickLocalisationChange();

	void					PopulateMenuQueue( const CName& name );

	void					ShowStartupMenus( bool firstStart );
	void					ShowInitialConfigMenus();
	void					ShowMainMenu( Bool first );

	Bool					HasSetupConfigVars() const;
};

