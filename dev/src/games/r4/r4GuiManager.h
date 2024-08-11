/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/flashRenderSceneProvider.h"
#include "../../common/engine/gestureListener.h"
#include "../../common/engine/videoPlayer.h"

#include "../../common/game/guiConfigResource.h"
#include "../../common/game/guiManager.h"
#include "../../common/game/guiScenePlayerListener.h"

#include "journalEvents.h"
#include "questUICondition.h"
#include "../../common/game/menu.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CR4Hud;
class CR4Menu;
class CGuiScenePlayer;
class IRenderVideo;

//////////////////////////////////////////////////////////////////////////
// CR4GuiManager
//////////////////////////////////////////////////////////////////////////
class CR4GuiManager	
	: public CGuiManager
	, public IW3JournalEventListener
	, public IGestureListener
	, public IGuiScenePlayerListener
{
	DECLARE_ENGINE_CLASS( CR4GuiManager, CGuiManager, CF_AlwaysTransient );

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

	struct SMenuBackgroundVideo
	{
		THandle< CR4Menu >	m_menu;
		String				m_videoFile;

		SMenuBackgroundVideo()
		{}

		SMenuBackgroundVideo( CR4Menu* menu, const String& videoFile )
			: m_menu( menu )
			, m_videoFile( videoFile )
		{}
	};

private:
	TDepotMap					m_cachedHudDepotMap;
	TDepotMap					m_cachedMenuDepotMap;
	TDepotMap					m_cachedPopupDepotMap;
	SGuiSceneDescription		m_cachedGuiSceneDescription;

private:
	CR4Hud*								m_hud;
	CR4Menu*							m_menu;
	TDynArray< THandle< CR4Popup > >	m_popups;

	TDynArray< CR4Menu* >				m_modalMenus;
	TDynArray< CR4Popup* >				m_modalPopups;
	TDynArray< CQuestUICondition* >		m_uiListeners;
	TDynArray< SMenuBackgroundVideo >	m_menuBackgroundVideos;
	CGuiScenePlayer*					m_guiScenePlayer;

private:
	enum EVideoEventType
	{
		eVideoEventType_Started,
		eVideoEventType_Stopped,
		eVideoEventType_Subtitles,
	};

	struct SVideoEvent
	{
		CName				m_videoClient;
		EVideoEventType		m_videoEventType;
		String				m_subtitles;

		SVideoEvent()
			: m_videoEventType( eVideoEventType_Started )
		{}

		SVideoEvent( CName videoClient, EVideoEventType videoEventType )
			: m_videoClient( videoClient )
			, m_videoEventType( videoEventType )
		{}

		SVideoEvent( CName videoClient, const String& subtitles )
			: m_videoClient( videoClient )
			, m_videoEventType( eVideoEventType_Subtitles )
			, m_subtitles( subtitles )
		{}
	};

	struct SVideoContext
	{
		CName						m_videoClient;
		IRenderVideo*				m_renderVideo;
		String						m_fileName;
		TDynArray< SVideoEvent >	m_videoEvents;

		SVideoContext()
			: m_renderVideo( nullptr )
		{}

		void			Update();
		void			CancelVideo();
		CName			GetVideoClient() const { return m_videoClient; }
		Bool			PlayVideo( CName videoClient, const SVideoParams& videoParms );
		const String&	GetFileName() const { return m_fileName; }
		void			AddVideoEvent( const SVideoEvent& event ) { m_videoEvents.PushBack( event ); }
		void			FlushVideoEvents( TDynArray< SVideoEvent >& outVideoEvents );
	};
	
private:
	SVideoContext				m_videoContext;	

private:
	//TBD: TODO better state management
	Bool						m_pausedGame;
	Bool						m_cursorShown;

private:
	SDebugSubtitle				m_debugSubtitle;
	Bool						m_showMenus;
	Bool						m_showInput;

public:
	CHud*						GetHud() const override;
	RED_INLINE void			ToggleMenus() { m_showMenus = !m_showMenus; }
	RED_INLINE void			ToggleInput() { m_showInput = !m_showInput; }

protected:
	virtual Bool				RequestMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData ) override;
	virtual Bool				CloseMenu( const CName& menuName ) override;
	virtual Bool				RequestPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData ) override;
	virtual Bool				ClosePopup( const CName& popupName ) override;

protected:
	virtual Bool				RegisterFlashHud( const String& hudName, const CFlashValue& flashHud ) override;
	virtual Bool				RegisterFlashMenu( const String& menuName, const CFlashValue& flashMenu ) override;
	virtual Bool				RegisterFlashPopup( const String& popupName, const CFlashValue& flashPopup ) override;

public:
	void						RegisterBackgroundVideo( CR4Menu* menu, const String& videoFile );
	void						UnregisterBackgroundVideo( CR4Menu* menu );

public:
								CR4GuiManager();
	virtual Bool				Initialize() override;
	virtual Bool				Deinitialize() override;

public:
	virtual void				OnGenerateDebugFragments( CRenderFrame* frame ) override;

public:
	virtual	void				Tick( float deltaTime ) override;

public:
	virtual Bool				NeedsInputCapture() const override;

public:
	//! CObject functions
	virtual void				OnSerialize( IFile& file ) override;

public:
	virtual void				OnGameStart( const CGameInfo& gameInfo ) override;
	virtual void				OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void				OnWorldStart( const CGameInfo& gameInfo ) override;
	virtual void				OnWorldEnd( const CGameInfo& gameInfo ) override;

public:
	virtual void				OnSwipe( EStandardSwipe swipe ) override;
	virtual void				OnPinch( Float value ) override;

public:
	//! IGuiScenePlayerListener functions
	virtual void				OnGuiSceneEntitySpawned( const THandle< CEntity >& spawnedEntity ) override;
	virtual void				OnGuiSceneEntityDestroyed() override;
//	virtual void				OnGuiSceneError() override;

public:
	virtual void				OnJournalEvent( const SW3JournalQuestStatusEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalObjectiveStatusEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalTrackEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalQuestTrackEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalQuestObjectiveTrackEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalQuestObjectiveCounterTrackEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalHighlightEvent& event ) override;

	virtual void				OnJournalEvent( const SW3JournalCharacterEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalCharacterDescriptionEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalGlossaryEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalGlossaryDescriptionEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalTutorialEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalCreatureEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalCreatureDescriptionEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalStoryBookPageEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalPlaceEvent& event );
	virtual void				OnJournalEvent( const SW3JournalPlaceDescriptionEvent& event );
	virtual void				OnJournalEvent( const SW3JournalHuntingQuestAddedEvent& event ) override;
	virtual void				OnJournalEvent( const SW3JournalHuntingQuestClueFoundEvent& event ) override;

public:
	virtual void				OnVideoStopped( CName videoClient ) override;
	virtual void				OnVideoStarted( CName videoClient ) override;
	virtual void				OnVideoSubtitles( CName videoClient, const String& subtitles ) override;

public:
	virtual void				OnEnteredConstrainedState();
	virtual void				OnExitedConstrainedState();

private:
	void						ProcessGameState();
	void						ProcessHudVisibility() const;
	void						ProcessBackgroundVideo();
	void						ProcessVideoEvents();
	void						ShutdownVideo();
	Bool						CanKeepHud() const;

private:
	void						PlayBackgroundVideoIfNeeded( const SMenuBackgroundVideo& video );
	void						StopBackgroundVideoIfNeeded();

private:
	void						PlayFlashbackVideoAsync( const String& videoFile, Bool looped );
	void						CancelFlashbackVideo();

private:
	CR4Hud*						CreateHud();

public:
	CR4Menu*					CreateMenu( const CName& menuName, CObject* parent );
	CR4Popup*					CreatePopup( const CName& popupName, CObject* parent );
	Bool						MakeModal( CR4Menu* menu, Bool make );
	Bool						MakeModal( CR4Popup* popup, Bool make );
	virtual Bool				IsAnyMenu() const override { return !!m_menu; }
	virtual CMenu*				GetRootMenu();
	virtual void				RefreshMenu() override;

	void						AttachUIListener( CQuestUICondition& listener );
	void						DetachUIListener( CQuestUICondition& listener );

public:
	void						OnClosingMenu( CR4Menu* menu );
	void						OnClosingPopup( CR4Popup* popup );
	void						OnOpenedMenuEvent( const CName& menuName );
	void						OnOpenedJournalEntryEvent( const THandle< CJournalBase >& entryHandle );
	void						OnSentCustomUIEvent( const CName& eventName );

private:
	void						UpdateInputFlags();
	void						ReleaseGuiScenePlayer();

public:
	virtual void				DialogHudShow() override;
	virtual void				DialogHudHide() override;
	virtual void				DialogSentenceSet( const String& htmlText, Bool alternativeUI ) override;
	virtual	void				DialogPreviousSentenceSet( const String& htmlText ) override;
	virtual void				DialogPreviousSentenceHide() override;
	virtual void				DialogSentenceHide() override;
	virtual void				DialogChoicesSet( const TDynArray< SSceneChoice >& choices, Bool alternativeUI ) override;
	virtual void				DialogChoiceTimeoutSet( Float timeOutPercent ) override;
	virtual void				DialogChoiceTimeoutHide() override;
	virtual void				DialogSkipConfirmShow() override;
	virtual void				DialogSkipConfirmHide() override;
	virtual Bool				IsDialogHudInitialized() const override;

	virtual void				SubtitleShow( ISceneActorInterface * actor, const String & text, Bool alternativeUI ) override;
	virtual void				SubtitleHide( ISceneActorInterface * actor ) override;

	virtual void				DebugTextShow( const String& text ) override;
	virtual void				DebugTextHide() override;

	virtual CInteractionComponent* GetActiveInteraction() const override;
	virtual void				SetActiveInteraction( CInteractionComponent * interaction, Bool force ) override;

	virtual void				ShowOneliner( const String& plainText, const CEntity* entity ) override;
	virtual void				HideOneliner( const CEntity* entity ) override;

private:
	void						funcIsAnyMenu( CScriptStackFrame& stack, void* result );
	void						funcGetRootMenu( CScriptStackFrame& stack, void* result );
	void						funcGetPopup( CScriptStackFrame& stack, void* result );
	void						funcGetPopupList( CScriptStackFrame& stack, void* result );
	void						funcSendCustomUIEvent( CScriptStackFrame& stack, void* result );
	void						funcPlayFlashbackVideoAsync( CScriptStackFrame& stack, void* result );
	void						funcCancelFlashbackVideo( CScriptStackFrame& stack, void* result );
	void						funcSetSceneEntityTemplate( CScriptStackFrame& stack, void* result );
	void						funcApplyAppearanceToSceneEntity( CScriptStackFrame& stack, void* result );
	void						funcUpdateSceneEntityItems( CScriptStackFrame& stack, void* result );
	void						funcSetSceneCamera( CScriptStackFrame& stack, void* result );
	void						funcSetupSceneCamera( CScriptStackFrame& stack, void* result );
	void						funcSetEntityTransform( CScriptStackFrame& stack, void* result );
	void						funcSetSceneEnvironmentAndSunPosition( CScriptStackFrame& stack, void* result );
	void						funcEnableScenePhysics( CScriptStackFrame& stack, void* result );
	void						funcSetBackgroundTexture( CScriptStackFrame& stack, void* result );
	void						funcRequestClearScene( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI_EX( CR4GuiManager, CF_AlwaysTransient );
	PARENT_CLASS( CGuiManager );
	NATIVE_FUNCTION( "IsAnyMenu", funcIsAnyMenu );
	NATIVE_FUNCTION( "GetRootMenu", funcGetRootMenu );
	NATIVE_FUNCTION( "GetPopup", funcGetPopup );
	NATIVE_FUNCTION( "GetPopupList", funcGetPopupList );
	NATIVE_FUNCTION( "SendCustomUIEvent", funcSendCustomUIEvent );
	NATIVE_FUNCTION( "PlayFlashbackVideoAsync", funcPlayFlashbackVideoAsync );
	NATIVE_FUNCTION( "CancelFlashbackVideo", funcCancelFlashbackVideo );
	NATIVE_FUNCTION( "SetSceneEntityTemplate", funcSetSceneEntityTemplate );
	NATIVE_FUNCTION( "ApplyAppearanceToSceneEntity", funcApplyAppearanceToSceneEntity );
	NATIVE_FUNCTION( "UpdateSceneEntityItems", funcUpdateSceneEntityItems );
	NATIVE_FUNCTION( "SetSceneCamera", funcSetSceneCamera );
	NATIVE_FUNCTION( "SetupSceneCamera", funcSetupSceneCamera );
	NATIVE_FUNCTION( "SetEntityTransform", funcSetEntityTransform );
	NATIVE_FUNCTION( "SetSceneEnvironmentAndSunPosition", funcSetSceneEnvironmentAndSunPosition );
	NATIVE_FUNCTION( "EnableScenePhysics", funcEnableScenePhysics );
	NATIVE_FUNCTION( "SetBackgroundTexture", funcSetBackgroundTexture );
	NATIVE_FUNCTION( "RequestClearScene", funcRequestClearScene );
END_CLASS_RTTI();
