/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/guiManager.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CR6Hud;
class CR6Menu;

//////////////////////////////////////////////////////////////////////////
// CR6GuiManager
//////////////////////////////////////////////////////////////////////////
class CR6GuiManager	: public CGuiManager
{
	DECLARE_ENGINE_CLASS( CR6GuiManager, CGuiManager, CF_AlwaysTransient );

private:
	typedef THashMap< CName, String > TDepotMap;

private:
	struct SDebugSubtitle
	{
		Bool	m_valid;
		THandle< CEntity > m_actor;
		String	m_text;

	public:
		SDebugSubtitle() : m_valid( false ) {}
	};

private:
	TDepotMap					m_cachedHudDepotMap;
	TDepotMap					m_cachedMenuDepotMap;

private:
	CR6Hud*						m_hud;
	CR6Menu*					m_menu;

private:
	//TBD: TODO better state management
	Bool						m_pausedGame;
	Bool						m_cursorShown;

private:
	SDebugSubtitle				m_debugSubtitle;

public:
	CHud*						GetHud() const override;

protected:
	virtual Bool				RequestMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData ) override;
	virtual Bool				CloseMenu( const CName& menuName ) override;
	virtual Bool				RequestPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData ) override;
	virtual Bool				ClosePopup( const CName& popuName ) override;

protected:
	virtual Bool				RegisterFlashHud( const String& hudName, const CFlashValue& flashHud ) override;
	virtual Bool				RegisterFlashMenu( const String& menuName, const CFlashValue& flashMenu ) override;
	virtual Bool				RegisterFlashPopup( const String& popupName, const CFlashValue& flashPopup ) override;

public:
								CR6GuiManager();
	virtual Bool				Initialize() override;

public:
	virtual void				OnGenerateDebugFragments( CRenderFrame* frame ) override;

public:
	virtual	void				Tick( Float deltaTime ) override;

public:
	virtual Bool				NeedsInputCapture() const override;

public:
	//! CObject functions
	virtual void				OnFinalize() override;
	virtual void				OnSerialize( IFile& file ) override;

public:
	virtual void				OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void				OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void				OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void				OnWorldEnd( const CGameInfo& gameInfo ) override;

private:
	void						ProcessGameState();
	void						ProcessHudVisibility() const;

private:
	CR6Hud*						CreateHud();
	CR6Menu*					CreateMenu( const CName& menuName );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI_EX( CR6GuiManager, CF_AlwaysTransient );
	PARENT_CLASS( CGuiManager );
END_CLASS_RTTI();

