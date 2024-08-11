/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gameDebugMenu.h"
#include "../../common/game/gameSaver.h"
#include "../../common/core/version.h"
#include "../../common/game/factsDB.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/depot.h"
#include "../../common/engine/gameTimeManager.h"


// Needed for SHGetSpecialFolderPath
#ifdef RED_PLATFORM_WINPC
	#include <shlobj.h>
#endif
#include "../../common/engine/gameSaveManager.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/engine/fonts.h"

/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary

CGatheredResource fntArial( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), 0 );

/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary

static void FindWorldLevels( CDirectory* dir, TDynArray< String >& worldFiles )
{
	// Recurse
	for ( CDirectory* child : dir->GetDirectories() )
	{
		FindWorldLevels( child, worldFiles );
	}

	// Enumerate files
	for ( CDiskFile* file : dir->GetFiles() )
	{
		if ( file->GetFileName().EndsWith( TXT("w2w") ) )
		{
			worldFiles.PushBack( file->GetDepotPath() );
		}		
	}
}

static Bool FindWorldLevelsChoices( TDynArray< SGameChapterChoices > &choices )
{
	String configFileContent;
	if ( !GFileManager->LoadFileToString( TXT("config/WorldChoices.ini"), configFileContent, true ) )
	{
		// cannot open file
		ASSERT( !TXT("Cannot open WorldChoices.ini file.") );
		return false;
	}
	
	TDynArray< String > configFileLines;
	configFileLines = configFileContent.Split( TXT("\r\n") );

	SGameChapterChoice gameChapterChoice;
	SGameChapterChoices gameChapterChoices;

	for ( Uint32 lineIdx = 0; lineIdx < configFileLines.Size(); )
	{
		// Read section header

		String line = configFileLines[ lineIdx ];

		line.Trim();
		if ( line.GetLength() < 3 ) continue;

		if ( !line.BeginsWith( TXT("[") ) || !line.EndsWith( TXT("]") ) )
		{
			ASSERT( !TXT("Bad file format. Expected line surrounded with [].") );
			return false;
		}

		String worldName = line.MidString( 1, line.GetLength() - 2 );
		gameChapterChoices.m_worldName = worldName;

		// Read section content

		++lineIdx;
		if ( lineIdx >= configFileLines.Size() ) break; // empty section

		line = configFileLines[ lineIdx ];
		while ( lineIdx < configFileLines.Size() )
		{
			// Parse question
			String question = line;
			question.Trim();
			gameChapterChoice.m_question = question;
			++lineIdx;
			if ( lineIdx >= configFileLines.Size() )
			{
				ASSERT( !TXT("Bad file format. Question without answers. ") );
				return false;
			}

			// Parse answers
			line = configFileLines[ lineIdx ];
			TDynArray< String > answers = line.Split( TXT(";") );
			if ( answers.Size() < 1 )
			{
				ASSERT( !TXT("Bad file format. Answer is empty") );
				return false;
			}
			for ( Uint32 answerIdx = 0; answerIdx < answers.Size(); ++answerIdx )
			{
				TDynArray< String > answerElems = answers[ answerIdx ].Split( TXT("=") );
				if ( answerElems.Size() != 2 )
				{
					ASSERT( !TXT("Bad file format. Answer format corrupted in '='") );
					return false;
				}
				String answerTxt = answerElems[0];
				String answerFactDbName = answerElems[1];
				answerTxt.Trim();
				answerFactDbName.Trim();

				gameChapterChoice.m_answers.PushBack( TPair<String,String>( answerTxt, answerFactDbName ) );
			}

			gameChapterChoices.m_choices.PushBack( gameChapterChoice );
			gameChapterChoice.m_answers.Clear();

			// Next line
			++lineIdx;
			if ( lineIdx >= configFileLines.Size() ) break;
			line = configFileLines[ lineIdx ];
			line.Trim();
			if ( line.BeginsWith( TXT("[") ) ) break;
		}

		choices.PushBack( gameChapterChoices );
		gameChapterChoices.m_choices.Clear();
	}

	return true;
}

/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary

enum EMenuMode
{
	MENU_MainMenu,
	MENU_LoadGame,
	MENU_LoadWorld,
	MENU_InitChoices,
	MENU_NewGameType,
	MENU_ImportSave
};

/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary
/// MEGA MESSY CODE - temporary

CGameDebugMenu::CGameDebugMenu( IViewport* viewport )
	: m_selectedMenuOption( 0 )
	, m_selectedSubMenuOption( 0 )
	, m_menuMode( 0 )
	, m_drawFont( fntArial.LoadAndGet< CFont >() )
	, m_viewport( viewport )
	, m_hasValidSaveGameInfo( false )
	, m_saveGamePreview( NULL )
	, m_currentChoicesIdx( 0 )
	, m_savesMenuIdx( 0 )
{
	FindRelatedFiles();
}

CGameDebugMenu::~CGameDebugMenu()
{
	fntArial.Release();
}

void CGameDebugMenu::FindRelatedFiles()
{
	// Clear crap
	m_worldToLoad.Clear();

	// Enumerate w2w files that can be loaded
	FindWorldLevels( GDepot, m_worldToLoad );

	m_worldLoadChoices.Clear();
	FindWorldLevelsChoices( m_worldLoadChoices );
}

void CGameDebugMenu::Show( Bool isVisible )
{
	if ( m_isVisible != isVisible )
	{
		m_isVisible = isVisible;

		if ( m_isVisible )
		{
			// Show menu
			m_selectedMenuOption = 0;
			m_selectedSubMenuOption = 0;
			m_menuMode = MENU_MainMenu;
			m_viewport->SetViewportHook( this );

			// Update file list
			FindRelatedFiles();
		}
		else
		{
			m_viewport->SetViewportHook( NULL );
		}			
	}
}

Bool CGameDebugMenu::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if ( m_menuMode == MENU_MainMenu && action == IACT_Press )
	{
		// Exit game
		if ( key == IK_Escape )
		{
			OnQuitGame();
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = m_selectedMenuOption - 1;
			if ( m_selectedMenuOption < 0 ) m_selectedMenuOption = 3;
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = m_selectedMenuOption + 1;
			if ( m_selectedMenuOption > 3 ) m_selectedMenuOption = 0;
			return true;
		}

		// Mode key 1
		if ( key == IK_1 )
		{
			m_selectedMenuOption = 0;
			key = IK_Enter;
		}

		// Mode key 2
		if ( key == IK_2 )
		{
			m_selectedMenuOption = 1;
			key = IK_Enter;
		}

		// Mode key 3
		if ( key == IK_3 )
		{
			m_selectedMenuOption = 2;
			key = IK_Enter;
		}

		// Mode key 4
		if ( key == IK_4 )
		{
			m_selectedMenuOption = 3;
			key = IK_Enter;
		}

		// Accept
		if ( key == IK_Enter )
		{
			if ( m_selectedMenuOption == 0 ) OnNewGameMenu();
			if ( m_selectedMenuOption == 1 ) OnLoadGameMenu();
			if ( m_selectedMenuOption == 2 ) OnLoadChapterMenu();
			if ( m_selectedMenuOption == 3 ) OnQuitGame();
			return true;
		}
	}

	if ( m_menuMode == MENU_LoadWorld && action == IACT_Press )
	{
		// Exit game
		if ( key == IK_Escape )
		{
			m_menuMode = MENU_MainMenu;
			m_selectedMenuOption = 2; // Load Chapter
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = m_selectedMenuOption - 1;
			if ( m_selectedMenuOption < 0 ) m_selectedMenuOption = (Int32)m_worldToLoad.Size() - 1;
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = m_selectedMenuOption + 1;
			if ( m_selectedMenuOption >= (Int32)m_worldToLoad.Size() ) m_selectedMenuOption = 0;
			return true;
		}

		if ( key == IK_Enter )
		{
			if ( m_selectedMenuOption >= 0 && m_selectedMenuOption < (Int32)m_worldToLoad.Size() )
			{
				const String& worldName = m_worldToLoad[ m_selectedMenuOption ];

				// If choices are available for chapter than ask for them
				for ( Uint32 i = 0; i < m_worldLoadChoices.Size(); ++i )
				{
					if ( m_worldLoadChoices[i].m_worldName.EqualsNC( worldName ) )
					{
						m_menuMode = MENU_InitChoices;
						m_worldLoadFactsToAdd.Clear();
						m_currentChoices = m_worldLoadChoices[i];
						m_currentChoicesIdx = 0;
						m_selectedMenuOption = 0;
						return true;
					}
				}

				OnLoadChapter( worldName );
			}
			return true;
		}
	}

	if ( m_menuMode == MENU_LoadGame && action == IACT_Press )
	{
		// Exit game
		if ( key == IK_Escape )
		{
			m_menuMode = MENU_MainMenu;
			m_selectedMenuOption = 1; // Load Game
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = m_selectedMenuOption - 1;
			if ( m_selectedMenuOption < 0 ) m_selectedMenuOption = (Int32)m_savesToLoad.Size() - 1;
			UpdateSaveGamePreview();
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = m_selectedMenuOption + 1;
			if ( m_selectedMenuOption >= (Int32)m_savesToLoad.Size() ) m_selectedMenuOption = 0;
			UpdateSaveGamePreview();
			return true;
		}

		if ( key == IK_Enter )
		{
			if ( m_selectedMenuOption >= 0 && m_selectedMenuOption < (Int32)m_savesToLoad.Size() )
			{
				OnLoadGame( m_savesToLoad[ m_selectedMenuOption ] );				
			}
			return true;
		}
	}

	if ( m_menuMode == MENU_InitChoices && action == IACT_Press )
	{
		SGameChapterChoice &chapterChoice = m_currentChoices.m_choices[ m_currentChoicesIdx ];

		// Exit game
		if ( key == IK_Escape )
		{
			m_menuMode = MENU_LoadWorld;
			m_selectedMenuOption = 0; // Load Game
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = m_selectedMenuOption - 1;
			if ( m_selectedMenuOption < 0 ) m_selectedMenuOption = (Int32)chapterChoice.m_answers.Size() - 1;
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = m_selectedMenuOption + 1;
			if ( m_selectedMenuOption >= (Int32)chapterChoice.m_answers.Size() ) m_selectedMenuOption = 0;
			return true;
		}

		if ( key == IK_Enter )
		{
			if ( m_selectedMenuOption >= 0 && m_selectedMenuOption < (Int32)chapterChoice.m_answers.Size() )
			{
				// Remember choice
				if ( chapterChoice.m_answers[m_selectedMenuOption].m_second != TXT("NONE") )
				{
					m_worldLoadFactsToAdd.PushBack( chapterChoice.m_answers[m_selectedMenuOption].m_second );
				}
				++m_currentChoicesIdx;
				if ( m_currentChoicesIdx >= m_currentChoices.m_choices.Size() )
				{
					OnLoadChapter( m_currentChoices.m_worldName );
				}
				else
				{
					// into another question
					m_selectedMenuOption = 0;
				}
			}
			return true;
		}
	}

	if ( m_menuMode == MENU_NewGameType && action == IACT_Press )
	{
		// Exit game
		if ( key == IK_Escape )
		{
			m_menuMode = MENU_MainMenu;
			m_selectedMenuOption = 0; 
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = Max( m_selectedMenuOption - 1, 0 );
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = Min( m_selectedMenuOption + 1, 1 );
			return true;
		}

		if ( key == IK_Enter )
		{
			if ( m_selectedMenuOption == 0 )
			{
				OnNewGame();
			}
			else if ( m_selectedMenuOption == 1 )
			{
				OnImportMenu();
			}
			return true;
		}
	}


	if ( m_menuMode == MENU_ImportSave && action == IACT_Press )
	{
		// Exit game
		if ( key == IK_Escape )
		{
			m_menuMode = MENU_NewGameType;
			m_selectedMenuOption = 0; 
			return true;
		}

		// Arrow up
		if ( key == IK_Up )
		{
			m_selectedMenuOption = Max( m_selectedMenuOption - 1, 0 );
			return true;
		}

		// Arrow down
		if ( key == IK_Down )
		{
			m_selectedMenuOption = Min( m_selectedMenuOption + 1, 2 );
			return true;
		}

		if ( key == IK_Enter )
		{
			return true;
		}
	}

	// Not handled
	return false;
}

void CGameDebugMenu::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{

	const Color titleColor( 255, 200, 200 );
	const Color itemColor( 140, 140, 140 );
	const Color itemSelColor( 255, 255, 200 );

	// Draw background
	const Uint32 width = frame->GetFrameOverlayInfo().m_width;
	const Uint32 height = frame->GetFrameOverlayInfo().m_height;

	const Bool showFlag = frame->GetFrameInfo().IsShowFlagOn( SHOW_VisualDebug ); 
	const_cast< CRenderFrameInfo* > ( &frame->GetFrameInfo() )->SetShowFlag( SHOW_VisualDebug, true );

	frame->AddDebugRect( 0, 0, width, height, Color::BLACK );

	// Main menu
	Uint32 y = 20;
	Uint32 m = 20;
	Bool isSelected;
	if ( m_menuMode == MENU_MainMenu )
	{
		frame->AddDebugScreenText( 20, y, TXT("Witcher 3 - Main Menu Placeholder"), titleColor, m_drawFont );
		y += 40;

		// New game
		isSelected = (m_selectedMenuOption == 0);
		frame->AddDebugScreenText( 20+m, y, TXT("1) New Game"), isSelected ? itemSelColor : itemColor, m_drawFont );
		y += 25;

		// Load game
		isSelected = (m_selectedMenuOption == 1);
		frame->AddDebugScreenText( 20+m, y, TXT("2) Load Game"), isSelected ? itemSelColor : itemColor, m_drawFont );
		y += 25;

		// Load world
		isSelected = (m_selectedMenuOption == 2);
		frame->AddDebugScreenText( 20+m, y, TXT("3) Load Chapter"), isSelected ? itemSelColor : itemColor, m_drawFont );
		y += 25;

		// Quit game
		isSelected = (m_selectedMenuOption == 3);
		frame->AddDebugScreenText( 20+m, y, TXT("4) Quit"), isSelected ? itemSelColor : itemColor, m_drawFont );
		y += 25;

		// Build info
		Char buildText[ 512 ];
		Red::System::SNPrintF( buildText, 512, TXT("BUILD %s ( %s )"), APP_VERSION_NUMBER, APP_DATE );
		frame->AddDebugScreenText( 20, frame->GetFrameOverlayInfo().m_height-20, buildText, Color::GRAY );
	}

	// Load save game
	if ( m_menuMode == MENU_LoadGame )
	{
		frame->AddDebugScreenText( 20, y, TXT("Load Game"), titleColor, m_drawFont );
		y += 40;

		// List of worlds
		if ( m_savesToLoad.Empty() )
		{
			// There's nothing to load, empty cook
			frame->AddDebugScreenText( 20+m, y, TXT("NO SAVE GAMES!"), Color::RED, m_drawFont );
		}
		else
		{
			// Draw save game info
			if ( m_hasValidSaveGameInfo )
			{
				Uint32 localY = y;
				Uint32 localX = 500;
				Uint32 startY = y;

				// Header
				frame->AddDebugScreenText( localX, localY, TXT("Save game info"), Color::WHITE, m_drawFont );
				localY += 30;

				// Draw screenshot
				if ( m_saveGamePreview )
				{
					Uint32 screenWidth = 1280 / 2;
					Uint32 screenHeight = 720 / 2;

					// Draw image with screenshot
					frame->AddDebugTexturedRect( localX, localY, screenWidth, screenHeight, m_saveGamePreview, Color::WHITE );
					localY += screenHeight + 20;
				}

				// Draw info
				frame->AddDebugScreenFormatedText( localX, localY, TXT("Game world: '%ls'"), m_saveGameWorld.AsChar() );
				localY += 18;
				frame->AddDebugScreenFormatedText( localX, localY, TXT("Game time: '%ls'"), m_saveGameTime.AsChar() );
				localY += 18;

				// Separator line
				frame->AddDebugFrame( localX-15, startY - 20, 1, localY - startY + 10, Color::WHITE );
			}

			// List save games
			const Uint32 maxTextLines = view->GetHeight() / 26;

			if ( m_selectedMenuOption < m_savesMenuIdx )
			{
				m_savesMenuIdx = m_selectedMenuOption;
			}
			else if ( m_selectedMenuOption >= m_savesMenuIdx + (Int32)maxTextLines )
			{
				m_savesMenuIdx = m_selectedMenuOption - (Int32)maxTextLines + 1;
			}

			Uint32 maxSavedIdx = m_savesMenuIdx + maxTextLines;
			if ( maxSavedIdx > m_savesToLoad.Size() ) maxSavedIdx = m_savesToLoad.Size();

			for ( Uint32 i=m_savesMenuIdx; i < maxSavedIdx; i++ )
			{
				isSelected = (m_selectedMenuOption == (Int32)i);
				frame->AddDebugScreenText( 20+m, y, m_savesToLoad[i].GetFileName().AsChar(), isSelected ? itemSelColor : itemColor, m_drawFont );
				y += 22;
			}
		}
	}

	// Load world menu
	if ( m_menuMode == MENU_LoadWorld )
	{
		frame->AddDebugScreenText( 20, y, TXT("Load World"), titleColor, m_drawFont );
		y += 40;

		// List of worlds
		if ( m_worldToLoad.Empty() )
		{
			// There's nothing to load, empty cook
			frame->AddDebugScreenText( 20+m, y, TXT("NO WORLDS TO LOAD!"), Color::RED, m_drawFont );
		}
		else
		{
			// List worlds
			for ( Uint32 i=0; i<m_worldToLoad.Size(); i++ )
			{
				isSelected = (m_selectedMenuOption == (Int32)i);
				frame->AddDebugScreenText( 20+m, y, m_worldToLoad[i].AsChar(), isSelected ? itemSelColor : itemColor, m_drawFont );
				y += 22;
			}
		}
	}

	// Facts choices
	if ( m_menuMode == MENU_InitChoices )
	{
		// Title
		frame->AddDebugScreenText( 20, y, String::Printf( TXT("Your choices for %s"), m_currentChoices.m_worldName.AsChar() ).AsChar(),
			titleColor, m_drawFont );
		y += 30;
		//frame->AddDebugScreenText( 20, y, m_currentChoices.m_worldName.AsChar(), titleColor, m_drawFont );
		//y += 40;

		SGameChapterChoice &chapterChoice = m_currentChoices.m_choices[ m_currentChoicesIdx ];

		// Question
		frame->AddDebugScreenText( 20, y, chapterChoice.m_question.AsChar(), itemColor, m_drawFont );
		y += 30;

		for ( Uint32 i = 0; i < chapterChoice.m_answers.Size(); ++i )
		{
			isSelected = (m_selectedMenuOption == (Int32)i);
			frame->AddDebugScreenText( 20+m, y, chapterChoice.m_answers[i].m_first.AsChar(), isSelected ? itemSelColor : itemColor, m_drawFont );
			y += 22;
		}
	}

	// W1 save import
	if ( m_menuMode == MENU_NewGameType )
	{
		// New character
		isSelected = (m_selectedMenuOption == 0);
		frame->AddDebugScreenText( 20+m, y, TXT("1) New Character"), isSelected ? itemSelColor : itemColor, m_drawFont );
		y += 25;

		// Import w1 save
		isSelected = (m_selectedMenuOption == 1);
		frame->AddDebugScreenText( 20+m, y, TXT("2) Import 'The Witcher' Save Game"), isSelected ? itemSelColor : itemColor, m_drawFont );
	}

	if ( m_menuMode == MENU_ImportSave )
	{
		for ( Int32 i = 0; i < 3; ++i )
		{
			isSelected = (m_selectedMenuOption == i);
			String saveTitle = String::Printf( TXT( "(%i) 00%i-EndingGame.TheWitcherSave" ), i+1, i+1 );
			frame->AddDebugScreenText( 20+m, y, saveTitle, isSelected ? itemSelColor : itemColor, m_drawFont );
			y += 25;
		}
	}

	const_cast< CRenderFrameInfo* > ( &frame->GetFrameInfo() )->SetShowFlag( SHOW_VisualDebug, showFlag );
}

void CGameDebugMenu::OnViewportTick( IViewport* view, Float timeDelta )
{
}

void CGameDebugMenu::OnNewGame()
{
	// Load new world
	Show( false );
	const Char* prologWorld = TXT("levels\\l01-keep\\l01-keep.w2w");
	SGameSessionManager::GetInstance().CreateSession( prologWorld );
}

void CGameDebugMenu::OnLoadGame( const SSavegameInfo& saveFile )
{
	// Load saved game
	Show( false );

	// Load game save
	GGame->GetGameSaver()->LoadGame( saveFile );
}

void CGameDebugMenu::OnLoadChapter( const String& worldFile )
{
	// Load new world
	Show( false );

	// Apply initial facts - hack
	if ( GGame )
	{
		GGame->ClearInitialFacts();

		Int32 time = GGame->GetTimeManager()->GetTime().GetSeconds();
		for ( TDynArray< String >::iterator factI = m_worldLoadFactsToAdd.Begin();
			factI != m_worldLoadFactsToAdd.End();
			++factI )
		{
			GGame->AddInitialFact( *factI );
		}

		// Add fake fact, so there will be no "Test chicken phase" in cook, but it will appear in editor
		GGame->AddInitialFact( TXT("CookGameInitialized") );
	}

	SGameSessionManager::GetInstance().CreateSession( worldFile );
}

void CGameDebugMenu::OnNewGameMenu()
{
	m_menuMode = MENU_NewGameType;
	m_selectedMenuOption = 0;
}

void CGameDebugMenu::OnImportMenu()
{
	m_menuMode = MENU_ImportSave;
	m_selectedMenuOption = 0;
}

void CGameDebugMenu::OnNewGameFromImport()
{

}

struct SaveGame
{
	String					m_saveFile;
	Red::System::DateTime	m_timeStamp;

	static Int32 Compare( const void* a, const void* b )
	{
		const SaveGame* saveA = ( const SaveGame* ) a;
		const SaveGame* saveB = ( const SaveGame* ) b;
		if ( saveA->m_timeStamp > saveB->m_timeStamp ) return -1;
		if ( saveA->m_timeStamp < saveB->m_timeStamp ) return 1;
		return 0;
	}
};

void CGameDebugMenu::OnLoadGameMenu()
{
	// Enumerate saves
	TDynArray< SSavegameInfo > m_savesToLoad;
	GUserProfileManager->GetSaveFiles( m_savesToLoad );

	// Sort crap
	Sort( m_savesToLoad.Begin(), m_savesToLoad.End(), SSavegameInfo::ComparePredicate() );

	// Show menu
	m_menuMode = MENU_LoadGame;
	m_selectedMenuOption = 0;

	// Update preview of first save game
	UpdateSaveGamePreview();
}

void CGameDebugMenu::OnLoadChapterMenu()
{
	m_menuMode = MENU_LoadWorld;
	m_selectedMenuOption = 0;
}

void CGameDebugMenu::OnQuitGame()
{
	GEngine->RequestExit();
}

#define SCREENSHOT_MAX_WIDTH		1280

void CGameDebugMenu::UpdateSaveGamePreview()
{
	// TODO
#if 0
	// Destroy current bitmap
	if ( m_saveGamePreview )
	{
		m_saveGamePreview->RemoveFromRootSet();
		m_saveGamePreview->Discard();
		m_saveGamePreview = NULL;
	}
	
	// Reset
	m_hasValidSaveGameInfo = false;
	m_saveGameTime = String::EMPTY;
	m_saveGameWorld = String::EMPTY;

	// Valid save ?
	if ( m_menuMode == MENU_LoadGame && ( m_selectedMenuOption >= 0 ) && ( m_selectedMenuOption < (Int32)m_savesToLoad.Size() ) )
	{
		// Get save name
		String saveName = m_savesToLoad[ m_selectedMenuOption ].GetFileName();

		// Get save info
		GameSaveInfo saveInfo;
		if ( GGame->GetGameSaver()->GetSaveGameInfo( saveName, saveInfo ) )
		{
			// Extract some save data
			m_saveGameWorld = saveInfo.m_worldFile.StringAfter( TXT("\\"), true );
			m_saveGameTime = saveInfo.m_gameTime.ToPreciseString();
			m_hasValidSaveGameInfo = true;

			// Get screenshot
			m_saveGamePreview = NULL; //LoadScreenshotBitmap( saveInfo.m_screenshot );
			if ( m_saveGamePreview )
			{
				m_saveGamePreview->AddToRootSet();
			}
		}
	}
#endif
}

CBitmapTexture* CGameDebugMenu::LoadScreenshotBitmap( const String& screenshotFile )
{
#if 0 
	// Open source file
	IFile* file = GFileManager->CreateFileReader( screenshotFile, FOF_AbsolutePath | FOF_Buffered );
	if ( !file )
	{
		return NULL;
	}

	// Read bitmap file header
	BITMAPFILEHEADER bmfh;
	file->Serialize( &bmfh, sizeof(bmfh) );

	// Not a bitmap...
	if ( bmfh.bfType != 19778 )
	{
		delete file;
		return NULL;
	}
	
	// Load image info
	BITMAPINFOHEADER bmih;
	file->Serialize( &bmih, sizeof(bmih) );

	// To big...
	if ( bmih.biWidth > SCREENSHOT_MAX_WIDTH )
	{
		delete file;
		return NULL;
	}

	// Create mip data
	CBitmapTexture::MipMap mipData( bmih.biWidth, bmih.biHeight, bmih.biWidth * 4 );

	// Seek to bitmap data
	file->Seek( bmfh.bfOffBits );

	// Load bitmap data
	for ( Int32 y=0; y<bmih.biHeight; y++ )
	{
		// Get write offset
		const Uint32 destY = (bmih.biHeight-1 ) - y;
		Uint8* writeData = ( Uint8* ) mipData.m_data.GetData() + ( destY * mipData.m_pitch );

		// Convert data
		if ( bmih.biBitCount == 24 )
		{
			// Load source data
			Uint8 srcData[ 3 * SCREENSHOT_MAX_WIDTH ];
			file->Serialize( srcData, 3 * bmih.biWidth );

			// Convert to dest data
			Uint8* readData = &srcData[0];
			for ( Int32 x=0; x<bmih.biWidth; x++, readData+=3, writeData+=4 )
			{
				writeData[0] = readData[2];
				writeData[1] = readData[1];
				writeData[2] = readData[0];
				writeData[3] = 255;
			}
		}
		else if ( bmih.biBitCount == 32 )
		{
			// Load source data
			Uint8 srcData[ 4 * SCREENSHOT_MAX_WIDTH ];
			file->Serialize( srcData, 4 * bmih.biWidth );

			// Convert to dest data
			Uint8* readData = &srcData[0];
			for ( Int32 x=0; x<bmih.biWidth; x++, readData+=4, writeData+=4 )
			{
				writeData[0] = readData[0];
				writeData[1] = readData[1];
				writeData[2] = readData[2];
				writeData[3] = 255;
			}
		}
	}

	// Close file
	delete file;

	// Create bitmap info
	CBitmapTexture::FactoryInfo info;
	info.m_format = TRF_TrueColor;
	info.m_width = bmih.biWidth;
	info.m_height = bmih.biHeight;
	info.m_keepArtistData = false;
	info.m_sourceArtistData = mipData;
	info.m_textureGroup = CNAME( TerrainMaskMap );
	info.m_parent = NULL;

	// Create bitmap
	return CBitmapTexture::Create( info );
#endif
	return NULL;
}
