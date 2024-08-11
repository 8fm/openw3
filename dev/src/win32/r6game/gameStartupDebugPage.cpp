#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "../../common/engine/gameResource.h"
#include "../../common/engine/TestFramework.h"
#include "../../common/core/depot.h"
#include "../../common/engine/debugCheckBox.h"
#include "../../common/engine/debugPageManagerBase.h"
#include "../../common/engine/gameSaveManager.h"

class CGameStartupOption : public IDebugCheckBox
{
public:
	CGameStartupOption( IDebugCheckBox* parent, const String& name )
		: IDebugCheckBox( parent, name, false, true )
	{}

	virtual Bool OnInput( enum EInputKey key, enum EInputAction action, Float data ) override
	{
		if ( key == IK_Enter || key == IK_Pad_A_CROSS || key == IK_Pad_X_SQUARE )
		{
			OnActivated();
			return true;
		}

		return IDebugCheckBox::OnInput( key, action, data );
	};

protected:

	virtual void OnActivated() = 0;
};

class CGameWorldOption : public CGameStartupOption
{
public:
	CGameWorldOption( IDebugCheckBox* parent, const String& worldFile )
		: CGameStartupOption( parent, worldFile )
	{}

protected:

	virtual void OnActivated()
	{
		LoadWorld( GetName() );
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
				LOG_R6( TXT("wrong .redgame resource: '%s'"), worldFile.AsChar() );
				return;
			}
		}

		GGame->ClearInitialFacts();
		IDebugPageManagerBase::GetInstance()->SelectDebugPage( NULL );
		SGameSessionManager::GetInstance().CreateSession( worldFile );
	}
};

class CNewGameOption : public CGameWorldOption
{
public:
	CNewGameOption()
		: CGameWorldOption( NULL, TXT("New Game") )
	{}

protected:

	virtual void OnActivated()
	{
		LoadWorld( TXT("") );
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
		#if ( defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_WIN32 ) ) 
			::PostQuitMessage( 0 );
		#else
			GEngine->RequestExit();
		#endif
	}
};

/// Debug page used to select savegame / game world / game quest
class CDebugPageGameStartup : public IDebugPage
{
protected:
	CDebugOptionsTree*			m_tree;

public:
	CDebugPageGameStartup()
		: IDebugPage( TXT("Game Startup") )
	{
		m_tree = new CDebugOptionsTree( 55, 65, 800, 500, this );

		IDebugCheckBox* menu = AddGameMenu();
		//AddSaveMenu();
		AddWorldMenu();

		m_tree->AddRoot( new CQuitGameOption() );
		m_tree->SelectItem( menu );
	}
	  /*
	IDebugCheckBox* AddSaveMenu()
	{
		// Get savegames
		TDynArray< SSavegameInfo > sortedGameSaves;
		GUserProfileManager->GetSaveFiles( sortedGameSaves );
		Sort( sortedGameSaves.Begin(), sortedGameSaves.End(), SaveGame::ComparePredicate() );

		// Build UI
		IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Load saved game"), true, false );
		m_tree->AddRoot( menu );

		for ( Uint32 i = 0; i < sortedGameSaves.Size(); ++i )
		{
			new CSaveGameOption( menu, sortedGameSaves[ i ] );
		}

		return menu;
	}	  */

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
		IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Load world"), true, false );
		m_tree->AddRoot( menu );
		for ( Uint32 i = 0; i < worlds.Size(); i++ )
		{
			new CGameWorldOption( menu, worlds[ i ] );
		}

		return menu;
	}

	IDebugCheckBox* AddGameMenu()
	{
		// Find quests

		TDynArray< String > games;
		FindFiles( GDepot, TXT("redgame"), games );

		// Filter out wrong resources
		//for ( Int32 i = games.SizeInt() - 1; i >= 0; --i )
		//{
		//	TSoftHandle< CR6GameResource > res = games[ i ];
		//	if ( res.IsLoaded() )
		//	{
		//		continue;
		//	}

		//	if ( false == res.Load() )
		//	{
		//		games.RemoveAt( i );
		//		continue;
		//	}

		//	res.Release();
		//}

		Sort( games.Begin(), games.End() );

		// Build UI

		IDebugCheckBox* menu = new IDebugCheckBox( NULL, TXT("Start game"), true, false );
		m_tree->AddRoot( menu );
		for ( Uint32 i = 0; i < games.Size(); i++ )
		{
			new CGameWorldOption( menu, games[ i ] );
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

void CreateDebugPageGameStartup()
{
	IDebugPage* page = new CDebugPageGameStartup();
	IDebugPageManagerBase::GetInstance()->RegisterDebugPage( page );

	// Make it visible at startup
	IDebugPageManagerBase::GetInstance()->SelectDebugPage( page );
}

#endif