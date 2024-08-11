/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx\display.h>
#include <shellapi.h>
#include "viewportWidgetMoveAxis.h"
#include "viewportWidgetRotateAxis.h"
#include "viewportWidgetScaleAxis.h"
#include "timeline.h"
#include "cutsceneTimeline.h"
#include "lazyWin32feedback.h"
#include "effectEditor.h"
#include "cutsceneNetPanel.h"

#include "../../common/game/questGraphBlock.h"
#include "../../common/game/questPhase.h"
#include "../../common/game/newNpc.h"

#include "../../common/game/definitionsManager.h"

#include "../../common/game/equipmentState.h"
#include "../../common/game/actorSpeech.h"
#include "../../common/game/storyscene.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/engine/extAnimCutsceneSoundEvent.h"
#include "../../common/engine/soundStartData.h"
#include "../../common/engine/fxTrackItemParticles.h"
#include "../../common/engine/fxSimpleSpawner.h"
#include "../../common/engine/renderCommands.h"

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/fxTrackGroup.h"
#include "../../common/engine/extAnimEvent.h"
#include "../../common/engine/extAnimCutsceneDialogEvent.h"
#include "../../common/engine/extAnimCutsceneEffectEvent.h"
#include "../../common/engine/environmentManager.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/cutsceneInstance.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/fonts.h"
#include "../../common/engine/worldTick.h"

#include "popupNotification.h"
#include "../../common/game/extAnimCutsceneQuestEvent.h"

CGatheredResource resCutsceneFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );

CGatheredResource geralt_entity( TXT("characters\\player_entities\\geralt\\geralt_player.w2ent"), RGF_Startup );
CGatheredResource witcher_entity( TXT("characters\\player_entities\\geralt\\geralt_player.w2ent"), RGF_Startup );
CGatheredResource eskel_entity( TXT("characters\\npc_entities\\main_npc\\eskel.w2ent"), RGF_Startup );
CGatheredResource radovid_entity( TXT("characters\\npc_entities\\main_npc\\radovid.w2ent"), RGF_Startup );
CGatheredResource cerys_entity( TXT("characters\\npc_entities\\main_npc\\cerys.w2ent"), RGF_Startup );
CGatheredResource eredin_entity( TXT("characters\\npc_entities\\main_npc\\eredin.w2ent"), RGF_Startup );
CGatheredResource uma_entity( TXT("characters\\npc_entities\\main_npc\\uma.w2ent"), RGF_Startup );
CGatheredResource iorweth_entity( TXT("characters\\npc_entities\\main_npc\\iorweth.w2ent"), RGF_Startup );
CGatheredResource yennefer_entity( TXT("characters\\npc_entities\\main_npc\\yennefer.w2ent"), RGF_Startup );
CGatheredResource avallach_entity( TXT("characters\\npc_entities\\main_npc\\avallach.w2ent"), RGF_Startup );
CGatheredResource lambert_entity( TXT("characters\\npc_entities\\main_npc\\lambert.w2ent"), RGF_Startup );
CGatheredResource ciri_entity( TXT("characters\\npc_entities\\main_npc\\ciri.w2ent"), RGF_Startup );
CGatheredResource caranthil_entity( TXT("characters\\npc_entities\\main_npc\\caranthil.w2ent"), RGF_Startup );
CGatheredResource ciri_young_entity( TXT("characters\\npc_entities\\main_npc\\ciri_young.w2ent"), RGF_Startup );
CGatheredResource young_ciri_entity( TXT("characters\\npc_entities\\main_npc\\ciri_young.w2ent"), RGF_Startup );
CGatheredResource imlerith_entity( TXT("characters\\npc_entities\\main_npc\\imlerith.w2ent"), RGF_Startup );
CGatheredResource baron_entity( TXT("characters\\npc_entities\\main_npc\\baron.w2ent"), RGF_Startup );
CGatheredResource geels_entity( TXT("characters\\npc_entities\\main_npc\\geels.w2ent"), RGF_Startup );
CGatheredResource sheala_entity( TXT("characters\\npc_entities\\main_npc\\sheala.w2ent"), RGF_Startup );
CGatheredResource triss_entity( TXT("characters\\npc_entities\\main_npc\\triss.w2ent"), RGF_Startup );
CGatheredResource talar_entity( TXT("characters\\npc_entities\\main_npc\\talar.w2ent"), RGF_Startup );
CGatheredResource vesemir_entity( TXT("characters\\npc_entities\\main_npc\\vesemir.w2ent"), RGF_Startup );
CGatheredResource keira_entity( TXT("characters\\npc_entities\\main_npc\\keira_metz.w2ent"), RGF_Startup );
CGatheredResource zoltan_entity( TXT("characters\\npc_entities\\main_npc\\zoltan.w2ent"), RGF_Startup );
CGatheredResource dandelion_entity( TXT("characters\\npc_entities\\main_npc\\dandelion.w2ent"), RGF_Startup );
CGatheredResource mousesack_entity( TXT("characters\\npc_entities\\main_npc\\mousesack.w2ent"), RGF_Startup );
CGatheredResource djikstra_entity( TXT("characters\\npc_entities\\main_npc\\djikstra.w2ent"), RGF_Startup );
CGatheredResource hjalmar_entity( TXT("characters\\npc_entities\\main_npc\\hjalmar.w2ent"), RGF_Startup );
CGatheredResource roche_entity( TXT("characters\\npc_entities\\main_npc\\roche.w2ent"), RGF_Startup );
CGatheredResource emhyr_entity( TXT("characters\\npc_entities\\main_npc\\emhyr.w2ent"), RGF_Startup );
CGatheredResource crach_entity( TXT("characters\\npc_entities\\main_npc\\crach_an_craite.w2ent"), RGF_Startup );


//////////////////////////////////////////////////////////////////////////

enum 
{
	ID_MENU_PLAY = 6000,
	ID_MENU_RESTART,
	ID_MENU_RELOAD,
	ID_MENU_SKELETON,
	ID_MENU_CAMERA,
	ID_MENU_PLAY_PREVIEW,
	ID_MENU_PLAY_GAME,
	ID_MENU_THE_END,
	ID_SCENE_LIST_SHOW,
	ID_SCENE_LIST_OPEN,
	ID_SCENE_LIST_LINES,
	ID_SCENE_LIST_REMOVE,
	ID_MENU_CS_ALL,
	ID_MENU_ADD_EFFECT_EMITTER,
	ID_MENU_ADD_EFFECT_CUSTOM,
	ID_SOCKET_CLIENT,
	ID_SOCKET_SERVER
};

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdCutsceneEditor, wxFrame )
	EVT_CLOSE( OnClose )
	
	EVT_MENU( XRCID( "save"), CEdCutsceneEditor::OnSave )
	EVT_MENU( XRCID("Mode_Move"), CEdCutsceneEditor::OnModeMove )
	EVT_MENU( XRCID("Mode_Rotate"), CEdCutsceneEditor::OnModeRotate )
	EVT_MENU( XRCID("Mode_Change"), CEdCutsceneEditor::OnModeChange )
	EVT_MENU( XRCID( "ResetLayout" ), CEdCutsceneEditor::OnResetLayout )
	EVT_MENU( XRCID( "SaveCustomLayout" ), CEdCutsceneEditor::SaveCustomLayout )
	EVT_MENU( XRCID( "LoadCustomLayout" ), CEdCutsceneEditor::LoadCustomLayout )
	EVT_MENU( XRCID( "reload"), CEdCutsceneEditor::OnReloadCutscene )
	EVT_MENU( XRCID( "reload_check"), CEdCutsceneEditor::OnCsDoAll )
	EVT_MENU( XRCID( "only_check"), CEdCutsceneEditor::OnCsDoCheck )
	EVT_MENU( XRCID( "recompress"), CEdCutsceneEditor::OnRecompressAnimations )
	EVT_MENU( XRCID( "importEvents"), CEdCutsceneEditor::OnImportEvents )
	EVT_MENU( XRCID( "importFoots"), CEdCutsceneEditor::OnImportFootStepEvents )
	EVT_MENU( XRCID( "recordCutscene"), CEdCutsceneEditor::OnDumpCutscene )
	EVT_MENU( XRCID( "streamingTool" ), CEdCutsceneEditor::OnReimportAndConvToStreamed )
	EVT_MENU( XRCID( "createrelaxevents" ), CEdCutsceneEditor::OnCreateRelaxEvents )
	EVT_MENU( XRCID( "deleterelaxevents" ), CEdCutsceneEditor::OnDeleteRelaxEvents )

	EVT_MENU( ID_MENU_PLAY, CEdCutsceneEditor::OnPlayPause )
	EVT_MENU( ID_MENU_RESTART, CEdCutsceneEditor::OnRestart )
	EVT_MENU( ID_MENU_RELOAD, CEdCutsceneEditor::OnReloadCutscene )
	EVT_MENU( ID_MENU_SKELETON, CEdCutsceneEditor::OnDispSkeletons )
	EVT_MENU( ID_MENU_CAMERA, CEdCutsceneEditor::OnCsCamera )
	EVT_MENU( ID_MENU_PLAY_PREVIEW, CEdCutsceneEditor::OnMenuCreateDestroyPreview )
	EVT_MENU( ID_MENU_PLAY_GAME, CEdCutsceneEditor::OnMenuCreateDestroyGame )
	EVT_MENU( ID_MENU_THE_END, CEdCutsceneEditor::OnSetEndTime )
	EVT_MENU( ID_MENU_CS_ALL, CEdCutsceneEditor::OnCsDoAll )

	EVT_MENU( ID_SCENE_LIST_SHOW, CEdCutsceneEditor::OnSceneListShow )
	EVT_MENU( ID_SCENE_LIST_OPEN, CEdCutsceneEditor::OnSceneListOpen )
	EVT_MENU( ID_SCENE_LIST_LINES, CEdCutsceneEditor::OnSceneListLines )
	EVT_MENU( ID_SCENE_LIST_REMOVE, CEdCutsceneEditor::OnSceneListRemove )
	EVT_MENU( XRCID( "DumpVoiceovers" ), CEdCutsceneEditor::OnDumpAllVoiceoversToFile)

	EVT_MENU( ID_MENU_ADD_EFFECT_EMITTER, CEdCutsceneEditor::OnAddEffectEmitter )
	EVT_MENU( ID_MENU_ADD_EFFECT_CUSTOM,  CEdCutsceneEditor::OnAddEffectCustom )

	EVT_TOOL( XRCID("repeat"), CEdCutsceneEditor::OnChangeRepeat )
	EVT_TOOL( XRCID("restart" ), CEdCutsceneEditor::OnRestart )
	EVT_TOOL( XRCID("play" ), CEdCutsceneEditor::OnPlayPause )
	EVT_TOOL( XRCID("resetEffect"), CEdCutsceneEditor::OnEffectReset )
	EVT_TOOL( XRCID("floor"), CEdCutsceneEditor::OnToggleFloor )
	EVT_TOOL( XRCID("playerCamera"), CEdCutsceneEditor::OnCsCamera )
	EVT_TOOL( XRCID("skeleton"), CEdCutsceneEditor::OnDispSkeletons )

	EVT_TOGGLEBUTTON( XRCID( "PlayCSPreview"), CEdCutsceneEditor::OnCreateDestroyPreview )
	EVT_TOGGLEBUTTON( XRCID( "PlayCSGame"), CEdCutsceneEditor::OnCreateDestroyGame )
	EVT_TOGGLEBUTTON( XRCID( "LoadWorld"), CEdCutsceneEditor::OnLoadWorld )
	EVT_TOGGLEBUTTON( XRCID( "netBtn"), CEdCutsceneEditor::OnNetConnected )

	EVT_BUTTON( XRCID( "csErrorDesc"), CEdCutsceneEditor::OnErrorDesc )
	EVT_BUTTON( XRCID( "bboxButton"), CEdCutsceneEditor::OnGenerateBBox )	
	EVT_BUTTON( XRCID( "csCheckTempl"), CEdCutsceneEditor::OnCheckActorsTemplates )
	EVT_BUTTON( XRCID( "addEffect" ),     CEdCutsceneEditor::OnAddEffect    )
	EVT_BUTTON( XRCID( "removeEffect" ),  CEdCutsceneEditor::OnRemoveEffect )
	EVT_BUTTON( XRCID( "copyEffect" ),    CEdCutsceneEditor::OnCopyEffect   )
	EVT_BUTTON( XRCID( "pasteEffect" ),   CEdCutsceneEditor::OnPasteEffect  )

	EVT_TREE_ITEM_ACTIVATED( XRCID( "csAnimTree"), CEdCutsceneEditor::OnAnimTreeDblClick )
	EVT_TREE_SEL_CHANGED( XRCID( "csAnimTree"), CEdCutsceneEditor::OnAnimTreeSelected )
	
	EVT_LIST_ITEM_RIGHT_CLICK( XRCID( "sceneList"), CEdCutsceneEditor::OnSceneColRightClick )
	
	EVT_CHOICE( XRCID( "animationCombo"), CEdCutsceneEditor::OnAnimChanged )
	
	EVT_LISTBOX( XRCID( "effectsListBox" ), CEdCutsceneEditor::OnEffectChange )
	
	EVT_SOCKET( ID_SOCKET_SERVER, CEdCutsceneEditor::OnSocketServerEvent )
	EVT_SOCKET( ID_SOCKET_CLIENT, CEdCutsceneEditor::OnSocketClientEvent )

	EVT_UPDATE_UI( wxID_ANY, CEdCutsceneEditor::OnUpdateUI )
	
END_EVENT_TABLE()

struct SRecordingInfo
{
	Int32 width, height, rate;
	Bool ubersample;
};

CEdCutsceneEditor::CEdCutsceneEditor( wxWindow *parent )
	: ISmartLayoutWindow( this )
	, m_csInstance( nullptr )
	, m_csTemplate( nullptr )
	, m_prevHook( nullptr )
	, m_mode( M_None )
	, m_updateTimer( this, wxID_ANY )
	, m_csCamera( nullptr )
	, m_selectEvent( -1 )
	, m_auiNotebook( nullptr )
	, m_showFloor( false )
	, m_silenceEnvironment( nullptr )
	, m_socketServer( nullptr )
	, m_socketClient( nullptr )
{
	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("CutsceneEditor2") );
}

CEdCutsceneEditor::CEdCutsceneEditor( wxWindow *parent, CCutsceneTemplate* cutsceneTempl, const TDynArray< SCutsceneActorLine >* lines /* = NULL  */)
	: ISmartLayoutWindow( this )
	, m_csInstance( nullptr )
	, m_csTemplate( cutsceneTempl )
	, m_prevHook( nullptr )
	, m_mode( M_None )
	, m_updateTimer( this, wxID_ANY )
	, m_csCamera( nullptr )
	, m_selectEvent( -1 )
	, m_auiNotebook( nullptr )
	, m_showFloor( false )
	, m_socketServer( nullptr )
	, m_socketClient( nullptr )
	, m_auiManager( nullptr, wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE )
{
	m_csTemplate->AddToRootSet();

	if ( lines )
	{
		m_actorLines = *lines;
	}

	// Load designed frame from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("CutsceneEditor2") );

	ASSERT( m_csTemplate->GetFile() );
	wxString caption = wxString::Format( wxT("Cutscene Editor - %s"), m_csTemplate->GetFile()->GetDepotPath().AsChar() );
	SetTitle( wxString::Format( caption ) );

	wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );
	
	// preview panel
	wxPanel* previewPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "PreviewPanel" ) );
	{
		m_previewPanel = new CEdCutsceneEditorPreview( previewPanel, this );
		previewPanel->GetSizer()->Add( m_previewPanel, 1, wxEXPAND );
		m_previewPanel->SetCameraPosition( Vector( 0, 4, 2 ) );
		m_previewPanel->SetCameraRotation( EulerAngles( 0, -10, 180 ) );
		m_previewPanel->GetViewport()->SetRenderingMode( RM_Shaded );
		m_previewPanel->SetEnabled( true );

		previewPanel->Layout();
	}

	// Property tab
	CEdPropertiesPage* propertyPage;
	wxPanel* propertyTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "PropertyTabPanel" ) );
	{
		propertyPage = new CEdPropertiesPage( propertyTabPanel, PropertiesPageSettings(), nullptr );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		sizer1->Add( propertyPage, 1, wxEXPAND, 0 );
		propertyTabPanel->SetSizer( sizer1 );
		propertyTabPanel->Layout();
	}
	
	// timeline
	wxPanel* timelinePanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "TimelinePanel" ) );
	m_timelineToolbar = wxXmlResource::Get()->LoadToolBar( timelinePanel, wxT( "CutsceneTimelineToolbar" ) );
	{		
		m_playPreviewButt = XRCCTRL( *m_timelineToolbar, "PlayCSPreview", wxToggleButton );
		m_playGameButt = XRCCTRL( *m_timelineToolbar, "PlayCSGame", wxToggleButton );
		m_loadWorldButt = XRCCTRL( *m_timelineToolbar, "LoadWorld", wxToggleButton );

		m_playIcon	= SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PLAY") );
		m_pauseIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONTROL_PAUSE") );

		m_cameraChoice = XRCCTRL( *m_timelineToolbar, "cameraChoice", wxChoice );
		m_cameraChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCutsceneEditor::OnCameraChoiceChanged ), NULL, this );

		m_fovText = XRCCTRL( *m_timelineToolbar, "cameraFOV", wxStaticText );

		ASSERT( timelinePanel != NULL );
		timelinePanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		
		m_newTimeline = new CEdCutsceneTimeline( this, timelinePanel, propertyPage );
		m_newTimeline->Connect( usrEVT_TIMELINE_REQUEST_SET_TIME, wxCommandEventHandler( CEdCutsceneEditor::OnRequestSetTime), NULL, this );

		
		timelinePanel->GetSizer()->Add( m_timelineToolbar, 0, wxEXPAND );
		timelinePanel->GetSizer()->Add( m_newTimeline, 1, wxEXPAND );

		timelinePanel->Layout();
	}

	// Create properties tab panel
	wxPanel* templateTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "TemplateTabPanel" ) );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );

		m_properties = new CEdPropertiesBrowserWithStatusbar( templateTabPanel, PropertiesPageSettings(), nullptr );
		m_properties->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdCutsceneEditor::OnPropertiesChanged ), NULL, this );
		sizer1->Add( m_properties, 1, wxEXPAND, 0 );

		m_properties->Get().SetObject( m_csTemplate );
		templateTabPanel->SetSizer( sizer1 );
		templateTabPanel->Layout();
	}

	// Create animation tab panel
	wxPanel* animationTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "AnimationTabPanel" ) );
	{
		wxPanel* cameraPropertiesPanel = XRCCTRL( *animationTabPanel, "AnimPropPanel", wxPanel );
		m_animPanelChoice = XRCCTRL( *animationTabPanel, "animationCombo", wxChoice );	

		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_animProperties = new CEdPropertiesPage( cameraPropertiesPanel, PropertiesPageSettings(), nullptr );
		sizer1->Add( m_animProperties, 1, wxEXPAND, 0 );
		cameraPropertiesPanel->SetSizer( sizer1 );
		cameraPropertiesPanel->Layout();
		SetupAnimationPage();
	}

	// Network tab panel
	wxPanel* networkTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "AANet" ) );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		CEdCutsceneNetPanel* panel = new CEdCutsceneNetPanel( networkTabPanel, this );
		sizer1->Add( panel, 1, wxEXPAND, 0 );
		networkTabPanel->SetSizer( sizer1 );
		networkTabPanel->Layout();
	}

	// Anim tree tab
	wxPanel* cutsceneTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "CutsceneTabPanel" ) );
	{
		m_animTree = XRCCTRL( *cutsceneTabPanel, "csAnimTree", wxTreeCtrl );
	}

	// Dialog tab
	wxPanel* dialogTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogTabPanel" ) );
	{
		m_dialogList = XRCCTRL( *dialogTabPanel, "dialogList", wxListCtrl );
		m_dialogList->InsertColumn( DIALOG_LIST_COL_ACTOR, wxT("Actor") );
		m_dialogList->InsertColumn( DIALOG_LIST_COL_TEXT, wxT("Text") );
		m_dialogList->InsertColumn( DIALOG_LIST_COL_SOUND, wxT("Sound") );
		m_dialogList->InsertColumn( DIALOG_LIST_COL_INDEX, wxT("Line index") );
	}
	
	// Scenes tab
	wxPanel* sceneTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "ScenesTabPanel" ) );
	{
		m_sceneList = XRCCTRL( *sceneTabPanel, "sceneList", wxListCtrl );
		m_sceneList->InsertColumn( SCENE_LIST_FILE_NAME, wxT("Name") );
		m_sceneList->InsertColumn( SCENE_LIST_FILE_PATH, wxT("File") );
	}
	
	// Effects tab
	wxPanel* effectTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "EffectsTabPanel" ) );
	{
		m_effectsList = XRCCTRL( *effectTabPanel, "effectsListBox", wxListBox );
	}
	
	// Cameras Tab
	wxPanel* cameraTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "CameraTabPanel" ) );
	{
		m_fovOverride = XRCCTRL( *cameraTabPanel, "checkCameraFovManual", wxCheckBox );
		m_dofOverride = XRCCTRL( *cameraTabPanel, "checkCameraDofManual", wxCheckBox );
	}

	// Deprecated panel
	wxPanel* deprecatedPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DepricatedPanel" ) );
	m_cutsceneToolbar = wxXmlResource::Get()->LoadToolBar( deprecatedPanel, wxT( "CutsceneToolbar" ) );
	{
		//////////////////////////////////////////////////////////////////////////

		m_timeMulSlider = XRCCTRL( *m_cutsceneToolbar, "timeMulSlider", wxSlider );
		m_timeMulSlider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CEdCutsceneEditor::OnTimeMulSliderUpdate ), NULL, this );
		m_timeMulSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdCutsceneEditor::OnTimeSliderMulUpdating ), NULL, this );
		m_timeMulSlider->GetToolTip()->SetDelay(1);
		m_timeMulSlider->SetTickFreq(100);

		m_timeMulEdit = XRCCTRL( *m_cutsceneToolbar, "timeMulEdit", wxTextCtrl );
		m_timeMulEdit->SetValidator( wxTextValidator(wxFILTER_NUMERIC) );
		m_timeMulEdit->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCutsceneEditor::OnTimeMulEditUpdate ), NULL, this );

		m_timeMul = 1.f;

		//////////////////////////////////////////////////////////////////////////
		deprecatedPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		deprecatedPanel->GetSizer()->Add( m_cutsceneToolbar, 0, wxEXPAND );
		deprecatedPanel->Layout();
	}
	
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnFovEdit, sliderCameraFov, editCameraFov, 1, 160 );
	SLIDER_AND_EDIT_WITH_RANGE_CONNECT( CEdCutsceneEditor::OnDofParam1, sliderDof1, editDof1, editDof1Min, editDof1Max );
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnDofParam2, sliderDof2, editDof2, 0, 100 );
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnDofParam3, sliderDof3, editDof3, 0, 100 );
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnDofParam4, sliderDof4, editDof4, 0, 100 );
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnDofParam5, sliderDof5, editDof5, 0, 100 );
	SLIDER_AND_EDIT_CONNECT( CEdCutsceneEditor::OnDofParam6, sliderDof6, editDof6, 0, 100 );
	
	// Timer event
	Connect( m_updateTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( CEdCutsceneEditor::OnGameTimer ) );

	// Panel Layout
	{
		m_auiManager.SetManagedWindow( containerPanel );

		m_auiNotebook =  new wxAuiNotebook( containerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP| wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT );
		m_auiNotebook->AddPage( propertyTabPanel, wxT( "Properties" ), true );
		m_auiNotebook->AddPage( cutsceneTabPanel, wxT( "Cutscene" ), false );
		m_auiNotebook->AddPage( templateTabPanel, wxT( "Template" ), false );
		m_auiNotebook->AddPage( effectTabPanel, wxT( "Effects" ), false );
		m_auiNotebook->AddPage( animationTabPanel, wxT( "Animation" ), false );
		m_auiNotebook->AddPage( networkTabPanel, wxT( "Network" ), false );
		m_auiNotebook->AddPage( dialogTabPanel, wxT( "Dialogs" ), false );
		m_auiNotebook->AddPage( sceneTabPanel, wxT( "Scenes" ), false );
		m_auiNotebook->AddPage( cameraTabPanel, wxT( "Camera" ), false );
		m_auiNotebook->AddPage( deprecatedPanel, wxT( "Deprecated" ), false );

		m_auiManager.AddPane( previewPanel,  wxTOP );
		m_auiManager.AddPane( timelinePanel, wxCENTER );
		m_auiManager.AddPane( m_auiNotebook, wxRIGHT );

		m_auiManager.GetPane( previewPanel ).CloseButton( false ).Name( wxT( "Main" ) ).CaptionVisible( true ).BestSize( 800, 800 );
		m_auiManager.GetPane( m_auiNotebook ).CloseButton( false ).Name( wxT( "Details" ) ).CaptionVisible( true ).BestSize( 400, 400 );
		m_auiManager.GetPane( timelinePanel ).CloseButton( false ).Name( wxT( "Time" ) ).CaptionVisible( true ).BestSize( 1000, 400 );
	}

	SEvents::GetInstance().RegisterListener( CNAME( WorldLoaded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WorldUnloaded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );

	// Final layout setup
	SetSize( 1024, 768 );
	wxCommandEvent dummyEvent;

	m_defaultLayout = SaveDefaultLayout( dummyEvent );
	LoadCustomLayout( dummyEvent );
	LoadOptionsFromConfig();

	UpdatePointDesc( false );

	// Default mode is None
	ActiveMode( M_None );

	CheckTemplate();

	FillTree();

	SelectAnimation( CCutsceneTemplate::CUTSCENE_ANIMATION_NAME, m_csTemplate->GetDuration() );

	if ( m_csTemplate->GetFilesWithThisCutscene().Size() > 0 )
	{
		ImportLinesFromSceneFile( m_csTemplate->GetFilesWithThisCutscene()[0] );
	}

	FillDialogPanel();
	FillScenePanel();

	ResetImportParams();
	UpdateEffectList();

	UpdateWidgets();

	m_auiManager.Update();
	Layout();
}

CEdCutsceneEditor::~CEdCutsceneEditor()
{
	m_auiManager.UnInit();

	SEvents::GetInstance().UnregisterListener( this );

	if ( m_socketClient )
	{
		delete m_socketClient;
	}

	if ( m_socketServer )
	{
		delete m_socketServer;
	}
}

void CEdCutsceneEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	CheckTemplate();
}

void CEdCutsceneEditor::OnEventPropertiesChanged( wxCommandEvent& event )
{

}

void CEdCutsceneEditor::OnClose( wxCloseEvent& event )
{
	SaveOptionsToConfig();

	if ( m_csInstance )
	{
		DestroyCutsceneInstance();
	}

	m_updateTimer.Stop();

	m_csTemplate->RemoveFromRootSet();

	Destroy();
}

void CEdCutsceneEditor::UpdateWidgets()
{
	FillCameraCombo();
	UpdateCameraIcon();
	UpdatePlayPauseButton();
	UpdateFOV();
}

void CEdCutsceneEditor::ActiveCsCamera()
{
	ASSERT( m_csInstance );

	if ( m_csInstance->GetCamera() )
	{
		ActiveCsCamera( m_csInstance->GetCamera() );
	}
}

void CEdCutsceneEditor::ActiveCsCamera( CCameraComponent* camera )
{
	ASSERT( camera );

	m_csCamera = camera;

	// Update DOF effect in environment manager
	m_previewPanel->GetPreviewWorld()->GetEnvironmentManager()->SetActiveCamera( m_csCamera );
	m_previewPanel->SetEnableFOVControl( false );

	if ( m_mode == M_Game )
	{
		GGame->GetViewport()->SetViewportHook( this );
		m_updateTimer.Stop();
	}

	m_updateTimer.Stop();

	UpdateCameraIcon();
}

void CEdCutsceneEditor::DeactiveCsCamera()
{
	// Update DOF effect in environment manager
	m_previewPanel->GetPreviewWorld()->GetEnvironmentManager()->SetActiveCamera( NULL );
	m_previewPanel->SetEnableFOVControl( true );

	if ( m_mode == M_Game )
	{
		ASSERT( m_prevHook );
		GGame->GetViewport()->SetViewportHook( m_prevHook );
		m_updateTimer.Start( 30, false );

		if ( m_csCamera && !GGame->IsActive() && wxTheFrame && wxTheFrame->GetWorldEditPanel() )
		{
			wxTheFrame->GetWorldEditPanel()->SetCameraPosition( m_csCamera->GetWorldPosition() );

			EulerAngles angles = m_csCamera->GetWorldRotation();
			angles.Roll = 0;
			wxTheFrame->GetWorldEditPanel()->SetCameraRotation( angles );
		}
	}
	if ( m_mode == M_Preview && m_csCamera )
	{
		m_previewPanel->SetCameraPosition( m_csCamera->GetWorldPosition() );
		
		EulerAngles angles = m_csCamera->GetWorldRotation();
		angles.Roll = 0;
		m_previewPanel->SetCameraRotation( angles );
	}

	m_csCamera = NULL;

	UpdateCameraIcon();
}

void CEdCutsceneEditor::OnCsCamera( wxCommandEvent& event )
{
	if ( m_csCamera )
	{
		DeactiveCsCamera();
		m_cameraChoice->Select(-1);
	}
	else if ( m_csInstance )
	{
		ActiveCsCamera();
		if ( m_csCamera )
		{
			TDynArray< CCameraComponent* > cameras;
			m_csInstance->GetAvailableCameras( cameras );		
			m_cameraChoice->Select( cameras.GetIndex( m_csCamera ) );
		}
	}
}

void CEdCutsceneEditor::OnCameraChoiceChanged( wxCommandEvent &event )
{
	Int32 selection = m_cameraChoice->GetSelection();
	
	if ( selection != wxNOT_FOUND && m_csInstance )
	{
		TDynArray< CCameraComponent* > cameras;
		m_csInstance->GetAvailableCameras( cameras );

		if ( selection < cameras.SizeInt() && cameras[selection] )
		{
			ActiveCsCamera( cameras[selection] );
		}
	}
}

void CEdCutsceneEditor::FillCameraCombo()
{
	m_cameraChoice->Clear();

	if ( m_csInstance )
	{
		TDynArray< CCameraComponent* > cameras;
		m_csInstance->GetAvailableCameras( cameras );

		for ( Uint32 i=0; i<cameras.Size(); ++i )
		{
			m_cameraChoice->AppendString( wxString::Format( wxT("%d"), i ) );
		}

		if ( m_csCamera )
		{
			m_cameraChoice->Select( cameras.GetIndex( m_csCamera ) );
		}
	}
}

String CEdCutsceneEditor::GetLineText( Int32 num ) const
{
	return (Int32)m_actorLines.Size() > num ? m_actorLines[ num ].m_text : String::EMPTY;
}

String CEdCutsceneEditor::GetLine( Int32 num ) const
{
	return GetLineActor( num ) + TXT(": ") + GetLineText( num );
}

String CEdCutsceneEditor::GetLineActor( Int32 num ) const
{
	return (Int32)m_actorLines.Size() > num ? m_actorLines[ num ].m_actorVoicetag : String::EMPTY;
}

Uint32 CEdCutsceneEditor::GetLineIndex( Int32 num ) const
{
	return (Int32)m_actorLines.Size() > num ? m_actorLines[ num ].m_lineIndex : 0;
}

StringAnsi CEdCutsceneEditor::GetSoundEventName( Int32 num ) const
{
	return (Int32)m_actorLines.Size() > num ? m_actorLines[ num ].m_soundEventName : 0;
}

CActor* CEdCutsceneEditor::FindLineActor( const String& actorVoicetag ) const
{
	if ( m_csInstance )
	{
		TDynArray< CEntity* > actors;
		m_csInstance->GetActors( actors );

		CName actorVoicetagName( actorVoicetag );

		for ( Uint32 i=0; i<actors.Size(); ++i )
		{
			CActor* actor = Cast< CActor >( actors[i] );
			if ( actor && actor->GetVoiceTag() == actorVoicetagName )
			{
				return actor;
			}
		}
	}

	return NULL;
}

Float CEdCutsceneEditor::GetLineTime( const String& line ) const
{
	return 0.6f + Max< Float >( 0.3f, line.GetLength() * 0.08f );
}

void CEdCutsceneEditor::FillDialogPanel()
{
	m_dialogList->Freeze();

	for ( Uint32 i=0; i<m_actorLines.Size(); ++i )
	{
		m_dialogList->InsertItem( i, m_actorLines[i].m_actorVoicetag.AsChar() );

		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( DIALOG_LIST_COL_TEXT );
			info.SetText( m_actorLines[i].m_text.AsChar() );
			m_dialogList->SetItem( info );
		}

		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( DIALOG_LIST_COL_SOUND );
			info.SetText( m_actorLines[i].m_sound.AsChar() );
			m_dialogList->SetItem( info );
		}

		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( DIALOG_LIST_COL_INDEX );
			info.SetText( ToString( m_actorLines[i].m_lineIndex ).AsChar() );
			m_dialogList->SetItem( info );
		}
	}

	m_dialogList->Thaw();
}

void CEdCutsceneEditor::FillScenePanel()
{
	m_sceneList->Freeze();
	m_sceneList->DeleteAllItems();

	const TDynArray< String >& files = m_csTemplate->GetFilesWithThisCutscene();

	for ( Uint32 i=0; i<files.Size(); ++i )
	{
		CFilePath filePath( files[ i ] );

		m_sceneList->InsertItem( i, filePath.GetFileName().AsChar() );

		{
			wxListItem info;
			info.SetId( i );
			info.SetColumn( SCENE_LIST_FILE_PATH );
			info.SetText( files[ i ].AsChar() );
			m_sceneList->SetItem( info );
		}
	}

	m_sceneList->Thaw();
}

void CEdCutsceneEditor::UpdateCameraIcon()
{
	m_timelineToolbar->ToggleTool( XRCID("playerCamera"), m_csCamera ? false : true );
}

void CEdCutsceneEditor::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( m_csCamera )
	{
		m_csCamera->OnViewportCalculateCamera( view, camera );
	}
}

void CEdCutsceneEditor::OnViewportTick( IViewport* view, Float timeDelta )
{
	UpdateCs( timeDelta );
}

void CEdCutsceneEditor::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	Uint32 offset = 50;

	// Camera switch flag
	Bool hasCamSwitches = m_csTemplate->HasCameraSwitches();
	if ( !hasCamSwitches )
	{
		frame->AddDebugScreenText( frame->GetFrameOverlayInfo().m_width * 0.1f, offset/2.f, TXT("Cam switches: Off"), Color::RED );
	}


	// Dialog lines
	for ( Uint32 i=0; i<m_dialogLines.Size(); ++i )
	{
		const String& text = m_dialogLines[i].m_second;
		
		if ( text.BeginsWith( TXT( "Choice" ) ) == true )
		{
			frame->AddDebugScreenText( 
				frame->GetFrameOverlayInfo().m_width * 0.05f, 
				frame->GetFrameOverlayInfo().m_height * 0.2f, 
				text.StringAfter( TXT( "Choice" ) ), Color::YELLOW, 
				resCutsceneFont.LoadAndGet< CFont >() );
		}
		else
		{
			frame->AddDebugScreenText( 
				frame->GetFrameOverlayInfo().m_width * 0.2f, 
				frame->GetFrameOverlayInfo().m_height - offset, text, Color::WHITE,
				resCutsceneFont.LoadAndGet< CFont >() );
			offset += 15;
		}
	}
}

const CAnimatedComponent* CEdCutsceneEditor::GetSelectedActor() const
{
	if ( m_csInstance && m_selectAnimation != CName::NONE )
	{
		return m_csInstance->GetActorsPlayableElementByAnimation( m_selectAnimation );
	}
	
	return NULL;
}

void CEdCutsceneEditor::OnSave( wxCommandEvent& event )
{
	ASSERT( m_csTemplate );

	if ( m_csTemplate->Save() )
	{
		LOG_EDITOR( TXT("Cutscene template %s saved"), m_csTemplate->GetFile()->GetDepotPath().AsChar() );
	}
}

void CEdCutsceneEditor::OnResetLayout( wxCommandEvent& event )
{
	m_auiManager.LoadPerspective( m_defaultLayout.AsChar(), true );
}

String CEdCutsceneEditor::SaveDefaultLayout( wxCommandEvent& event )
{
	return m_auiManager.SavePerspective().wc_str();
}

void CEdCutsceneEditor::SaveCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/CutsceneEditor") );

	String layoutPerspective = m_auiManager.SavePerspective().wc_str();
	config.Write( TXT("custom_layout"), layoutPerspective );
}


void CEdCutsceneEditor::LoadCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/CutsceneEditor") );

	String layoutPerspective = config.Read( TXT("custom_layout"), String::EMPTY );

	if ( !layoutPerspective.Empty() )
	{
		m_auiManager.LoadPerspective( layoutPerspective.AsChar(), true );
	}
}

TEdShortcutArray* CEdCutsceneEditor::GetAccelerators()
{
	if ( m_shortcuts.Empty() )
	{
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Reset"),wxAcceleratorEntry(	wxACCEL_CTRL,	WXK_LEFT,	ID_MENU_RESTART )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Play"),wxAcceleratorEntry(	wxACCEL_CTRL,	WXK_UP,		ID_MENU_PLAY )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\End"),wxAcceleratorEntry(		wxACCEL_CTRL,	WXK_RIGHT,	ID_MENU_THE_END )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Reload"),wxAcceleratorEntry(	wxACCEL_CTRL,	'R',		ID_MENU_RELOAD )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Skeleton"),wxAcceleratorEntry(wxACCEL_CTRL,	'S',		ID_MENU_SKELETON)) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Camera"),wxAcceleratorEntry(	wxACCEL_CTRL,	'M',		ID_MENU_CAMERA )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Preview"),wxAcceleratorEntry(	wxACCEL_CTRL,	'P',		ID_MENU_PLAY_PREVIEW )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\Game"),wxAcceleratorEntry(	wxACCEL_CTRL,	'G',		ID_MENU_PLAY_GAME )) );
		m_shortcuts.PushBack(SEdShortcut(TXT("CutsceneEditor\\All"),wxAcceleratorEntry(		wxACCEL_CTRL,	'A',		ID_MENU_CS_ALL )) );
	}

	return &m_shortcuts;
}

void CEdCutsceneEditor::OnLoadWorld( wxCommandEvent& event )
{
	if ( !m_csTemplate->GetLastLevelLoaded().Empty() )
	{
		if ( GFeedback->AskYesNo( TXT("Do you want to open the associated level %s"), m_csTemplate->GetLastLevelLoaded().AsChar() ) )
		{
			wxTheFrame->OpenWorld( m_csTemplate->GetLastLevelLoaded() );
			m_loadWorldButt->SetValue( false );
		}
	}
}
void CEdCutsceneEditor::OnCreateDestroyGame( wxCommandEvent& event )
{
	if ( m_playGameButt->GetValue() )
	{
		GFeedback->BeginTask( TXT("Spawning cutscene in the world"), false );
		{
			m_playGameButt->SetValue( false );

			if ( CreateCutsceneInstance( M_Game, nullptr ) )
			{
				m_playGameButt->SetLabel( wxT("Unbind") );
				m_playGameButt->SetValue( true );
			}
		}
		GFeedback->EndTask();
	}
	else
	{
		// Restore fade state
		( new CRenderCommand_ScreenFadeIn( 0.0f ) )->Commit();

		m_playGameButt->SetLabel( wxT("World") );
		DestroyCutsceneInstance();
	}
}

void CEdCutsceneEditor::OnCreateDestroyPreview( wxCommandEvent& event )
{
	if ( m_playPreviewButt->GetValue() )
	{
		GFeedback->BeginTask( TXT("Spawning cutscene in the preview"), false );
		{
			m_playPreviewButt->SetValue( false );

			if ( CreateCutsceneInstance( M_Preview, nullptr ) )
			{
				m_properties->Disable();
				m_properties->Refresh();
				m_playPreviewButt->SetLabel( wxT("Unbind") );
				m_playPreviewButt->SetValue( true );
			}

			m_previewPanel->SetEnableFOVControl( false );
		}
		GFeedback->EndTask();
	}
	else
	{
		// Restore fade state
		( new CRenderCommand_ScreenFadeIn( 0.0f ) )->Commit();

		m_playPreviewButt->SetLabel( wxT("Preview") );
		DestroyCutsceneInstance();
		m_properties->Enable();
		m_properties->Refresh();

		m_previewPanel->SetEnableFOVControl( true );
	}
}

void CEdCutsceneEditor::OnMenuCreateDestroyPreview( wxCommandEvent& event )
{
	if ( m_playPreviewButt->IsEnabled() )
	{
		m_playPreviewButt->SetValue( !m_playPreviewButt->GetValue() );
		OnCreateDestroyPreview( event );
	}
}

void CEdCutsceneEditor::OnMenuCreateDestroyGame( wxCommandEvent& event )
{
	if ( m_playGameButt->IsEnabled() )
	{
		m_playGameButt->SetValue( !m_playGameButt->GetValue() );
		OnCreateDestroyGame( event );
	}
}

void CEdCutsceneEditor::OnSetEndTime( wxCommandEvent& event )
{
	if ( m_csInstance )
	{
		m_csInstance->SetTime( m_csInstance->GetDuration() );
		
		if ( m_csInstance->IsPause() == false )
		{
			OnPlayPause( event );
		}
	}
}

#define MACH_CHARACTER( x ) \
if( aDef->m_name == TXT( #x ) ) \
{ \
	CEntityTemplate* res = x##_entity.LoadAndGet<CEntityTemplate>(); \
	if ( res ){aDef->m_template = res;} \
}

void CEdCutsceneEditor::TryMatchingActors()
{
	Bool canModify = false;
	TDynArray< String > actorNames;
	m_csTemplate->GetActorsName( actorNames );
	Uint32 size = actorNames.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const String& actor = actorNames[i];
		SCutsceneActorDef* aDef = m_csTemplate->GetActorDefinition( actor );
		ASSERT( aDef );
		if ( !aDef )
		{
			continue;
		}
		if( !aDef->m_template.Get() )
		{
			if ( canModify == false )
			{
				if ( m_csTemplate->MarkModified() == false )
				{
					wxMessageBox( wxT("Resource couldn't be modify!"), wxT("Error") );
					return;
				}
				else
				{
					canModify = true;
				}
			}

			MACH_CHARACTER( geralt )
			MACH_CHARACTER( witcher )
			MACH_CHARACTER( eskel )
			MACH_CHARACTER( radovid )
			MACH_CHARACTER( cerys )
			MACH_CHARACTER( eredin )
			MACH_CHARACTER( uma )
			MACH_CHARACTER( iorweth )
			MACH_CHARACTER( yennefer )
			MACH_CHARACTER( avallach )
			MACH_CHARACTER( lambert )
			MACH_CHARACTER( ciri )
			MACH_CHARACTER( caranthil )
			MACH_CHARACTER( ciri_young )
			MACH_CHARACTER( young_ciri )
			MACH_CHARACTER( imlerith )
			MACH_CHARACTER( baron )
			MACH_CHARACTER( geels )
			MACH_CHARACTER( sheala )
			MACH_CHARACTER( triss )
			MACH_CHARACTER( talar )
			MACH_CHARACTER( vesemir )
			MACH_CHARACTER( keira )
			MACH_CHARACTER( zoltan )
			MACH_CHARACTER( dandelion )
			MACH_CHARACTER( mousesack )
			MACH_CHARACTER( djikstra )
			MACH_CHARACTER( hjalmar )
			MACH_CHARACTER( roche )
			MACH_CHARACTER( emhyr )
			MACH_CHARACTER( crach )

		}
	}
	if ( canModify )
	{
		m_csTemplate->Save();
	}
}
#undef MACH_CHARACTER

void CEdCutsceneEditor::OnCsDoAll( wxCommandEvent& event )
{
	if ( ReloadCutscene() == false )
	{
		WARN_EDITOR( TXT("Cutscene '%s' reload... Fail"), m_csTemplate->GetFile()->GetFileName().AsChar() );
		return;
	}

	WARN_EDITOR( TXT("Cutscene '%s' reload... Ok"), m_csTemplate->GetFile()->GetFileName().AsChar() );

	TryMatchingActors();

	if ( Validate() == false )
	{
		WARN_EDITOR( TXT("Cutscene '%s' validate... Fail"), m_csTemplate->GetFile()->GetFileName().AsChar() );
		return;
	}

	WARN_EDITOR( TXT("Cutscene '%s' validate... Ok"), m_csTemplate->GetFile()->GetFileName().AsChar() );

	if ( m_csTemplate->Save() )
	{
		WARN_EDITOR( TXT("Cutscene '%s' save... Ok"), m_csTemplate->GetFile()->GetFileName().AsChar() );
	}
	else
	{
		WARN_EDITOR( TXT("Cutscene '%s' save... Fail"), m_csTemplate->GetFile()->GetFileName().AsChar() );
	}

	SEdPopupNotification::GetInstance().Show( this, TXT("Reimport And Templates Check"), TXT("Success") );
	// wxMessageBox( wxT("Success") );
	// Pawel S asked for this.
}

void CEdCutsceneEditor::OnCsDoCheck( wxCommandEvent& event )
{
	TryMatchingActors();

	if ( Validate() == false )
	{
		WARN_EDITOR( TXT("Cutscene '%s' validate... Fail"), m_csTemplate->GetFile()->GetFileName().AsChar() );
		return;
	}

	WARN_EDITOR( TXT("Cutscene '%s' validate... Ok"), m_csTemplate->GetFile()->GetFileName().AsChar() );

	if ( m_csTemplate->Save() )
	{
		WARN_EDITOR( TXT("Cutscene '%s' save... Ok"), m_csTemplate->GetFile()->GetFileName().AsChar() );
	}
	else
	{
		WARN_EDITOR( TXT("Cutscene '%s' save... Fail"), m_csTemplate->GetFile()->GetFileName().AsChar() );
	}

	SEdPopupNotification::GetInstance().Show( this, TXT("Templates Check"), TXT("Success") );
	// wxMessageBox( wxT("Success") );
	// Pawel S asked for this.
}

CLayer* CEdCutsceneEditor::GetLayer( CEdCutsceneEditor::ECutsceneEditorMode mode ) const
{
	ASSERT( mode != M_None );

	if ( mode == M_Game )
	{
		ASSERT( GGame && GGame->GetActiveWorld() );
		return GGame->GetActiveWorld()->GetDynamicLayer();
	}
	else if ( mode == M_Preview )
	{
		return m_previewPanel->GetPreviewWorld()->GetDynamicLayer();
	}
	
	return NULL;
}

Bool CEdCutsceneEditor::CanUseMode( CEdCutsceneEditor::ECutsceneEditorMode mode ) const
{
	ASSERT( mode != M_None );

	if ( ( mode == M_Game && m_playGameButt->IsEnabled() ) || ( mode == M_Preview && m_playPreviewButt->IsEnabled() ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

Uint32 CEdCutsceneEditor::GetCsPoint( Matrix& point, CEdCutsceneEditor::ECutsceneEditorMode mode, String& desc ) const
{
	ASSERT( mode != M_None );

	if ( mode == M_Game )
	{
		ASSERT( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetTagManager() );

		const TagList& tags = m_csTemplate->GetPointTag();

		TDynArray< CNode* > nodes;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tags, nodes );

		if ( !nodes.Empty() )
		{
			point = nodes[0]->GetRotation().ToMatrix();
			point.SetTranslation( nodes[0]->GetPosition() );

			desc = nodes[0]->GetFriendlyName();

			return nodes.Size();
		}
	}

	return 0;
}

void CEdCutsceneEditor::OnGameTimer( wxTimerEvent& event )
{
	ASSERT( m_mode == M_Game );
	if ( m_mode == M_Game )
	{
		Int32 interval = event.GetInterval();
		Float dt = interval/1000.f;
		UpdateCs( dt );
	}
}

void CEdCutsceneEditor::UpdateCs( Float dt )
{
	if ( m_csInstance && !m_csInstance->IsFinished() )
	{
		m_csInstance->Update( m_timeMul * dt );

		/// mcinek HACK begins - it is for correct updating ambients in cutscene preview

		if ( m_mode == M_Game && m_csInstance->GetCsCamera() != nullptr )
		{
			const CCameraDirector* director = GGame->GetActiveWorld()->GetCameraDirector();
			const Vector& cameraPosition = director->GetCameraPosition();
			m_hackPreviousCameraPosition = cameraPosition;
			GGame->GetActiveWorld()->UpdateCameraPosition( cameraPosition );
			GGame->GetActiveWorld()->UpdateCameraForward( director->GetCameraForward() );
			GGame->GetActiveWorld()->UpdateCameraUp( director->GetCameraUp() );
		}

		/// mcinek HACK ends

		// DIALOGS
		{
			// Remove previous dialog lines
			for ( Int32 i = ( Int32 ) m_dialogLines.Size() - 1; i >= 0; --i )
			{
				m_dialogLines[ i ].m_first -= m_timeMul * dt;
				if ( m_dialogLines[i].m_first < 0.f )
				{
					m_dialogLines.Erase( m_dialogLines.Begin() + i );
				}
			}

			TDynArray< const CExtAnimCutsceneDialogEvent* > dialogEvents;
			m_csTemplate->GetEventsOfType( dialogEvents );

			struct Comparer
			{
				Bool operator()( const CExtAnimEvent* event, Float time ) const
				{
					return event->GetStartTime() < time;
				}
			};

			// Count events passed
			TDynArray< const CExtAnimCutsceneDialogEvent* >::const_iterator iter =
				LowerBound( dialogEvents.Begin(), dialogEvents.End(), m_csInstance->GetTime(), Comparer() );

			Int32 lineNum = iter - dialogEvents.Begin() - 1;

			if( lineNum >= 0 )
			{
				// Check if line is not already shown
				struct Finder
				{
					String m_line;

					Finder( const String& line )
						: m_line( line ) { }

					Bool operator()( const tCsEdDialogLine& a ) const
					{
						return a.m_second == m_line;
					}
				};

				if( FindIf( m_dialogLines.Begin(), m_dialogLines.End(), Finder( GetLine( lineNum ) ) ) 
					== m_dialogLines.End() 
					&& ( *( iter - 1 ) )->GetStartTime() + GetLineTime( GetLine( lineNum ) ) > m_csInstance->GetTime() )
				{
					// Show line
					tCsEdDialogLine dLine;
					dLine.m_second = GetLine( lineNum );
					dLine.m_first = GetLineTime( dLine.m_second );
					m_dialogLines.PushBack( dLine );

					// Play actor line
					CActor* actor = FindLineActor( GetLineActor( lineNum ) );
					if ( actor )
					{
						ActorSpeechData data( GetLineIndex( lineNum ), GetSoundEventName( lineNum ), true, ASM_Voice | ASM_Lipsync );
						actor->SpeakLine( data );
					}
				}
			}
		}

		m_newTimeline->SetCurrentTime( m_csInstance->GetTime() );

		UpdateFOV();

		UpdateManualCameraOptions();

		if ( m_csInstance->IsFinished() )
		{
			SetTime( 0.f );

			if ( m_repeat )
			{
				m_csInstance->Unpause();

			}
			else if ( !m_csInstance->IsPause() )
			{
				m_csInstance->Pause();
			}

			UpdatePlayPauseButton();
		}

		// Update item entities
		SItemEntityManager::GetInstance().OnTick( dt );
	}
	else
	{
		m_newTimeline->SetCurrentTime( 0.0f );
	}

	m_newTimeline->Repaint();
}

void CEdCutsceneEditor::ActiveMode( CEdCutsceneEditor::ECutsceneEditorMode mode )
{
	m_previewPanel->SetEnabled( true );

	if ( mode == M_None && m_mode != M_None )
	{
		ResetCachets();
	}

	if ( mode == M_Game )
	{
		ASSERT( GGame->GetViewport()->GetViewportHook() != this );
		
		m_prevHook = GGame->GetViewport()->GetViewportHook();

		m_updateTimer.Start( 30, false );

		m_previewPanel->SetEnabled( false );
	}
	else if ( m_mode == M_Game )
	{
		ASSERT( m_prevHook );
		
		GGame->GetViewport()->SetViewportHook( m_prevHook );
		m_prevHook = NULL;

		m_updateTimer.Stop();
	}

	m_mode = mode;

	if ( mode == M_None )
	{
		DestroyItemContainer();
	}
	else
	{
		RecreateEffectPreviewItems();
		SetCachets();
	}
}

/*
 * Quick demo fix 
 * Is meant to solve bug caused by Geralt head becoming an item 
 * Items are spawned later after entity ( in SItemEntityManager tick ) 
 * one of the items ( head ) contains mimic component necessary for playing mimic animation 
 * Components are gathered in cutscene creation stage and if this particular one is missing ( havent spavned yet ) geralt mimic anim wont be played
 * this func spawns geralt and waits till his items are spawned so Cs can simply take existing entity from world instead of spawning one
 *
////////////////////////////////////////////////////////////////////////////////////////*/
void CEdCutsceneEditor::HACK_SpawnGeralt( CLayer* layer, CCutsceneTemplate* csTemplate, const Vector& spawnPos )
{
	CName player = CNAME( GERALT );
	TDynArray<CName> voicetags;
	csTemplate->GetActorsVoicetags( voicetags );
	if( !voicetags.Exist( player ) || layer->GetWorld()->GetTagManager()->GetTaggedEntity( player ) )
	{
		m_preSpawnedActor = nullptr;
		return;
	}	

	EntitySpawnInfo sinfo;
	const SCutsceneActorDef* def = csTemplate->GetActorDefinition( player );
	sinfo.m_template = def->m_template.Get();
	sinfo.m_entityClass = CActor::GetStaticClass();
	sinfo.m_appearances.PushBack( def->m_appearance );
	sinfo.m_spawnPosition = spawnPos;

	CActor* actor = Cast<CActor>( layer->GetWorld()->GetDynamicLayer()->CreateEntitySync( sinfo ) );
	TagList tags = actor->GetTags();
	CName playerTag = CNAME( PLAYER);
	if( !tags.HasTag( playerTag ) )
	{
		tags.AddTag( playerTag );
		actor->SetTags( tags );
	}

	if( actor->GetInventoryComponent() )
	{
		actor->InitInventory();
		actor->GetInventoryComponent()->SpawnMountedItems();
	}

	CTimeCounter timer;
	while ( timer.GetTimePeriod() < 10.f && SItemEntityManager::GetInstance().IsDoingSomething() )
	{
		SItemEntityManager::GetInstance().OnTick( 0.001f );
	}

	m_preSpawnedActor = actor;
}

void CEdCutsceneEditor::HACK_DepawnGeralt( CLayer* layer )
{
	if( m_preSpawnedActor )
	{
		m_preSpawnedActor->Destroy();
		m_preSpawnedActor = nullptr;
	}
	
}

Bool CEdCutsceneEditor::CreateCutsceneInstance( CEdCutsceneEditor::ECutsceneEditorMode mode, struct SRecordingInfo* recordingInfo )
{
	Bool ret = false;

	DestroyAllMixers();

	if ( CanUseMode( mode ) )
	{
		if ( !m_csInstance )
		{
			CLayer* layer = GetLayer( mode );
			String errors;

			if( CWorld* world = layer->GetWorld() )
			{
				SCameraLightsModifiersSetup setup;
				setup.SetModifiersAllIdentityOneEnabled( ECLT_Scene );
				setup.SetScenesSystemActiveFactor( 1.f );
				world->GetEnvironmentManager()->SetCameraLightsModifiers( setup );
			}

			Bool loadCutsceneInThisLevel = false;

			Matrix point = Matrix::IDENTITY;
			String desc;
			Uint32 foundPoints = GetCsPoint( point, mode, desc );

			if ( mode == M_Preview )
			{
				loadCutsceneInThisLevel = true;
			}
			else
			{			
				if ( foundPoints == 0 )
				{
					if ( GFeedback->AskYesNo( TXT("There is no Spawn Point in this world that matches the one for this cutscene.\nDo you still want to load the Cutscene in this world?") ) )
					{
						loadCutsceneInThisLevel = true;
					}
				}
				else
				{
					loadCutsceneInThisLevel = true;
				}
			}
			if ( loadCutsceneInThisLevel )
			{
				UpdatePointDesc( true, point, desc, foundPoints );

				HACK_SpawnGeralt( layer, m_csTemplate, point.GetTranslationRef() );

				m_csInstance = m_csTemplate->CreateInstance( layer, point, errors, NULL, new CCutsceneEmptyWorldInterface( layer->GetWorld() ) );

				if ( m_csInstance )
				{
					m_csTemplate->SetValid( true );

					m_csInstance->Play( false, false, true );

					ActiveMode( mode );

					ActiveCsCamera();

					// Put on equipment defined in actual appearance (for each actor)
					ApplyPreviewActorsEquipments();

					if ( m_effectEditor )
					{
						// Sets the entity to play effects on
						m_effectEditor->SetEntity( m_csInstance );
					}

					if ( !m_recordingInViewport && recordingInfo != nullptr )
					{
						switch ( recordingInfo->rate )
						{
						case 0: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 30 ); break;
						case 1: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 60 ); break;
						case 2: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 120 ); break;
						}
						GRender->SetAsyncCompilationMode( false );
						wxTheFrame->GetWorldEditPanel()->SetClientSize( recordingInfo->width, recordingInfo->height );
						wxTheFrame->GetWorldEditPanel()->GetViewport()->AdjustSize( recordingInfo->width, recordingInfo->height );
						GRender->RequestResizeRenderSurfaces( recordingInfo->width, recordingInfo->height );
						GGame->ToggleContignous( FCSF_PNG, recordingInfo->ubersample );
						m_recordingInViewport = true;
					}

 					if ( m_mode != M_Preview )
 					{
						// Get the world we are in at the moment
						String world = layer->GetWorld()->GetDepotPath();

						if ( m_csTemplate->GetLastLevelLoaded() != world )
						{
							if ( GFeedback->AskYesNo( TXT("Do you want to associate this level with this cutscene?\nThis will allow users to open the correct world, for this cutscene.") ) )
 							{
								// Need to check out the asset etc
								m_csTemplate->SetLastLevelLoaded( world );
								m_csTemplate->Save();
							}
						}
 					}

					TDynArray< CEntity* > entities;
					m_csInstance->GetActors( entities );
					for ( Uint32 i=0; i<entities.Size(); ++i )
					{
						if ( CActor* a = Cast< CActor >( entities[ i ] ) )
						{
							a->SetHiResShadows( true );
						}
					}

					ret = true;
				}
				else
				{
					m_csTemplate->SetValid( false );

					errors = TXT("Cutscene instance couldn't be created.\n") + errors;
					wxMessageBox( errors.AsChar(), wxT("Error"), wxOK | wxCENTRE, this );
					CheckTemplate();
				}
			}

			
		}
		else
		{
			// Recreate
			DestroyCutsceneInstance();
			ret = CreateCutsceneInstance( mode, nullptr );
		}
	}

	UpdateWidgets();

	return ret;
}

void CEdCutsceneEditor::DestroyCutsceneInstance()
{
	if ( m_csInstance )
	{
		if ( m_recordingInViewport )
		{
			Int32 fullScreenWidth = 0, fullScreenHeight = 0;
			Uint32 displayCount = ::wxDisplay::GetCount();
			for ( Uint32 i = 0; i < displayCount; i++ )
			{
				fullScreenWidth = max( fullScreenWidth, ::wxDisplay(i).GetGeometry().width );
				fullScreenHeight = max( fullScreenHeight, ::wxDisplay(i).GetGeometry().height );
			}
			GRender->RequestResizeRenderSurfaces( fullScreenWidth, fullScreenHeight );
			Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 30 );
			GRender->SetAsyncCompilationMode( true );
			wxTheFrame->GetWorldEditPanel()->GetParent()->Layout();
			GGame->ToggleContignous( FCSF_PNG );
			m_recordingInViewport = false;
		}

		m_csInstance->GetSoundEmitterComponent()->SoundStop();			
			
		m_csInstance->DestroyCs();
		m_csInstance = NULL;

		HACK_DepawnGeralt( GetLayer( m_mode ) );

		if( CWorld* world = GetLayer( m_mode )->GetWorld() )
		{
			world->GetEnvironmentManager()->SetCameraLightsModifiers( SCameraLightsModifiersSetup() );
		}

		DeactiveCsCamera();

		ASSERT( m_mode != M_None );

		ActiveMode( M_None );

		UpdatePointDesc( false );

		UpdateWidgets();
	}
}

void CEdCutsceneEditor::SetTime( Float time )
{
	if ( m_csInstance )
	{
		// Reset shown dialogs
		m_dialogLines.Clear();

		m_newTimeline->SetCurrentTime( time );
		m_csInstance->SetTime( time );

		m_newTimeline->Repaint();
	}
}

void CEdCutsceneEditor::ApplyPreviewActorsEquipments()
{
	if ( m_csInstance )
	{
		TDynArray< CEntity* > entities;
		m_csInstance->GetActors( entities );

		for ( Uint32 i=0; i<entities.Size(); ++i )
		{
			CActor* actor = Cast< CActor >( entities[ i ] );
			if( actor != NULL )
			{
				if ( actor->GetInventoryComponent() && actor != m_preSpawnedActor )
				{
					actor->InitInventory();
					actor->GetInventoryComponent()->SpawnMountedItems();
				}
			}
		}
	}
}

void CEdCutsceneEditor::OnRequestSetTime( wxCommandEvent &event )
{
	TClientDataWrapper< Float>* clientData = dynamic_cast< TClientDataWrapper< Float >* >( event.GetClientObject() );
	ASSERT( clientData != NULL );

	Float time = clientData->GetData();
	
	if ( m_csInstance != NULL )
	{
		m_csInstance->SetTime( time );
	}
}

void CEdCutsceneEditor::OnEventSelected( wxCommandEvent& event )
{
	//CName* name = reinterpret_cast< CName* >( event.GetClientData() );
	//ASSERT( name != NULL );
}

void CEdCutsceneEditor::OnModeMove( wxCommandEvent &event )
{
	m_previewPanel->SetWidgetMode( RPWM_Move );
}

void CEdCutsceneEditor::OnModeRotate( wxCommandEvent &event )
{
	m_previewPanel->SetWidgetMode( RPWM_Rotate );
}

void CEdCutsceneEditor::OnModeChange( wxCommandEvent &event )
{
	if ( m_previewPanel->GetWidgetMode() == RPWM_Move )
	{
		m_previewPanel->SetWidgetMode( RPWM_Rotate );
	}
	else
	{
		m_previewPanel->SetWidgetMode( RPWM_Move );
	}
}

/*
namespace
{
	Float GetFloatValue( const wxTextCtrl* edit )
	{
		String valueStr = edit->GetValue().wc_str();
		Float value = 0.f;
		FromString( valueStr, value );
		return value;
	}
}
*/

void CEdCutsceneEditor::ImportLinesFromSceneFile( const String& filePath )
{
	CStoryScene* scene = LoadResource< CStoryScene >( filePath );
	if ( scene )
	{
		Uint32 sectionsNum = scene->GetNumberOfSections();

		for ( Uint32 i=0; i<sectionsNum; ++i )
		{
			CStorySceneCutsceneSection* csSection = Cast< CStorySceneCutsceneSection >( scene->GetSection( i ) );
			if ( csSection && csSection->GetCsTemplate() && csSection->GetCsTemplate() == m_csTemplate )
			{
				// Found
				csSection->GetDialogLines( m_actorLines );
			}
		}
	}
}

void CEdCutsceneEditor::OnRestart( wxCommandEvent& event )
{
	SetTime( 0.f );

	if ( m_csInstance )
	{
		m_csInstance->StopAllEffects();
	}

	if ( m_csInstance && !m_csInstance->IsLooped() )
	{
		m_csInstance->Pause();
	}
}

void CEdCutsceneEditor::OnChangeRepeat( wxCommandEvent& event )
{
	if ( m_csInstance )
	{
		m_csInstance->SetLooped( !m_csInstance->IsLooped() );
		m_timelineToolbar->ToggleTool( XRCID("repeat"), m_csInstance->IsLooped() );
	}
}

Float CEdCutsceneEditor::GetManualFov() const
{
	wxSlider* slider = XRCCTRL( *this, "sliderCameraFov", wxSlider );
	return (Float)slider->GetValue();
}

SDofParams CEdCutsceneEditor::GetManualDof() const
{
	SDofParams dof;
	dof.overrideFactor = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	dof.dofIntensity = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	dof.dofBlurDistNear = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	dof.dofFocusDistNear = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	dof.dofFocusDistFar = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	dof.dofBlurDistFar = (Float)XRCCTRL( *this, "sliderDof1", wxSlider )->GetValue() / 100.f;
	return dof;
}

void CEdCutsceneEditor::UpdateFOV()
{
	if ( m_csInstance && m_csInstance->GetCamera() )
	{
		String fovStr = String::Printf( TXT("FOV: %.1f"), m_csInstance->GetCamera()->GetFov() );
		m_fovText->SetLabel( fovStr.AsChar() );
	}
}

void CEdCutsceneEditor::UpdateManualCameraOptions()
{
	if ( m_csInstance && m_csInstance->GetCsCamera() )
	{
		CCamera* cam = const_cast< CCamera* >( m_csInstance->GetCsCamera() );

		Float fov = 0.f;

		if ( m_fovOverride->IsChecked() )
		{
			fov = GetManualFov();	
		}
		
		if ( cam )
		{
			cam->SetFov( fov );

			if ( m_dofOverride->IsChecked() )
			{
				SDofParams dof = GetManualDof();
				cam->SetDofParams( dof );
			}
			else
			{
				cam->ResetDofParams();
			}
		}
	}
}

void CEdCutsceneEditor::UpdatePlayPauseButton()
{
	const wxBitmap & icon = 
		m_csInstance 
			? ( m_csInstance->IsPause() ? m_playIcon : m_pauseIcon )
			: m_pauseIcon;

	m_timelineToolbar->SetToolNormalBitmap( XRCID("play"), icon );
}

void CEdCutsceneEditor::OnPlayPause()
{
	if ( m_csInstance )
	{
		Bool pause = m_csInstance->IsPause();

		if ( pause )
		{
			m_csInstance->Unpause();
		}
		else
		{
			m_csInstance->Pause();
		}
	}

	UpdatePlayPauseButton();
}

void CEdCutsceneEditor::OnPlayPause( wxCommandEvent& event )
{
	OnPlayPause();
}

Bool CEdCutsceneEditor::ReloadCutscene()
{
	ASSERT( m_csTemplate );

	if ( m_csInstance )
	{
		wxMessageBox( wxT("Please close cutscene preview first"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return false;
	}

#ifndef NO_EDITOR
	m_csTemplate->FillSkeletonsInAnimationsBasingOnTemplates();
#endif

	CFilePath path( m_csTemplate->GetImportFile() );

	// Get suitable importer for this resource
	IImporter* importer = IImporter::FindImporter( m_csTemplate->GetClass(), path.GetExtension() );

	ASSERT( importer );
	if ( !importer )
	{
		return false;
	}

	// Do the import !
	IImporter::ImportOptions options;
	options.m_existingResource = m_csTemplate;
	options.m_parentObject = m_csTemplate->GetParent();
	options.m_sourceFilePath = m_csTemplate->GetImportFile();
	options.m_params = &m_importParams;
	CResource* imported = importer->DoImport( options );

	Bool ret = true;

	if ( imported )
	{
		// Old resource and imported resource should point to the same location
		ASSERT( imported == m_csTemplate );

		// Set import file 
		imported->SetImportFile( options.m_sourceFilePath );

		// Save this resource for the first time, this will also create thumbnail
		if ( !imported->Save() )
		{
			// Report error
			WARN_EDITOR( TXT("Unable to save reimported cutscene '%s'"), path.GetFileName().AsChar() );

			ret = false;
		}
	}
	else
	{
		// Report error
		GFeedback->ShowError( String::Printf( TXT("Unable to reimport cutscene '%s' from '%s'"), 
			m_csTemplate->GetClass()->GetName().AsString().AsChar(), m_csTemplate->GetImportFile().AsChar() )
			.AsChar() );

		ret = false;
	}

	m_csTemplate->SetValid( false );

	FillTree();
	CheckTemplate();

	m_properties->Get().SetObject( m_csTemplate );

	ResetImportParams();

	m_csTemplate->LogMemoryStats();

	return ret;
}

void CEdCutsceneEditor::OnReimportAndConvToStreamed( wxCommandEvent& event )
{
	static Int32 parts = 15;
	m_importParams.m_maxPartFrames = parts;

	ReloadCutscene();
}

void CEdCutsceneEditor::ResetImportParams()
{
	m_importParams.m_maxPartFrames = -1;
}

void CEdCutsceneEditor::OnReloadCutscene( wxCommandEvent& event )
{
	ReloadCutscene();
}

void CEdCutsceneEditor::OnRecompressAnimations( wxCommandEvent& event )
{
	if ( m_csInstance )
	{
		wxMessageBox( wxT("Please close cutscene preview first"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	// Begin import phase
	GFeedback->BeginTask( TXT("Recompressing animations"), true );

	// Get animations
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

#ifndef NO_EDITOR
	m_csTemplate->FillSkeletonsInAnimationsBasingOnTemplates();
#endif

	CLazyWin32Feedback feedback;

	// Recompress all animations
	for ( Uint32 i=0; i<animations.Size(); i++ )
	{
		// Cancel task
		if ( GFeedback->IsTaskCanceled() )
		{
			break;
		}

		CSkeletalAnimationSetEntry* entry = animations[i];
		if ( entry && entry->GetAnimation() )
		{
			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Recompressing '%s'..."), entry->GetAnimation()->GetName().AsString().AsChar() );
			GFeedback->UpdateTaskProgress( i, animations.Size() );

			if ( entry->GetAnimation() )
			{
				entry->GetAnimation()->Recompress( );
			}
		}
	}

	feedback.ShowAll();

	// End recompress phase
	GFeedback->EndTask();

	m_csTemplate->LogMemoryStats();
}

void CEdCutsceneEditor::OnImportFootStepEvents( wxCommandEvent& event )
{
	if( ! YesNo( TXT( "You are importing foot step events from file. All current foot steps events will be discarded. Continue?") ) )
	{
		return;
	}

	ASSERT( m_csTemplate );

	if ( m_csInstance )
	{
		wxMessageBox( wxT("Please close cutscene preview first"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	String csvFilePath = m_csTemplate->GetImportFile();

	if ( GFileManager->GetFileSize( csvFilePath ) == 0 )
	{
		wxString msg = wxString::Format( wxT("Couldn't find file '%s'"), csvFilePath.AsChar() );
		wxMessageBox( msg, wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	// Copy old event properties
	Bool copyProperties = YesNo( TXT( "Do you want to preserve events properties?" ) );

	if ( m_csTemplate->MarkModified() )
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& animSetEntries = m_csTemplate->GetAnimations();
		for ( Uint32 i=0; i<animSetEntries.Size(); ++i )
		{
			CSkeletalAnimationSetEntry* entry = animSetEntries[ i ];
			if ( entry && entry->GetAnimation() )
			{
				CSkeletalAnimation* animation = entry->GetAnimation();

				const CName& animName = animation->GetName();
				String animFilePath = animation->GetImportFile();


				CFilePath filePath( animFilePath );
				filePath.SetFileName( filePath.GetFileName() + TXT("_footsteps") );
				filePath.GetDirectories().PushBack(TXT("footsteps"));
				animFilePath=filePath.ToString();

				const CExtAnimCutsceneSoundEvent* sourceEvent = NULL;
				TDynArray< CProperty* > sourceProps;

				// Remove old
				TDynArray< CExtAnimCutsceneSoundEvent* > oldEvents;
				entry->GetEventsOfType( oldEvents );

				// Filter events by name (it is important, because now footstep events are the same type as sound events)
				for( Uint32 i = 0; i < oldEvents.Size(); ++i )
				{
					if( oldEvents[ i ]->GetEventName() != CNAME( FootLeft ) 
						&& oldEvents[ i ]->GetEventName() != CNAME( FootRight ) )
					{
						// It is not a footstep event
						oldEvents.RemoveAt( i );
					}
				}

				// Preserve first event to copy its properties later
				if( oldEvents.Size() > 0 )
				{
					sourceEvent = oldEvents[ 0 ];
					
					// Get event properties
					sourceEvent->GetClass()->GetProperties( sourceProps );

				}

				for( TDynArray< CExtAnimCutsceneSoundEvent* >::iterator eventIter = oldEvents.Begin(); eventIter != oldEvents.End(); ++eventIter )
				{
					entry->RemoveEvent( *eventIter );
				}

				if ( GFileManager->GetFileSize( animFilePath ) == 0 )
				{
					WARN_EDITOR( TXT("Couldnt find file '%s'"), animFilePath.AsChar() );
					continue;
				}

#ifdef USE_HAVOK_ANIMATION // VALID
				// Add new
				{
					TDynArray< CHKXAnimImporter::SFootStepEventData > events;
					if ( CHKXAnimImporter::GetFootStepEventsFromAnimation( animFilePath, events ) )
					{
						for ( Uint32 j=0; j<events.Size(); ++j )
						{
							CHKXAnimImporter::SFootStepEventData& footEvent = events[ j ];

							if ( footEvent.m_type == CHKXAnimImporter::SFootStepEventData::T_Strike )
							{
								CName name = footEvent.m_foot == CHKXAnimImporter::SFootStepEventData::F_Left ? 
									CNAME( FootLeft ) : CNAME( FootRight );

								CExtAnimCutsceneSoundEvent* newEvent = new CExtAnimCutsceneSoundEvent();

								if( copyProperties && sourceEvent != NULL )
								{
									// Copy properties
									for( TDynArray< CProperty* >::iterator propIter = sourceProps.Begin();
										propIter != sourceProps.End(); ++propIter )
									{
										CProperty* prop = *propIter;

										// Replace value
										prop->Set( newEvent, prop->GetOffsetPtr( sourceEvent ) );
									}
								}

								// Override needed properties
								newEvent->SetEventName( name );
								newEvent->SetAnimationName( animName );
								newEvent->SetTrackName( TXT( "FootSteps" ) );
								newEvent->SetStartTime( footEvent.m_time );
								newEvent->SetBoneName( ( footEvent.m_foot == CHKXAnimImporter::SFootStepEventData::F_Left ) ? 
									CName( TXT( "l_toe" ) ) : CName( TXT( "r_toe" ) ) );

								entry->AddEvent( newEvent );
							}
						}
					}
				}
#endif
			}
		}
	}
}

void CEdCutsceneEditor::OnImportEvents( wxCommandEvent& event )
{
	if( ! YesNo( TXT( "You are importing dialog events from external file. All current dialog events will be discarded. Continue?") ) )
	{
		return;
	}

	ASSERT( m_csTemplate );

	if ( m_csInstance )
	{
		wxMessageBox( wxT("Please close cutscene preview first"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	CEdFileDialog dlg;
	dlg.ClearFormats();
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CsImportEvents") );

	if ( !dlg.DoOpen( (HWND) GetHandle(), true ) ) 
	{
		return;
	}

	String absFilePath = dlg.GetFile();

	// Parse events file
	C2dArray* eventArray = C2dArray::CreateFromString( absFilePath );
	if ( !eventArray )
	{
		wxMessageBox( wxT("Unable to load file"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	// Get columns index
	Int32 colTracks = eventArray->GetColumnIndex( TXT("event_name") );
	Int32 colEvents = eventArray->GetColumnIndex( TXT("event_time") );

	// Check index
	if ( colTracks == -1 )
	{
		eventArray->Discard();
		String msg = String::Printf( TXT("Unable to find tracks's column 'event_name' in file %s!"), absFilePath.AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}
	if ( colEvents == -1 )
	{
		eventArray->Discard();
		String msg = String::Printf( TXT("Unable to find events's column 'event_time' in file %s!"), absFilePath.AsChar() );
		wxMessageBox( msg.AsChar(), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	// Get row num
	Uint32 rows = eventArray->GetNumberOfRows();
	if ( rows == 0 )
	{
		eventArray->Discard();
		wxMessageBox( wxT("Tracks list is empty"), wxT("Warning"), wxOK | wxCENTRE | wxICON_WARNING, this );
		return;
	}

	// Remove current dialog events
	TDynArray< CExtAnimCutsceneDialogEvent* > dialogEvents;
	m_csTemplate->GetEventsOfType( dialogEvents );
	for( TDynArray< CExtAnimCutsceneDialogEvent* >::iterator eventIter = dialogEvents.Begin();
		eventIter != dialogEvents.End(); ++eventIter )
	{
		m_csTemplate->RemoveEvent( *eventIter );
	}

	// Parse file
	for ( Uint32 i=0; i<rows; i++ )
	{
		const String& trackName = eventArray->GetValue( colTracks, i );

		if( trackName != TXT( "Dialog" ) )
		{
			continue;
		}

		// Get times
		String eventListStr = eventArray->GetValue( colEvents, i );
		TDynArray< String > eventTimes = eventListStr.Split( TXT("-") );

		for ( Uint32 j=0; j<eventTimes.Size(); ++j )
		{
			const String& str = eventTimes[j];
			if ( !str.Empty() )
			{
				Float time;
				Bool ret = FromString( str, time );
				if  ( ret )
				{
					CExtAnimCutsceneDialogEvent* event = 
						new CExtAnimCutsceneDialogEvent( CName( String::Printf( TXT( "Dialog_%.2f" ), time ) ),
						CCutsceneTemplate::CUTSCENE_ANIMATION_NAME, time, TXT( "Dialog" ) );
					m_csTemplate->AddEvent( event );
					m_csTemplate->MarkModified();
				}
			}
		}
	}

	// Discard definition array
	eventArray->Discard();

	m_newTimeline->Recreate();
}

void CEdCutsceneEditor::OnTimeMulSliderUpdate( wxCommandEvent& event )
{
	Float var = ((Float)(m_timeMulSlider->GetValue()) / 100.0f);
	m_timeMul = var;

	wxString varStr = wxString::Format(wxT("%d"), m_timeMulSlider->GetValue());
	m_timeMulEdit->SetLabel(varStr);
}

void CEdCutsceneEditor::OnTimeSliderMulUpdating( wxCommandEvent& event )
{
	Int32 var = m_timeMulSlider->GetValue();
	wxString varStr = wxString::Format(wxT("%d"), var);
	m_timeMulSlider->GetToolTip()->SetTip( varStr );
	m_timeMulEdit->SetLabel(varStr);
}

void CEdCutsceneEditor::OnTimeMulEditUpdate( wxCommandEvent& event )
{
	String strVal = m_timeMulEdit->GetValue().wc_str();
	if (strVal.Empty()) strVal = TXT("0");
	Int32 newTimeMul;
	FromString(strVal, newTimeMul);

	m_timeMul = ((Float)newTimeMul)/100.f;

	m_timeMulSlider->SetValue( newTimeMul );
}

void CEdCutsceneEditor::SelectAnimation( const CName& animationName, Float duration )
{
	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	if( animationName == CCutsceneTemplate::CUTSCENE_ANIMATION_NAME )
	{
		const CSkeletalAnimationSetEntry* animation =  animations.Size() > 0 ? animations[0] : nullptr;
		m_newTimeline->SetTrack( CCutsceneTemplate::CUTSCENE_ANIMATION_NAME, duration, m_csTemplate, animation, m_csTemplate );
	}
	else
	{
		CSkeletalAnimationSetEntry* animation = m_csTemplate->FindAnimation( animationName );
		m_newTimeline->SetTrack( animationName, duration, animation, animation, m_csTemplate );
	}

	m_selectAnimation = animationName;

	TDynArray< CSkeletalAnimation* > prop1;

	for ( Uint32 i=0; i<animations.Size(); ++i )
	{
		CSkeletalAnimationSetEntry *currAnim = animations[ i ];
		if ( currAnim->GetAnimation() && currAnim->GetAnimation()->GetName() == animationName )
		{
			prop1.PushBack( currAnim->GetAnimation() );
		}
	}

	if (!prop1.Empty())
	{
		m_animProperties->SetObjects(prop1);
	}
	else
	{
		m_animProperties->SetNoObject();
	}
}

void CEdCutsceneEditor::FillTree()
{
	m_animTree->DeleteAllItems();

	m_animTree->AddRoot( TXT("Cutscene"), -1, -1, new ObjectItemWrapper( m_csTemplate ) );

	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	for( Uint32 i=0; i<animations.Size(); ++i )
	{
		CSkeletalAnimationSetEntry *currAnim = animations[ i ];
		if ( currAnim->GetAnimation() )
		{
			wxString itemName( currAnim->GetAnimation()->GetName().AsString().AsChar() );

			m_animTree->AppendItem( m_animTree->GetRootItem() , itemName, -1, -1, new SerializableItemWrapper( currAnim ) );
		}
	}

	m_animTree->ExpandAll();

	SelectAnimation( CName::NONE, 0.0f );
}

void CEdCutsceneEditor::UpdatePointDesc( Bool show, const Matrix& point, const String& inputdesc, Uint32 points )
{
	wxStaticText* desc = XRCCTRL( *this, "tagPointDesc", wxStaticText );
	wxTextCtrl* pos = XRCCTRL( *this, "tagPointPos", wxTextCtrl );
	wxTextCtrl* rot = XRCCTRL( *this, "tagPointRot", wxTextCtrl );

	if ( show )
	{
		if ( points == 0 )
		{
			desc->SetLabel( wxT("Tag is empty\n") );
			pos->SetLabel( wxT("") );
			rot->SetLabel( wxT("") );
		}
		else
		{
			String str = String::Printf( TXT("Found %d tags\n"), points ) + inputdesc;
			desc->SetLabel( str.AsChar() );

			Vector ppos = point.GetTranslation();
			EulerAngles prot = point.ToEulerAngles();

			String posStr = String::Printf( TXT("%.2f; %.2f; %.2f"), ppos.X, ppos.Y, ppos.Z );
			String rotStr = String::Printf( TXT("%.2f; %.2f; %.2f"), prot.Pitch, prot.Roll, prot.Yaw );

			pos->SetLabel( posStr.AsChar() );
			rot->SetLabel( rotStr.AsChar() );
		}

		desc->Enable( true );
		pos->Enable( true );
		rot->Enable( true );
	}
	else
	{
		desc->SetLabel( wxT("Tag point\n") );
		pos->SetLabel( wxT("") );
		rot->SetLabel( wxT("") );

		desc->Enable( false );
		pos->Enable( false );
		rot->Enable( false );
	}
}

void CEdCutsceneEditor::ShowCutscenePage()
{

}

void CEdCutsceneEditor::ShowTemplatePage()
{

}

void CEdCutsceneEditor::SetupAnimationPage()
{
	ASSERT( m_csTemplate );

	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	Bool foundAnim = false;

	// fill anim list and choose one for panel
	m_animPanelChoice->Freeze();
	m_animPanelChoice->Clear();

	for( Uint32 i=0; i<animations.Size(); ++i )
	{
		CSkeletalAnimationSetEntry *currAnim = animations[ i ];
		if ( currAnim->GetAnimation() )
		{
			m_animPanelChoice->Append( currAnim->GetAnimation()->GetName().AsString().AsChar(), currAnim );
			if ( currAnim->GetAnimation()->GetName() == m_selectAnimation )
			{
				// will set up properties panel
				SelectAnimation(m_selectAnimation, currAnim->GetDuration());
				m_animPanelChoice->SetStringSelection( currAnim->GetAnimation()->GetName().AsString().AsChar() );
				foundAnim = true;
			}
		}
	}

	m_animPanelChoice->Thaw();

	if (! foundAnim)
	{
		m_animProperties->SetNoObject();
	}
}

void CEdCutsceneEditor::ShowDialogPage()
{

}

void CEdCutsceneEditor::ShowScenesPage()
{

}

void CEdCutsceneEditor::ShowCameraPage()
{

}

void CEdCutsceneEditor::OnAnimTreeDblClick( wxTreeEvent &event )
{
	// Przlacz na zakladke a animacjami
}

void CEdCutsceneEditor::OnAnimTreeSelected( wxTreeEvent &event )
{
	wxTreeItemId selectedId = event.GetItem();

	SerializableItemWrapper* animData = (SerializableItemWrapper* )m_animTree->GetItemData( selectedId );
	ASSERT( animData );
	ASSERT( animData->m_object );

	if ( animData && animData->m_object )
	{
		CSkeletalAnimationSetEntry* entry = Cast< CSkeletalAnimationSetEntry >( animData->m_object );

		if ( entry )
		{
			SelectAnimation( CName( entry->GetName() ), entry->GetDuration() );
		}
		else
		{
			SelectAnimation( CCutsceneTemplate::CUTSCENE_ANIMATION_NAME, m_csTemplate->GetDuration() );
		}
	}
}

void CEdCutsceneEditor::OnDispSkeletons( wxCommandEvent& event )
{
	Bool curVal = m_previewPanel->ToggleDispSkeletons();
	m_timelineToolbar->ToggleTool( XRCID("skeleton"), curVal );
}

void CEdCutsceneEditor::CheckTemplate()
{
	wxStaticText* errorText = XRCCTRL( *this, "csError", wxStaticText );
	wxStaticBitmap* errorImg = XRCCTRL( *this, "csErrorImg", wxStaticBitmap );
	wxBitmapButton* errorButt = XRCCTRL( *this, "csErrorDesc", wxBitmapButton );

	ASSERT( m_csTemplate );

	if ( m_csTemplate->IsValid() )
	{
		wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_CHECKED_OUT" ) );

		errorText->SetLabel( wxT("Cuscene template is valid") );
		errorImg->SetBitmap( bitmap );
	}
	else
	{
		wxBitmap bitmap = SEdResources::GetInstance().LoadBitmap( TEXT( "IMG_ERROR" ) );

		errorText->SetLabel( wxT("Cuscene template is invalid") );
		errorImg->SetBitmap( bitmap );
	}

	errorButt->Enable( true );
}

void CEdCutsceneEditor::CheckMotionEx( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const
{
	Uint32 size = animations.Size();

	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i,size );

		CSkeletalAnimationSetEntry* anim = animations[i];

		if ( anim && anim->GetAnimation() && anim->GetAnimation()->HasExtractedMotion() )
		{

		}		
	}
}

void CEdCutsceneEditor::OnCheckActorsTemplates( wxCommandEvent& event )
{
	TDynArray< String > actorNames;
	m_csTemplate->GetActorsName( actorNames );

	Uint32 size = actorNames.Size();

	Bool canModify = false;
	String msg;

	for ( Uint32 i=0; i<size; ++i )
	{
		const String& actor = actorNames[i];

		SCutsceneActorDef* aDef = m_csTemplate->GetActorDefinition( actor );
		ASSERT( aDef );
		if ( !aDef )
		{
			continue;
		}

		CEntityTemplate* templ = aDef->m_template.Get();

		if ( templ )
		{
			const CName& className = templ->GetEntityClassName();

			CClass* entClass = SRTTI::GetInstance().FindClass( className );
			ASSERT( entClass );

			if ( entClass->IsA< CNewNPC >() )
			{
				String oldPath = templ->GetFile()->GetDepotPath().StringBefore( TXT("."), true );
				String newPath = oldPath + TXT("_appearances.w2ent");
				CResource* res = GDepot->FindResource( newPath );

				if ( canModify == false )
				{
					if ( m_csTemplate->MarkModified() == false )
					{
						wxMessageBox( wxT("Resource couldn't be modify!"), wxT("Error") );
						return;
					}
					else
					{
						canModify = true;
					}
				}

				if ( res )
				{
					CEntityTemplate* newTempl = SafeCast< CEntityTemplate >( res );
					if ( newTempl )
					{
						aDef->m_template = newTempl;
					}
				}
				else
				{
					msg += String::Printf( TXT("Actor '%s' is CNewNPC and resource '%s' is not existed\n"), actor.AsChar(), newPath.AsChar() );
					m_csTemplate->SetValid( false );
				}
			}
		}
		else
		{
			msg += String::Printf( TXT("Actor '%s' hasn't got template!\n"), actor.AsChar() );
			m_csTemplate->SetValid( false );
		}
	}

	if ( msg.Empty() == false )
	{
		wxMessageBox( msg.AsChar(), wxT("Message") );
	}

	if ( canModify )
	{
		m_csTemplate->Save();
	}
	
	CheckTemplate();
}

void CEdCutsceneEditor::CheckAndFillActors( String& msg ) const
{
	TDynArray< String > actorNames;
	m_csTemplate->GetActorsName( actorNames );

	Uint32 size = actorNames.Size();

	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i,size );

		const String& actor = actorNames[i];

		SCutsceneActorDef* aDef = m_csTemplate->GetActorDefinition( actor );
		ASSERT( aDef );
		if ( !aDef )
		{
			continue;
		}

		CEntityTemplate* templ = aDef->m_template.Get();

		if ( !templ )
		{
			msg += String::Printf( TXT("Actor %s hasn't got template\n"), actor.AsChar() );
		}

		if ( aDef->m_type == CAT_None )
		{
			ECutsceneActorType exType = CAT_None;

			if ( templ )
			{
				exType = CCutsceneTemplate::ExtractActorTypeFromTemplate( templ );
			}

			if ( exType != CAT_None )
			{
				aDef->m_type = exType;
			}
			else
			{
				msg += String::Printf( TXT("Actor %s has got type 'None'\n"), actor.AsChar() );
			}
		}
	}
}

void CEdCutsceneEditor::CheckBBox( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const
{
	for ( Uint32 i=0; i<animations.Size(); ++i )
	{
		if ( animations[i] && animations[i]->GetAnimation() && !animations[i]->GetAnimation()->HasBoundingBox() )
		{
			msg += String::Printf( TXT("Animation %s hasn't got bounding box\n"), animations[i]->GetName().AsString().AsChar() );
		}
	}
}

void CEdCutsceneEditor::CheckComponents( String& msg ) const
{
	
}

void CEdCutsceneEditor::CheckTrajectories( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const
{
	
}

void CEdCutsceneEditor::CheckAnimData( String& msg, const TDynArray< CSkeletalAnimationSetEntry* >& animations ) const
{
	Uint32 size = animations.Size();

	for ( Uint32 i=0; i<size; ++i )
	{
		GFeedback->UpdateTaskProgress( i,size );

		CSkeletalAnimationSetEntry* anim = animations[i];

		if ( !anim->GetAnimation() )
		{
			msg += String::Printf( TXT("Animation %d hasn't got animation data\n"), i );
			continue;
		}
	}
}

String CEdCutsceneEditor::FindErrors( Bool deep )
{
	String err;

	GFeedback->BeginTask( TXT("Check cutscene"), false );

	TDynArray< CSkeletalAnimationSetEntry* > animations;
	m_csTemplate->GetAnimations( animations );

	GFeedback->UpdateTaskInfo( TXT("Checking animation data...") );
	CheckAnimData( err, animations );

	GFeedback->UpdateTaskInfo( TXT("Checking motion extraction...") );
	CheckMotionEx( err, animations );

	GFeedback->UpdateTaskInfo( TXT("Checking actors template...") );
	CheckAndFillActors( err );

	if ( deep )
	{
		GFeedback->UpdateTaskInfo( TXT("Checking components...") );
		CheckComponents( err );

		GFeedback->UpdateTaskInfo( TXT("Checking trajectories...") );
		CheckTrajectories( err, animations );
	}

	if ( err.Empty() )
	{
		GFeedback->UpdateTaskInfo( TXT("Checking cutscene instance...") );
		CLayer* layer = m_previewPanel->GetPreviewWorld()->GetDynamicLayer();
		String errors;

		CCutsceneInstance* tempInstance = m_csTemplate->CreateInstance( layer, Matrix::IDENTITY, errors, NULL, new CCutsceneEmptyWorldInterface( m_previewPanel->GetPreviewWorld() ) );
		if ( tempInstance )
		{
			tempInstance->DestroyCs();
		}

		err += errors;
	}

	if ( !err.Empty() )
	{
		err += TXT("\nPlease reimport");
	}

	GFeedback->EndTask();

	return err;
}

Bool CEdCutsceneEditor::Validate()
{
	String err = FindErrors( true );
	if ( err.Empty() )
	{
		m_csTemplate->SetValid( true );
	}
	else
	{
		wxMessageBox( err.AsChar(), wxT("Error"), wxOK | wxCENTRE | wxICON_ERROR, this );
		m_csTemplate->SetValid( false );
	}

	CheckTemplate();

	return err.Empty();
}

void CEdCutsceneEditor::OnErrorDesc( wxCommandEvent& event )
{
	Validate();
}

void CEdCutsceneEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( ActiveWorldChanging ) )
	{
		if ( m_mode == M_Game && m_csInstance )
		{
			DestroyCutsceneInstance();
		}
	}
	else if ( name == CNAME( WorldLoaded ) || name == CNAME( WorldUnloaded ) )
	{
		CheckTemplate();
		UpdateWidgets();
	}
	else if ( name == CNAME( EditorPropertyPostChange ) )
	{
        const CEdPropertiesPage::SPropertyEventData& eventData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( eventData.m_typedObject.m_class == ClassID< CExtAnimCutsceneEffectEvent >() )
		{
			CExtAnimCutsceneEffectEvent* effectEvent = static_cast< CExtAnimCutsceneEffectEvent* >( eventData.m_typedObject.m_object );
			THandle< CEntity > effect;
			if ( m_csInstance && m_csInstance->GetEffectMap().Find( effectEvent, effect ) )
			{
				effect.Get()->SetPosition( effectEvent->GetSpawnPos() );
				effect.Get()->SetRotation( effectEvent->GetSpawnRot() );
			}
			RefreshItems();
		}
	}
}

CWorld* CEdCutsceneEditor::GetWorld() const
{
	if ( m_mode == M_Game )
	{
		return GGame->GetActiveWorld();
	}
	else if ( m_mode == M_Preview )
	{
		return m_previewPanel->GetPreviewWorld();
	}

	return NULL;
}

IViewport* CEdCutsceneEditor::GetViewport() const
{
	if ( m_mode == M_Game )
	{
		return GGame->GetViewport();
	}
	else if ( m_mode == M_Preview )
	{
		return m_previewPanel->GetViewport();
	}

	return NULL;
}

void CEdCutsceneEditor::TakeScreenshot( const String &destinationFile ) const
{
	IViewport* viewPort = GetViewport();

	TDynArray< Color > buffer;

	Uint32 width = viewPort->GetWidth();
	Uint32 height = viewPort->GetHeight();

	wxImage targetImage( width, height );
	Uint8 *charBuffer = targetImage.GetData();

	viewPort->GetPixels( 0, 0, width, height, buffer );
	for( Uint32 y = 0; y < height; y++ )
	{
		Uint8 *line = &charBuffer[ y * width * 3 ];
		Color *src = &buffer[ y * width ];

		for( Uint32 x = 0; x < width; x++, line+=3, src++ )
		{
			line[0] = src->R;
			line[1] = src->G;
			line[2] = src->B;
		}
	}

	wxBitmap bitmap( targetImage );

	bitmap.SaveFile( wxString( destinationFile.AsChar() ), wxBITMAP_TYPE_BMP );

}

void CEdCutsceneEditor::OnDumpCutscene( wxCommandEvent& event )
{
	static Int32 index = 6, width, height, fps = 0;
	static Bool uber = true;
	static struct { int w, h; } res[10] = { {384, 216},
											{512, 288},
											{640, 360},
											{768, 432},
											{1024, 576},
											{1280, 720},
											{1920, 1080},
											{2560, 1440},
											{3840, 2160},
											{7680, 4320} };
	if ( index != 10 )
	{
		width = res[index].w;
		height = res[index].h;
	}

	if ( m_recordingInViewport )
	{
		wxMessageBox( wxT("Already recording"), wxT("Error"), wxICON_ERROR|wxOK|wxCENTRE );
		return;
	}

	if ( FormattedDialogBox( wxT("Record Cutscene"), wxT("V'Resolution'{T{'Predefined:'C('384x216''512x288''640x360''768x432''1024x576''1280x720''1920x1080''2560x1440''3840x2160''7680x4320''Custom')'...or custom:'|'Width:'I'Height:'I}}|V'Framerate'{'Predefined:'C('30fps''60fps''120fps')}|V{X'Ubersample'}|H{~B@'Record'|B'Cancel'}"), &index, &width, &height, &fps, &uber ) == 0 )
	{
		if ( index != 10 )
		{
			width = res[index].w;
			height = res[index].h;
		}

		SRecordingInfo info;
		info.width = width;
		info.height = height;
		info.rate = fps;
		info.ubersample = uber;
		if ( CreateCutsceneInstance( M_Game, &info ) )
		{
			m_playGameButt->SetLabel( wxT("Unbind") );
			m_playGameButt->SetValue( true );
		}
	}
}

void CEdCutsceneEditor::CreateAvi( const String& csName , const String& folderName, Uint32 frameNumber ) const
{
	String avsContent = TXT( "ImageSource(\"" );
	avsContent += csName;
	avsContent += TXT( "%04d.bmp\", 0, " );
	avsContent += String::Printf( TXT("%d"), frameNumber - 1 ) ;
	avsContent += TXT( ", 30)" );
	String avsName = folderName + csName;
	avsName += TXT( ".avs" );
	String pureAvsName = csName;
	pureAvsName += TXT( ".avs" );

	GFileManager->SaveStringToFile( avsName, avsContent );

	ShellExecute( NULL, NULL, TXT( "cs2avi.bat" ), pureAvsName.AsChar(), folderName.AsChar(), SW_HIDE);
}

void CEdCutsceneEditor::OnEffectReset( wxCommandEvent& event )
{
	if ( m_csInstance )
	{
		m_csInstance->StopAllEffects();
	}
}

void CEdCutsceneEditor::OnDofIntCheck( wxCommandEvent& event )
{

}

void CEdCutsceneEditor::OnDofOverCheck( wxCommandEvent& event )
{

}

void CEdCutsceneEditor::OnGenerateBBox( wxCommandEvent& event )
{
	GenerateBBox();
}

void CEdCutsceneEditor::OnToggleFloor( wxCommandEvent& event )
{
	m_showFloor = !m_showFloor;
}

namespace
{
	const CSkeletalAnimationSetEntry* FindAnimInternal( const TDynArray< CSkeletalAnimationSetEntry* >& anims, CName animName )
	{
		for ( const CSkeletalAnimationSetEntry* a : anims )
		{
			if ( a->GetName() == animName )
			{
				return a;
			}
		}
		return nullptr;
	}
}

Bool CEdCutsceneEditor::GenerateBBox()
{
	if ( !m_csInstance )
	{
		wxMessageBox( wxT("Please create cutscene first"), wxT("Warning"), wxOK | wxCENTRE, this );
	}
	else if ( m_csTemplate->MarkModified() )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		m_csTemplate->GetAnimations( animations );

		TDynArray< const CAnimatedComponent* > csActorsAcs;
		TDynArray< CName > csActorsAnims;
		m_csInstance->CollectAllPlayableActorsWithAnims( csActorsAcs, csActorsAnims );

		GFeedback->BeginTask( TXT("Generate bounding box"), false );

		const Uint32 size = csActorsAcs.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			GFeedback->UpdateTaskProgress( i, size );
			GFeedback->UpdateTaskInfo( TXT("Animation %s..."), csActorsAnims[ i ].AsChar() );

			if ( const CSkeletalAnimationSetEntry* anim = FindAnimInternal( animations, csActorsAnims[ i ] ) )
			{
				const CAnimatedComponent* component = csActorsAcs[i];
				ASSERT( component );

				const Bool ret = anim->GetAnimation()->GenerateBoundingBox( component );
				if ( !ret )
				{
					GFeedback->EndTask();
					String animErr = TXT("Couldn't generate bounding box for animation ") + csActorsAnims[ i ];

					wxMessageBox( animErr.AsChar(), wxT("Error"), wxOK | wxCENTRE, this );

					return false;
				}
			}
		}

		GFeedback->EndTask();
	}

	return true;
}

void CEdCutsceneEditor::OnSelectItem( IPreviewItem* item )
{
	CName name( item->GetName() );
	m_newTimeline->SelectEvent( name );
	m_newTimeline->Repaint();
}

void CEdCutsceneEditor::OnDeselectAllItem()
{
}

void CEdCutsceneEditor::OnItemTransformChangedFromPreview( IPreviewItem* item )
{
	m_newTimeline->RefreshPropPageValues();
}

void CEdCutsceneEditor::RecreateEffectPreviewItems()
{
	// World can be changed - have to recreate container
	DestroyItemContainer();

	if ( m_csInstance )
	{
		InitItemContainer();

		// Add items
		TDynArray< const CExtAnimCutsceneEffectEvent* > events;
		m_csTemplate->GetEventsOfType( events );

		for( TDynArray< const CExtAnimCutsceneEffectEvent* >::const_iterator eventIter = events.Begin();
			eventIter != events.End(); ++eventIter )
		{
			const CExtAnimCutsceneEffectEvent* event = *eventIter;
			ASSERT( event != NULL );

			String eventName = event->GetEventName().AsString();

			if ( !HasItem( eventName ) )
			{
				// Create new
				CEffectPreviewItem* item = new CEffectPreviewItem( this );
				item->Init( eventName );
				AddItem( item );
			}
		}

		// Update
		RefreshItems();
	}
}

void CEdCutsceneEditor::SetCachets()
{
	IViewport* viewport = GetViewport();
	if ( viewport )
	{
		viewport->AdjustSizeWithCachets( wxTheFrame->GetWorldEditPanel()->GetViewportCachetAspectRatio() );
	}
	else 
	{
		ASSERT( viewport );
	}
}

void CEdCutsceneEditor::ResetCachets()
{
	IViewport* viewport = GetViewport();
	if ( viewport )
	{
		viewport->RestoreSize();
	}
}

void CEdCutsceneEditor::OnSceneColRightClick( wxListEvent& event )
{
	wxListItem item = event.GetItem();
	if ( item.GetId() != -1 )
	{
		// Show popupmenu at position
		wxMenu menu( wxT("Scene") );

		menu.Append( ID_SCENE_LIST_SHOW , _T("Show") );
		menu.Append( ID_SCENE_LIST_OPEN , _T("Open") );
		menu.Append( ID_SCENE_LIST_LINES , _T("Get lines") );
		menu.Append( ID_SCENE_LIST_REMOVE , _T("Remove") );

		wxPoint point;
		m_sceneList->GetItemPosition( item.GetId(), point );

		PopupMenu( &menu );
	}
}

void CEdCutsceneEditor::OnSceneListShow( wxCommandEvent& event )
{
	Int32 selCount = m_sceneList->GetSelectedItemCount();
	
	if ( selCount >= 1 )
	{
		ASSERT( selCount == 1 );

		const TDynArray< String >& files = m_csTemplate->GetFilesWithThisCutscene();

		long item = m_sceneList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item != -1 && item >= 0 && (long)files.Size() > item )
		{
			const String& filePath = files[ item ];
			wxTheFrame->GetAssetBrowser()->SelectFile( filePath );	
		}
	}
	else
	{
		ASSERT( selCount == 0 );
	}
}

void CEdCutsceneEditor::OnSceneListOpen( wxCommandEvent& event )
{
	Int32 selCount = m_sceneList->GetSelectedItemCount();

	if ( selCount >= 1 )
	{
		ASSERT( selCount == 1 );

		const TDynArray< String >& files = m_csTemplate->GetFilesWithThisCutscene();

		long item = m_sceneList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item != -1 && item >= 0 && (long)files.Size() > item )
		{
			const String& filePath = files[ item ];
			wxTheFrame->GetAssetBrowser()->OpenFile( filePath );	
		}
	}
	else
	{
		ASSERT( selCount == 0 );
	}
}

void CEdCutsceneEditor::OnSceneListLines( wxCommandEvent& event )
{
	Int32 selCount = m_sceneList->GetSelectedItemCount();

	if ( selCount >= 1 )
	{
		ASSERT( selCount == 1 );

		const TDynArray< String >& files = m_csTemplate->GetFilesWithThisCutscene();

		long item = m_sceneList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item != -1 && item >= 0 && (long)files.Size() > item )
		{
			const String& filePath = files[ item ];
		
			ImportLinesFromSceneFile( filePath );
		}
	}
	else
	{
		ASSERT( selCount == 0 );
	}	
}

void CEdCutsceneEditor::OnSceneListRemove( wxCommandEvent& event )
{
	Int32 selCount = m_sceneList->GetSelectedItemCount();

	if ( selCount >= 1 )
	{
		ASSERT( selCount == 1 );

		const TDynArray< String >& files = m_csTemplate->GetFilesWithThisCutscene();

		long item = m_sceneList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
		if ( item != -1 && item >= 0 && (long)files.Size() > item )
		{
			const String filePath = files[ item ];
			if ( m_csTemplate->ReleaseFileWithThisCutscene( filePath ) == false )
			{
				// Error
				ASSERT( 0 );
			}

			FillScenePanel();
		}
	}
	else
	{
		ASSERT( selCount == 0 );
	}
}

void CEdCutsceneEditor::OnDumpAllVoiceoversToFile( wxCommandEvent &event )
{
	CEdFileDialog dlg;
	dlg.SetMultiselection( false );
	dlg.SetIniTag( TXT("CEdCutsceneEditor_OnDumpAllVoiceoversToFile") );
	dlg.AddFormat( TXT("csv"), TXT( "CSV" ) );

	if ( dlg.DoSave( (HWND)GetHandle() ) )
	{				
		const String filePath = dlg.GetFile();

		String data;

		data += String::Printf( TXT("%s\n"), m_csTemplate->GetFile()->GetFileName().AsChar() );

		for ( Uint32 i=0; i<m_actorLines.Size(); ++i )
		{
			const SCutsceneActorLine& line = m_actorLines[ i ];

			data += String::Printf( TXT("%s\n"), line.m_sound.AsChar() );
		}

		GFileManager->SaveStringToFile( filePath, data );
	}
}

void CEdCutsceneEditor::OnAnimChanged( wxCommandEvent &event )
{
	Int32 selection = m_animPanelChoice->GetSelection();

	if ( selection != wxNOT_FOUND )
	{
		if ( CSkeletalAnimationSetEntry* selectedAnim = (CSkeletalAnimationSetEntry*)m_animPanelChoice->GetClientData( selection ) )
		{
			if ( selectedAnim->GetAnimation() )
			{
				SelectAnimation( selectedAnim->GetAnimation()->GetName(), selectedAnim->GetDuration());
			}
		}
	}

	SetupAnimationPage();
}

// ---------------------------------------------------------------------------

void CEdCutsceneEditor::UpdateEffectList()
{
	ASSERT( m_effectsList && m_csTemplate );
	m_effectsList->Freeze();
	m_effectsList->Clear();

	TDynArray< CFXDefinition* > effects = m_csTemplate->GetEffects();
	for ( auto efI = effects.Begin(); efI != effects.End(); ++efI )
	{
		CFXDefinition* effect = *efI;
		m_effectsList->Append( effect->GetName().AsChar() );
	}

	m_effectsList->Thaw();
}

void CEdCutsceneEditor::EditEffect( CFXDefinition* effect )
{
	wxPanel* parent = XRCCTRL( *this, "effectEditor", wxPanel );	
	parent->Freeze();

	if ( m_effectEditor )
	{
		if ( effect && m_effectEditor->GetFXDefinition() == effect )
		{
			parent->Thaw();
			return; // already being edited - nothing to do 
		}
		else
		{
			m_effectEditor->Destroy();
			m_effectEditor = nullptr;
		}
	}

	if ( effect )
	{
		m_effectEditor = new CEdEffectEditor( parent, m_csInstance, nullptr, effect );
		parent->GetSizer()->Add( m_effectEditor, 1, wxEXPAND );
	}

	parent->Layout();
	parent->Thaw();
}

void CEdCutsceneEditor::OnEffectChange( wxCommandEvent& event )
{
	int sel = event.GetSelection();
	if ( sel >= 0 )
	{
		const TDynArray< CFXDefinition* >& effects = m_csTemplate->GetEffects();
		if ( sel < effects.SizeInt() )
		{
			EditEffect( effects[ sel ] );
		}
	}
	else
	{
		EditEffect( nullptr );
	}
}

void CEdCutsceneEditor::OnAddEffect( wxCommandEvent& event )
{
	ASSERT( m_csTemplate );
	
	wxMenu menu( TXT("") );
	menu.Append( ID_MENU_ADD_EFFECT_EMITTER, TXT("Add particle emitter") );
	menu.Append( ID_MENU_ADD_EFFECT_CUSTOM, TXT("Add custom effect") );

	wxBitmapButton* addBtn = XRCCTRL( *this, "addEffect", wxBitmapButton );
	addBtn->PopupMenu( &menu, 0, addBtn->GetSize().y );
}

CFXDefinition* CEdCutsceneEditor::DoAddEffect()
{
	if ( m_csTemplate->MarkModified() )
	{
		wxString wxName = ::wxGetTextFromUser( TXT("Name of the effect to add:"), TXT("Add effect"), wxEmptyString, this );
		if ( !wxName.IsEmpty() )
		{
			String effectName = String( wxName.wc_str() );

			if ( m_csTemplate->HasEffect( CName( effectName ) ) )
			{
				GFeedback->ShowError( TXT("There is already an effect with this name in the cutscene") );
			}
			else
			{
				if ( CFXDefinition* newEffect = m_csTemplate->AddEffect( effectName ) ) 
				{
					UpdateEffectList();
					m_effectsList->Select( m_effectsList->GetCount()-1 );
					return newEffect;
				}
			}
		}
	}

	return nullptr;
}

void CEdCutsceneEditor::OnAddEffectCustom( wxCommandEvent& event )
{
	if ( CFXDefinition* fxDef = DoAddEffect() )
	{
		EditEffect( fxDef );
	}
}

void CEdCutsceneEditor::OnAddEffectEmitter( wxCommandEvent& event )
{
	if ( CFXDefinition* fxDef = DoAddEffect() )
	{
		CFXTrackGroup* group = fxDef->AddTrackGroup( TXT("Default") );
		CFXTrack* track = group->AddTrack( TXT("Emitter") );

		CFXTrackItemParticles* item = CreateObject< CFXTrackItemParticles >( track );
		item->SetSpawner( CreateObject< CFXSimpleSpawner >( item ) );
		track->AddTrackItem( item, 0.f );

		group->Expand();
		EditEffect( fxDef );
	}
}

void CEdCutsceneEditor::OnRemoveEffect( wxCommandEvent& event )
{
	ASSERT( m_csTemplate );

	if ( m_csTemplate->MarkModified() )
	{
		int selection = m_effectsList->GetSelection();
		if ( selection >= 0 )
		{
			String effectName = String( m_effectsList->GetString( selection ).c_str() );
			if ( GFeedback->AskYesNo( String::Printf( TXT("Are you sure to remove effect %s?"), effectName.AsChar() ).AsChar() ) )
			{
				if ( CFXDefinition* effectToRemove = m_csTemplate->FindEffect( CName( effectName ) ) )
				{
					m_csTemplate->RemoveEffect( effectToRemove );
					UpdateEffectList();
					EditEffect( nullptr );
				}
			}
		}
	}
}

void CEdCutsceneEditor::OnCopyEffect( wxCommandEvent& event )
{
}

void CEdCutsceneEditor::OnPasteEffect( wxCommandEvent& event )
{
	ASSERT( m_csTemplate );

	if ( m_csTemplate->MarkModified() )
	{
	}
}

void CEdCutsceneEditor::OnUpdateUI( wxUpdateUIEvent& event )
{
	if ( !m_auiNotebook )
	{
		return; // not initialized
	}

	Bool haveValidTemplate = m_csTemplate && m_csTemplate->IsValid();
	Bool haveWorldWithDynLayer = GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetDynamicLayer();

	m_timeMulSlider->Enable( m_mode != M_None );
	m_timeMulEdit->Enable( m_mode != M_None );

	m_playPreviewButt->Enable( haveValidTemplate && ( m_mode == M_None || m_mode == M_Preview ) );
	m_playGameButt->Enable( haveValidTemplate && ( ( m_mode == M_None && haveWorldWithDynLayer ) || m_mode == M_Game ) );

	if ( !haveWorldWithDynLayer && !m_csTemplate->GetLastLevelLoaded().Empty() ) 
	{
		m_loadWorldButt->Enable( true );
	}
	else if ( !haveWorldWithDynLayer && m_csTemplate->GetLastLevelLoaded().Empty() )
	{
		m_loadWorldButt->Enable( false );
	}
	else
	{
		if ( GGame->GetActiveWorld()->GetDepotPath() != m_csTemplate->GetLastLevelLoaded() && !m_csTemplate->GetLastLevelLoaded().Empty() )
		{
			m_loadWorldButt->Enable( true );
		}
		else
		{
			m_loadWorldButt->Enable( false );
		}
	}

	// Notebook pages
	{
		Bool selected = ( m_effectsList->GetSelection() >= 0 );
		XRCCTRL( *this, "removeEffect", wxButton )->Enable( selected );
		XRCCTRL( *this, "copyEffect",   wxButton )->Enable( selected );
	}

	// menu
	ERPWidgetMode mode = m_previewPanel->GetWidgetMode();
 	GetMenuBar()->FindItem( XRCID("Mode_Move") )->Check( mode == RPWM_Move );
 	GetMenuBar()->FindItem( XRCID("Mode_Rotate") )->Check( mode == RPWM_Rotate );
}

void CEdCutsceneEditor::CsToGameplayRequest()
{
	static Bool toggle = true;

	if ( m_csInstance )
	{
		m_csInstance->CsToGameplayRequest( 0.f,  toggle );
		toggle = !toggle;
	}
}

void CEdCutsceneEditor::OnNetConnected( wxCommandEvent& event )
{
	const Bool state = event.IsChecked();
	if ( state )
	{
		ConnectNetwork();
	}
	else
	{
		DisconnectNetwork();
	}
}

void CEdCutsceneEditor::ConnectNetwork()
{
	wxTextCtrl* text = XRCCTRL( *this, "netMsg", wxTextCtrl );
	wxStaticText* status = XRCCTRL( *this, "netText", wxStaticText );
	
	wxIPV4address addr;
	addr.Service( 3000 );

	if ( !m_socketServer )
	{
		m_socketServer = new wxSocketServer( addr );

		if ( !m_socketServer->Ok() )
		{
			text->AppendText(_("Could not listen at the specified port !\n\n"));
			status->SetLabelText( _("Status: Disconnected") );
			return;
		}
		else
		{
			text->AppendText(_("Server listening.\n\n"));
		}

		m_socketServer->SetEventHandler( *this, ID_SOCKET_SERVER );
		m_socketServer->SetNotify( wxSOCKET_CONNECTION_FLAG );
		m_socketServer->Notify( true );

		status->SetLabelText( _("Status: Connected") );
	}
}

void CEdCutsceneEditor::DisconnectNetwork()
{
	if ( m_socketServer )
	{
		m_socketServer->Destroy();
		m_socketServer = nullptr;
	}

	wxStaticText* status = XRCCTRL( *this, "netText", wxStaticText );
	status->SetLabelText( _("Status: Disconnected") );
}

void CEdCutsceneEditor::OnSocketClientEvent( wxSocketEvent& event )
{
	wxString s = _("OnSocketEvent: ");
	wxSocketBase *sock = event.GetSocket();

	switch( event.GetSocketEvent() )
	{
		case wxSOCKET_INPUT : s.Append(_("wxSOCKET_INPUT\n")); break;
		case wxSOCKET_LOST  : s.Append(_("wxSOCKET_LOST\n")); break;
		default             : s.Append(_("Unexpected event !\n")); break;
	}

	wxTextCtrl* text = XRCCTRL( *this, "netMsg", wxTextCtrl );

	switch( event.GetSocketEvent() )
	{
	case wxSOCKET_INPUT:
		{
			sock->SetNotify( wxSOCKET_LOST_FLAG );

			Int32 msgType;
			sock->Read( &msgType, sizeof( Int32 ) );

			switch ( msgType )
			{
			case 'cam_': 
				{
					ProcessNetworkPacket_Camera( sock, text ); 
					break;
				}
			default:
				{
					text->AppendText(_("Unknown test id received from client\n\n"));
				}
			}

			sock->SetNotify( wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG );
			break;
		}
	case wxSOCKET_LOST:
		{
			text->AppendText(_("Deleting socket.\n\n"));
			sock->Destroy();
			break;
		}
	}
}

void CEdCutsceneEditor::OnSocketServerEvent( wxSocketEvent& event )
{
	wxString s = _("OnServerEvent: ");
	wxSocketBase *sock;

	switch( event.GetSocketEvent() )
	{
		case wxSOCKET_CONNECTION : s.Append(_("wxSOCKET_CONNECTION\n")); break;
		default                  : s.Append(_("Unexpected event !\n")); break;
	}

	wxTextCtrl* text = XRCCTRL( *this, "netMsg", wxTextCtrl );
	text->AppendText( s );

	sock = m_socketServer->Accept( false );
	if ( sock )
	{
		text->AppendText(_("New client connection accepted\n\n"));
	}
	else
	{
		text->AppendText(_("Error: couldn't accept a new connection\n\n"));
		return;
	}

	sock->SetEventHandler(*this, ID_SOCKET_CLIENT );
	sock->SetNotify( wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
	sock->Notify( true );
}

void CEdCutsceneEditor::ProcessNetworkPacket_Camera( wxSocketBase *sock, wxTextCtrl* text )
{
	Int32 size;
	Uint8* buf;
	Int32 status = 0;

	sock->SetFlags( wxSOCKET_WAITALL );

	sock->Read( &size, sizeof(Int32) );
	buf = new Uint8[ size ]; // todo

	const Uint32 CAM_DATA_SIZE = 19;

	if ( size == sizeof( Float ) * CAM_DATA_SIZE )
	{
		Float cameraData[ CAM_DATA_SIZE ];
		sock->Read( &cameraData, sizeof( Float ) * CAM_DATA_SIZE );

		Matrix mat( Matrix::IDENTITY );
		
		mat.V[0].A[ 0 ] = cameraData[ 3 ];
		mat.V[0].A[ 1 ] = cameraData[ 4 ];
		mat.V[0].A[ 2 ] = cameraData[ 5 ];
		mat.V[1].A[ 0 ] = cameraData[ 6 ];
		mat.V[1].A[ 1 ] = cameraData[ 7 ];
		mat.V[1].A[ 2 ] = cameraData[ 8 ];
		mat.V[2].A[ 0 ] = cameraData[ 9 ];
		mat.V[2].A[ 1 ] = cameraData[ 10 ];
		mat.V[2].A[ 2 ] = cameraData[ 11 ];

		Vector camPos( cameraData[ 0 ], cameraData[ 1 ], cameraData[ 2 ] );
		EulerAngles camRot = mat.ToEulerAngles();
		Float camFov = cameraData[ 12 ];

		m_previewPanel->SetCameraFov( camFov );
		m_previewPanel->SetCameraPosition( camPos );
		m_previewPanel->SetCameraRotation( camRot );
	}
	else
	{
		//...
	}

	sock->Write( &status, sizeof(Int32) );

	delete[] buf;

	if ( status == 1 )
	{
	}
	else
	{
	}
}

void CEdCutsceneEditor::OnNetwork_SetCamera( const Matrix& cam, Float fov )
{
	if ( m_mode == M_Game )
	{
		const Matrix& l2w = m_csInstance->GetCsPosition();
		const Matrix camWS = Matrix::Mul( l2w, cam );

		wxTheFrame->GetWorldEditPanel()->SetCameraFov( fov );
		wxTheFrame->GetWorldEditPanel()->SetCameraPosition( camWS.GetTranslation() );
		wxTheFrame->GetWorldEditPanel()->SetCameraRotation( camWS.ToEulerAngles() );
	}
	else
	{
		m_previewPanel->SetCameraFov( fov );
		m_previewPanel->SetCameraPosition( cam.GetTranslation() );
		m_previewPanel->SetCameraRotation( cam.ToEulerAngles() );
	}
}

void CEdCutsceneEditor::OnNetwork_SetTime( Float t )
{
	SetTime( t );
}

CBehaviorMixerSlotInterface* CEdCutsceneEditor::FindMixer( const AnsiChar* actorName, Bool bodyOrMimics )
{
	const Uint32 size = m_mixers.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_mixers[ i ].m_first == actorName )
		{
			if ( !bodyOrMimics )
			{
				return &(m_mixers[ i ].m_second.m_first);
			}
			else
			{
				return &(m_mixers[ i ].m_second.m_second);
			}
		}
	}

	return nullptr;
}

void CEdCutsceneEditor::CreateMixers( const AnsiChar* actorName )
{
	CBehaviorMixerSlotInterface mixerBody;
	CBehaviorMixerSlotInterface mixerMimics;

	StringAnsi str( actorName );

	if ( m_csInstance )
	{
		String actorNameStr( ANSI_TO_UNICODE( actorName ) );

		size_t ssi( 0 );
		if ( actorNameStr.FindSubstring( TXT("_face"), ssi ) )
		{
			actorNameStr = actorNameStr.StringBefore( TXT("_face") );
		}

		if ( CEntity* e = m_csInstance->FindActorByName( actorNameStr ) )
		{
			if ( CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
			{
				if ( CBehaviorGraphStack* stack = ac->GetBehaviorStack() )
				{
					const Bool ret = stack->GetSlot( CNAME( MIXER_SLOT_BODY ), mixerBody );
					if ( !ret )
					{
						//...
					}
				}
			}
			if ( CActor* a = Cast< CActor >( e ) )
			{
				if ( CMimicComponent* m = a->GetMimicComponent() )
				{
					if ( CBehaviorGraphStack* stack = m->GetBehaviorStack() )
					{
						const Bool ret = stack->GetSlot( CNAME( MIXER_SLOT_MIMICS ), mixerMimics );
						if ( !ret )
						{
							//...
						}
					}
				}
			}
		}
	}

	m_mixers.PushBack( TPair< StringAnsi, TPair< CBehaviorMixerSlotInterface, CBehaviorMixerSlotInterface > >( str, TPair< CBehaviorMixerSlotInterface, CBehaviorMixerSlotInterface >( mixerBody, mixerMimics ) ) );
}

Bool CEdCutsceneEditor::DestroyMixer( const AnsiChar* actorName )
{
	const Uint32 size = m_mixers.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_mixers[ i ].m_first == actorName )
		{
			m_mixers.RemoveAtFast( i );
			return true;
		}
	}

	return false;
}

void CEdCutsceneEditor::DestroyAllMixers()
{
	m_mixers.ClearFast();
}

void CEdCutsceneEditor::OnNetwork_SetPose( const AnsiChar* actorName, Int32 actorNameSize, const SAnimationMappedPose& pose )
{
	Bool poseBodyOrMimic = false;

	if ( actorNameSize > 5 && 
		actorName[ actorNameSize-5 ] == 'f' &&
		actorName[ actorNameSize-4 ] == 'a' &&
		actorName[ actorNameSize-3 ] == 'c'&& 
		actorName[ actorNameSize-2 ] == 'e' )
	{
		poseBodyOrMimic = true; // mimics
	}

	CBehaviorMixerSlotInterface* mixer = FindMixer( actorName, poseBodyOrMimic );
	if ( !mixer )
	{
		CreateMixers( actorName );
		mixer = FindMixer( actorName, poseBodyOrMimic );
	}

	if ( mixer )
	{
		mixer->AddPoseToSample( 0, pose );
	}
}

//////////////////////////////////////////////////////////////////////////

CEdCutsceneEditorPreview::CEdCutsceneEditorPreview( wxWindow* parent, CEdCutsceneEditor *editor )
	: CEdPreviewPanel( parent, true, true )
	, m_enabled( false )
	, m_csEditor( editor )
{
	GetViewport()->SetRenderingMode( RM_Shaded );
	GetViewport()->SetRenderingMask( SHOW_Sprites );
	GetViewport()->SetRenderingMask( SHOW_EnvProbesInstances );

	// Add widgets - move only
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EX, Color::RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EY, Color::GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMoveAxis( Vector::EZ, Color::BLUE ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EY, Vector::EZ, Color::LIGHT_RED ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EZ, Color::LIGHT_GREEN ) );
	m_widgetManager->AddWidget( TXT("Move"), new CViewportWidgetMovePlane( Vector::EX, Vector::EY, Color::LIGHT_BLUE ) );

	const Float rotateGismoSize = 0.7f;
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 30, 0 ), Vector::EX, Color::RED, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 30, 0, 0 ), Vector::EY, Color::GREEN, rotateGismoSize ) );
	m_widgetManager->AddWidget( TXT("Rotate"), new CViewportWidgetRotateAxis( EulerAngles( 0, 0, 30 ), Vector::EZ, Color::BLUE, rotateGismoSize ) );

	// Select only components
	GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	// Register as event listener
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
}

CEdCutsceneEditorPreview::~CEdCutsceneEditorPreview()
{
	SEvents::GetInstance().UnregisterListener( this );
}

ERPWidgetMode CEdCutsceneEditorPreview::GetWidgetMode() const
{
	return m_widgetManager->GetWidgetMode();
}

void CEdCutsceneEditorPreview::SetWidgetMode( ERPWidgetMode mode )
{
	m_widgetManager->SetWidgetMode( mode );
}

void CEdCutsceneEditorPreview::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) && m_previewWorld )
	{
 		typedef CSelectionManager::SSelectionEventData SEventData;
 		const SEventData& eventData = GetEventData< SEventData >( data );
 		if ( eventData.m_world == m_previewWorld )
 		{
 			TDynArray< CNode* > nodes;
 			GetSelectionManager()->GetSelectedNodes( nodes );
 			m_widgetManager->EnableWidgets( nodes.Size() > 0 );
 		}
	}
}

void CEdCutsceneEditorPreview::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	m_csEditor->HandleItemSelection( objects );
}

void CEdCutsceneEditorPreview::SetEnabled( Bool flag )
{
	m_enabled = flag;
}

Bool CEdCutsceneEditorPreview::ToggleDispSkeletons()
{
	m_showSkeletons = !m_showSkeletons;
	return m_showSkeletons;
}

void CEdCutsceneEditorPreview::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( m_enabled )
	{
		if ( m_csEditor->GetCsCamera() )
		{
			// Use camera from cs
			m_csEditor->GetCsCamera()->OnViewportCalculateCamera( view, camera );
			return;
		}
	}	

	// Use default camera
	CEdPreviewPanel::OnViewportCalculateCamera( view, camera );
}

void CEdCutsceneEditorPreview::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	// Scene
	CEdPreviewPanel::OnViewportGenerateFragments( view, frame );

	// Axis
	frame->AddDebugAxisOnScreen( 0.2f, 0.2f, Matrix::IDENTITY, 0.1f );

	// Skeleton
	if ( m_enabled )
	{
		if ( m_showSkeletons )
		{
			DisplaySkeletons( frame );

			DisplayBBox( frame );
		}
	}

	m_csEditor->OnViewportGenerateFragments( view, frame );
}

void CEdCutsceneEditorPreview::OnViewportTick( IViewport* view, Float timeDelta )
{
	if ( m_enabled )
	{
		// Tick world
		CWorldTickInfo info( m_previewWorld, timeDelta );
		info.m_updatePhysics = true;
		m_previewWorld->Tick( info );

		const Float slowMo = m_csEditor->GetCsInstance() ? m_csEditor->GetCsInstance()->GetSlowMo() : 1.f;
		m_csEditor->UpdateCs( slowMo * timeDelta );

		if ( CCameraComponent* cam = m_csEditor->GetCsCamera() )
		{
			const Int32 camRot = (Int32)cam->GetWorldRotation().Yaw;
			const Int32 lightPos = GetLightPosition();
			if ( camRot != lightPos && !GetLockState() )
			{
				SetLightPosition( camRot-180 );
			}
		}
	}

	// Tick panel
	CEdRenderingPanel::OnViewportTick( view, timeDelta );
}

void CEdCutsceneEditorPreview::DisplayBBox( CRenderFrame *frame )
{
	const CCutsceneInstance* csInstance = m_csEditor->GetCsInstance();
	if ( !csInstance )
	{
		return;
	}
	
	Matrix offset;
	TDynArray< Box > bboxes;
	csInstance->GetBoundingBoxes( bboxes, offset );

	for ( Uint32 i=0; i<bboxes.Size(); i++ )
	{
		frame->AddDebugBox( bboxes[i], offset, Color::GREEN );
	}
}

void CEdCutsceneEditorPreview::DisplaySkeletons( CRenderFrame *frame )
{
	const CCutsceneInstance* csInstance = m_csEditor->GetCsInstance();
	if ( !csInstance )
	{
		return;
	}

	// Camera float tracks
	const CCamera* csCamera = csInstance->GetCsCamera();
	if ( csCamera )
	{
		CAnimatedComponent* cameraRoot = csCamera->GetRootAnimatedComponent();

		// Draw float tracks
		Uint32 tracksNum = cameraRoot->GetFloatTrackNum();
		Uint32 offset = 0;

		for ( Uint32 i=0; i<tracksNum; ++i )
		{
			Float track = cameraRoot->GetFloatTrack( i );

			// Disp i:track
			String text = String::Printf( TXT("Track %d: %.2f"), i, track );
			frame->AddDebugScreenText( frame->GetFrameOverlayInfo().m_width - 100, 20 + offset, text, Color::RED );
			offset += 15;
		}
	}

	const CAnimatedComponent* selActor = m_csEditor->GetSelectedActor();

	if ( selActor )
	{
		Color color = Color( 255, 255, 255 );
		DisplaySkeleton( frame, selActor, color );
	}
	else
	{
		// Cutscene actors
		TDynArray< const CAnimatedComponent* > elements;
		csInstance->GetAllPlayableElements( elements );

		Uint32 colorMask = 1;

		for ( Uint32 i=0; i<elements.Size(); ++i )
		{
			colorMask = (colorMask + 1) % 7;	
			if ( !colorMask ) colorMask = 1;

			Color color = Color(	colorMask & 1 ? 255 : 0, 
									colorMask & 2 ? 255 : 0,
									colorMask & 4 ? 255 : 0 );

			DisplaySkeleton( frame, elements[i], color );
		}
	}
}

void CEdCutsceneEditorPreview::DisplaySkeleton( CRenderFrame *frame, const CAnimatedComponent* component, Color color )
{
	TDynArray< ISkeletonDataProvider::BoneInfo > bones;
	Uint32 bonesNum = component->GetBones( bones );

	TDynArray< DebugVertex > skeletonPoints;

	// Draw bones
	for( Uint32 i=0; i<bonesNum; i++ )
	{
		Int32 parentIndex = bones[i].m_parent;
		if ( parentIndex != -1 )
		{
			Matrix start = component->GetBoneMatrixWorldSpace( parentIndex );
			Matrix end = component->GetBoneMatrixWorldSpace( i );

			skeletonPoints.PushBack( DebugVertex( start.GetTranslation(), color ) );
			skeletonPoints.PushBack( DebugVertex( end.GetTranslation(), color ) );

			if ( bones[i].m_name == CNAME( Trajectory ) )
			{	
				frame->AddDebugText( end.GetTranslation(), bones[i].m_name.AsString(), false );
			}
		}
	}

	if (skeletonPoints.Size() > 0)
	{
		new (frame) CRenderFragmentDebugLineList( frame, 
			Matrix::IDENTITY,
			&skeletonPoints[0],
			skeletonPoints.Size(),
			RSG_DebugOverlay );
	}
}

//////////////////////////////////////////////////////////////////////////

CEffectPreviewItem::CEffectPreviewItem( CEdCutsceneEditor* csEditor )
	: m_editor( csEditor )
	, m_isRefreshing( false )
{
}

Bool CEffectPreviewItem::IsValid() const
{
	return GetEvent() != NULL;
}

void CEffectPreviewItem::Refresh()
{
	m_isRefreshing = true;

	const CExtAnimCutsceneEffectEvent* event = GetEvent();
	if ( event )
	{
		SetPosition( event->GetSpawnPos() );
		SetRotation( event->GetSpawnRot() );
	}

	m_isRefreshing = false;
}

void CEffectPreviewItem::SetPositionFromPreview( const Vector& prevPos, Vector& newPos ) 
{
	if ( m_isRefreshing )
	{
		return;
	}

	if ( const CExtAnimCutsceneEffectEvent* event = GetEvent() )
	{
		CCutsceneTemplate* csTemplate = m_editor->GetCsTemplate();
		if ( csTemplate->MarkModified() )
		{
			CExtAnimCutsceneEffectEvent* editableEvent = const_cast< CExtAnimCutsceneEffectEvent* >( event );
			editableEvent->SetSpawnPositon( newPos );
		}
	}

	if ( CEntity* effect = GetEffect() )
	{
		effect->SetPosition( newPos );
	}

	GetItemContainer()->OnItemTransformChangedFromPreview( this );
}

void CEffectPreviewItem::SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot )
{
	if ( m_isRefreshing )
	{
		return;
	}

	if ( const CExtAnimCutsceneEffectEvent* event = GetEvent() )
	{
		CCutsceneTemplate* csTemplate = m_editor->GetCsTemplate();
		if ( csTemplate->MarkModified() )
		{
			CExtAnimCutsceneEffectEvent* editableEvent = const_cast< CExtAnimCutsceneEffectEvent* >( event );
			editableEvent->SetSpawnRotation( newRot );
		}
	}

	if ( CEntity* effect = GetEffect() )
	{
		effect->SetRotation( newRot );
	}

	GetItemContainer()->OnItemTransformChangedFromPreview( this );
}

IPreviewItemContainer* CEffectPreviewItem::GetItemContainer() const
{
	return m_editor;
}

void CEffectPreviewItem::Init( const String& name )
{
	IPreviewItem::Init( name );
	m_component->SetOverlay( false );
	m_component->SetDrawArrows( true );
	m_component->SetSize(IPreviewItem::PS_Tiny);
}

const CExtAnimCutsceneEffectEvent* CEffectPreviewItem::GetEvent() const
{
	CCutsceneTemplate* csTemplate = m_editor->GetCsTemplate();
	ASSERT( csTemplate );

	TDynArray< const CExtAnimCutsceneEffectEvent* > events;
	csTemplate->GetEventsOfType( events );

	CName name( GetName() );

	for( TDynArray< const CExtAnimCutsceneEffectEvent* >::const_iterator eventIter = events.Begin();
		eventIter != events.End(); ++eventIter )
	{
		const CExtAnimCutsceneEffectEvent* event = *eventIter;
		ASSERT( event != NULL );

		if ( event->GetEventName() == name )
		{
			return event;
		}
	}

	return NULL;
}

CEntity* CEffectPreviewItem::GetEffect() const
{
	const CExtAnimCutsceneEffectEvent* event = GetEvent();
	CCutsceneInstance* instance = m_editor->GetCsInstance();
	if ( instance && event )
	{
		THashMap< const CExtAnimCutsceneEffectEvent*, THandle< CEntity > >& effectMap = instance->GetEffectMap();
		THandle< CEntity >* handle = effectMap.FindPtr( event );
		if ( handle )
		{
			CEntity* entity = handle->Get();
			return entity;
		}
	}

	return NULL;
}

void CEdCutsceneEditor::InsertSwitchesRelaxEvents()
{
	CCutsceneTemplate* csTemplate = GetCsTemplate();
	if ( csTemplate->MarkModified() )
	{
		ASSERT( csTemplate );
		const TDynArray<Float> & cuts = csTemplate->GetCameraCuts();
		Int32 numc = cuts.Size();
		const TDynArray< CSkeletalAnimationSetEntry* >& animSetEntries = m_csTemplate->GetAnimations();
		for ( Uint32 i=0; i<animSetEntries.Size(); ++i )
		{
			CSkeletalAnimationSetEntry* entry = animSetEntries[ i ];
			TDynArray< CExtAnimCutsceneResetClothAndDangleEvent* > oldEvents;
			entry->GetEventsOfType( oldEvents );
			for( TDynArray< CExtAnimCutsceneResetClothAndDangleEvent* >::iterator eventIter = oldEvents.Begin(); eventIter != oldEvents.End(); ++eventIter )
			{
				entry->RemoveEvent( *eventIter );
			}

			TDynArray< String > animNameSplitted;
			if ( entry->GetAnimation() )
			{
				animNameSplitted = entry->GetAnimation()->GetName().AsString().Split( TXT(":") );
			}

			String actorName = animNameSplitted.Empty() ? String::EMPTY : animNameSplitted[0];
			String skeletonPart = animNameSplitted.Size() > 1 ? animNameSplitted[1] : String::EMPTY;
			SCutsceneActorDef* actor = csTemplate->GetActorDefinition( actorName );

			if ( actor && actor->m_type == CAT_Actor && ( skeletonPart.Empty() || skeletonPart == TXT( "Root" ) ) )
			{
				for( Int32 j=0;j<numc;j++ )
				{
					CExtAnimCutsceneResetClothAndDangleEvent* ev = new CExtAnimCutsceneResetClothAndDangleEvent();
					ev->SetStartTime( cuts[j] );
					ev->SetTrackName( TXT("Switches_Relax") );
					entry->AddEvent(ev);
				}
			}
		}
	}
}

void CEdCutsceneEditor::OnCreateRelaxEvents( wxCommandEvent& event )
{
	InsertSwitchesRelaxEvents();
}

void CEdCutsceneEditor::OnDeleteRelaxEvents( wxCommandEvent& event )
{
	if ( GFeedback->AskYesNo(TXT("Are you sure to delete all relax events?") ) )
	{
		CCutsceneTemplate* csTemplate = GetCsTemplate();
		if ( csTemplate->MarkModified() )
		{
			ASSERT( csTemplate );
			const TDynArray<Float> & cuts = csTemplate->GetCameraCuts();
			Int32 numc = cuts.Size();
			const TDynArray< CSkeletalAnimationSetEntry* >& animSetEntries = m_csTemplate->GetAnimations();
			for ( Uint32 i=0; i<animSetEntries.Size(); ++i )
			{
				CSkeletalAnimationSetEntry* entry = animSetEntries[ i ];
				TDynArray< CExtAnimCutsceneResetClothAndDangleEvent* > oldEvents;
				entry->GetEventsOfType( oldEvents );
				for( TDynArray< CExtAnimCutsceneResetClothAndDangleEvent* >::iterator eventIter = oldEvents.Begin(); eventIter != oldEvents.End(); ++eventIter )
				{
					entry->RemoveEvent( *eventIter );
				}
			}
		}
	}
}
