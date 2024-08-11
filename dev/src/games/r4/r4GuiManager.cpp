/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "../../common/engine/flashPlayer.h"
#include "../../common/game/commonGameResource.h"
#include "../../common/game/hudResource.h"
#include "../../common/game/menuResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/core/profiler.h"
#include "../../common/game/guiScenePlayer.h"

#include "r4Hud.h"
#include "r4Menu.h"
#include "r4Popup.h"
#include "r4GuiManager.h"
#include "r4JournalManager.h"
#include "journalEvents.h"
#include "../../common/engine/guiGlobals.h"
#include "menuEvents.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderer.h"
#include "../../common/core/contentManager.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CR4GuiManager );

//////////////////////////////////////////////////////////////////////////

// NOTE: Will actually be "{r4data}\\movies\\[path]\\" after it reaches the videoplayer
const String MENU_BACKGROUND_VIDEO_PATH(TXT("gui\\menus\\"));
const String FLASHBACKS_VIDEO_PATH(TXT("cutscenes\\"));

RED_DEFINE_STATIC_NAME( OnSwipe )
RED_DEFINE_STATIC_NAME( OnPinch )
RED_DEFINE_STATIC_NAME( OnDialogHudShow )
RED_DEFINE_STATIC_NAME( OnDialogHudHide )
RED_DEFINE_STATIC_NAME( OnDialogSentenceSet )
RED_DEFINE_STATIC_NAME( OnDialogPreviousSentenceSet )
RED_DEFINE_STATIC_NAME( OnDialogPreviousSentenceHide )
RED_DEFINE_STATIC_NAME( OnDialogSentenceHide )
RED_DEFINE_STATIC_NAME( OnDialogChoicesSet )
RED_DEFINE_STATIC_NAME( OnDialogChoiceTimeoutSet )
RED_DEFINE_STATIC_NAME( OnDialogChoiceTimeoutHide )
RED_DEFINE_STATIC_NAME( OnDialogSkipConfirmShow )
RED_DEFINE_STATIC_NAME( OnDialogSkipConfirmHide )
RED_DEFINE_STATIC_NAME( MenuTimeScale )
RED_DEFINE_STATIC_NAME( OnMenuEvent );
RED_DEFINE_STATIC_NAME( OnGuiSceneEntitySpawned );
RED_DEFINE_STATIC_NAME( OnGuiSceneEntityDestroyed );

RED_DEFINE_STATIC_NAME( OnVideoStopped );
RED_DEFINE_STATIC_NAME( OnVideoStarted );
RED_DEFINE_STATIC_NAME( OnVideoSubtitles );

RED_DEFINE_STATIC_NAME( OnChangedConstrainedState )

RED_DEFINE_STATIC_NAME( VideoClient_MenuBackground );
RED_DEFINE_STATIC_NAME( VideoClient_Flashback );

//////////////////////////////////////////////////////////////////////////
// Gathered Resources
//////////////////////////////////////////////////////////////////////////
CGatheredResource resDefaultGuiConfigR4( TXT("gameplay\\gui_new\\guirsrc\\r4default.guiconfig"), RGF_Startup );

//////////////////////////////////////////////////////////////////////////
// CR4GuiManager::SVideoContext
//////////////////////////////////////////////////////////////////////////
void CR4GuiManager::SVideoContext::Update()
{
	// NOTE: there could be a race if there was a pending cancelation and we think the video is still playing
	// So we'll miss it this tick, but shouldn't ever really happen in any practical case
	if ( m_renderVideo && !m_renderVideo->IsValid() )
	{
		m_videoEvents.PushBack( SVideoEvent( m_videoClient, eVideoEventType_Stopped ) );
		m_renderVideo->Release();
		m_renderVideo = nullptr;
		m_videoClient = CName::NONE;
		m_fileName.Clear();
	}

	if ( m_renderVideo )
	{
		String subtitles;
		if ( m_renderVideo->FlushSubtitles( subtitles ) )
		{
			m_videoEvents.PushBack( SVideoEvent( m_videoClient, subtitles ) );
		}
	}
}

void CR4GuiManager::SVideoContext::CancelVideo()
{
	if ( m_renderVideo )
	{
		m_videoEvents.PushBack( SVideoEvent( m_videoClient, eVideoEventType_Stopped ) );
		m_renderVideo->Cancel();
		m_renderVideo->Release();
		m_renderVideo = nullptr;
	}

	m_videoClient = CName::NONE;
	m_fileName.Clear();
}

Bool CR4GuiManager::SVideoContext::PlayVideo( CName videoClient, const SVideoParams& videoParms )
{
	CancelVideo();

	CVideoPlayer* videoPlayer = GGame ? GGame->GetVideoPlayer() : nullptr;
	if ( ! videoPlayer )
	{
		GUI_ERROR(TXT("No video player support available!"));
		return false;
	}

	m_renderVideo = videoPlayer->CreateVideo( videoClient, videoParms );
	if ( ! m_renderVideo )
	{
		GUI_ERROR(TXT("Failed to create videoClient %ls for file '%ls'"), videoClient.AsChar(), videoParms.m_fileName.AsChar() );
		return false;
	}

	m_videoClient = videoClient;
	m_fileName = videoParms.m_fileName;

	m_videoEvents.PushBack( SVideoEvent( videoClient, eVideoEventType_Started ) );

	return true;
}

void CR4GuiManager::SVideoContext::FlushVideoEvents( TDynArray< SVideoEvent >& outVideoEvents )
{
	outVideoEvents = Move( m_videoEvents );
}

//////////////////////////////////////////////////////////////////////////
// CR4GuiManager
//////////////////////////////////////////////////////////////////////////

CR4GuiManager::CR4GuiManager()
	: m_hud( nullptr )
	, m_menu( nullptr )
	, m_guiScenePlayer( nullptr )
	, m_cursorShown( false )
	, m_showMenus( false )
	, m_showInput( false )
{
	// Push back sentry with blank video to clear
	m_menuBackgroundVideos.PushBack( SMenuBackgroundVideo() );
}

Bool CR4GuiManager::Initialize()
{
	if ( !TBaseClass::Initialize() )
	{
		return false;
	}

	m_cachedHudDepotMap.ClearFast();
	m_cachedMenuDepotMap.ClearFast();
	m_cachedPopupDepotMap.ClearFast();

	// Hardcoded for now, as need to put in the gamedef

	CGuiConfigResource* guiConfigResource = resDefaultGuiConfigR4.LoadAndGet< CGuiConfigResource >();
	ASSERT( guiConfigResource );
	if ( ! guiConfigResource )
	{
		return false;
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
	const TDynArray< SPopupDescription >& popupDescList = guiConfigResource->GetPopupDescriptions();

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

	for ( TDynArray< SPopupDescription >::const_iterator it = popupDescList.Begin(); it != popupDescList.End(); ++it )
	{
		const SPopupDescription& popupDesc = *it;
		const CName& popupName = popupDesc.m_popupName;
		const String& resourceDepotPath = popupDesc.m_popupResource.GetPath();

		if ( ! popupName )
		{
			GUI_WARN( TXT("Menu name is not defined in guiconfig. Skipping.") );
			continue;
		}
		if ( resourceDepotPath.Empty() )
		{
			GUI_WARN( TXT("Menu '%ls' from guiconfig doesn't have a resource file. Skipping."), popupName.AsString().AsChar() );
			continue;
		}

		if ( ! m_cachedPopupDepotMap.Insert( popupName, resourceDepotPath ) )
		{
			GUI_WARN( TXT("Popup '%ls' with resource path '%ls' already defined. Skipping."), popupName.AsString().AsChar(), resourceDepotPath.AsChar() );
		}
	}

	m_cachedGuiSceneDescription = guiConfigResource->GetGuiSceneDescription();

	return true;
}

Bool CR4GuiManager::Deinitialize()
{
	//TODO: move into game state manager
	if ( m_cursorShown && GGame )
	{
		IViewport* vp = GGame->GetViewport();
		if ( vp )
		{
			//vp->CaptureInput( ICM_Full );
		}
		m_cursorShown = false;
	}

	ReleaseGuiScenePlayer();

	if ( m_hud )
	{
		m_hud->Discard();
		m_hud = nullptr;
	}

	if ( m_menu )
	{
		m_menu->OnClosing();
		m_menu->Discard();
		m_menu = nullptr;
	}

	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		m_popups[ i ]->OnClosing();
		m_popups[ i ]->Discard();
		m_popups[ i ] = nullptr;
	}
	m_popups.Clear();

	m_cachedHudDepotMap.Clear();
	m_cachedMenuDepotMap.Clear();
	m_cachedPopupDepotMap.Clear();

	ShutdownVideo();

	TBaseClass::Deinitialize();

	return true;
}

void CR4GuiManager::ReleaseGuiScenePlayer()
{
	ClearRenderSceneProvider();
	if ( m_guiScenePlayer )
	{
		VERIFY( m_guiScenePlayer->UnregisterListener( this ) );
		m_guiScenePlayer->Uninit();
		m_guiScenePlayer->Discard();
		m_guiScenePlayer = nullptr;
	}
}

void CR4GuiManager::OnSerialize( IFile& file )
{
	if ( file.IsGarbageCollector() )
	{
		if ( m_hud )
		{
			file << m_hud;
		}

		CMenu* menu = m_menu;
		while ( menu )
		{
			file << menu;
			menu = Cast< CMenu >( menu->GetParentGuiObject() );
		}

		for ( Uint32 i = 0; i < m_popups.Size(); ++i )
		{
			if ( m_popups[ i ] )
			{
				file << m_popups[ i ];
			}
		}

		file << m_guiScenePlayer;
	}

	TBaseClass::OnSerialize( file );
}

void CR4GuiManager::OnGameStart( const CGameInfo& gameInfo )
{
	TBaseClass::OnGameStart( gameInfo );
	RED_ASSERT( GGame && GGame->GetInputManager() && GGame->GetInputManager()->GetGestureSystem() );
	GGame->GetInputManager()->GetGestureSystem()->RegisterListener( this );

	// Do here instead of in Initialize() because otherwise the renderer might not be ready yet and a whole bunch of stuff will crash in the world
	//const SGuiSceneDescription& guiSceneDesc = guiConfigResource->GetGuiSceneDescription();
	// FIXME: Cleanup in OnGameEnd() and OnFinalize()
	if ( m_cachedGuiSceneDescription.m_enabled )
	{
		CClass* worldClass = SRTTI::GetInstance().FindClass( m_cachedGuiSceneDescription.m_worldClass );
		m_guiScenePlayer = ::CreateObject< CGuiScenePlayer >( this, OF_Transient );
		if ( m_guiScenePlayer->Init( worldClass ) )
		{
			SetRenderSceneProvider( m_guiScenePlayer );
			VERIFY( m_guiScenePlayer->RegisterListener( this ) );

			// DEBUG!!! DO NOT REALLY DO IT THIS WAY IN THE FINAL GAME!
			// Since it might be too much at this stage to create a C++ CR4InventoryMenu
			// maybe have the CR4GuiManager check the class of the created menu and then spawn/clone
			// the player then.
			//TSoftHandle< CEntityTemplate > resGeralt( TXT("characters\\player_entities\\geralt\\geralt_player.w2ent") );
			//TSoftHandle< CEntityTemplate > resGeralt( TXT("environment\\decorations\\containers\\chests\\detailed_chest\\detailed_chest.w2ent" ) );
			//TSoftHandle< CEntityTemplate > resGeralt( TXT("characters\\player_entities\\ciri\\ciri_player.w2ent") );
			//TSoftHandle< CEntityTemplate > resGeralt( TXT("characters\\npc_entities\\secondary_npc\\filippa_eilhart.w2ent") );

			//m_guiScenePlayer->SetEntityTemplate( resGeralt.Get() );
		}
		else
		{
			m_guiScenePlayer->Discard();
			m_guiScenePlayer = nullptr;
			//GUI_WARN( TXT("Cannot create a world with class '%ls'"), guiSceneDesc.m_worldClass.AsString().AsChar() );
		}
	}

	ShutdownVideo();

	CallEvent( CNAME( OnGameStart ), gameInfo.IsNewGame() || gameInfo.IsSavedGame() );
}

void CR4GuiManager::OnGameEnd( const CGameInfo& gameInfo )
{
	CallEvent( CNAME( OnGameEnd ) );

	RED_ASSERT( GGame && GGame->GetInputManager() && GGame->GetInputManager()->GetGestureSystem() );
	GGame->GetInputManager()->GetGestureSystem()->UnregisterListener( this );

	ReleaseGuiScenePlayer();

	ShutdownVideo();

	TBaseClass::OnGameEnd( gameInfo );
}

void CR4GuiManager::OnWorldStart( const CGameInfo& gameInfo )
{
	TBaseClass::OnWorldStart( gameInfo );

	m_hud = CreateHud();
	ASSERT( m_hud );
	if ( m_hud )
	{
		VERIFY( m_hud->Init( this ) );
	}

	CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( journalMgr )
	{
		journalMgr->RegisterEventListener( this );

		// initilize from journal manager - it's state is already loaded
		if ( gameInfo.IsSavedGame() )
		{
			journalMgr->OnInitGuiDuringGameLoad();	
		}
		else if ( gameInfo.IsChangingWorld() )
		{
			journalMgr->OnInitGuiDuringWorldChange();
		}
	}

	CallEvent( CNAME( OnWorldStart ), gameInfo.IsNewGame() || gameInfo.IsSavedGame() );
}

void CR4GuiManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	CallEvent( CNAME( OnWorldEnd ) );

	TBaseClass::OnWorldEnd( gameInfo );

	m_debugSubtitle.m_valid = false;

	if ( m_hud )
	{
		m_hud->Discard();
		m_hud = nullptr;
	}

	if ( m_menu )
	{
		m_menu->OnClosing();
		m_menu->Discard();
		m_menu = nullptr;
	}

	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		m_popups[ i ]->OnClosing();
		m_popups[ i ]->Discard();
		m_popups[ i ] = nullptr;
	}
	m_popups.Clear();

	CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();
	if ( journalMgr )
	{
		journalMgr->UnregisterEventListener( this );
	}
}

void CR4GuiManager::OnSwipe( EStandardSwipe swipe )
{
	CallEvent( CNAME( OnSwipe ), ( Uint32 )swipe );

	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnSwipe ), ( Uint32 )swipe );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

void CR4GuiManager::OnPinch( Float value )
{
	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnPinch ), value );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

CMenu* CR4GuiManager::GetRootMenu()
{
	return Cast<CMenu>(m_menu);
}

void CR4GuiManager::RefreshMenu()
{
	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnRefresh ) );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

void CR4GuiManager::OnGuiSceneEntitySpawned( const THandle< CEntity >& spawnedEntity )
{
	CEntity* entity = spawnedEntity.Get();
	if ( ! entity )
	{
		return;
	}

	// TBD: Force update xform
	entity->ForceUpdateTransformNodeAndCommitChanges();

	/*
	// SHITHACK: Hello??? streaming...??? I really don't like this since it will freeze the UI.
	////////////////////////////////////////////////////////////////////////// 
	//////////////////////////////////////////////////////////////////////////
	// Not available in non-editor...
	//	entity->ForceFinishAsyncResourceLoads();

	// Position the camera to look at the entity. Will for sure want to do this better.

	// const Box bbox = entity->CalcBoundingBox();
	// FIXME: Geralt's bbox is messed up for whatever reason, so just using values from Ciri for the moment... should probably let designers choose the camera
	// angle and position instead of autoframing anyway
	const Box bbox( Vector(-0.839446843f, -0.743546844f, -0.332557201f, 1.00000000f), Vector(0.839442253f, 0.777346849f, 1.90112615f, 1.00000000f) );

	const Float bsize = bbox.CalcExtents().Mag3();

	EulerAngles cameraRotation( 0.f, 0.f, -180.f );

	// Initialize to ZEROS because Vector by default is junk and then you'll get fucked up W projection values!
	// Not Point3D just so W not affected later here (could just reset to 1 though)
	Vector forward( Vector::ZEROS );
	Vector right( Vector::ZEROS );
	Vector up( Vector::ZEROS );

	cameraRotation.ToAngleVectors( &forward, &right, &up );

	const Vector center = bbox.CalcCenter();
	Vector cameraPosition = center - forward * bsize * 1.f;

	m_guiScenePlayer->UpdateCamera( cameraPosition, cameraRotation );
	*/

	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnGuiSceneEntitySpawned ), THandle<CEntity>(entity) );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

void CR4GuiManager::OnGuiSceneEntityDestroyed()
{
	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnGuiSceneEntityDestroyed ) );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

// void CR4GuiManager::OnGuiSceneError()
// {
// }

void CR4GuiManager::OnJournalEvent( const SW3JournalQuestStatusEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( m_hud )
	{
		if ( event.m_quest == journalMgr->GetTrackedQuest() )
		{
			m_hud->OnTrackedQuestUpdated( event.m_quest );
		}
		if ( !event.m_silent )
		{
			m_hud->OnQuestEvent( event.m_quest );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalObjectiveStatusEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	const CJournalQuest* quest = event.m_objective->GetParentQuest();

	if ( m_hud )
	{
		if ( quest == journalMgr->GetTrackedQuest() )
		{
			m_hud->OnTrackedQuestObjectivesUpdated( event.m_objective );
		}
		if ( !event.m_silent )
		{
			m_hud->OnQuestObjectiveEvent( quest, event.m_objective );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalTrackEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnQuestTrackingStarted( event.m_quest );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalQuestTrackEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnTrackedQuestUpdated( event.m_quest );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalQuestObjectiveTrackEvent& event )
{
	// FIXME: Journal manager really should keep a backlink to the quest parent
	// And how about a "is tracked" or something useful?

	CObject* ancestor = event.m_objective->GetParent();
	while ( ancestor && ! ancestor->IsA< CJournalQuest >() )
	{
		ancestor = ancestor->GetParent();
	}

	ASSERT( ancestor && ancestor->IsA< CJournalQuest >() );
	CJournalQuest* quest = Cast< CJournalQuest >( ancestor );

	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnTrackedQuestObjectivesUpdated( event.m_objective );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalQuestObjectiveCounterTrackEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnTrackedQuestObjectiveCounterUpdated( event.m_questObjective );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalHighlightEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnTrackedQuestObjectiveHighlighted( event.m_objective, event.m_objectiveIndex );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalCharacterEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnCharacterEvent(  event.m_character );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalCharacterDescriptionEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnCharacterDescriptionEvent(  event.m_characterDescription );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalGlossaryEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnGlossaryEvent(  event.m_glossary );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalGlossaryDescriptionEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnGlossaryDescriptionEvent(  event.m_glossaryDescription );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalTutorialEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnTutorialEvent(  event.m_tutorial );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalCreatureEvent& event )
{
	if ( m_hud )
	{
		if ( !event.m_silent )
		{
			m_hud->OnCreatureEvent( event.m_creature );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalCreatureDescriptionEvent& event )
{
	if ( m_hud )
	{
		if ( !event.m_silent )
		{
			m_hud->OnCreatureDescriptionEvent( event.m_creatureDescription );
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalStoryBookPageEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnStoryBookPageEvent( event.m_storyBookPage );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalPlaceEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnPlaceEvent( event.m_place );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalPlaceDescriptionEvent& event )
{
	if ( m_hud )
	{
		m_hud->OnPlaceDescriptionEvent( event.m_placeDescription );
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalHuntingQuestAddedEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnHuntingQuestAdded();
		}
	}
}

void CR4GuiManager::OnJournalEvent( const SW3JournalHuntingQuestClueFoundEvent& event )
{
	const CWitcherJournalManager* journalMgr = GCommonGame->GetSystem< CWitcherJournalManager >();

	if ( event.m_quest == journalMgr->GetTrackedQuest() )
	{
		if ( m_hud )
		{
			m_hud->OnHuntingQuestClueFound();
		}
	}
}

CHud* CR4GuiManager::GetHud() const
{ 
	return m_hud;
}

Bool CR4GuiManager::RequestMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData )
{
	//TBD: if already open... transition properly
	if ( m_menu )
	{
		m_menu->OnClosing();
		m_menu->Discard();
		m_menu = nullptr;
	}

	m_menu = CreateMenu( menuName, this );
	ASSERT( m_menu );

	if ( m_menu )
	{
		//TBD: menu parents
		VERIFY( m_menu->Init( menuName, this ) );
		m_menu->SetMenuScriptData( scriptInitData );
		m_menu->ApplyParams( true );

		OnOpenedMenuEvent( menuName );
	}
	else
	{
		return false;
	}
	
	return true;
}

Bool CR4GuiManager::CloseMenu( const CName& menuName )
{
	if ( ! m_menu )
	{
		// No menu found
		return false;
	}

	if ( m_menu->GetMenuName() == menuName )
	{
		// Close root menu and all its children
		m_menu->OnClosing();
		m_menu->Discard();
		m_menu = nullptr;
		return true;
	}

	// Look for specific child menu, close it and all its children
	CR4Menu* parentMenu = nullptr;
	CR4Menu* menu = m_menu;
	while ( menu )
	{
		if ( menu->GetMenuName() == menuName )
		{
			menu->OnClosing();
			menu->Discard();
			if ( parentMenu )
			{
				parentMenu->SetParentGuiObject( nullptr );
			}
			return true;
		}
		parentMenu = menu;
		menu = Cast< CR4Menu >( menu->GetParentGuiObject() );
	}

	return false;
}

Bool CR4GuiManager::RequestPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData )
{
	//TBD: if already open... transition properly

	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		if ( m_popups[ i ]->GetPopupName() == popupName )
		{
			//already found popup with this name, delete it
			m_popups[ i ]->OnClosing();
			m_popups[ i ]->Discard();
			m_popups.RemoveAt( i );
			break;
		}
	}

	CR4Popup* popup = CreatePopup( popupName, this );
	ASSERT( popup );

	if ( popup )
	{
		//TBD: menu parents
		VERIFY( popup->Init( popupName, this ) );
		popup->SetPopupScriptData( scriptInitData );
		popup->ApplyParams( true );

		OnOpenedMenuEvent( popupName );

		m_popups.PushBack( popup );
	}

	return true;
}

Bool CR4GuiManager::ClosePopup( const CName& popupName )
{
	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		if ( m_popups[ i ]->GetPopupName() == popupName )
		{
			// Close root menu and all its children
			m_popups[ i ]->OnClosing();
			m_popups[ i ]->Discard();
			m_popups.RemoveAt( i );
			return true;
		}
	}

	return false;
}

Bool CR4GuiManager::RegisterFlashHud( const String& hudName, const CFlashValue& flashHud )
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

Bool CR4GuiManager::RegisterFlashMenu( const String& menuName, const CFlashValue& flashMenu )
{
	//TBD: Have loading token to make sure the correct menu is being used

	if ( ! m_menu )
	{
		GUI_ERROR( TXT("No menu expected") );
		return false;
	}

	// Look for menu
	CMenu* menu = m_menu;
	while ( menu )
	{
		if ( menu->GetMenuName().AsString() == menuName )
		{
			if ( menu->InitWithFlashSprite( flashMenu ) )
			{
				return true;
			}
			break;
		}
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}

	GUI_ERROR( TXT("Failed to register menu '%ls'."), menuName.AsChar() );
	return false;
}

Bool CR4GuiManager::RegisterFlashPopup( const String& popupName, const CFlashValue& flashPopup )
{
	//TBD: Have loading token to make sure the correct menu is being used

	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		if ( m_popups[ i ]->GetPopupName().AsString() == popupName )
		{
			if ( m_popups[ i ]->InitWithFlashSprite( flashPopup ) )
			{
				return true;
			}
		}
	}

	GUI_ERROR( TXT("Failed to register popup '%ls'."), popupName.AsChar() );
	return false;
}

CR4Hud* CR4GuiManager::CreateHud()
{
	ASSERT( ! m_hud, TXT("HUD already exists") );

	String hudDepotPath;
	
	if ( ! m_cachedHudDepotMap.Find( CNAME(DefaultHud), hudDepotPath ) )
	{
		GUI_ERROR( TXT("Default HUD not found") );
		return nullptr;
	}

	THandle< CHudResource > hudResource = LoadResource< CHudResource >( hudDepotPath );
	ASSERT( hudResource );
	if ( !hudResource )
	{
		GUI_ERROR( TXT("HUD resource '%ls' from gamedef not loaded."), hudDepotPath.AsChar() );
		return nullptr;
	}

	CR4Hud* hud = SafeCast< CR4Hud >( CHud::CreateObjectFromResource( hudResource, this ) );
	ASSERT( hud );
	if ( ! hud )
	{
		GUI_ERROR( TXT("Failed to create HUD from resource") );
		return nullptr;
	}

	return hud;
}

void CR4GuiManager::RegisterBackgroundVideo( CR4Menu* menu, const String& videoFile )
{
	const Int32 MIN_USER_VIDEO_INDEX = 1; // As an additional guard just in case
	for ( Int32 j = m_menuBackgroundVideos.SizeInt()-1; j >= MIN_USER_VIDEO_INDEX; --j )
	{
		if ( m_menuBackgroundVideos[ j ].m_menu == menu )
		{
			m_menuBackgroundVideos.RemoveAt( j );
		}
	}

	m_menuBackgroundVideos.PushBack( SMenuBackgroundVideo( menu, videoFile ) );
}

void CR4GuiManager::UnregisterBackgroundVideo( CR4Menu* menu )
{
	const Int32 MIN_USER_VIDEO_INDEX = 1; // As an additional guard just in case
	for ( Int32 j = m_menuBackgroundVideos.SizeInt()-1; j >= MIN_USER_VIDEO_INDEX; --j )
	{
		if ( m_menuBackgroundVideos[ j ].m_menu == menu )
		{
			m_menuBackgroundVideos.RemoveAt( j );
		}
	}
}

Bool CR4GuiManager::NeedsInputCapture() const 
{
	//TODO: 
	return m_menu != nullptr;
}

void CR4GuiManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
#ifndef RED_FINAL_BUILD

	if ( m_debugSubtitle.m_valid )
	{
		CEntity* actor = m_debugSubtitle.m_actor.Get();
		String actorName = actor ? actor->GetDisplayName() : TXT("<actor?>");
		const String displayText = String::Printf( TXT("%s: %s"), actorName.AsChar(), m_debugSubtitle.m_text.AsChar() );
		frame->AddDebugScreenText( 20, frame->GetFrameOverlayInfo().m_height - 100, displayText, Color::WHITE );
	}

	if ( m_showMenus )
	{
		Color textColor = Color::WHITE;
		Uint32 i = 0;
		String line;

		CR4Menu* menu = m_menu;
		if ( menu )
		{
			while ( menu )
			{
				Uint32 layer = 0;
				if ( menu->GetMenuResource() )
				{
					layer = menu->GetMenuResource()->GetLayer();
				}
				Bool mouseEnabled = false;
				Bool keysEnabled = false;
				Bool analogEnabled = false;
				CFlashMovie* movie = menu->GetFlashMovie();
				if ( movie )
				{
					mouseEnabled	= movie->IsInputSourceEnabled( CFlashMovie::ISF_Mouse );
					keysEnabled		= movie->IsInputSourceEnabled( CFlashMovie::ISF_Key );
					analogEnabled	= movie->IsInputSourceEnabled( CFlashMovie::ISF_Analog );
				}
				if ( !mouseEnabled && !keysEnabled && !keysEnabled )
				{
					textColor = Color::RED;
				}
				else if ( mouseEnabled && keysEnabled && keysEnabled )
				{
					textColor = Color::GREEN;
				}
				else
				{
					textColor = Color::YELLOW;
				}
				line = String::Printf( TXT("%d %d %d%d%d %s %X %s"), i, layer, mouseEnabled, keysEnabled, analogEnabled, menu->GetMenuName().AsChar(), menu->GetParentGuiObject(), menu->GetParent()? menu->GetParent()->GetFriendlyName().AsChar() : TXT("null") );
				frame->AddDebugScreenText( 30,  150 + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
				i++;
				menu = Cast< CR4Menu >( menu->GetParentGuiObject() );
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  150 + i * 19, TXT("NO MENU OPENED"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		i +=2;

		if ( !m_popups.Empty() )
		{
			for ( Uint32 idx = 0; idx < m_popups.Size(); ++idx )
			{
				CR4Popup* popup = m_popups[ idx ];
				if ( popup )
				{
					line = String::Printf( TXT("%d %s"), idx, popup->GetPopupName().AsChar() );
					frame->AddDebugScreenText( 30,  150 + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
					i++;
				}
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  150 + i * 19, TXT("NO POPUP OPENED"), 0, true, Color::RED, Color::BLACK, nullptr );
		}
	}

	if ( m_showInput )
	{
		const TDynArray< CName >& storedContext = GGame->GetInputManager()->GetStoredContext();
		const CName& currentContext = GGame->GetInputManager()->GetCurrentContext();

		Color textColor = Color::WHITE;
		Uint32 i = 0;
		String line;

		if ( !storedContext.Empty() )
		{
			for ( i = 0; i < storedContext.Size(); i++ )
			{
				line = String::Printf( TXT("%d %s"), i, storedContext[ i ].AsChar() );
				frame->AddDebugScreenText( 30,  400 + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
			}
		}
		else
		{
			frame->AddDebugScreenText( 30,  400 + i * 19, TXT("NO STORED CONTEXT"), 0, true, Color::RED, Color::BLACK, nullptr );
		}

		i += 2;
		line = String::Printf( TXT("Current: %s"), currentContext.AsChar() );
		frame->AddDebugScreenText( 30,  400 + i * 19, line, 0, true, textColor, Color::BLACK, nullptr );
	}
#endif // RED_FINAL_BUILD
}

void CR4GuiManager::Tick( float deltaTime )
{
	PC_SCOPE( CR4GuiManager_TickAll );

	TBaseClass::Tick( deltaTime );

	//TODO: Have a way of potentially locking the gui event queue if needs be.

	ProcessGameState();
	ProcessBackgroundVideo();
	ProcessVideoEvents();
}

void CR4GuiManager::ProcessGameState()
{
	PC_SCOPE( ProcessGameState );

	//TODO: Not so hardcoded. 
	// Just don't have time right now and who knows what R4's real needs will be atm.

	ProcessHudVisibility();

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
void CR4GuiManager::ProcessHudVisibility() const
{
	// blackscreen condition needed to be commented out, because there's a deadlock when a game starts with movie to be played:
	// - story player waits for hud to be loaded and ready, than makes the screen fade in
	// - hud is not loaded because there's blackscreen and waits for fading in
	const Bool gameActive = GGame && /*! GGame->IsBlackscreen() && ! GGame->IsLoadingScreenShown() &&*/ GGame->IsActive();
	
	const Bool keepHud = CanKeepHud();

	const Bool showHud = gameActive && keepHud;
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

Bool CR4GuiManager::CanKeepHud() const
{
	if ( m_menu )
	{
		if ( !m_menu->CanKeepHud() )
		{
			return false;
		}
	}
	return true;
}

void CR4GuiManager::ProcessBackgroundVideo()
{
	PC_SCOPE( ProcessBackgroundVideo );

	RED_ASSERT( !m_menuBackgroundVideos.Empty(), TXT("Removed sentry value!") );

	const SMenuBackgroundVideo& bgVideo = m_menuBackgroundVideos.Last();
	RED_ASSERT( bgVideo.m_menu || bgVideo.m_videoFile.Empty(), TXT("BG video menu discarded!") );

	if ( bgVideo.m_videoFile.Empty() )
	{
		StopBackgroundVideoIfNeeded();
	}
	else
	{
		PlayBackgroundVideoIfNeeded( bgVideo );
	}
}

void CR4GuiManager::ProcessVideoEvents()
{
	PC_SCOPE( ProcessVideoEvents );

	m_videoContext.Update();

	TDynArray< SVideoEvent > videoEvents;
	m_videoContext.FlushVideoEvents( videoEvents );

	IScriptable* context = (m_hud && m_hud->IsAttached()) ? static_cast<IScriptable*>(m_hud) : static_cast<IScriptable*>(m_menu);
	if ( ! context )
	{
		return;
	}

	for ( const SVideoEvent& event : videoEvents )
	{
		if ( event.m_videoClient == CNAME( VideoClient_MenuBackground ) )
		{
			continue;
		}
		const CName scriptEvent;
		switch ( event.m_videoEventType )
		{
		case eVideoEventType_Started:
			context->CallEvent( CNAME( OnVideoStarted ) );
			break;
		case eVideoEventType_Stopped:
			context->CallEvent( CNAME( OnVideoStopped ) );
			break;
		case eVideoEventType_Subtitles:
			if ( GCommonGame->AreSubtitlesEnabled() )
			{
				context->CallEvent( CNAME( OnVideoSubtitles ), event.m_subtitles );
			}
			break;
		default:
			HALT( "Unhandled video event %u", event.m_videoEventType );
			break;
		}
	}
}

void CR4GuiManager::ShutdownVideo()
{
	m_videoContext.CancelVideo();
	m_videoContext = SVideoContext();
}

void CR4GuiManager::PlayBackgroundVideoIfNeeded( const SMenuBackgroundVideo& video )
{
	const CName curVideoClient = m_videoContext.GetVideoClient();
	if ( ! curVideoClient || curVideoClient == CNAME(VideoClient_MenuBackground) )
	{
		const String videoPath = MENU_BACKGROUND_VIDEO_PATH + video.m_videoFile;
		if ( m_videoContext.GetFileName() != videoPath )
		{
			const SVideoParams videoParams( videoPath, eVideoParamFlag_PlayLooped );
			m_videoContext.PlayVideo( CNAME( VideoClient_MenuBackground ), videoParams );
		}
	}
}

void CR4GuiManager::StopBackgroundVideoIfNeeded()
{
	const CName curVideoClient = m_videoContext.GetVideoClient();
	if ( curVideoClient == CNAME(VideoClient_MenuBackground) )
	{
		m_videoContext.CancelVideo();
	}
}

void CR4GuiManager::PlayFlashbackVideoAsync( const String& videoFile, Bool looped )
{
	const String videoPath = FLASHBACKS_VIDEO_PATH + videoFile;

	//////////////////////////////////////////////////////////////////////////
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !! TRC: since we play the recap_wip.usm through here, we need extra buffering if trying to also fetch as much
	// !! as possible off Blu-ray
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//////////////////////////////////////////////////////////////////////////
	EVideoBuffer videoBuffer = eVideoBuffer_Default;
	if ( GContentManager->GetStallForMoreContent() == eContentStall_DiscLaunch )
	{
		videoBuffer = eVideoBuffer_Long;
	}

	EVideoParamFlags flags = eVideoParamFlag_Preemptive;
	if ( looped )
	{
		flags = eVideoParamFlag_PlayLooped;
	}
	SVideoParams videoParams( videoPath, flags, videoBuffer );

	m_videoContext.PlayVideo( CNAME(VideoClient_Flashback), videoParams );
}

void CR4GuiManager::CancelFlashbackVideo()
{
	const CName curVideoClient = m_videoContext.GetVideoClient();
	if ( curVideoClient == CNAME( VideoClient_Flashback ) )
	{
		m_videoContext.CancelVideo();
	}
}

void CR4GuiManager::OnVideoStopped( CName videoClient )
{
	m_videoContext.AddVideoEvent( SVideoEvent( videoClient, eVideoEventType_Stopped ) );
}

void CR4GuiManager::OnVideoStarted( CName videoClient )
{
	m_videoContext.AddVideoEvent( SVideoEvent( videoClient, eVideoEventType_Started ) );
}

void CR4GuiManager::OnVideoSubtitles( CName videoClient, const String& subtitles )
{
	if ( GCommonGame->AreSubtitlesEnabled() )
	{
		m_videoContext.AddVideoEvent( SVideoEvent( videoClient, subtitles ) );
	}
}

void CR4GuiManager::OnEnteredConstrainedState()
{
	// required only for menus, so there's no need to send it to hud & popups
	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnChangedConstrainedState ), true );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

void CR4GuiManager::OnExitedConstrainedState()
{
	// required only for menus, so there's no need to send it to hud & popups
	CMenu* menu = m_menu;
	while ( menu )
	{
		menu->CallEvent( CNAME( OnChangedConstrainedState ), false );
		menu = Cast< CMenu >( menu->GetParentGuiObject() );
	}
}

CR4Menu* CR4GuiManager::CreateMenu( const CName& menuName, CObject* parent )
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

	CR4Menu* menu = SafeCast< CR4Menu >( CMenu::CreateObjectFromResource( Cast< CMenuResource >( menuResource ), parent ) );
	ASSERT( menu );
	if ( ! menu )
	{
		GUI_ERROR( TXT("Failed to create menu '%ls' from resource '%ls'"), menuName.AsString().AsChar(), menuDepothPath.AsChar() );
		return nullptr;
	}

	return menu;
}

CR4Popup* CR4GuiManager::CreatePopup( const CName& popupName, CObject* parent )
{
	String popupDepothPath;
	if ( ! m_cachedPopupDepotMap.Find( popupName, popupDepothPath ) )
	{
		GUI_ERROR( TXT("Popup '%ls' not found"), popupName.AsString().AsChar() );
		return nullptr;
	}

	CPopupResource* popupResource = Cast< CPopupResource >( GDepot->LoadResource( popupDepothPath ) );
	ASSERT( popupResource );
	if ( ! popupResource )
	{
		GUI_ERROR( TXT("Failed to open menu '%ls' with resource '%ls'"), popupName.AsString().AsChar(), popupDepothPath.AsChar() );
		return nullptr;
	}

	CR4Popup* popup = SafeCast< CR4Popup >( CPopup::CreateObjectFromResource( Cast< CPopupResource >( popupResource ), parent ) );
	ASSERT( popup );
	if ( ! popup )
	{
		GUI_ERROR( TXT("Failed to create menu '%ls' from resource '%ls'"), popupName.AsString().AsChar(), popupDepothPath.AsChar() );
		return nullptr;
	}

	return popup;
}

Bool CR4GuiManager::MakeModal( CR4Menu* menu, Bool make )
{
	if ( make )
	{
		if ( m_modalMenus.Exist( menu ) )
		{
			return false;
		}
		m_modalMenus.PushBack( menu );
	}
	else
	{
		if ( !m_modalMenus.Exist( menu ) )
		{
			return false;
		}
		m_modalMenus.Remove( menu );
	}
	UpdateInputFlags();

	return true;
}

Bool CR4GuiManager::MakeModal( CR4Popup* popup, Bool make )
{
	if ( make )
	{
		if ( m_modalPopups.Exist( popup ) )
		{
			return false;
		}
		m_modalPopups.PushBack( popup );
	}
	else
	{
		if ( !m_modalPopups.Exist( popup ) )
		{
			return false;
		}
		m_modalPopups.Remove( popup );
	}
	UpdateInputFlags();

	return true;
}

void CR4GuiManager::AttachUIListener( CQuestUICondition& listener )
{
	m_uiListeners.PushBackUnique( &listener );
}

void CR4GuiManager::DetachUIListener( CQuestUICondition& listener )
{
	m_uiListeners.Remove( &listener );
}

void CR4GuiManager::OnClosingMenu( CR4Menu* menu )
{
	if ( m_modalMenus.Exist( menu ) )
	{
		m_modalMenus.Remove( menu );
		UpdateInputFlags();
	}
}

void CR4GuiManager::OnClosingPopup( CR4Popup* popup )
{
	if ( m_modalPopups.Exist( popup ) )
	{
		m_modalPopups.Remove( popup );
		UpdateInputFlags();
	}
}

void CR4GuiManager::OnOpenedMenuEvent( const CName& menuName )
{
	if ( !m_uiListeners.Empty() )
	{
		SMenuEvent event( CNAME( OnMenuEvent ), menuName, nullptr );
		for ( Uint32 i = 0; i < m_uiListeners.Size(); ++i )
		{
			m_uiListeners[ i ]->OnEvent( event );
		}
	}

	UpdateInputFlags();
}

void CR4GuiManager::OnOpenedJournalEntryEvent( const THandle< CJournalBase >& entryHandle )
{
	if ( !m_uiListeners.Empty() )
	{
		SMenuEvent event( CNAME( OnMenuEvent ), CName::NONE, entryHandle );
		for ( Uint32 i = 0; i < m_uiListeners.Size(); ++i )
		{
			m_uiListeners[ i ]->OnEvent( event );
		}
	}
}

void CR4GuiManager::OnSentCustomUIEvent( const CName& eventName )
{
	if ( !m_uiListeners.Empty() )
	{
		SMenuEvent event( eventName, CName::NONE, nullptr );
		for ( Uint32 i = 0; i < m_uiListeners.Size(); ++i )
		{
			m_uiListeners[ i ]->OnEvent( event );
		}
	}
}

void CR4GuiManager::UpdateInputFlags()
{
	Bool isModalMenu     = !m_modalMenus.Empty();
	Bool isModalPopup    = !m_modalPopups.Empty();
	Bool isModalAnything = isModalMenu || isModalPopup;

	// enable/disable input for hud
	if ( m_hud )
	{
		m_hud->EnableInput( !isModalAnything );
	}

	if ( isModalPopup )
	{
		// disable for all menus
		if ( m_menu )
		{
			m_menu->EnableInput( false );
		}
		// disable for all popups
		for ( Uint32 i = 0; i < m_popups.Size(); ++i )
		{
			THandle< CR4Popup > currentPopup = m_popups[ i ];
			if (currentPopup)
			{
				currentPopup->EnableInput( false );
			}			
		}
		// enable for last modal popup
		CR4Popup* modalPopup = m_modalPopups[ m_modalPopups.Size() - 1 ];
		if ( modalPopup )
		{
			modalPopup->EnableInput( true );
		}
	}
	else if ( isModalMenu )
	{
		// disable for all menus
		if ( m_menu )
		{
			m_menu->EnableInput( false );
		}
		// enable for last modal menu
		CR4Menu* modalMenu = m_modalMenus[ m_modalMenus.Size() - 1 ];
		if ( modalMenu )
		{
			modalMenu->EnableInput( true, false );
		}
		// enable for all popups
		for ( Uint32 i = 0; i < m_popups.Size(); ++i )
		{
			THandle< CR4Popup > currentPopup = m_popups[ i ];
			if ( currentPopup )
			{
				currentPopup->EnableInput( true );
			}
		}
	}
	else
	{
		// enable for all menus
		if ( m_menu )
		{
			m_menu->EnableInput( true );
		}
		// enable for all popups
		for ( Uint32 i = 0; i < m_popups.Size(); ++i )
		{
			THandle< CR4Popup > currentPopup = m_popups[ i ];
			if ( currentPopup )
			{
				currentPopup->EnableInput( true );
			}
		}
	}
}

void CR4GuiManager::DialogHudShow()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogHudShow ) );
	}
}

void CR4GuiManager::DialogHudHide()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogHudHide ) );
	}
}

void CR4GuiManager::DialogSentenceSet( const String& htmlText, Bool alternativeUI )
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogSentenceSet ), htmlText, alternativeUI );
	}
}

void CR4GuiManager::DialogPreviousSentenceSet( const String& htmlText )
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogPreviousSentenceSet ), htmlText );
	}
}

void CR4GuiManager::DialogPreviousSentenceHide()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogPreviousSentenceHide ) );
	}
}



void CR4GuiManager::DialogSentenceHide()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogSentenceHide ) );
	}
}

void CR4GuiManager::DialogChoicesSet( const TDynArray< SSceneChoice >& choices, Bool alternativeUI )
{
	if ( m_hud )
	{
		TDynArray< SSceneChoice > scriptedChoices;
		for ( Uint32 i = 0; i < choices.Size(); i++ )
		{
			scriptedChoices.PushBack( choices[ i ] );
		}
		m_hud->CallEvent( CNAME( OnDialogChoicesSet ), scriptedChoices, alternativeUI );
	}
}

void CR4GuiManager::DialogChoiceTimeoutSet( Float timeOutPercent )
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogChoiceTimeoutSet ), timeOutPercent );
	}
}

void CR4GuiManager::DialogChoiceTimeoutHide()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogChoiceTimeoutHide ) );
	}
}

void CR4GuiManager::DialogSkipConfirmShow()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogSkipConfirmShow ) );
	}
}

void CR4GuiManager::DialogSkipConfirmHide()
{
	if ( m_hud )
	{
		m_hud->CallEvent( CNAME( OnDialogSkipConfirmHide ) );
	}
}

Bool CR4GuiManager::IsDialogHudInitialized() const
{
#ifdef USE_SCALEFORM
	if ( m_hud )
	{
		CGuiObject* object = m_hud->GetChild( TXT( "DialogModule" ) );
		if ( object && object->IsInitWithFlashSprite() )
		{
			return true;
		}
	}
#endif
	return false;

}

void CR4GuiManager::SubtitleShow( ISceneActorInterface * actor, const String & text, Bool alternativeUI  )
{
	if ( m_hud )
	{
		m_hud->ShowSubtitle( actor, text, alternativeUI );
	}
}

void CR4GuiManager::SubtitleHide( ISceneActorInterface * actor )
{
	if ( m_hud )
	{
		m_hud->HideSubtitle( actor );
	}
}

void CR4GuiManager::DebugTextShow( const String& text )
{
	if ( m_hud )
	{
		m_hud->DebugTextShow( text );
	}
}

void CR4GuiManager::DebugTextHide()
{
	if ( m_hud )
	{
		m_hud->DebugTextHide();
	}
}

CInteractionComponent* CR4GuiManager::GetActiveInteraction() const
{
	if ( m_hud )
	{
		return m_hud->GetActiveInteraction();
	}
	return nullptr;
}

void CR4GuiManager::SetActiveInteraction( CInteractionComponent * interaction, Bool force )
{
	if ( m_hud )
	{
		m_hud->SetActiveInteraction( interaction, force );
	}
}

void CR4GuiManager::ShowOneliner( const String& plainText, const CEntity* entity )
{
	if ( m_hud )
	{
		m_hud->ShowOneliner( plainText, entity );
	}
}

void CR4GuiManager::HideOneliner( const CEntity* entity )
{
	if ( m_hud )
	{
		m_hud->HideOneliner( entity );
	}
}

void CR4GuiManager::funcIsAnyMenu( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( IsAnyMenu() );
}

void CR4GuiManager::funcGetRootMenu( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_HANDLE( CR4Menu, m_menu );
}

void CR4GuiManager::funcGetPopup( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, popupName, CName::NONE )
	FINISH_PARAMETERS;

	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		if ( m_popups[ i ] != nullptr && popupName == m_popups[ i ]->GetPopupName() )
		{
			RETURN_HANDLE( CR4Popup, m_popups[ i ] );
			return;
		}
	}
	RETURN_HANDLE( CR4Menu, nullptr );
}

void CR4GuiManager::funcGetPopupList( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< CName >, popupNames, TDynArray< CName >() )
	FINISH_PARAMETERS;

	popupNames.ClearFast();
	for ( Uint32 i = 0; i < m_popups.Size(); ++i )
	{
		popupNames.PushBack( m_popups[ i ]->GetPopupName() );
	}
}

void CR4GuiManager::funcSendCustomUIEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE )
	FINISH_PARAMETERS;

	OnSentCustomUIEvent( eventName );
}

void CR4GuiManager::funcPlayFlashbackVideoAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, videoFile, String::EMPTY )
	GET_PARAMETER_OPT( Bool, looped, false )
	FINISH_PARAMETERS;

	PlayFlashbackVideoAsync( videoFile, looped );
}

void CR4GuiManager::funcCancelFlashbackVideo( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CancelFlashbackVideo();
}

void CR4GuiManager::funcSetSceneEntityTemplate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntityTemplate >, entityTemplate, nullptr )
	GET_PARAMETER_OPT( CName, animationName, CName::NONE )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->SetEntityTemplate( entityTemplate , animationName );
	}
}

void CR4GuiManager::funcApplyAppearanceToSceneEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, appearanceName, CName::NONE )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->ApplyAppearanceToSceneEntity( appearanceName );
	}
}

void CR4GuiManager::funcUpdateSceneEntityItems( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< SItemUniqueId >, itemIds, TDynArray< SItemUniqueId >() )
	GET_PARAMETER( TDynArray< SGuiEnhancementInfo >, enhancements, TDynArray< SGuiEnhancementInfo >() )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->UpdateEntityItems( itemIds, enhancements );
	}
}

void CR4GuiManager::funcSetSceneCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS )
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->UpdateCamera( position, rotation, 70.0f );
	}
}

void CR4GuiManager::funcSetupSceneCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, lookAtPosition, Vector::ZEROS )
	GET_PARAMETER( EulerAngles, cameraRotation, EulerAngles::ZEROS )
	GET_PARAMETER( float, distance, 1.0f )
	GET_PARAMETER( float, fov, 50.0f )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		Vector forward( Vector::ZEROS );
		cameraRotation.ToAngleVectors( &forward, nullptr, nullptr );
		forward.Normalize3();
		Vector cameraPosition = lookAtPosition + ( forward * distance * -1.0f );
		m_guiScenePlayer->UpdateCamera( cameraPosition, cameraRotation, fov );
	}
}

void CR4GuiManager::funcSetEntityTransform( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS )
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS )
	GET_PARAMETER( Vector, scale, Vector::ONES )
	FINISH_PARAMETERS;

	if (m_guiScenePlayer)
	{
		m_guiScenePlayer->UpdateEntityTransform(&position, &rotation, &scale);
	}
}

void CR4GuiManager::funcSetSceneEnvironmentAndSunPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEnvironmentDefinition >, envDef, nullptr )
	GET_PARAMETER( EulerAngles, sunRotation, EulerAngles::ZEROS )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->SetEnvironment( envDef, sunRotation );
	}
}

void CR4GuiManager::funcEnableScenePhysics( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enable, true )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->EnablePhysics( enable );
	}
}


void CR4GuiManager::funcSetBackgroundTexture( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CResource>, resource, nullptr )
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer && resource && resource->IsA<CBitmapTexture>() )
	{
		CResource* res = resource;
		m_guiScenePlayer->UpdateBackgroundTexture( static_cast<CBitmapTexture*>( res ) );
	}
}

void CR4GuiManager::funcRequestClearScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_guiScenePlayer )
	{
		m_guiScenePlayer->ClearScene();
	}
}
