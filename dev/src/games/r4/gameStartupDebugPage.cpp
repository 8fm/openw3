#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "../../common/engine/gameResource.h"
#include "../../common/engine/TestFramework.h"
#include "../../common/game/gameSaver.h"
#include "../../common/core/depot.h"
#include "../../common/core/2darray.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/debugCheckBox.h"
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/gameSaveManager.h"

extern Bool			GUseSaveHack;
Bool				GCloseDebugPage = false;

CGatheredResource resCustomQuestListDebug( TXT("game\\custom_quests.csv"), RGF_NotCooked ); // it's added manually to the seed files

class CGameStartupOption : public IDebugCheckBox
{
public:
	CGameStartupOption( IDebugCheckBox* parent, const String& name )
		: IDebugCheckBox( parent, name, false, true )
	{}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data ) override
	{
		if ( action == IACT_Press )
		{
			if ( key == IK_Enter || key == IK_Pad_A_CROSS || key == IK_Pad_X_SQUARE )
			{
				OnActivated();
				return true;
			}

			if ( key == IK_Delete || key == IK_Pad_Y_TRIANGLE || key == IK_Pad_B_CIRCLE )
			{
				OnCancelItem();
				return true;
			}
		}

		return IDebugCheckBox::OnInput( key, action, data );
	};

protected:

	virtual void OnActivated() = 0;
	virtual void OnCancelItem() {}
};

class CSaveGameOption : public CGameStartupOption
{
protected:
	SSavegameInfo m_savegame;

public:
	CSaveGameOption( IDebugCheckBox* parent, const SSavegameInfo& savegame )
		: CGameStartupOption( parent, String::Printf( TXT("%ls   %ls   %02ld.%02ld.%ld %02ld:%02ld"), savegame.GetFileName().AsChar(), savegame.GetDisplayName().AsChar(), savegame.m_timeStamp.GetDay(), savegame.m_timeStamp.GetMonth(), savegame.m_timeStamp.GetYear(), savegame.m_timeStamp.GetHour(), savegame.m_timeStamp.GetMinute() ) )
		, m_savegame( savegame )
	{
	}

protected:
	virtual void OnActivated()
	{
		CR4Game* game = Cast< CR4Game > ( GGame );
		if ( game )
		{
			game->SetSavegameToLoad( m_savegame );
			game->CloseMainMenu();
		}		

		GCloseDebugPage = true;
	}

	virtual void OnCancelItem()
	{
		GUserProfileManager->DeleteSaveGame( m_savegame );
		GCloseDebugPage = true;
	}
};

class CLoadGameMenu : public IDebugCheckBox
{
public:
	CLoadGameMenu()
		: IDebugCheckBox( nullptr, TXT("Load game"), true, false )
	{
	}

	virtual void Expand( Bool isExpanded )
	{
		if ( isExpanded )
		{
			// Clear child list
			TDynArray< IDebugCheckBox* > options = m_children;
			m_children.Clear();

			// Delete local children
			for ( Uint32 i=0; i<options.Size(); i++ )
			{
				delete options[i];
			}

			TDynArray< SSavegameInfo > savesToLoad;
			GUserProfileManager->GetSaveFiles( savesToLoad );

			// Build UI
			for ( Uint32 i = 0; i < savesToLoad.Size(); i++ )
			{
				new CSaveGameOption( this, savesToLoad[ i ] );
			}
		}

		m_canExpand = m_children.Size() > 0;
		if ( m_canExpand )
		{
			m_isExpanded = isExpanded;
		}
	}
};

class CNewGamePlusOption : public CGameStartupOption
{
protected:
	SSavegameInfo m_savegame;

public:
	CNewGamePlusOption( IDebugCheckBox* parent, const SSavegameInfo& savegame )
		: CGameStartupOption( parent, String::Printf( TXT("%ls   %ls   %02ld.%02ld.%ld %02ld:%02ld"), savegame.GetFileName().AsChar(), savegame.GetDisplayName().AsChar(), savegame.m_timeStamp.GetDay(), savegame.m_timeStamp.GetMonth(), savegame.m_timeStamp.GetYear(), savegame.m_timeStamp.GetHour(), savegame.m_timeStamp.GetMinute() ) )
		, m_savegame( savegame )
	{
	}

protected:
	virtual void OnActivated()
	{
		CR4Game* game = Cast< CR4Game > ( GGame );
		if ( game )
		{
			if ( NGP_Success == game->StartNewGamePlus( m_savegame ) )
			{
				game->CloseMainMenu();
			}
		}		

		GCloseDebugPage = true;
	}

	virtual void OnCancelItem()
	{
		GCloseDebugPage = true;
	}
};

class CNewGamePlusMenu : public IDebugCheckBox
{
public:
	CNewGamePlusMenu()
		: IDebugCheckBox( nullptr, TXT("New game plus"), true, false )
	{
	}

	virtual void Expand( Bool isExpanded )
	{
		if ( isExpanded )
		{
			// Clear child list
			TDynArray< IDebugCheckBox* > options = m_children;
			m_children.Clear();

			// Delete local children
			for ( Uint32 i=0; i<options.Size(); i++ )
			{
				delete options[i];
			}

			TDynArray< SSavegameInfo > savesToLoad;
			GUserProfileManager->GetSaveFiles( savesToLoad );

			// Build UI
			for ( Uint32 i = 0; i < savesToLoad.Size(); i++ )
			{
				new CNewGamePlusOption( this, savesToLoad[ i ] );
			}
		}

		m_canExpand = m_children.Size() > 0;
		if ( m_canExpand )
		{
			m_isExpanded = isExpanded;
		}
	}
};

class CSaveHackCheckbox : public IDebugCheckBox
{
	CLoadGameMenu* m_lgmenu;

public:
	CSaveHackCheckbox( CLoadGameMenu* lgmenu )
		: IDebugCheckBox( nullptr, TXT("Use save hack (enable 'development' saving mode, as loose .sav files)"), false, true )
		, m_lgmenu( lgmenu )
	{}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data ) override
	{
		if ( action == IACT_Press && ( key == IK_Space || key == IK_Enter || key == IK_Pad_A_CROSS || key == IK_Pad_X_SQUARE ) )
		{
			OnToggle();
			return true;
		}

		return IDebugCheckBox::OnInput( key, action, data );
	};

	virtual Bool IsChecked() const { return GUseSaveHack; }
	virtual void OnToggle() { GUseSaveHack = !GUseSaveHack; m_lgmenu->Expand( false ); }
};

class CDeleteAllSavesCheckbox : public IDebugCheckBox
{
	CLoadGameMenu* m_lgmenu;

public:
	CDeleteAllSavesCheckbox( CLoadGameMenu* lgmenu )
		: IDebugCheckBox( nullptr, TXT("Delete all saves"), false, true )
		, m_lgmenu( lgmenu )
	{}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data ) override
	{
		if ( action == IACT_Press && ( key == IK_Space || key == IK_Enter || key == IK_Pad_A_CROSS || key == IK_Pad_X_SQUARE ) )
		{
			TDynArray< SSavegameInfo > allSaves;
			GUserProfileManager->GetSaveFiles( allSaves );
			for ( auto save : allSaves )
			{
				GUserProfileManager->DeleteSaveGame( save );
			}
			m_lgmenu->Expand( false );
			return true;
		}

		return IDebugCheckBox::OnInput( key, action, data );
	};

	virtual Bool IsChecked() const { return false; }
};


#ifndef NO_SAVE_IMPORT
class CImportGameOption : public CGameStartupOption
{
protected:
	SSavegameInfo m_savegame;

public:
	CImportGameOption( IDebugCheckBox* parent, const SSavegameInfo& savegame )
		: CGameStartupOption( parent, savegame.GetFileName() )
		, m_savegame( savegame )
	{}

protected:
	virtual void OnActivated()
	{
		GUserProfileManager->ImportSave( m_savegame );
		GCloseDebugPage = true;
	}
};
#endif

class CGameWorldOption : public CGameStartupOption
{
public:
	CGameWorldOption( IDebugCheckBox* parent, const String& worldFile, const String& name = String::EMPTY )
		: CGameStartupOption( parent, name.Empty() ? worldFile : name )
		, m_worldPath( worldFile )
	{}

protected:
	String m_worldPath;

	virtual void OnActivated()
	{
		LoadWorld( m_worldPath );
	}

	void LoadWorld( String worldFile )
	{
		// Get the actual world name from game configuration

		if ( worldFile.EndsWith( TXT(".redgame") ) )
		{
			GGame->SetupGameResourceFromFile( worldFile );
			CGameResource *res = GGame->GetGameResource();
			if ( res )
			{
				worldFile = res->GetStartWorldPath();		
			}
			else
			{
				LOG_R4( TXT("wrong .redgame resource: '%ls'"), worldFile.AsChar() );
				return;
			}
		}

		GGame->ClearInitialFacts();
		// Add fake fact, so there will be no "Test chicken phase" in cook, but it will appear in editor
		GGame->AddInitialFact( TXT("CookGameInitialized") );

		GCloseDebugPage = true;

		SGameSessionManager::GetInstance().CreateSession( worldFile );
	}
};

class CQuitGameOption : public CGameStartupOption
{
public:
	CQuitGameOption()
		: CGameStartupOption( NULL, TXT("Quit") )
	{}

protected:

	virtual void OnActivated()
	{
		GEngine->RequestExit();
	}
};

class CMainMenuOption : public CGameStartupOption
{
public:
	CMainMenuOption()
		: CGameStartupOption( NULL, TXT("Main Menu") )
	{}

protected:

	virtual void OnActivated()
	{
		GCloseDebugPage = true;
	}
};

class CAutosaveOption : public CGameWorldOption
{
public:
	CAutosaveOption()
		: CGameWorldOption( NULL, TXT("Autosave") )
	{}

protected:

	virtual void OnActivated()
	{
		GCommonGame->RequestGameSave( SGT_AutoSave, -1, TXT("debug page") );
	}
};

/// Debug page used to select savegame / game world / game quest
class CDebugPageGameStartup : public IDebugPage
{
protected:
	CDebugOptionsTree*			m_tree;
	Bool						m_scanDepot;

public:
	CDebugPageGameStartup( Bool scanDepot )
		: IDebugPage( TXT("Game Startup") )
		, m_scanDepot( scanDepot )
	{
	}

	void OnPageShown()
	{
		m_tree = new CDebugOptionsTree( 55, 65, 800, 500, this );

		CLoadGameMenu* saveMenu = AddSaveMenu();
		AddNewGamePlusMenu();

		if ( m_scanDepot )
		{
			AddQuestMenu();
			AddWorldMenu();
			AddImportMenu();
			m_tree->AddRoot( new CDeleteAllSavesCheckbox( saveMenu ) );
			m_tree->AddRoot( new CQuitGameOption() );
			m_tree->AddRoot( new CMainMenuOption() );
		}
		else // Temporary hardwired thing
		{
			IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Start Game"), true, false );
			m_tree->AddRoot( menu );

			new CGameWorldOption( menu, TXT("game\\witcher3.redgame"), TXT("witcher3") );

			// add custom quests
			THandle< C2dArray > customQuests = resCustomQuestListDebug.LoadAndGet< C2dArray >();
			if ( customQuests )
			{
				const Uint32 count = customQuests->GetNumberOfRows();
				for ( Uint32 i=0; i<count; ++i )
				{
					const String depotPath = customQuests->GetValue(0,i);
					const String name = customQuests->GetValue(1,i);

					// only show existing options
					if ( GDepot->FindFileUseLinks( depotPath, 0 ) != nullptr )
					{
						new CGameWorldOption( menu, name, depotPath );
					}
				}

				// unload
				customQuests->Discard();
			}

			AddImportMenu();

			m_tree->AddRoot( new CDeleteAllSavesCheckbox( saveMenu ) );

			m_tree->AddRoot( new CAutosaveOption() );
		}

		#ifdef RED_PLATFORM_DURANGO
			m_tree->AddRoot( new CSaveHackCheckbox( saveMenu ) );
		#endif

		m_tree->SelectItem( saveMenu );
	}

	void OnPageHidden()
	{
		delete m_tree;
		m_tree = nullptr;
	}

	CLoadGameMenu* AddSaveMenu()
	{
		CLoadGameMenu* menu = new CLoadGameMenu();
		m_tree->AddRoot( menu );
		return menu;
	}

	CNewGamePlusMenu* AddNewGamePlusMenu()
	{
		CNewGamePlusMenu* menu = new CNewGamePlusMenu();
		m_tree->AddRoot( menu );
		return menu;
	}

	IDebugCheckBox* AddImportMenu()
	{
		IDebugCheckBox* menu( nullptr );

		#ifndef NO_SAVE_IMPORT
			// Get savegames
			TDynArray< SSavegameInfo > savesToImport;
			GUserProfileManager->Import_GetSaveFiles( savesToImport );

			// Build UI
			menu = new IDebugCheckBox( NULL, TXT("Import facts from W2 saved game"), true, false );
			m_tree->AddRoot( menu );
			for ( Uint32 i = 0; i < savesToImport.Size(); i++ )
			{
				new CImportGameOption( menu, savesToImport[ i ] );
			}
		#endif // ifndef NO_SAVE_IMPORT

		return menu;
	}

	static void FindFiles( CDirectory* dir, const String& extension, TDynArray< String >& output )
	{
		// Recurse
		for ( CDirectory* child : dir->GetDirectories() )
		{
			FindFiles( child, extension, output );
		}

		// Enumerate files
		for ( CDiskFile* file : dir->GetFiles() )
		{
			if ( file->GetFileName().EndsWith( extension ) )
			{
				output.PushBack( file->GetDepotPath() );
			}		
		}
	}

	IDebugCheckBox* AddWorldMenu()
	{
		// Find worlds

		TDynArray< String > worlds;
		FindFiles( GDepot, TXT("w2w"), worlds );

		Sort( worlds.Begin(), worlds.End() );

		// Build UI

		IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Load World"), true, false );
		m_tree->AddRoot( menu );
		for ( Uint32 i = 0; i < worlds.Size(); i++ )
		{
			new CGameWorldOption( menu, worlds[i] );
		}

		return menu;
	}

	IDebugCheckBox* AddQuestMenu()
	{
		// Find quests

		TDynArray< String > quests;
		FindFiles( GDepot, TXT("redgame"), quests );
		Sort( quests.Begin(), quests.End() );
		// Build UI

#ifdef RED_PLATFORM_CONSOLE
		quests.Remove( TXT("engine\\default.redgame") );
#endif

		IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Start Quest"), true, false );
		m_tree->AddRoot( menu );
		for ( Uint32 i = 0; i < quests.Size(); i++ )
		{
			new CGameWorldOption( menu, quests[i] );
		}

		return menu;
	}

	~CDebugPageGameStartup()
	{
		delete m_tree;
	}

	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		m_tree->OnRender( frame );		
	}

	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		// Send the event
		if ( m_tree->OnInput( key, action, data ) )
		{
			if ( GCloseDebugPage )
			{
				IDebugPageManagerBase::GetInstance()->SelectDebugPage( nullptr );

				CR4Game* game = Cast< CR4Game > ( GGame );
				if ( game )
				{
					game->CloseMainMenu();
				}

				GCloseDebugPage = false;
				return true;
			}

			return true;
		}

		// Not processed
		return false;
	}

	virtual void OnTick( Float timeDelta )
	{
		m_tree->OnTick( timeDelta );
	}

};

void CreateDebugPageGameStartup( Bool scanDepot, Bool makeVisible )
{
	IDebugPage* page = new CDebugPageGameStartup( scanDepot );
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );

	if ( makeVisible )
	{
		// Make it visible at startup
		IDebugPageManagerBase::GetInstance()->SelectDebugPage( page );
	}
}

#endif
