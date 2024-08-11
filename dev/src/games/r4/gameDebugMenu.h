/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/gameSaveManager.h"

struct SGameChapterChoice
{
	String m_question;
	TDynArray< TPair< String, String > > m_answers;
};

struct SGameChapterChoices
{
	String m_worldName;
	TDynArray< SGameChapterChoice > m_choices;
};

/// Debug menu for game
class CGameDebugMenu : public IViewportHook
{
protected:
	Int32								m_selectedMenuOption;
	Int32								m_selectedSubMenuOption;
	Int32								m_menuMode;
	TDynArray< String >					m_worldToLoad;
	TDynArray< SSavegameInfo >			m_savesToLoad;
	CFont*								m_drawFont;
	Bool								m_isVisible;
	IViewport*							m_viewport;
	CBitmapTexture*						m_saveGamePreview;
	String								m_saveGameWorld;
	String								m_saveGameTime;
	Bool								m_hasValidSaveGameInfo;

	// Choices
	TDynArray< SGameChapterChoices >	m_worldLoadChoices;
	SGameChapterChoices					m_currentChoices;
	Uint32								m_currentChoicesIdx;
	TDynArray< String >					m_worldLoadFactsToAdd;

	// Saves menu
	Int32									m_savesMenuIdx;

public:
	CGameDebugMenu( IViewport* viewport );
	~CGameDebugMenu();

	//! Show menu
	void Show( Bool isVisible );

	//! Generalized input event
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! External viewport tick
	virtual void OnViewportTick( IViewport* view, Float timeDelta );

public:
	void OnNewGame();
	void OnNewGameMenu();
	void OnImportMenu();
	void OnNewGameFromImport();
	void OnLoadGame( const SSavegameInfo& saveFile );
	void OnLoadChapter( const String& worldFile );
	void OnLoadGameMenu();
	void OnLoadChapterMenu();
	void OnQuitGame();

protected:
	void FindRelatedFiles();
	void UpdateSaveGamePreview();
	CBitmapTexture* LoadScreenshotBitmap( const String& screenshotFile );
};
