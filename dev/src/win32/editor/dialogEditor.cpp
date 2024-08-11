
#include "build.h"
#include <wx\display.h>
#include "dialogEditor.h"
#include "filterPanel.h"

#include "../../common/core/feedback.h"
#include "../../common/core/depot.h"

#include "../../common/engine/curveEntity.h"
#include "../../common/engine/curveControlPointEntity.h"
#include "../../common/engine/curveTangentControlPointEntity.h"
#include "../../common/engine/gameResource.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/camera.h"
#include "../../common/engine/soundSystem.h"
#include "../../common/engine/mimicComponent.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/renderCommands.h"

#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneCutscene.h"
#include "../../common/game/storySceneScriptLine.h"
#include "../../common/game/storyScenePauseElement.h"
#include "../../common/game/storySceneBlockingElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventDuration.h"
#include "../../common/game/storySceneEventAnimation.h"
#include "../../common/game/storySceneEventEnterActor.h"
#include "../../common/game/storySceneEventExitActor.h"
#include "../../common/game/storySceneEventLookat.h"
#include "../../common/game/storySceneEventDespawn.h"
#include "../../common/game/storySceneEventFade.h"
#include "../../common/game/storySceneEventMimics.h"
#include "../../common/game/storySceneEventMimicsAnim.h"
#include "../../common/game/storySceneEventSound.h"
#include "../../common/game/storySceneEventRotate.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "../../common/game/storySceneEventCustomCameraInstance.h"
#include "../../common/game/storySceneEventCameraBlend.h"
#include "../../common/game/storySceneEventChangePose.h"
#include "../../common/game/storySceneEventEquipItem.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneComment.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/actor.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneEventMimicPose.h"
#include "../../common/game/storySceneEventMimicFilter.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventGroup.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneChoice.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/game/storySceneUtils.h"
#include "../../common/game/storySceneEventCameraAnimation.h"
#include "../../common/game/storySceneEventLightProperties.h"

#include "editorExternalResources.h"
#include "dialogEditorActions.h"
#include "dialogEditorPage.h"
#include "gridEditor.h"
#include "gridCustomTypes.h"
#include "gridCustomColumns.h"
#include "gridPropertyWrapper.h"
#include "dialogTimeline.h"
#include "propertiesPageComponent.h"
#include "popupNotification.h"
#include "storyScenePreviewPlayer.h"
#include "dialogUtilityHandlers.h"
#include "dialogPreview.h"
#include "dialogEditorDialogsetPanel.h"
#include "voice.h"
#include "lipsyncDataSceneExporter.h"
#include "animFriend.h"
#include "dialogAnimationParameterEditor.h"
#include "dialogEventGeneratorSetupDialog.h"
#include "dialogEventGenerator.h"
#include "../importer/REAAnimImporter.h"
#include "dialogEditorTempLipsyncDlg.h"
#include "dialogEditorEditMimicsDlg.h"
#include "dialogEditorImportW2StringsDlg.h"
#include "dialogEditorMainControlPanel.h"
#include "dialogTimeline.h"
#include "dialogTimeline_items.h"
#include "sceneExplorer.h"
#include "patToolPanel.h"
#include "dialogNetPanel.h"
#include "controlRigPanel.h"
#include "../../common/engine/viewport.h"
#include "../../common/game/storySceneGraph.h"
#include "sceneValidator.h"
#include "../../common/engine/localizableObject.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

struct SRecordingInfo
{
	Int32 width, height, rate;
	Bool ubersample;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CEdSceneEditor, wxSmartLayoutPanel )
	EVT_MENU( XRCID( "SaveMenuItem" ), CEdSceneEditor::OnSaveScene )
	EVT_MENU( XRCID( "ExitMenuItem" ), CEdSceneEditor::OnMenuExit )
	EVT_MENU( XRCID( "DumpAnimationsItem" ), CEdSceneEditor::OnMenuDumpAnimations )
	EVT_MENU( XRCID( "editCut" ), CEdSceneEditor::OnEditCut )
	EVT_MENU( XRCID( "editCopy" ), CEdSceneEditor::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdSceneEditor::OnEditPaste )
	EVT_MENU( XRCID( "editDelete" ), CEdSceneEditor::OnEditDelete )
	EVT_MENU( XRCID( "editUndo" ), CEdSceneEditor::OnEditUndo )
	EVT_MENU( XRCID( "editRedo" ), CEdSceneEditor::OnEditRedo )
	EVT_MENU( XRCID( "LargerFont" ), CEdSceneEditor::OnLargerFont )
	EVT_MENU( XRCID( "SmallerFont" ), CEdSceneEditor::OnSmallerFont )
	EVT_MENU( XRCID( "LocalizationTooltips" ), CEdSceneEditor::OnShowLocalizationTooltips )
	EVT_MENU( XRCID( "TemporaryLipsync" ), CEdSceneEditor::OnTemporaryLipsync )
	EVT_MENU( XRCID( "GenerateSpeakingTo" ), CEdSceneEditor::OnGenerateSpeakingToData )
	EVT_MENU( XRCID( "ShowVoiceMarkers" ), CEdSceneEditor::OnShowVoiceMarkers )
	EVT_MENU( XRCID( "UpdateSectionsOrder" ), CEdSceneEditor::OnUpdateSectionsOrder )
	EVT_MENU( XRCID( "AddPlayerToActors" ), CEdSceneEditor::OnAddPlayerToActors )
	EVT_MENU( XRCID( "EditMimics" ), CEdSceneEditor::OnEditMimics )
	EVT_MENU( XRCID( "ImportW2Strings" ), CEdSceneEditor::OnImportW2Strings )
	EVT_MENU( XRCID( "toolPropagateCameraLights" ), CEdSceneEditor::OnPropagateCameraLights )
	EVT_TOGGLEBUTTON( XRCID( "btnRun" ), CEdSceneEditor::OnDebuggerConnected )

	EVT_MENU( XRCID( "OnlyText" ), CEdSceneEditor::OnToggleShowOnlyScriptTexts )
	EVT_MENU( XRCID( "ResetLayout" ), CEdSceneEditor::OnResetLayout )
	EVT_MENU( XRCID( "SaveCustomLayout" ), CEdSceneEditor::SaveCustomLayout )
	EVT_MENU( XRCID( "LoadCustomLayout" ), CEdSceneEditor::LoadCustomLayout )

	EVT_MENU( XRCID( "BehaviorDebug" ), CEdSceneEditor::OnStartBehaviorDebug )
	
	EVT_MENU( XRCID( "SaveLayerPresets" ), CEdSceneEditor::OnSaveLayerPresets )
	EVT_MENU( XRCID( "LoadLayerPresets" ), CEdSceneEditor::OnLoadLayerPresets )

	EVT_MENU( XRCID( "CreateLightPreset" ), CEdSceneEditor::OnCreateLightPreset )

	EVT_MENU( XRCID( "ConvertCameraBlends" ), CEdSceneEditor::OnToolConvertCameraBlends )
	EVT_MENU( XRCID( "FixCameraBlendsEndTime" ), CEdSceneEditor::OnToolFixCameraBlendsEndTime )
	EVT_MENU( XRCID( "RemoveScale" ), CEdSceneEditor::OnToolRemoveScale )
	//EVT_MENU( XRCID( "LocMenu" ), CEdSceneEditor::OnToolLocCtl )
	EVT_MENU( XRCID( "LocAnalyser" ), CEdSceneEditor::OnLocAnalyzer )
	EVT_MENU( XRCID( "LoadVoiceFiles" ), CEdSceneEditor::OnRefreshVoiceovers )

	EVT_CHOICE( XRCID( "locChoice" ), CEdSceneEditor::OnChangeLanguage )
	EVT_BUTTON( XRCID( "locLangToggle" ), CEdSceneEditor::OnChangeLanguageToggle )
	EVT_MENU( XRCID( "menuMuteSpeech" ), CEdSceneEditor::OnMuteSpeech )
	EVT_MENU( XRCID( "screenshotMenu" ), CEdSceneEditor::OnToolScreenshotCtl )

	EVT_MENU( XRCID( "trackSun" ), CEdSceneEditor::OnPreviewTrackSun )
	EVT_MENU( XRCID( "floatingHelpers" ), CEdSceneEditor::OnFloatingHelpersCheckbox )
	EVT_MENU( XRCID( "pg_none" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_33" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_f_lt" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_f_lb" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_f_rt" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_f_rb" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_fr" ), CEdSceneEditor::OnPreviewGridChanged )
	EVT_MENU( XRCID( "pg_ds" ), CEdSceneEditor::OnPreviewGridChanged )

	EVT_MENU_RANGE( wxID_LIGHT_PRESET_LOAD_FIRST, wxID_LIGHT_PRESET_LOAD_LAST, CEdSceneEditor::OnLightPresetLoad )

	EVT_MENU( wxID_STORYSCENEEDITOR_CAMERA_PREVIEW, CEdSceneEditor::OnPreviewCameraPreviewMode )
	EVT_MENU( wxID_STORYSCENEEDITOR_CAMERA_FREE, CEdSceneEditor::OnPreviewCameraFreeMode )
	EVT_MENU( wxID_STORYSCENEEDITOR_CAMERA_EDIT, CEdSceneEditor::OnPreviewCameraEditMode )
	EVT_MENU( wxID_STORYSCENEEDITOR_NEW_KEYFRAME, CEdSceneEditor::OnNewKeyframe )
	EVT_MENU( wxID_STORYSCENEEDITOR_PLAY_TOGGLE, CEdSceneEditor::OnPreviewPlayToggle )
	EVT_MENU( wxID_STORYSCENEEDITOR_PLAY_FROM_START, CEdSceneEditor::OnPreviewPlayFromStart )
	EVT_MENU( wxID_STORYSCENEEDITOR_RECORD, CEdSceneEditor::OnRecord )
	EVT_MENU( wxID_STORYSCENEEDITOR_TOGGLEDOF, CEdSceneEditor::OnToggleDoF )

	EVT_SIZE( CEdSceneEditor::OnResize )
END_EVENT_TABLE()

IMPLEMENT_CLASS( CEdSceneEditor, wxSmartLayoutPanel );

CEdSceneEditor::CEdSceneEditor( wxWindow* parent, CStorySceneController* controller )
	: wxSmartLayoutPanel( parent, TEXT( "DialogEditor" ), false )
	, m_generatorDialog( nullptr )
	, m_frozen( false )
	, m_isCameraLocked( false )
	, m_keyframeCtrl( this )
	, m_mimicsControlRigAutoRefresh( false )
	, m_recordingInViewport( false )
	, m_recordingInfo( nullptr )
	, m_mainControlPanel( nullptr )
	, m_useApprovedVoDurations( false )
{
	// DIALOG_TOMSIN_TODO - jak tryb debug to zmiana tylko playera, kontroler zostaje bez zmian bo questy maja pointer do niego
}

CEdSceneEditor::CEdSceneEditor( wxWindow* parent, CStoryScene* scene )
	: wxSmartLayoutPanel( parent, TEXT( "DialogEditor" ), false )
	, m_generatorDialog( nullptr )
	, m_screenplayPanel( nullptr )
	, m_previewPanel( nullptr )
	, m_storyScene( scene )
	, m_actorsDefinitionsGrid( nullptr )
	, m_propsDefinitionsGrid( nullptr )
	, m_effectsDefinitionsGrid( nullptr )
	, m_lightDefinitionsGrid( nullptr )
	, m_databaseTimer( nullptr )
	, m_actorsProvider( nullptr )
	, m_frozen( false )
	, m_isCameraLocked( false )
	, m_keyframeCtrl( this )
	, m_mimicsControlRigAutoRefresh( false )
	, m_recordingInViewport( false )
	, m_recordingInfo( nullptr )
	, m_mainControlPanel( nullptr )
	, m_useApprovedVoDurations( false )
{
	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_BDI_DIALOG" ) ) );
	SetIcon( iconSmall );

	wxPanel* containerPanel = XRCCTRL( *this, "SmartContainerPanel", wxPanel );

	//m_statusLabel = XRCCTRL( *this, "StatusBar", wxStaticText );

	GFeedback->BeginTask( TXT("Initializing scene editor"), false );
	GFeedback->UpdateTaskInfo( TXT("Scene graph") );
	GFeedback->UpdateTaskProgress( 0, 10 );

	ASSERT( m_storyScene != NULL );
	wxString titleString = m_storyScene->GetFile()->GetDepotPath().AsChar();
	SetTitle( titleString );

	GFeedback->UpdateTaskInfo( TXT("Scene graph") );
	GFeedback->UpdateTaskProgress( 1, 10 );

	m_storyScene->InitializeSceneGraphs();

	GFeedback->UpdateTaskProgress( 2, 10 );
	GFeedback->UpdateTaskInfo( TXT("Editor panels") );

	// screenplay panel
	m_screenplayPanel = new CEdSceneEditorScreenplayPanel( containerPanel, this );

	// preview panel
	m_previewPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogPreviewPanel" ) );
	{
		wxPanel* previewViewportPanel = XRCCTRL( *m_previewPanel, "PreviewPanelViewport", wxPanel );
		m_preview = new CEdDialogPreview( previewViewportPanel, this );
		previewViewportPanel->GetSizer()->Add( m_preview, 1, wxEXPAND );

		m_preview->SetCameraPosition( Vector( 0, 4, 2 ) );
		m_preview->SetCameraRotation( EulerAngles( 0, -10, 180 ) );
		m_preview->GetViewport()->SetRenderingMode( RM_Shaded );
	}

	// starting conditions panel
	{
		m_startingConditionsGrid = new CGridEditor( this );
		m_startingConditionsGrid->RegisterCustomType( new CGridTagListDesc );
		m_startingConditionsGrid->RegisterCustomType( new CGridGameTimeDesc );

		m_startingConditionsGrid->RegisterCustomColumnDesc( TXT( "Voicetag" ), new CGridReadOnlyColumnDesc() );

		m_startingConditionsGrid->RegisterCustomColumnDesc( TXT( "Left Item" ), new CGridInventoryItemColumnDesc );
		m_startingConditionsGrid->RegisterCustomColumnDesc( TXT( "Right Item" ), new CGridInventoryItemColumnDesc );
		m_startingConditionsGrid->Bind( wxEVT_SHOW, &CEdSceneEditor::OnShowStartingConditions, this );

		ReloadSceneInputsTable();
	}

	// definitions panel
	wxPanel* definitionsPanel = new wxPanel( containerPanel );
	{
		// actor defs
		m_actorsDefinitionsGrid = new CGridEditor( definitionsPanel, true );
		m_actorsDefinitionsGrid->RegisterCustomColumnDesc( TXT( "Id" ), new CGridVoicetagColumnDesc( 3 ) );
		m_actorsDefinitionsGrid->RegisterCustomType( new CGridTagListDesc );
		m_actorsDefinitionsGrid->Bind( wxEVT_GRID_VALUE_CHANGED, &CEdSceneEditor::OnActorListChanged, this );
		m_actorsDefinitionsGrid->Bind( wxEVT_GRID_ELEMENTS_CHANGED, &CEdSceneEditor::OnActorListChanged, this );
		SetGridDataFromProperty( m_actorsDefinitionsGrid, m_storyScene, CNAME( sceneTemplates ) );
		m_actorsDefinitionsGrid->SetColSize( 1, 200 );

		// prop defs
		m_propsDefinitionsGrid = new CGridEditor( definitionsPanel, true );
		m_propsDefinitionsGrid->Bind( wxEVT_GRID_VALUE_CHANGED, &CEdSceneEditor::OnPropListChanged, this );
		m_propsDefinitionsGrid->Bind( wxEVT_GRID_ELEMENTS_CHANGED, &CEdSceneEditor::OnPropListChanged, this );
		SetGridDataFromProperty( m_propsDefinitionsGrid, m_storyScene, CNAME( sceneProps ) );
		m_propsDefinitionsGrid->SetColSize( 1, 200 );

		// effect defs
		m_effectsDefinitionsGrid = new CGridEditor( definitionsPanel, true );
		m_effectsDefinitionsGrid->Bind( wxEVT_GRID_VALUE_CHANGED, &CEdSceneEditor::OnEffectListChanged, this );
		m_effectsDefinitionsGrid->Bind( wxEVT_GRID_ELEMENTS_CHANGED, &CEdSceneEditor::OnEffectListChanged, this );
		SetGridDataFromProperty( m_effectsDefinitionsGrid, m_storyScene, CNAME( sceneEffects ) );
		m_effectsDefinitionsGrid->SetColSize( 1, 200 );

		// light defs
		m_lightDefinitionsGrid = new CGridEditor( definitionsPanel, true );
		m_lightDefinitionsGrid->Bind( wxEVT_GRID_VALUE_CHANGED, &CEdSceneEditor::OnLightListChanged, this );
		m_lightDefinitionsGrid->Bind( wxEVT_GRID_ELEMENTS_CHANGED, &CEdSceneEditor::OnLightListChanged, this );
		SetGridDataFromProperty( m_lightDefinitionsGrid, m_storyScene, CNAME( sceneLights ) );
		m_lightDefinitionsGrid->SetColSize( 1, 200 );

		// layout stuffs
		definitionsPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		definitionsPanel->GetSizer()->Add( m_actorsDefinitionsGrid, 1, wxEXPAND );
		definitionsPanel->GetSizer()->Add( m_propsDefinitionsGrid, 1, wxEXPAND );
		definitionsPanel->GetSizer()->Add( m_effectsDefinitionsGrid, 1, wxEXPAND );
		definitionsPanel->GetSizer()->Add( m_lightDefinitionsGrid, 1, wxEXPAND );
	}

	// debug panel
	wxPanel* debugTabPanel = m_debugger.Setup( this, containerPanel );

	// graph panel
	wxPanel* graphPanel = new wxPanel( containerPanel );
	{
		graphPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		m_sceneGraphEditor = new CEdSceneGraphEditor( graphPanel, this );
		graphPanel->GetSizer()->Add( m_sceneGraphEditor, 1, wxEXPAND | wxALL );
	}

	// main properties panel
	wxPanel* propertiesPanel = new wxPanel( containerPanel );
	{
		propertiesPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		m_propertiesBrowser = new CEdComponentProperties( propertiesPanel, nullptr );
		propertiesPanel->GetSizer()->Add( m_propertiesBrowser, 1, wxEXPAND );
	} 

	// timeline panel
	wxPanel* timelinePanel = new wxPanel( containerPanel );
	wxToolBar* timelineToolbar = wxXmlResource::Get()->LoadToolBar( timelinePanel, wxT( "DialogTimelineToolbar" ) );
	{
		timelinePanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		m_timeline = new CEdDialogTimeline( timelinePanel, this, m_propertiesBrowser );
		m_timeline->Bind( usrEVT_TIMELINE_RESIZED, &CEdSceneEditor::OnTimelineResized, this );
		m_timeline->Bind( usrEVT_TIMELINE_REQUEST_SET_TIME, &CEdSceneEditor::OnTimeline_RequestSetTime, this );
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		timelinePanel->GetSizer()->Add( timelineToolbar, 0, wxEXPAND );
		timelinePanel->GetSizer()->Add( m_timeline, 1, wxEXPAND );
		m_timeline->Freeze();

		// DIALOG_TOMSIN_TODO - hack
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnPlayToggle,		this, XRCID( "PlayButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnStopButton,		this, XRCID( "StopButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnPrevButton,		this, XRCID( "PrevButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnNextButton,		this, XRCID( "NextButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnContinueButton,	this, XRCID( "ContinueButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnResetButton,	this, XRCID( "Reset") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnRunEventsGenerator,	this, XRCID( "DefaultEventsButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnClearEventsButton,	this, XRCID( "ClearEventsButton") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnDebugTimelineItems,	this, XRCID( "DebugItems") );
		timelineToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnPlayOneFrame,	this, XRCID( "PlayOneFrame") );

		// bind "preview" buttons in timeline toolbar -> preview
		m_preview->SetupToolbarEvents( timelineToolbar );
		MovePreviewToolbarToTimeline( false );
	}

	// Control rig panel
	m_controlRigPanel = new CEdControlRigPanel( containerPanel, this );

	// Mimics control rig panel
	wxPanel* mimicsControlRigPanel = new wxPanel( containerPanel );
	{
		mimicsControlRigPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

		m_mimicsControlRig = new CPatToolPanel( mimicsControlRigPanel, "get_faceed_joysticks", nullptr, 0, this );

		wxToolBar* mimicsControlRigToolbar = wxXmlResource::Get()->LoadToolBar( mimicsControlRigPanel, wxT( "MimicsControlRigToolbar" ) );
		mimicsControlRigToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnMimicsControlRigPanelRefresh, this, XRCID( "MimicsControlRigRefresh") );
		mimicsControlRigToolbar->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdSceneEditor::OnMimicsControlRigPanelAutoRefresh, this, XRCID( "MimicsControlRigAutoRefresh") );
		mimicsControlRigToolbar->Bind( wxEVT_COMMAND_SLIDER_UPDATED, &CEdSceneEditor::OnMimicsControlRigPanelWeightRefresh, this, XRCID( "MimicsControlRigWeightSlider") );

		mimicsControlRigPanel->GetSizer()->Add( mimicsControlRigToolbar, 0, wxEXPAND );
		mimicsControlRigPanel->GetSizer()->Add( m_mimicsControlRig, 1, wxEXPAND );
	}

	// Net panel
	CEdDialogNetPanel* netPanel = new CEdDialogNetPanel( containerPanel, this );

	wxPanel* plotPanel = new wxPanel( containerPanel );
	{
		wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );
		m_curveEditor = new CEdDialogFCurveEditor( plotPanel );
		//m_curveEditor->SetActiveRegion( 0.f, anim->GetDuration() + 0.0001f );
		sizer1->Add( m_curveEditor, 1, wxEXPAND, 0 );
		plotPanel->SetSizer( sizer1 );
		plotPanel->Layout();
	}

	SEdAnimationTreeBrowserSettings animTreeSettings;
	animTreeSettings.m_oneClickSelect = true;
	m_animTreeBody = new CEdAnimationTreeBrowser( containerPanel, animTreeSettings );
	m_animTreeMimics = new CEdAnimationTreeBrowser( containerPanel, animTreeSettings );

	// dialogset settings panel
	{
		m_dialogsetPanel = new CEdSceneEditorDialogsetPanel( this );
	}

	// cameras panel
	wxPanel* cameraTabPanel = wxXmlResource::Get()->LoadPanel( containerPanel, wxT( "DialogCameraPanel" ) );
	{
		wxPanel* cameraPropertiesPanel = XRCCTRL( *cameraTabPanel, "CameraPropertiesPanel", wxPanel );
		m_cameraProperties = new CEdPropertiesPage( cameraPropertiesPanel, PropertiesPageSettings(), nullptr );
		cameraPropertiesPanel->GetSizer()->Add( m_cameraProperties, 1, wxEXPAND );
		m_cameraProperties->SetReadOnly( m_camera.IsPreviewMode() );

		m_customCamerasList = XRCCTRL( *cameraTabPanel, "customCamerasList", wxListCtrl );
		m_customCamerasList->Bind( wxEVT_LEFT_DCLICK  , &CEdSceneEditor::OnCustomCamerasListDblClick, this );
		m_customCamerasList->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxCommandEventHandler( CEdSceneEditor::OnCameraSelectedFromList ), NULL, this );
		m_customCamerasList->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxCommandEventHandler( CEdSceneEditor::OnRightClickCameraFromList ), NULL, this );
		m_cameraProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdSceneEditor::OnCameraDefinitionChanged ), NULL, this );
	}

	// main control panel
	m_mainControlPanel = new SceneEditor::CMainControlPanel( containerPanel, this );

	// Panel layout
	{
		m_auiManager.SetManagedWindow( containerPanel );

		m_controlTabs =  new CEdAuiNotebook( containerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT );
		m_mainTabs =  new CEdAuiNotebook( containerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT );
		m_detailsTabs =  new CEdAuiNotebook( containerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP | wxAUI_NB_TAB_EXTERNAL_MOVE | wxAUI_NB_TAB_MOVE | wxAUI_NB_TAB_SPLIT );
		m_detailsTabs->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( CEdSceneEditor::OnDetailsTabsPageChanged ), NULL, this );

		m_mainTabs->AddPage( m_screenplayPanel, wxT( "Screenplay" ), false );
		m_mainTabs->AddPage( m_previewPanel, wxT( "Preview" ), true );
		m_mainTabs->AddPage( m_startingConditionsGrid, TXT("Starting Conditions"), false );
		m_mainTabs->AddPage( definitionsPanel, TXT( "Definitions" ), false );
		m_mainTabs->AddPage( debugTabPanel, wxT( "Debug" ) );
		m_mainTabs->AddPage( plotPanel, TXT( "FCurves" ), false );

		m_detailsTabs->AddPage( propertiesPanel, wxT( "Properties" ), false );
		m_detailsTabs->AddPage( cameraTabPanel, wxT( "Cameras" ), false );
		m_detailsTabs->AddPage( m_controlRigPanel, wxT( "Control rig" ), false );
		m_detailsTabs->AddPage( mimicsControlRigPanel, TXT( "Mimics CRig" ), false );
		m_detailsTabs->AddPage( m_animTreeBody, TXT( "Body Anims" ), false );
		m_detailsTabs->AddPage( m_animTreeMimics, TXT( "Mimics Anims" ), false );
		m_detailsTabs->AddPage( netPanel, TXT( "Network" ), false );
		//m_detailsTabs->AddPage( plotPanel, TXT( "FCurves" ), false );
		m_detailsTabs->AddPage( m_mainControlPanel, "Section", false );

		m_controlTabs->AddPage( graphPanel, wxT( "Graph" ), false );
		m_controlTabs->AddPage( timelinePanel, wxT( "Timeline" ), true );
		m_controlTabs->AddPage( m_dialogsetPanel, TXT( "Dialogset settings" ), false );

		m_auiManager.AddPane( m_controlTabs, wxCENTER );
		m_auiManager.AddPane( m_mainTabs, wxTOP );
		m_auiManager.AddPane( m_detailsTabs, wxRIGHT );

		m_auiManager.GetPane( m_mainTabs ).CloseButton( false ).Name( wxT( "Main" ) ).CaptionVisible( true ).BestSize( 400, 400 );
		m_auiManager.GetPane( m_detailsTabs ).CloseButton( false ).Name( wxT( "Details" ) ).CaptionVisible( true ).BestSize( 300, 400 );
		m_auiManager.GetPane( m_controlTabs ).CloseButton( false ).Name( wxT( "Controls" ) ).CaptionVisible( true ).BestSize( 600, 350 );
	}
	
	// Fill langs
	{
		wxChoice* choice = XRCCTRL( *this, "locChoice", wxChoice );

		TDynArray< String > textLanguages, speechLanguages;
		SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

		for ( Uint32 i=0; i<speechLanguages.Size(); ++i )
		{
			choice->AppendString( speechLanguages[ i ].AsChar() );
		}

		choice->SetStringSelection( SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() );
	}

	// Final layout setup
	SetSize( 1024, 768 );
	
	m_storyScene->AddToRootSet();

	LoadOptionsFromConfig();
	
	m_auiManager.Update();
	Layout();

	m_sceneGraphEditor->ZoomExtentsLater();

	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPreChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( EditorPropertyPostChange ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurrentLocaleChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneEditorEnvironmentsUpdate ), this );	
	SEvents::GetInstance().RegisterListener( CNAME( SimpleCurveChanged ), this );	

	m_databaseTimer = new CEdStringDbConnectionTimer();
	m_databaseTimer->Start( CEdStringDbConnectionTimer::CONNECTION_CHECK_TIME );

	// TODO: Connect with shortcuts editor
	wxAcceleratorEntry shortcuts[ 9 ];
	shortcuts[ 0 ].Set( wxACCEL_NORMAL,	'1',		wxID_STORYSCENEEDITOR_CAMERA_PREVIEW );
	shortcuts[ 1 ].Set( wxACCEL_NORMAL,	'2',		wxID_STORYSCENEEDITOR_CAMERA_FREE );
	shortcuts[ 2 ].Set( wxACCEL_NORMAL,	'3',		wxID_STORYSCENEEDITOR_CAMERA_EDIT );
	shortcuts[ 3 ].Set( wxACCEL_NORMAL,	'k',		wxID_STORYSCENEEDITOR_NEW_KEYFRAME );
	shortcuts[ 4 ].Set( wxACCEL_NORMAL,	'o',		wxID_STORYSCENEEDITOR_TOGGLEDOF );
	shortcuts[ 5 ].Set( wxACCEL_NORMAL,	WXK_SPACE,	wxID_STORYSCENEEDITOR_PLAY_TOGGLE );
	shortcuts[ 6 ].Set( wxACCEL_CTRL,	WXK_SPACE,	wxID_STORYSCENEEDITOR_PLAY_FROM_START );
	shortcuts[ 7 ].Set( wxACCEL_CTRL,	'z',		XRCID( "editUndo" ) ); // needed because menu item shortcuts don't work for floating AUI panes
	shortcuts[ 8 ].Set( wxACCEL_CTRL,	'y',		XRCID( "editRedo" ) ); // same as with editUndo

	// Set accelerator tables for all panels. Why not just set accelerator table once for this CEdSceneEditor?
	// Because accelerators would not work for floating AUI panes - this is a wxWidgets problem that's not yet fixed.
	// And, of course, not all panels have the same accelerators.

	wxAcceleratorTable shortcutsTable0( 7, shortcuts + 0);
	m_previewPanel->SetAcceleratorTable( shortcutsTable0 );
	timelinePanel->SetAcceleratorTable( shortcutsTable0 );

	wxAcceleratorTable shortcutsTable1( 4, shortcuts + 4 );
	graphPanel->SetAcceleratorTable( shortcutsTable1 );

	wxAcceleratorTable shortcutsTable2( 2, shortcuts + 6 );
	m_screenplayPanel->SetAcceleratorTable( shortcutsTable2 );
	m_startingConditionsGrid->SetAcceleratorTable( shortcutsTable2 );
	definitionsPanel->SetAcceleratorTable( shortcutsTable2 );
	debugTabPanel->SetAcceleratorTable( shortcutsTable2 );
	propertiesPanel->SetAcceleratorTable( shortcutsTable2 );
	cameraTabPanel->SetAcceleratorTable( shortcutsTable2 );
	m_dialogsetPanel->SetAcceleratorTable( shortcutsTable2 );

	{
		m_undoManager = new CEdUndoManager( GetOriginalFrame() );
		m_undoManager->AddToRootSet();
		m_undoManager->SetMenuItems( GetMenuBar()->FindItem( XRCID( "editUndo" ) ), GetMenuBar()->FindItem( XRCID( "editRedo" ) ) );

		m_sceneGraphEditor->SetUndoManager( m_undoManager );
		m_timeline->SetUndoManager( m_undoManager );
		m_actorsDefinitionsGrid->SetUndoManager( m_undoManager );
		m_propsDefinitionsGrid->SetUndoManager( m_undoManager );
		m_effectsDefinitionsGrid->SetUndoManager( m_undoManager );
		m_lightDefinitionsGrid->SetUndoManager( m_undoManager );
		m_startingConditionsGrid->SetUndoManager( m_undoManager );
		m_dialogsetPanel->SetUndoManager( m_undoManager );
		m_screenplayPanel->SetUndoManager( m_undoManager );
		m_preview->SetUndoManager( m_undoManager );
		m_propertiesBrowser->SetUndoManager( m_undoManager );
		m_cameraProperties->SetUndoManager( m_undoManager );
	}

	GFeedback->UpdateTaskProgress( 3, 10 );
	GFeedback->UpdateTaskInfo( TXT("Reloading screenplay") );

	m_screenplayPanel->ReloadScreenplay(); // must be called after the undoManager is set up.

	//m_screenplayPanel->EnablePreview( true );
	//m_sceneScriptEditorPage->RefreshDialog();
	m_screenplayPanel->FocusOnFirstSection();

	GFeedback->UpdateTaskProgress( 4, 10 );
	GFeedback->UpdateTaskInfo( TXT("Spawning entities") );

	m_flowCtrl.Init( this );
	m_modCtrl.Init( this );
	m_worldCtrl.Init( this, m_preview->GetPreviewWorld() );
	m_actorsProvider = CreateObject< CEdSceneActorsProvider >();
	m_actorsProvider->Init( this, m_storyScene );
	m_actorsProvider->AddToRootSet();

	GFeedback->UpdateTaskProgress( 5, 10 );
	GFeedback->UpdateTaskInfo( TXT("Creating scene controller") );

	m_controller.Init( this, m_storyScene );
	m_controller.Pause();
	if ( m_controller.GetPlayer() == nullptr )
	{
		GFeedback->ShowError( TXT("Failed to create scene player, the file is probably damaged.") );
		DestroyLater(this);
		return;
	}
	m_camera.Init( m_preview );
	m_debugger.Init();

	m_helperEntitiesCtrl.Init( this );

	LoadSectionNameFromConfig();

	m_storyScene->ConnectSceneModifier( &m_modCtrl );

	m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );

	m_dialogsetPanel->UpdateDialogsetList();

	RefreshCamerasList();

	GFeedback->UpdateTaskProgress( 10, 10 );
	GFeedback->EndTask();

	m_keyframeCtrl.Init();

	FreezeBodyAnimTree();
	FreezeMimicsAnimTree();
	UpdateLightPresets();

	LoadCustomLayout( wxCommandEvent() );

	UpdateScriptBlocks();

	ValidateSceneAfterOpen( GFeedback );

	m_redUserPrivileges = RetrieveRedUserPrivileges();
}

CEdSceneEditor::~CEdSceneEditor()
{
	if ( m_recordingInViewport )
	{
		wxCommandEvent dummyEvent;
		OnStopButton( dummyEvent );
	}

	// First unhook from the game viewport.
	if ( m_worldCtrl.IsGameMode() )
	{
		m_worldCtrl.SetPreviewMode();
	}

	GCommonGame->StartFade( true,  TXT( "Scene event fade" ), 0.0f );

	SaveSectionNameToConfig();
	SaveTracksStateToConfig();

	m_helperEntitiesCtrl.Destroy();

	m_controller.Destroy();

	m_worldRelinker.Destroy( GetWorld() );

	m_auiManager.UnInit();

	if ( m_storyScene )
	{
		m_storyScene->RemoveFromRootSet();
		m_storyScene = NULL;
	}

	m_actorsProvider->RemoveFromRootSet();

	{
		m_undoManager->RemoveFromRootSet();
		m_undoManager->Discard();
	}

	m_databaseTimer->Stop();
	delete m_databaseTimer;

	if( m_generatorDialog )
	{
		delete m_generatorDialog;
	}

	delete m_recordingInfo;

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().QueueEvent( RED_NAME( DialogEditorDestroyed ), nullptr );
#endif

	SEvents::GetInstance().UnregisterListener( this );
}


void CEdSceneEditor::UpdateScriptBlocks()
{
	#ifndef NO_EDITOR_GRAPH_SUPPORT
	if( CStorySceneGraph* graph =  m_storyScene->GetGraph() )
	{
		TDynArray< CGraphBlock* >& blocks = graph->GraphGetBlocks();
		for( CGraphBlock* it : blocks )
		{
			if( CStorySceneScriptingBlock* scriptBlock = Cast< CStorySceneScriptingBlock >( it ) )
			{
				scriptBlock->UpdateScriptInfo();
			}
		}
	}
	#endif
}

Bool CEdSceneEditor::CanModifyScene() const
{
	return true;
}

Bool CEdSceneEditor::CanModifyEntityFromWorld( const CEntity* e ) const
{
	EntityHandle h;
	h.Set( const_cast< CEntity* >( e ) );
	if ( !h.Get() )
	{
		return false;
	}

	// TODO - block static mesh components etc.
	//...

	return true;
}

void CEdSceneEditor::OnMenuExit( wxCommandEvent& event )
{
	Close();	
}

void CEdSceneEditor::OnSaveScene( wxCommandEvent& event )
{
	m_screenplayPanel->CommitChanges();

	CStoryScene* s = GetStoryScene();

	// DIALOG_TOMSINS_TODO - dlaczego to jest tu i po co?
	{
		TDynArray< CStorySceneInput* > sceneInputs;
		s->CollectControlParts( sceneInputs );

		for ( Uint32 i = 0; i < sceneInputs.Size(); ++i )
		{
			sceneInputs[i]->FillVoicetagMappings();
		}
	}

	CEdSceneValidator validator;
	CEdSceneValidator::SValidationOutput res = validator.Process( s );

	ProcessEditorState( res );

	if ( res.NumOfErrors() != 0 || res.NumOfWarnings() != 0 )
	{
		String report = CEdSceneValidator::GenerateHTMLReport( res.m_messages );
		HtmlBox( this, TXT("Scene validation report"), report );
	}

	//if( res.NumOfErrors() == 0 )
	{
		s->Save();

		SEdPopupNotification::GetInstance().Show( this, TXT("SAVE"), s->GetSceneName() );
	}
}

void CEdSceneEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/SceneScriptEditor") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor") );

	Bool showOnlyScriptTextsEnabled = false;
	if( m_screenplayPanel )
	{
		showOnlyScriptTextsEnabled = m_screenplayPanel->IsShowOnlyScriptTextsEnabled();
	}
	Bool showVoiceMarkers = false;
	if( m_timeline )
	{
		showVoiceMarkers = m_timeline->GetShowVoiceMarkers(); 	
	}
	Bool showLocalizationTooltips         = CEdDialogTranslationHelperHandler::AreTooltipsEnabled();

	config.Write( TXT("showOnlyScriptTextsEnabled"),  showOnlyScriptTextsEnabled ? 1 : 0 );
	config.Write( TXT("showLocalizationTooltips"),    showLocalizationTooltips ? 1 : 0 );
	config.Write( TXT("showVoiceMarkers"),  showVoiceMarkers ? 1 : 0 );
	if ( m_preview ) config.Write( TXT("trackSun"),  m_preview->IsTrackSunEnabled() ? 1 : 0 );
	config.Write( TXT("enableFloatingHelpers"), m_helperEntitiesCtrl.FloatingHelpersEnabled() ? 1 : 0 );
	config.Write( TXT("SCENE_EDITOR_LAYOUT_VERSION"), SCENE_EDITOR_LAYOUT_VERSION );
	
	String layoutPerspective = m_auiManager.SavePerspective().wc_str();
	config.Write( TXT("layout"), layoutPerspective );

	SaveSectionNameToConfig();
}

void CEdSceneEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/SceneScriptEditor") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor") );

	Bool showOnlyScriptTextsEnabled       = config.Read( TXT("showOnlyScriptTextsEnabled"), 0 ) == 1 ? true : false;
	Bool showLocalizationTooltips         = config.Read( TXT("showLocalizationTooltips"), 0 ) == 1 ? true : false;
	Bool showVoiceMarkers				  = config.Read( TXT("showVoiceMarkers"), 0 ) == 1 ? true : false;
	Bool isTrackSunEnabled				  = config.Read( TXT("trackSun"), 0 ) == 1 ? true : false;
	Bool floatingHelpersEnabled			  = config.Read( TXT("enableFloatingHelpers"), 0 ) == 1 ? true : false;
	Int32 layoutVersion					  = config.Read( TXT("SCENE_EDITOR_LAYOUT_VERSION"), -1 );

	String layoutPerspective = config.Read( TXT("layout"), String::EMPTY );

	wxMenuBar *menuBar = GetMenuBar();
	m_screenplayPanel->EnableShowOnlyScriptTexts( showOnlyScriptTextsEnabled );
	m_timeline->SetShowVoiceMarkers( showVoiceMarkers );
	menuBar->Check( XRCID( "ShowVoiceMarkers" ), showVoiceMarkers );
	menuBar->Check( XRCID( "OnlyText" ), showOnlyScriptTextsEnabled );
	CEdDialogTranslationHelperHandler::EnableTooltips( showLocalizationTooltips );
	menuBar->Check( XRCID( "LocalizationTooltips" ), showLocalizationTooltips );
	
	if ( m_preview )
	{
		m_preview->TrackSun( isTrackSunEnabled );
	}

	m_helperEntitiesCtrl.EnableFloatingHelpers( floatingHelpersEnabled );

	// layout version needs to match otherwise use default settings
	if ( layoutPerspective.Empty() == false && layoutVersion == SCENE_EDITOR_LAYOUT_VERSION )
	{
		m_auiManager.LoadPerspective( layoutPerspective.AsChar(), false );
	}
	
	//Layout();
}

void CEdSceneEditor::StoreTimelineLayout( Uint32 sectionId, const CTimelineLayout& trackLayout )
{
	m_timelineLayoutMgr.StoreLayout( sectionId, trackLayout );
}

/*
Gets timeline layout for specified section.

\param sectionId Specifies section for which to get timeline layout.
\return Timeline layout for specified section or nullptr if no such layout is stored.
*/
const CTimelineLayout* CEdSceneEditor::GetTimelineLayout( Uint32 sectionId ) const
{
	return m_timelineLayoutMgr.GetLayout( sectionId );
}

void CEdSceneEditor::SaveSectionNameToConfig()
{
	if ( m_controller.IsValid() )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		const String path = String::Printf( TXT("/Frames/SceneEditor/%s"), m_storyScene->GetDepotPath().AsChar() );
		CConfigurationScopedPathSetter pathSetter( config, path );

		const String sectionName = m_controller.GetCurrentSection() ? m_controller.GetCurrentSection()->GetName() : String::EMPTY;
		config.Write( TXT("sectionName"), sectionName );
	}
}

void CEdSceneEditor::LoadSectionNameFromConfig()
{
	if ( m_controller.IsValid() )
	{
		CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
		const String path = String::Printf( TXT("/Frames/SceneEditor/%s"), m_storyScene->GetDepotPath().AsChar() );
		CConfigurationScopedPathSetter pathSetter( config, path );

		const String sectionName = config.Read( TXT("sectionName"), String::EMPTY );
		if ( !sectionName.Empty() )
		{
			const CStorySceneSection* s = m_storyScene->FindSection( sectionName );
			m_controlRequest.RequestSection( s );
		}
	}
}

void CEdSceneEditor::SaveTracksStateToConfig()
{
	if ( m_timeline )
	{
		m_timeline->SaveTracksStateToConfig( m_storyScene->GetDepotPath() );
	}
}

void CEdSceneEditor::LoadTracksStateFromConfig()
{
	if ( m_timeline )
	{
		m_timeline->LoadTracksStateFromConfig( m_storyScene->GetDepotPath() );
	}
}

void CEdSceneEditor::OnEditCopy( wxCommandEvent& event )
{
	wxWindow* focusWindow = wxWindow::FindFocus();
	if ( IsFocusInsideWindow( focusWindow, GetScreenplayPanel() ) == true )
	{
		GetScreenplayPanel()->OnEditCopy( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, GetSceneGraphEditor() ) == true )
	{
		GetSceneGraphEditor()->OnEditCopy( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, m_timeline ) == true )
	{
		m_timeline->OnEditCopy( event );
	}
	else
	{
		event.Skip();
	}
}

void CEdSceneEditor::OnEditCut( wxCommandEvent& event )
{
	wxWindow* focusWindow = wxWindow::FindFocus();
	if ( IsFocusInsideWindow( focusWindow, GetScreenplayPanel() ) == true )
	{
		GetScreenplayPanel()->OnEditCut( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, GetSceneGraphEditor() ) == true )
	{
		GetSceneGraphEditor()->OnEditCut( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, m_timeline ) == true )
	{
		m_timeline->OnEditCut( event );
	}
	else
	{
		event.Skip();
	}
}

void CEdSceneEditor::OnEditPaste( wxCommandEvent& event )
{
	wxWindow* focusWindow = wxWindow::FindFocus();
	if ( IsFocusInsideWindow( focusWindow, GetScreenplayPanel() ) == true )
	{
		GetScreenplayPanel()->OnEditPaste( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, GetSceneGraphEditor() ) == true )
	{
		GetSceneGraphEditor()->OnEditPaste( event );
	}
	else if ( IsFocusInsideWindow( focusWindow, m_timeline ) == true )
	{
		if( !TimelineEditingEnabled() )
		{
			GFeedback->ShowMsg( TXT("Scene Editor"), TXT("Paste item - operation not allowed as timeline editing is disabled.") );
		}
		else
		{
			m_timeline->OnEditPaste( event );
		}
	}
	else
	{
		event.Skip();
	}
}

void CEdSceneEditor::OnEditDelete( wxCommandEvent& event )
{
	DoEditDelete();
}

void CEdSceneEditor::DoEditDelete()
{
	wxWindow* focusWindow = wxWindow::FindFocus();
	if ( IsFocusInsideWindow( focusWindow, GetScreenplayPanel() ) == true )
	{
		GetScreenplayPanel()->DoEditDelete();
	}
	else if ( IsFocusInsideWindow( focusWindow, GetSceneGraphEditor() ) == true )
	{
		wxCommandEvent deleteEvent( wxEVT_COMMAND_MENU_SELECTED );
		deleteEvent.SetId( wxID_STORYSCENEEDITOR_DELETESECTION );
		GetSceneGraphEditor()->GetEventHandler()->ProcessEvent( deleteEvent );
	}
}

void CEdSceneEditor::OnEditUndo( wxCommandEvent& event )
{
	m_undoManager->Undo();
}

void CEdSceneEditor::OnEditRedo( wxCommandEvent& event )
{
	m_undoManager->Redo();
}

void CEdSceneEditor::OnLargerFont( wxCommandEvent& event )
{
	m_screenplayPanel->ChangeFontSize( 1 );
}

void CEdSceneEditor::OnSmallerFont( wxCommandEvent& event )
{
	m_screenplayPanel->ChangeFontSize( -1 );
}

void CEdSceneEditor::OnPauseButton( wxCommandEvent& event )
{
	m_controller.TogglePause();
}

void CEdSceneEditor::OnStopButton( wxCommandEvent& event )
{
	// Stop recording
	if ( m_recordingInViewport )
	{
		Int32 fullScreenWidth = 0, fullScreenHeight = 0;
		Uint32 displayCount = ::wxDisplay::GetCount();
		for ( Uint32 i = 0; i < displayCount; i++ )
		{
			fullScreenWidth = max( fullScreenWidth, ::wxDisplay(i).GetGeometry().width );
			fullScreenHeight = max( fullScreenHeight, ::wxDisplay(i).GetGeometry().height );
		}
		if ( !m_recordingInfo->ubersample )
		{
			GRender->RequestResizeRenderSurfaces( fullScreenWidth, fullScreenHeight );
		}
		Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 30 );
		GRender->SetAsyncCompilationMode( true );
		wxTheFrame->GetWorldEditPanel()->GetParent()->Layout();
		GGame->ToggleContignous( FCSF_PNG );
		m_recordingInViewport = false;
		
		HACK_GetScenePreview()->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
		HACK_GetScenePreview()->GetViewport()->SetRenderingMask( wxTheFrame->GetFilterPanel()->GetViewportFlags( VFT_PREVIEW ) );

		delete m_recordingInfo;
		m_recordingInfo = nullptr;
	}

	m_controller.RestartSection();

	m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );
}

void CEdSceneEditor::OnContinueButton( wxCommandEvent& event )
{
	const Bool flag = event.GetInt() > 0;
	m_controller.SetAutoGoToNextSection( flag );
}

void CEdSceneEditor::OnPrevButton( wxCommandEvent& event )
{
	TDynArray< CStorySceneSection* > prevSections;
	GetPrecedingSections( m_controller.GetCurrentSection(), prevSections, m_storyScene );

	if ( prevSections.Size() == 1 )
	{
		RequestSection( prevSections[ 0 ] );
	}
	else if ( prevSections.Size() > 1 )
	{
		wxMenu sectionsMenu;

		for ( Uint32 i = 0; i < prevSections.Size(); ++i )
		{
			sectionsMenu.Append( i, prevSections[ i ]->GetName().AsChar() );
			sectionsMenu.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneEditor::OnPrevSectionMenu ), NULL, this ); 
		}

		PopupMenu( &sectionsMenu );
	}
}

void CEdSceneEditor::OnNextButton( wxCommandEvent& event )
{
	TDynArray< TPair< CStorySceneSection*, String > > nextSections;
	GetFollowingSections( m_controller.GetCurrentSection(), nextSections);

	if ( nextSections.Size() == 1 )
	{
		RequestSection( nextSections[ 0 ].m_first );
	}
	else if ( nextSections.Size() > 1 )
	{
		wxMenu sectionsMenu;

		for ( Uint32 i = 0; i < nextSections.Size(); ++i )
		{
			sectionsMenu.Append( i, nextSections[ i ].m_second.AsChar() );
			sectionsMenu.Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdSceneEditor::OnNextSectionMenu ), NULL, this ); 
		}

		PopupMenu( &sectionsMenu );
	}
}

void CEdSceneEditor::OnNextSectionMenu( wxCommandEvent& event )
{
	TDynArray< TPair< CStorySceneSection*, String > > nextSections;
	GetFollowingSections( m_controller.GetCurrentSection(), nextSections );

	Int32 choosenMenuId = event.GetId();
	if ( choosenMenuId >= 0 && choosenMenuId < (Int32) nextSections.Size() )
	{
		RequestSection( nextSections[ choosenMenuId ].m_first );
	}
}

void CEdSceneEditor::OnPrevSectionMenu( wxCommandEvent& event )
{
	TDynArray< CStorySceneSection* > prevSections;
	GetPrecedingSections( m_controller.GetCurrentSection(), prevSections, m_storyScene );

	Int32 choosenMenuId = event.GetId();
	if ( choosenMenuId >= 0 && choosenMenuId < (Int32) prevSections.Size() )
	{
		RequestSection( prevSections[ choosenMenuId ] );
	}
}

void CEdSceneEditor::OnResetButton( wxCommandEvent& event )
{
	//RebuildPlayer();
	RefreshPlayer();

	//m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );

	//m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnDebugTimelineItems( wxCommandEvent& event )
{
	m_timeline->ToggleDebugItems();
}

void CEdSceneEditor::OnRunEventsGenerator( wxCommandEvent& event )
{
	if( !TimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT("Scene Editor"), TXT("Event generator - operation not allowed as timeline editing is disabled.") );
		return;
	}

	if( m_generatorDialog == nullptr )
	{
		m_generatorDialog = new CEdStorySceneEventGeneratorSetupDialog( this );
	}
	m_generatorDialog->ReloadActorStatus();
	m_generatorDialog->Show();
	m_generatorDialog->Raise();
} 

void CEdSceneEditor::OnEvtGenerator_RunEventsGenerator( CDialogEventGeneratorConfig & dialogConfig )
{
	RED_FATAL_ASSERT( TimelineEditingEnabled(), "" );

	TDynArray< CStorySceneSection* > sectionsToProcess;
	if ( dialogConfig.m_localScope )
	{
		sectionsToProcess.PushBack( m_controller.GetCurrentSection() );
	}
	else
	{
		Uint32 numberOfSections = m_storyScene->GetNumberOfSections();
		for ( Uint32 i = 0; i < numberOfSections; ++i )
		{
			sectionsToProcess.PushBackUnique( m_storyScene->GetSection( i ) );
		}
	}

	CStorySceneEventGenerator generator( this, dialogConfig );



	GFeedback->BeginTask( TXT("Generating events"), true );
	for ( Uint32 i = 0; i < sectionsToProcess.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, sectionsToProcess.Size() );
		String info = TXT("Generating events for section: ") + sectionsToProcess[i]->GetName();
		GFeedback->UpdateTaskInfo( info.AsChar() );

		if( !GFeedback->IsTaskCanceled() )
		{
			CStorySceneSection* section = sectionsToProcess[i];
			generator.GenerateEvents( section );
		}	
	}
	GFeedback->EndTask();
	
	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

/*
Does one of following (depending on user's choice):
1. Removes all events from section variant associated with current locale (might be different than currently selected variant!)
2. Removes all events from all sections, for each section it processes variant associated with current locale.
*/
void CEdSceneEditor::OnClearEventsButton( wxCommandEvent& event )
{
	if( !TimelineEditingEnabled() )
	{
		GFeedback->ShowMsg( TXT("Scene Editor"), TXT("Clear events - operation not allowed as timeline editing is disabled.") );
		return;
	}

	TDynArray< CStorySceneSection* > sectionsToProcess;

	// Ask if user is sure
	Int32 userAnswer = YesNoCancel( TXT( "Do you want to remove all events and create default ones in all sections in this scene (or just the current section)?\n Yes - Apply to all sections\n No - Apply only to current section\n Cancel - Do not apply at all" ) );
	if( userAnswer == IDCANCEL )
	{
		return;
	}
	else if ( userAnswer == IDYES )
	{
		Uint32 numberOfSections = m_storyScene->GetNumberOfSections();
		for ( Uint32 i = 0; i < numberOfSections; ++i )
		{
			sectionsToProcess.PushBackUnique( m_storyScene->GetSection( i ) );
		}
	}
	else if ( userAnswer == IDNO )
	{
		sectionsToProcess.PushBack( m_controller.GetCurrentSection() );
	}

	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();

	for ( TDynArray< CStorySceneSection* >::iterator sectionIter = sectionsToProcess.Begin();
		sectionIter != sectionsToProcess.End(); ++sectionIter )
	{
		CStorySceneSection* section = *sectionIter;
		const CStorySceneSectionVariantId sectionVariantId = section->GetVariantUsedByLocale( currentLocaleId );
		section->RemoveAllEvents( sectionVariantId );
	}

	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnShowLocalizationTooltips( wxCommandEvent& event )
{
	CEdDialogTranslationHelperHandler::EnableTooltips( event.IsChecked() );
	event.Skip();
}

void CEdSceneEditor::OnShowStartingConditions( wxShowEvent& event )
{
	if ( event.IsShown() )
	{
		TDynArray< CStorySceneInput* > sceneInputs;
		m_screenplayPanel->HACK_GetStoryScene()->CollectControlParts( sceneInputs );

		for ( Uint32 i = 0; i < sceneInputs.Size(); ++i )
			sceneInputs[i]->FillVoicetagMappings();

		TDynArray< CObject* > & inputs = CastArray< CObject, CStorySceneInput >( sceneInputs );
		m_startingConditionsGrid->SetObjects( inputs );
	}
}

void CEdSceneEditor::OnTemporaryLipsync( wxCommandEvent& event )
{
	CEdDialogEditorTempLipsyncDlg dlg( this );
	dlg.ShowModal();
}

void CEdSceneEditor::OnEditMimics( wxCommandEvent& event )
{
	CEdEditMimicsDlg* dlg = new CEdEditMimicsDlg( this );
	dlg->Show();
}

void CEdSceneEditor::OnImportW2Strings( wxCommandEvent& event )
{
	CEdImportW2StringsDlg dlg( this );
	dlg.ShowModal();
}

void CEdSceneEditor::OnPropagateCameraLights( wxCommandEvent& event )
{
	// Get current section
	if ( GetStoryScene() == nullptr )
	{
		return;
	}
	CStorySceneSection* section = m_controller.GetCurrentSection();
	if ( section == nullptr )
	{
		return;
	}

	// Get current section variant id
	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId sectionVariantid = section->GetVariantUsedByLocale( currentLocaleId );

	// Collect camera lights
	TDynArray< CStorySceneEventCameraLight* > cameraLights;
	for ( auto eventGuid : section->GetEvents( sectionVariantid ) )
	{
		CStorySceneEvent* sevent = section->GetEvent( eventGuid );
		RED_ASSERT( sevent != nullptr );

		if ( sevent->GetClass()->IsA< CStorySceneEventCameraLight >() )
		{
			CStorySceneEventCameraLight* event = static_cast< CStorySceneEventCameraLight* >( sevent );
			CStorySceneEventCameraLight* clone = new CStorySceneEventCameraLight( 
				event->GetEventName(),
				event->GetSceneElement(),
				event->GetStartPosition(),
				CName::NONE,
				event->GetTrackName() );
			clone->m_cameralightType = event->m_cameralightType;
			clone->m_cameraOffsetFront = event->m_cameraOffsetFront;
			clone->m_lightMod1 = event->m_lightMod1;
			clone->m_lightMod2 = event->m_lightMod2;
			cameraLights.PushBack( clone );
		}
	}

	// Build a list of sections
	TDynArray< CStorySceneSection* > sections;
	for ( Uint32 i=0; i < GetStoryScene()->GetNumberOfSections(); ++i )
	{
		sections.PushBack( GetStoryScene()->GetSection( i ) );
	}

	// Ask the user the target sections
	TDynArray< Bool > isTarget;
	isTarget.Grow( sections.Size() );
	::memset( isTarget.Data(), 0, sizeof(Bool)*sections.Size() );
	Int32 button = FormattedDialogBox( wxT("Propagate Camera Lights to..."), wxString::Format( wxT("'Propagate the current section\\'s camera lights to:'H{V{M%s=150}=300|V{B@'&OK'|B'&Cancel'}}"),
		CEdFormattedDialog::ArrayToList( sections, []( CStorySceneSection* section ) { return section->GetName(); } ).AsChar() ), isTarget.Data() );
	if ( button != 0 ) // Cancel
	{
		return; 
	}

	// Check if there was any selection
	Bool anySelection = false;
	for ( Bool checked : isTarget )
	{
		anySelection = anySelection || checked;
	}
	if ( !anySelection )
	{
		return;
	}

	// Go through all sections to replace their camera lights with the collected
	for ( Uint32 i=0; i < GetStoryScene()->GetNumberOfSections(); ++i )
	{
		// Skip unchecked sections
		if ( !isTarget[i] )
		{
			continue;
		}

		section = GetStoryScene()->GetSection( i );
		
		// Find the first schedulable playable element in the section
		CStorySceneElement* bestElement = nullptr;
		for ( CStorySceneElement* element : section->GetElements() )
		{
			TDynArray< const CStorySceneElement* > elements;
			element->OnGetSchedulableElements( elements );
			for ( const CStorySceneElement* schedulable : elements )
			{
				if ( schedulable->IsPlayable() )
				{
					bestElement = element;
					break;
				}
			}
			if ( bestElement != nullptr )
			{
				break;
			}
		}
		if ( bestElement == nullptr )
		{
			bestElement = section->GetChoice();
		}

		// Get all section variants
		TDynArray< CStorySceneSectionVariantId > variantIds;
		section->EnumerateVariants( variantIds );

		// Put all the lights to all variants
		for ( CStorySceneSectionVariantId id : variantIds )
		{
			// Remove existing lights (if any) 
			section->RemoveAllEventsByClass< CStorySceneEventCameraLight >( id );

			// Add light events
			if ( bestElement != nullptr )
			{
				for ( CStorySceneEventCameraLight* event : cameraLights )
				{
					CStorySceneEventCameraLight* clone = new CStorySceneEventCameraLight( 
						event->GetEventName(),
						bestElement,
						event->GetStartPosition(),
						CName::NONE,
						event->GetTrackName() );
					clone->m_cameralightType = event->m_cameralightType;
					clone->m_cameraOffsetFront = event->m_cameraOffsetFront;
					clone->m_lightMod1 = event->m_lightMod1;
					clone->m_lightMod2 = event->m_lightMod2;
					section->AddEvent( clone, id );
				}
			}
		}
	}

	// Discard the collected events
	for ( CStorySceneEventCameraLight* event : cameraLights )
	{
		event->GetClass()->DestroyObject( event );
	}

	// Rebuild
	RebuildPlayer();
}

void CEdSceneEditor::OnDebuggerConnected( wxCommandEvent& event )
{
	m_debugger.EnableDebugging( event.IsChecked() );
}

void CEdSceneEditor::OnGenerateSpeakingToData( wxCommandEvent& event )
{
	m_screenplayPanel->OnGenerateSpeakingToData( event );
}

void CEdSceneEditor::OnShowVoiceMarkers( wxCommandEvent& event )
{
	m_timeline->SetShowVoiceMarkers( GetMenuBar()->IsChecked( XRCID( "ShowVoiceMarkers" ) ) );
	m_timeline->RefreshSection();
}

void CEdSceneEditor::OnUpdateSectionsOrder( wxCommandEvent& event )
{
	// Collect all story scene sections
	TDynArray< const CStorySceneSection* > unprocessedSections;
	m_screenplayPanel->HACK_GetStoryScene()->CollectControlParts( unprocessedSections );

	// Get all scene inputs
	TDynArray< CStorySceneInput* > sceneInputs;
	m_screenplayPanel->HACK_GetStoryScene()->CollectControlParts( sceneInputs );

	// Sort 'sceneInputs' by comment, inputName
	CStorySceneInputSorterHelper sorterPred;
	Sort( sceneInputs.Begin(), sceneInputs.End(), sorterPred );

	// Sort sections in sheet that has starts from scene inputs (has scene inputs)
	Uint32 sheetSectionIndex = 0; // '0' means the top of the sheet
	for ( Uint32 i = 0; i < sceneInputs.Size(); ++i )
	{
		TDynArray< const CStorySceneLinkElement* > sceneElements;
		sceneInputs[ i ]->GetAllNextLinkedElements( sceneElements );

		// Extract story scene sections only
		TDynArray< const CStorySceneSection* > sections;
		for ( Uint32 k = 0; k < sceneElements.Size(); ++k )
		{
			const CStorySceneSection* section = Cast< const CStorySceneSection >( sceneElements[ k ] );
			if ( section )
			{
				sections.PushBack( section );
			}
		}

		// Sort sections in sheet based on 'sections' order
		for ( Uint32 k = 0; k < sections.Size(); ++k )
		{
			const CStorySceneSection* section = sections[ k ];

			Bool isSuccess = m_screenplayPanel->MoveSectionPanel( section, sheetSectionIndex );
			if ( isSuccess )
			{
				// Advance index
				++sheetSectionIndex;
			}
			unprocessedSections.RemoveFast( section );
		}
	}


	// Find head sections without scene input
	TDynArray< const CStorySceneSection* > headAloneSections; // the head sections without scene input
	for ( Uint32 i = 0; i < unprocessedSections.Size(); ++i )
	{
		const CStorySceneSection* section = unprocessedSections[ i ];
		
		const TDynArray< CStorySceneLinkElement* > &previousLinkedElements = section->GetLinkedElements();
		if ( previousLinkedElements.Empty() )
		{
			headAloneSections.PushBack( section );
		}
	}

	// Sort sections in sheet that doesn't have scene inputs
	for ( Uint32 i = 0; i < headAloneSections.Size(); ++i )
	{
		const CStorySceneSection* section = headAloneSections[ i ];

		TDynArray< const CStorySceneLinkElement* > sceneElements;
		section->GetAllNextLinkedElements( sceneElements );
		for ( Uint32 k = 0; k < sceneElements.Size(); ++k )
		{
			const CStorySceneSection* section = Cast< const CStorySceneSection >( sceneElements[ k ] );
			if ( section )
			{
				Bool isSuccess = m_screenplayPanel->MoveSectionPanel( section, sheetSectionIndex );
				if ( isSuccess ) ++sheetSectionIndex;
			}
		}
	}
}

void CEdSceneEditor::OnToggleShowOnlyScriptTexts( wxCommandEvent& event )
{
	if ( m_screenplayPanel != NULL )
	{
		m_screenplayPanel->ToggleShowOnlyScriptTexts();
		m_screenplayPanel->RefreshDialog();
	}
}

void CEdSceneEditor::OnResize( wxSizeEvent& event )
{
	Freeze();
	Layout();
	Thaw();

	event.Skip();
}

void CEdSceneEditor::OnActorListChanged( wxCommandEvent& event )
{
	
}

void CEdSceneEditor::OnPropListChanged( wxCommandEvent& event )
{
	m_actorsProvider->Refresh( m_storyScene );
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::OnEffectListChanged( wxCommandEvent& event )
{

}

void CEdSceneEditor::OnLightListChanged( wxCommandEvent& event )
{
	m_helperEntitiesCtrl.DeselectAllHelpers();
	m_timeline->CancelSelection();
	RebuildLights();
}

void CEdSceneEditor::RebuildLights()
{
	const Bool needsRebuild = m_actorsProvider->RebuildLights( m_storyScene );
	if ( needsRebuild )
	{
		m_controlRequest.RequestRebuild();
	}
	else
	{
		m_controlRequest.RequestRefresh();
	}

	m_controlRequest.RequestRefreshTimeline();

	m_flowCtrl.RecalcOnlyEvents( Hack_GetPlayer() );
}

void CEdSceneEditor::RefreshLights()
{
	if ( m_actorsProvider )
	{
		m_actorsProvider->RefreshLights( m_storyScene );
	}

	m_controlRequest.RequestRefresh();
}

void CEdSceneEditor::ReloadSceneInputsTable()
{
	ASSERT( m_storyScene != NULL );
	TDynArray< CStorySceneInput* > sceneInputs;
	m_storyScene->CollectControlParts< CStorySceneInput >( sceneInputs );

	TDynArray< CObject* > inputObjects;
	for ( TDynArray< CStorySceneInput* >::iterator inputIter = sceneInputs.Begin();
		inputIter != sceneInputs.End(); ++inputIter )
	{
		inputObjects.PushBack( *inputIter );
	}
	m_startingConditionsGrid->SetObjects( inputObjects );
}

Bool CEdSceneEditor::PlayInGameplayWorld( Bool ingame )
{
	if ( ingame )
	{
		return PlayInWorld( false, false, true, false );
	}
	else
	{
		return PlayInWorld( true, false, false, false );
	}
}

Bool CEdSceneEditor::PlayInMainWorld( Bool ingame )
{
	if ( ingame )
	{
		return PlayInWorld( false, true, false, m_recordingInfo != nullptr );
	}
	else
	{
		return PlayInWorld( true, false, false, false );
	}
}

Bool CEdSceneEditor::PlayInWorld( Bool previewWorld, Bool mainWorld, Bool gameplayWorld, Bool record )
{
	SCENE_ASSERT( !(mainWorld && gameplayWorld) );
	if ( previewWorld )
	{
		SCENE_ASSERT( !(mainWorld || gameplayWorld) );
	}

	if ( !GGame || !GGame->GetActiveWorld() )
	{
		GFeedback->ShowMsg( TXT("Dialog Editor"), TXT("Editor cannot play dialog in game mode. Load world to main panel and close all tools.") );
		return false;
	}

	Bool ret = false;

	if ( mainWorld || gameplayWorld )
	{
		// if we're in preview mode then switch to game mode (if allowed)
		if ( m_worldCtrl.IsPreviewMode() || record )
		{
			if ( m_worldCtrl.CanUseGameMode() )
			{
#ifdef USE_UMBRA
				// inform Umbra gates about scene start
				( new CRenderCommand_SetCutsceneModeForGates( GGame->GetActiveWorld()->GetRenderSceneEx(), true ) )->Commit();
#endif // USE_UMBRA

				if ( mainWorld )
				{
					m_worldCtrl.SetGameMode();
					m_camera.SetCameraGameMode();
				}
				else // gameplayWorld
				{
					m_worldCtrl.SetGameplayMode();
					m_camera.SetCameraGameMode();
					m_camera.SyncFromGameplayCamera();
				}

				ret = true;
				MovePreviewWindow( true );
				MovePreviewToolbarToTimeline( true );
			}
			else
			{
				GFeedback->ShowMsg( TXT("Dialog Editor"), TXT("Editor cannot play dialog in game mode. Load world to main panel and close all tools.") );
				ret = false;
			}
		}
		// if we're in game mode then switch to preview mode
		else
		{
#ifdef USE_UMBRA
			// inform Umbra gates about scene end
			( new CRenderCommand_SetCutsceneModeForGates( GGame->GetActiveWorld()->GetRenderSceneEx(), false ) )->Commit();
#endif // USE_UMBRA

			m_worldCtrl.SetPreviewMode();
			m_camera.SetCameraPreviewMode();
			ret = false;
			MovePreviewWindow( false );
			MovePreviewToolbarToTimeline( false );
		}
	}
	else
	{
		if ( !m_worldRelinker.IsLinked() )
		{
			const TagList& list = GetCurrentDialogsetInstance()->GetPlacementTag();

			TDynArray< CNode* > taggedNodes;
			GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( list , taggedNodes, BCTO_MatchAll );

			if ( taggedNodes.Size() > 0 )
			{
				wxTheFrame->GetWorldEditPanel()->LookAtNode( taggedNodes[0] );
			}
		}

		ret = m_worldRelinker.ToggleLinkProcess( GetWorld(), m_storyScene );

		m_controlRequest.RequestRebuild();
	}

	if ( record )
	{
		if ( !m_controller.IsPaused() ) m_controller.TogglePause();
		m_controller.RestartSection();
		m_controller.TogglePause();
		m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );

		// Setup recording state
		switch ( m_recordingInfo->rate )
		{
		case 0: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 30 ); break;
		case 1: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 60 ); break;
		case 2: Red::System::Clock::GetInstance().GetTimer().SetScreenshotFramerate( 120 ); break;
		}
		GRender->SetAsyncCompilationMode( false );
		wxTheFrame->GetWorldEditPanel()->SetClientSize( m_recordingInfo->width, m_recordingInfo->height );
		wxTheFrame->GetWorldEditPanel()->GetViewport()->AdjustSize( m_recordingInfo->width, m_recordingInfo->height );
		if ( !m_recordingInfo->ubersample )
		{
			GRender->RequestResizeRenderSurfaces( m_recordingInfo->width, m_recordingInfo->height );
		}
		GGame->ToggleContignous( FCSF_PNG, m_recordingInfo->ubersample );
		m_recordingInViewport = true;
	}
	else
	{
		// we don't want dialog to play automatically
		if ( !m_controller.IsPaused() )
		{
			m_controller.TogglePause();
			m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );
		}
	}

	//CheckScenePlacement();

	return ret;
}

Bool CEdSceneEditor::IsFocusInsideWindow( wxWindow* focusWindow, wxWindow* panelToTest ) const
{
	if ( focusWindow == NULL || panelToTest == NULL )
	{
		return false;
	}
	
	return panelToTest->FindWindow( focusWindow->GetId() ) != NULL;
}

void CEdSceneEditor::OnTimelineResized( wxCommandEvent& event )
{
	
}

void CEdSceneEditor::OnResetLayout( wxCommandEvent& event )
{
	String layoutPerspective = TXT( "layout2|name=Controls;caption=;state=1792;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=600;besth=350;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Main;caption=;state=2044;dir=1;layer=0;row=0;pos=0;prop=100000;bestw=400;besth=400;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=Details;caption=;state=2044;dir=2;layer=1;row=0;pos=0;prop=100000;bestw=300;besth=400;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-148;floaty=482;floatw=316;floath=434|dock_size(5,0,0)=575|dock_size(1,0,0)=593|dock_size(2,1,0)=372|" );								
	m_auiManager.LoadPerspective( layoutPerspective.AsChar(), true );
	Layout();
}

void CEdSceneEditor::SaveCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor") );
	
	m_auiManager.Update();
	String layoutPerspective = m_auiManager.SaveWholeLayout().wc_str();
	config.Write( TXT("custom_layout"), layoutPerspective );

	SaveSectionNameToConfig();
}

void CEdSceneEditor::LoadCustomLayout( wxCommandEvent& event )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/SceneScriptEditor") );

	String layoutPerspective = config.Read( TXT("custom_layout"), String::EMPTY );

	if ( !layoutPerspective.Empty() )
	{
		m_auiManager.LoadWholeLayout( layoutPerspective, true );
	}
	else
	{
		OnResetLayout( wxCommandEvent() );
	}
}

CAnimatedComponent* CEdSceneEditor::GetAnimatedComponentForActor( const CName& actorName )
{
	ASSERT( m_preview != NULL );
	if( m_preview == NULL || actorName == CName::NONE )
	{
		return NULL;
	}

	CEntity* actor = GetSceneEntity( actorName );
	if ( actor != NULL )
	{
		return actor->GetRootAnimatedComponent();
	}
	return NULL;
}

CAnimatedComponent* CEdSceneEditor::GetCameraComponent()
{
	if ( CCamera* camera = m_controller.GetCamera() )
	{
		return camera->GetRootAnimatedComponent();
	}

	return NULL;
}

CAnimatedComponent* CEdSceneEditor::GetHeadComponentForActor( const CName& actorName )
{
	ASSERT( m_preview != NULL );
	CEntity* ent = GetSceneEntity( actorName );
	if ( ent )
	{
		if ( IActorInterface* actor = ent->QueryActorInterface() )
		{
			return actor->GetMimicComponent();
		}		
	}
	return NULL;
}

RED_DEFINE_STATIC_NAME( brightness );
RED_DEFINE_STATIC_NAME( attenuation );
RED_DEFINE_STATIC_NAME( marginFactor );
RED_DEFINE_STATIC_NAME( ambientLevel );
RED_DEFINE_STATIC_NAME( overrideColor );


void CEdSceneEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPropertyPreChange ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( propertyData.m_page == m_cameraProperties && propertyData.m_propertyName.AsString() == TXT( "cameraName" ) )
		{ 
			if ( StorySceneCameraDefinition* cam = propertyData.m_typedObject.As< StorySceneCameraDefinition >() )
			{
				m_oldCameraNameCache = cam->m_cameraName;
			}
		}
	}
	else if ( name == CNAME( EditorPropertyPostChange ) )
	{
		const CEdPropertiesPage::SPropertyEventData& propertyData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
		if ( propertyData.m_page == m_cameraProperties && propertyData.m_propertyName.AsString() == TXT( "cameraName" ) )
		{ 
			if ( StorySceneCameraDefinition* cam = propertyData.m_typedObject.As< StorySceneCameraDefinition >() )
			{
				CName newName = cam->m_cameraName;

				Uint32 count = m_storyScene->GetNumberOfSections();
				for ( Uint32 i = 0; i < count; ++i )
				{
					const TDynArray< CStorySceneEvent* >& events = m_storyScene->GetSection( i )->GetEventsFromAllVariants();
					Uint32 eventsCount = events.Size();
					for ( Uint32 j = 0; j < eventsCount; j++ )
					{ 
						CStorySceneEventCustomCameraInstance* camEvent = Cast< CStorySceneEventCustomCameraInstance >( events[j] );
						if ( camEvent && m_oldCameraNameCache == camEvent->GetCustomCameraName() )
						{
							camEvent->SetCustomCameraName( newName );
						}
					}
				}

				cam->ParseCameraParams();
				RefreshCamerasList();
			}
		}
		if ( propertyData.m_page == m_propertiesBrowser && propertyData.m_propertyName.AsString() == TXT( "dialogsetChangeTo" ) )
		{ 
			m_controlRequest.RequestRebuild();
		}
		if ( propertyData.m_page == m_propertiesBrowser && propertyData.m_propertyName.AsString() == TXT( "backgroundLine" ) )
		{ 
			const CEdPropertiesPage::SPropertyEventData& propertyData = GetEventData< CEdPropertiesPage::SPropertyEventData >( data );
			CStorySceneLine* line = propertyData.m_typedObject.As< CStorySceneLine >();
			SCENE_ASSERT( line );
			SCENE_ASSERT( line->GetSection() );

			if ( line->GetSection()->IsA< CStorySceneCutsceneSection >() )
			{
				line->SetAsBackgroundLine( true );
			}
			else
			{
				RelinkAllEventsForElement( line );
			}
		}
		CClass* propParent = propertyData.m_typedObject.m_class;
		if ( propertyData.m_page == m_propertiesBrowser && propParent && propParent->IsA( ClassID< CStorySceneEventAnimClip >() ) )
		{ 
			if ( propertyData.m_propertyName.AsString() == TXT( "weight" ) )
			{
				RefreshPlayer();
			}
			else
			{
				// TODO - this is too aggressive
				RebuildPlayer();
			}
		}
		else if ( propertyData.m_page == m_propertiesBrowser )
		{ 
			if ( const StorySceneCameraDefinition* def = propertyData.m_typedObject.As< StorySceneCameraDefinition >() )
			{
				OnCameraDefinitionChanged( def );
			}

			if ( propertyData.m_typedObject.m_class->IsA( ClassID< CStorySceneEventOverridePlacement >() ) && propertyData.m_propertyName.AsString() == TXT( "placement" ) )
			{
				RefreshPlayer();			
			}
		}
		if ( !propertyData.m_page && propertyData.m_propertyName.AsString() == TXT( "transform" ) )
		{
			if ( const CNode* node = propertyData.m_typedObject.As< CNode >() )
			{
				m_helperEntitiesCtrl.OnWidgetPostChange( node );
			}
		}
	}
	else if ( name == CNAME( SelectionChanged ) )
	{		
		const CSelectionManager::SSelectionEventData& eventData = GetEventData< CSelectionManager::SSelectionEventData >( data );
		m_helperEntitiesCtrl.HandleSelection( eventData.m_world );	
	}
	else if ( name == CNAME( CurrentLocaleChanged ) )
	{
		wxChoice* locChoice = XRCCTRL( *this, "locChoice", wxChoice );
		locChoice->SetStringSelection( SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() );

		m_controlRequest.RequestRebuild();
		m_controlRequest.RequestRefreshTimeline();
	}
	else if( name == CNAME( SceneEditorEnvironmentsUpdate ) )
	{
		CWorld* envWorld = GetEventData< CWorld* >( data );
		if ( envWorld && GetWorld() == envWorld )
		{
			m_controlRequest.RequestRefresh();		
		}		
	}
	else if ( name == CNAME( SimpleCurveChanged ) )
	{	
		const SSimpleCurveChangeData& curveData = GetEventData< SSimpleCurveChangeData >( data );
		if ( curveData.m_propertyName == CNAME( radius ) || curveData.m_propertyName == CNAME( angleOffset ) || curveData.m_propertyName == CNAME( brightness ) ||
			 curveData.m_propertyName == CNAME( attenuation ) || curveData.m_propertyName == CNAME( ambientLevel ) || curveData.m_propertyName == CNAME( marginFactor ) || CNAME( overrideColor ) )
		{
			m_controlRequest.RequestRefresh();		
		}		
	}
}

void CEdSceneEditor::RefreshPropertiesPage()
{
	m_propertiesBrowser->RefreshValues();
	m_timeline->RefreshPropPageValues();
}

const CStorySceneDialogsetInstance* CEdSceneEditor::GetCurrentDialogsetInstance() const
{
	return m_controller.GetCurrentDialogsetInstance();
}

CEdSceneEditor* CEdSceneEditor::RetrieveSceneEditorObject( CPropertyItem* propertyItem )
{
	ASSERT( propertyItem != NULL );

	wxWindow* sceneEditorWindow = propertyItem->GetPage();
	while ( sceneEditorWindow != NULL && sceneEditorWindow->IsKindOf( wxCLASSINFO( CEdSceneEditor ) ) == false )
	{
		sceneEditorWindow = sceneEditorWindow->GetParent();
	}

	CEdSceneEditor* sceneEditor = static_cast< CEdSceneEditor* >( sceneEditorWindow );

	return sceneEditor;
}

void CEdSceneEditor::OnAddPlayerToActors( wxCommandEvent& event )
{
	CStoryScene* scene = GetStoryScene();
	ASSERT( scene != NULL );

	const CGameResource* gameResource = GGame->GetGameResource();
	if ( gameResource == NULL )
	{
		// No game resource!!!
		GFeedback->ShowError( TXT( "No game resource to get player data from!!!" ) );
		return;
	}

	CEntityTemplate* playerTemplate = gameResource->GetPlayerTemplate().Get();
	if ( playerTemplate == NULL )
	{
		// No player template
		GFeedback->ShowError( TXT( "No player template defined in game resource" ) );
		return;
	}

	CName playerID = CName::NONE;
	TDynArray< CName > appearances = playerTemplate->GetEnabledAppearancesNames();
	if ( appearances.Size() > 0 )
	{
		playerID = playerTemplate->GetApperanceVoicetag( appearances[ 0 ] );
	}
	if ( playerID == CName::NONE )
	{
		// No player voicetag use default
		playerID = CNAME( GERALT );
	}

	if ( scene->GetActorDescriptionForVoicetag( playerID ) != NULL )
	{
		// Player already present. Exit without doing nothing
		return;
	}

	CStorySceneActor* playerActorSetting = CreateObject< CStorySceneActor >( scene );
	playerActorSetting->m_entityTemplate = gameResource->GetPlayerTemplate();
	playerActorSetting->m_actorTags = playerTemplate->GetEntityObject()->GetTags();
	playerActorSetting->m_id = playerID;
	scene->AddActorDefinition( playerActorSetting );
}

void CEdSceneEditor::SetGridDataFromProperty( CGridEditor* gridEditor, CObject* object, const CName& propertyName )
{
	CProperty* propertyPointer = object->GetClass()->FindProperty( propertyName );
	if ( propertyPointer != NULL )
	{
		gridEditor->SetObject( propertyPointer->GetOffsetPtr( object ), propertyPointer );
		gridEditor->SetDefaultObjectParent( object );
	}
}

void CEdSceneEditor::GetFollowingSections( const CStorySceneSection* _section, TDynArray< TPair< CStorySceneSection*, String > >& nextSections )
{
	CStorySceneSection* section = const_cast< CStorySceneSection* >( _section );

	TDynArray< CStorySceneControlPart* > connectedControlParts;

	for ( Uint32 i = 0; i < section->GetNumberOfInputPaths(); ++i )
	{
		CStorySceneLinkElement* nextLinkElement = section->GetInputPathLinkElement( i )->GetNextElement();
		CStorySceneControlPart* nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement );
		if ( nextControlPart == NULL && nextLinkElement != NULL)
		{
			nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement->GetParent() );
		}
		if ( nextControlPart != NULL )
		{
			nextControlPart->CollectControlParts( connectedControlParts );
		}
	}

	for ( Uint32 k = 0; k < connectedControlParts.Size(); ++k )
	{
		CStorySceneSection* nextSection = Cast< CStorySceneSection >( connectedControlParts[ k ] );
		if ( nextSection != NULL )
		{
			String sectionChoiceName = nextSection->GetName();
			nextSections.PushBackUnique( TPair< CStorySceneSection*, String >( nextSection, sectionChoiceName ) );
		}
	}

	CStorySceneChoice* sectionChoice = section->GetChoice();
	if ( sectionChoice != NULL )
	{
		for ( Uint32 j = 0; j < sectionChoice->GetNumberOfChoiceLines(); ++j )
		{
			connectedControlParts.Clear();
			CStorySceneChoiceLine* choiceLine = sectionChoice->GetChoiceLine( j );
			CStorySceneLinkElement* nextLinkElement = choiceLine->GetNextElement();
			CStorySceneControlPart* nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement );

			if ( nextControlPart == NULL && nextLinkElement != NULL)
			{
				nextControlPart = Cast< CStorySceneControlPart >( nextLinkElement->GetParent() );
			}
			if ( nextControlPart != NULL )
			{
				nextControlPart->CollectControlParts( connectedControlParts );
			}

			for ( Uint32 l = 0; l < connectedControlParts.Size(); ++l )
			{
				CStorySceneSection* nextSection = Cast< CStorySceneSection >( connectedControlParts[ l ] );
				if ( nextSection != NULL )
				{
					String sectionChoiceName = nextSection->GetName();
					sectionChoiceName += String::Printf( TXT( " (%s)" ), choiceLine->GetChoiceLine().AsChar() );

					nextSections.PushBackUnique( TPair< CStorySceneSection*, String >( nextSection, sectionChoiceName ) );
				}
			}
		}
	}
}

void CEdSceneEditor::GetPrecedingSections( const CStorySceneSection* _section, TDynArray< CStorySceneSection* >& prevSections, CStoryScene* storyScene )
{
	CStorySceneSection* section = const_cast< CStorySceneSection* >( _section );

	TDynArray< TPair< CStorySceneSection*, String > > followingSections;

	for ( Uint32 i = 0; i < storyScene->GetNumberOfSections(); ++i )
	{
		CStorySceneSection* sceneSection = storyScene->GetSection( i );

		if ( sceneSection == section )
		{
			continue;
		}

		followingSections.Clear();
		GetFollowingSections( sceneSection, followingSections );

		for ( Uint32 j = 0; j < followingSections.Size(); ++j )
		{
			if ( followingSections[ j ].m_first == section )
			{
				prevSections.PushBackUnique( sceneSection );
				break;
			}
		}
	}
}

CStorySceneSection* CEdSceneEditor::GetPrecedingSection( const CStorySceneSection* section )
{
	TDynArray< CStorySceneSection* > prevSections;
	GetPrecedingSections( section, prevSections, m_storyScene );
	return prevSections.Size() > 0 ? prevSections[ 0 ] : NULL;
}

void CEdSceneEditor::RequestSection( const CStorySceneSection* section )
{
	if( m_controller.IsValid() )
	{
		CStorySceneSection* currentSection = m_controller.GetCurrentSection();
		if( currentSection )
		{
			currentSection->SetVariantChosenInEditor( currentSection->GetVariantForcedInEditor() );
		}
	}

	m_controlRequest.RequestSection( section );
}

void CEdSceneEditor::RebuildPlayer_Lazy()
{
	::RunLaterOnceEx( new RebuildPlayerTask( this ) );
}

void CEdSceneEditor::RebuildPlayer()
{
	RebuildPlayer( m_controller.GetCurrentSection() );
}

void CEdSceneEditor::RefreshPlayer()
{
	m_controller.Refresh();
}

void CEdSceneEditor::RebuildPlayer( const CStorySceneSection* startFromSection )
{
	RED_FATAL_ASSERT( startFromSection, "CEdSceneEditor::RebuildPlayer(): section is nullptr." );

	// DIALOG_TOMSIN_TODO - co to jest za kod?

#ifdef DEBUG_SCENES 
	CTimeCounter timer;
#endif

	ScenePlayerInputState state = m_controller.Rebuild( startFromSection );

	ASSERT( !m_controller.IsChangingSection() );
	ASSERT( m_controller.GetCurrentSection() );

	ScenePlayerInputState tempState;
	tempState.RequestTime( state.m_time2 );
	tempState.RequestPause( state.m_paused2 );
	

#ifdef DEBUG_SCENES 
	const Float timeElapsedA = timer.GetTimePeriod();
#endif
	m_controller.ForceState( tempState );

	ASSERT( !m_controller.IsChangingSection() );
	ASSERT( m_controller.GetCurrentSection() );

	if ( !m_controller.GetCurrentSection() )
	{
		m_controller.Freeze();
	}
	

#ifdef DEBUG_SCENES
	const Float timeElapsedB = timer.GetTimePeriod();

	const Float timeElapsed = timeElapsedB;
	SCENE_LOG( TXT("RebuildPlayer: %1.5f - rebuild: %1.5f; force state: %1.5f"), timeElapsed, timeElapsedA, timeElapsedB-timeElapsedA );
#endif
}

void CEdSceneEditor::FreezeEditor()
{
	m_frozen = true;
}

void CEdSceneEditor::UnfreezeEditor()
{
	m_frozen = false;
}

Bool CEdSceneEditor::IsEditorFrozen() const
{
	return m_frozen;
}

void CEdSceneEditor::OnViewportCameraMoved()
{
	if ( m_camera.IsEditMode() || m_camera.IsFreeMode() || m_camera.IsGameplayMode() )
	{
		SaveCameraToDefinition( m_camera.GetSelectedDefinition() );

		// Notify the owner event of the change

		if ( CStorySceneEvent* event = m_camera.GetSelectedDefinitionOwner() )
		{
			if ( DialogTimelineItems::CTimelineItemEvent* item = m_timeline->FindItemEvent( event->GetGUID() ) )
			{
				item->UpdatePresentation();
			}
		}
	}
}

void CEdSceneEditor::ToggleTimePause()
{
	m_preview->ToggleTimePause();
}

extern void StartBehaviorGraphDebug( CEntity* entity );

void CEdSceneEditor::OnStartBehaviorDebug( wxCommandEvent& event )
{
	TDynArray< String > names;

	TDynArray< CName > actors = m_controller.GetActorIds( AT_ACTOR );
	TDynArray< CName > props = m_controller.GetActorIds( AT_PROP );

	for ( auto it : actors )
	{
		names.PushBack( it.AsString() );
	}

	for ( auto it : props )
	{
		names.PushBack( it.AsString() );
	}

	if ( names.Empty() == true )
	{
		return;
	}

	const String selectedActorName = InputComboBox( this, TXT( "Choose dialog actor" ), TXT( "Choose actor to debug behavior" ), names[ 0 ], names );
	
	if ( CEntity* selectedEntity = m_controller.GetSceneEntity( CName( selectedActorName ) ) )
	{
		StartBehaviorGraphDebug( selectedEntity );
	}
}

void CEdSceneEditor::OnSaveLayerPresets( wxCommandEvent& event )
{
	if( !GGame->GetActiveWorld() )
	{
		GFeedback->ShowWarn( TXT( "Load a world first." ) );
		return;
	}

	// create a temp preset in a temp preset manager
	String tempPreset( GGame->GetActiveWorld()->GetFriendlyName() );
	CEdScenePresetManager tempPresetManager;
	tempPresetManager.StoreGroupsTo( tempPreset );

	// store this preset string away
	GetStoryScene()->SetLayerPreset( tempPresetManager.GetPresetAsString( tempPreset ) );

	// save scene to disk
	OnSaveScene( event );
}

void CEdSceneEditor::OnLoadLayerPresets( wxCommandEvent& event )
{
	// has active world?
	if( !GGame->GetActiveWorld() )
	{
		GFeedback->ShowWarn( TXT( "Load a world first." ) );
		return;
	}

	// get preset string
	String preset = GetStoryScene()->GetLayerPreset();
	if( preset.Empty() || preset.ToLower() == TXT( "none" ) )
	{
		GFeedback->ShowWarn( TXT( "No layer preset stored for this story scene." ) );
		return;
	}

	// load preset
	CEdScenePresetManager tempPresetManager;
	String presetName;
	if( !tempPresetManager.SetPresetFromString( preset, presetName ) )
	{
		GFeedback->ShowWarn( TXT( "Failed to load preset from string." ) );
		return;
	}

	// does world match preset name?
	if( !tempPresetManager.Has( GGame->GetActiveWorld()->GetFriendlyName() ) )
	{
		GFeedback->ShowWarn( TXT( "The preset does not match the loaded world." ) );
		return;
	}

	// restore groups
	tempPresetManager.RestoreGroupsFrom( GGame->GetActiveWorld()->GetFriendlyName() );
}

void CEdSceneEditor::OnCreateLightPreset( wxCommandEvent& event )
{
	RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Create light preset!" ) );

	// first make sure that we have the latest presets loaded.
	UpdateLightPresets();

	// get preset name
	String presetName = InputBox( this, TXT( "Preset name" ), TXT( "Enter preset name:" ), TXT( "" ) );
	if( presetName.Size() == 0 )
	{
		GFeedback->ShowError( TXT( "You need to enter a valid name." ) );
		return;
	}

	// check for duplicate names
	for( Uint32 i=0; i<m_lightPresets.Size(); ++i )
	{
		if( presetName == m_lightPresets[i].m_presetName )
		{
			GFeedback->ShowError( TXT( "There is already a preset with this name (%s)." ), presetName.AsChar() );
			return;
		}
	}

	// load preset CSV
	C2dArray* lightDefs = Cast< C2dArray >( GDepot->LoadResource( DIALOG_EDITOR_LIGHT_PRESETS_CSV ) );
	if( !lightDefs )
	{
		GFeedback->ShowError( TXT( "Failed to load preset CSV (%s)." ), DIALOG_EDITOR_LIGHT_PRESETS_CSV );
		return;
	}

	// add all the lights into a new preset
	const TDynArray< CStorySceneLight* > lights = m_storyScene->GetSceneLightDefinitions();
	for( Uint32 i=0; i<lights.Size(); ++i )
	{
		if( !lights[ i ] )
		{
			continue;
		}

		// create new preset
		LightPreset lp;
		lp.m_presetName = presetName;
		lp.m_lightID = lights[ i ]->m_id.AsString();
		lp.m_type = lights[ i ]->m_type;
		lp.m_innerAngle = lights[ i ]->m_innerAngle;
		lp.m_outerAngle = lights[ i ]->m_outerAngle;
		lp.m_softness = lights[ i ]->m_softness;
		lp.m_shadowCastingMode = lights[ i ]->m_shadowCastingMode;
		lp.m_shadowFadeDistance = lights[ i ]->m_shadowFadeDistance;
		lp.m_shadowFadeRange = lights[ i ]->m_shadowFadeRange;
		m_lightPresets.PushBack( lp );
	}

	// add CSV columns
	lightDefs->Clear();
	lightDefs->AddColumn( TXT( "PresetName" ), TXT( "Name" ) );
	lightDefs->AddColumn( TXT( "LightID" ), TXT( "ID" ) );
	lightDefs->AddColumn( TXT( "Type" ), TXT( "0" ) );
	lightDefs->AddColumn( TXT( "Color" ), TXT( "0" ) );
	lightDefs->AddColumn( TXT( "Radius" ), TXT( "5" ) );
	lightDefs->AddColumn( TXT( "Brightness" ), TXT( "1" ) );
	lightDefs->AddColumn( TXT( "InnerAngle" ), TXT( "30" ) );
	lightDefs->AddColumn( TXT( "OuterAngle" ), TXT( "45" ) );
	lightDefs->AddColumn( TXT( "Softness" ), TXT( "5" ) );
	lightDefs->AddColumn( TXT( "ShadowCastingMode" ), TXT( "0" ) );
	lightDefs->AddColumn( TXT( "ShadowFadeDistance" ), TXT( "0" ) );
	lightDefs->AddColumn( TXT( "ShadowFadeRange" ), TXT( "0" ) );
	lightDefs->AddColumn( TXT( "PosX" ), TXT( "" ) );
	lightDefs->AddColumn( TXT( "PosY" ), TXT( "" ) );
	lightDefs->AddColumn( TXT( "PosZ" ), TXT( "" ) );
	lightDefs->AddColumn( TXT( "Yaw" ), TXT( "" ) );
	lightDefs->AddColumn( TXT( "Pitch" ), TXT( "" ) );
	lightDefs->AddColumn( TXT( "Roll" ), TXT( "" ) );

	// save all presets rows
	for( Uint32 i=0; i<m_lightPresets.Size(); ++i )
	{
		TDynArray< String > rowData;

		rowData.PushBack( m_lightPresets[ i ].m_presetName );
		rowData.PushBack( m_lightPresets[ i ].m_lightID );
		rowData.PushBack( String::Printf( TXT( "%u" ), (Uint32)m_lightPresets[ i ].m_type ) );
		rowData.PushBack( String::Printf( TXT( "%u" ), m_lightPresets[ i ].m_color.ToUint32() ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_radius ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_brightness ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_innerAngle ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_outerAngle ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_softness ) );
		rowData.PushBack( String::Printf( TXT( "%u" ), (Uint32)m_lightPresets[ i ].m_shadowCastingMode ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_shadowFadeDistance ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), m_lightPresets[ i ].m_shadowFadeRange ) );

		Vector pos = m_lightPresets[ i ].m_transform.GetPosition();
		EulerAngles rot = m_lightPresets[ i ].m_transform.GetRotation();
		rowData.PushBack( String::Printf( TXT( "%f" ), pos.X ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), pos.Y ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), pos.Z ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), rot.Yaw ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), rot.Pitch ) );
		rowData.PushBack( String::Printf( TXT( "%f" ), rot.Roll ) );

		lightDefs->AddRow( rowData );
	}

	// save to disk
	lightDefs->Save();

	// update the light preset menu now that another preset has been added
	UpdateLightPresets();
}

void CEdSceneEditor::OnPreviewCameraPreviewMode( wxCommandEvent& event )
{
	OnPreview_PreviewCameraMode();
}

void CEdSceneEditor::OnPlayFromStart( wxCommandEvent& event )
{
	m_controller.RestartSection();
}

void CEdSceneEditor::OnPreviewCameraFreeMode( wxCommandEvent& event )
{
	OnPreview_FreeCameraMode();
}

void CEdSceneEditor::OnPreviewCameraEditMode( wxCommandEvent& event )
{
	OnPreview_EditCameraMode();
}

void CEdSceneEditor::OnNewKeyframe( wxCommandEvent& event )
{
	m_keyframeCtrl.OnNewKeyframe();
	m_controlRigPanel->OnNewKeyframe();
}

void CEdSceneEditor::OnPreviewPlayToggle( wxCommandEvent& event )
{
	wxCommandEvent dummyEvent;
	OnPlayToggle( dummyEvent );
}

void CEdSceneEditor::OnPreviewPlayFromStart( wxCommandEvent& event )
{
	wxCommandEvent dummyEvent;
	OnPlayFromStart( dummyEvent );
}

void CEdSceneEditor::OnRecord( wxCommandEvent& event )
{
	static Int32 index = 6, width, height, fps = 0;
	static Bool uber = true;
	static Bool noforce = true;
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

	if ( FormattedDialogBox( wxT("Record Scene"), wxT("V'Resolution'{T{'Predefined:'C('384x216''512x288''640x360''768x432''1024x576''1280x720''1920x1080''2560x1440''3840x2160''7680x4320''Custom')'...or custom:'|'Width:'I'Height:'I}}|V'Framerate'{'Predefined:'C('30fps''60fps''120fps')}|V'Options'{X'Ubersample'X'Disable near clip forcing'}|H{~B@'Record'|B'Cancel'}"), &index, &width, &height, &fps, &uber, &noforce ) == 0 )
	{
		if ( index != 10 )
		{
			width = res[index].w;
			height = res[index].h;
		}

		m_recordingInfo = new SRecordingInfo();
		m_recordingInfo->width = width;
		m_recordingInfo->height = height;
		m_recordingInfo->rate = fps;
		m_recordingInfo->ubersample = uber;
		PlayInMainWorld( true );

		HACK_GetScenePreview()->GetViewport()->ClearRenderingMask( SHOW_ALL_FLAGS );
		HACK_GetScenePreview()->GetViewport()->SetRenderingMask( wxTheFrame->GetFilterPanel()->GetViewportFlags( VFT_GAME ) );
	}
}

void CEdSceneEditor::OnPlayToggle( wxCommandEvent& event )
{
	m_controller.TogglePause();

	m_timeline->RefreshPlayPauseIcon( m_controller.IsPaused() );
}

void CEdSceneEditor::OnPlayOneFrame( wxCommandEvent& event )
{
	m_controller.PlayOneFrame();
}

void CEdSceneEditor::ProcessViewportTick( Float timeDelta )
{
#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer;
	static CTimeCounter funcCallGapTimer;
	const Float callGapTimer_time = funcCallGapTimer.GetTimePeriodMS();
#endif

	if ( m_controlRequest.m_propertyBrowserObject )
	{
		SetPropertiesBrowserObject( m_controlRequest.m_propertyBrowserObject, nullptr );

		m_controlRequest.m_propertyBrowserObject = NULL;
	}

	if ( !m_actorsProvider->AreAllActorsReady() )
	{
		m_controller.Freeze();

		m_actorsProvider->ProcessSpawningActors();
		
		if ( m_actorsProvider->AreAllActorsReady() )
		{
		}
	}

	if ( m_controlRequest.m_freeze )
	{
		m_controller.Freeze();

		m_controlRequest.ResetAllRequests();

		return;
	}

	if ( m_controlRequest.m_section )
	{
		SaveTracksStateToConfig();

		ScenePlayerInputState state;
		state.RequestSection( m_controlRequest.m_section );
		state.RequestPause( m_controller.IsPaused() );

		m_controller.Rebuild( state.m_section2 );
		if ( m_controller.SetState( state ) )
		{
			const CStorySceneSection* newSection = m_controller.GetCurrentSection();
			ASSERT( newSection == m_controlRequest.m_section );

			m_timeline->SetSection( newSection );
			//m_preview->SetupSectionPreview( newSection );
			m_sceneGraphEditor->SetContolPartActive( newSection );
			m_screenplayPanel->SetCurrentSection( const_cast< CStorySceneSection* >( newSection ) );
			m_mainControlPanel->UpdateUI();

			//...
			
			LoadTracksStateFromConfig();

			m_controlRequest.m_section = NULL;

			SEvents::GetInstance().QueueEvent( CNAME( SectionChanged ), EventDataCreator< CStorySceneSection* >().Create( newSection ) );
		}

		//...

		return;
	}
	
	if ( m_controlRequest.m_rebuildCurrentStateRequest )
	{
		RebuildPlayer();
		m_mainControlPanel->UpdateUI();
		m_controlRequest.m_rebuildCurrentStateRequest = false;
		m_controlRequest.m_refreshCurrentStateRequest = false;
	}

	if ( m_controlRequest.m_timeRequest )
	{
		ScenePlayerInputState state;
		state.RequestTime( m_controlRequest.m_timeValue );

		VERIFY( m_controller.ForceState( state ) );

		m_timeline->SetCurrentTime( m_controller.GetSectionTime() );

		m_controlRequest.m_timeValue = 0.f;
		m_controlRequest.m_timeRequest = false;

		m_keyframeCtrl.MoveDefaultHelperToSelectedEntity();

		return;
	}

	if ( m_controlRequest.m_refreshCurrentStateRequest )
	{
		RefreshPlayer();
		m_controlRequest.m_refreshCurrentStateRequest = false;
	}

	if ( m_controlRequest.m_refreshTimelineRequest )
	{
		m_timeline->RefreshSection();
		m_controlRequest.m_refreshTimelineRequest = false;
	}

	// handle scene line state request
	if ( m_controlRequest.m_sceneLineStateRequest.m_active )
	{
		ControlRequest::SceneLineStateRequest& req = m_controlRequest.m_sceneLineStateRequest;
		
		req.m_line->SetAsBackgroundLine( req.m_isBackgroundLine );
		RebuildPlayer();
		m_timeline->RefreshSection();

		req.Reset();
	}

	if ( m_controlRequest.m_eventDuringChange )
	{
		ASSERT( m_controlRequest.m_eventDuringChangeTimestamp != 0 );

		if ( m_controlRequest.m_eventDuringChangeTimestamp + 1 < GEngine->GetCurrentEngineTick() )
		{
			RebuildPlayer();
			m_timeline->RefreshSection();

			m_controlRequest.m_eventDuringChange = NULL;
			m_controlRequest.m_eventDuringChangeTimestamp = 0;
		}
	}

	{
		TDynArray< CEdSceneRunnable* > localRunnables = m_runnables;
		m_runnables.ClearFast();

		const Uint32 numR = localRunnables.Size();
		for ( Uint32 i=0; i<numR; ++i )
		{
			localRunnables[ i ]->Run();
		}
		localRunnables.ClearPtr();

		if ( m_runnables.Size() > 0 )
		{
			// Wait one frame
			return;
		}
	}

#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer_controllerTick;
#endif

	m_controller.Tick( timeDelta );

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timer_controllerTick_time = timer_controllerTick.GetTimePeriodMS();
#endif


#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer_timeline;
#endif
	// update timeline current time
	Float currentTime = m_controller.GetSectionTime();
	m_timeline->SetCurrentTime( currentTime );

	// if we crossed timeline time limits then request time change
	if( !m_controller.IsPaused() && m_timeline->TimeLimitsEnabled() )
	{
		Float timeLimitMin, timeLimitMax;
		m_timeline->GetTimeLimits( timeLimitMin, timeLimitMax );
		if( currentTime < timeLimitMin || timeLimitMax < currentTime )
		{
			m_controlRequest.RequestTime( timeLimitMin );
		}
	}

	if ( m_timeline->IsFrozen() )
	{
		m_timeline->Thaw();
	}
	m_timeline->Repaint();

	if( m_timeline->GetSection() != m_controller.GetCurrentSection() )
	{
		m_mainControlPanel->UpdateUI();
	}
	m_timeline->SetSection( m_controller.GetCurrentSection() );

	//if ( m_locCtrl.IsEnabled() )
	//{
	//	if ( m_locCtrl.ParseSection( m_controller.GetCurrentSection(), GFeedback ) )
	//	{
	//		m_locCtrl.RefreshWindow();
	//	}
	//}

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timer_timeline_time = timer_timeline.GetTimePeriodMS();
#endif

#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer_dialogset;
#endif

	if ( const CStorySceneDialogsetInstance* dialogset = GetCurrentDialogsetInstance() )
	{
		m_dialogsetPanel->MarkCurrentDialogsetInstance( dialogset->GetName() );
	}

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timer_dialogset_time = timer_dialogset.GetTimePeriodMS();
#endif

#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer_graph;
#endif

	m_sceneGraphEditor->MarkDebugControlPart( m_controller.GetCurrentSection() );

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timer_graph_time = timer_graph.GetTimePeriodMS();
#endif

	m_camera.Update( m_controller.GetCameraState() );

#ifdef PROFILE_DIALOG_EDITOR_TICK
	CTimeCounter timer_sound;
#endif
	// Sound and camera position
	{
		CCamera* camera = m_controller.GetCamera();
		if( camera )
		{
			StorySceneCameraState state = m_controller.GetCameraState();

			// Update camera position when the playback is in world editor so that systems that rely
			// on camera position (f.e distance calculations for culling effects) will use the proper
			// position instead of whatever happened to be the last in viewport before playback
			if ( m_worldCtrl.IsGameMode() )
			{
				GGame->GetActiveWorld()->UpdateCameraPosition( state.m_position );
			}

			// Update sound
			const THashMap< CName, THandle< CEntity > > actors = m_controller.GetPlayer()->GetSceneActors();
			for( auto i = actors.Begin(); i != actors.End(); ++i )
			{
				CEntity* entity = i->m_second;
				if( entity )
				{
					CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent( false );
					UpdateSoundListener( state.m_position, state.m_localToWorld.V[2], state.m_localToWorld.V[1], soundEmitterComponent );
				}
			}
		}
	}
#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timer_sound_time = timer_sound.GetTimePeriodMS();
#endif

#ifdef PROFILE_DIALOG_EDITOR_TICK
	const Float timeElapsed = Max< Float >( timer.GetTimePeriodMS(), 0.0001f );
	const Float gapTime = timeElapsed - timer_controllerTick_time - timer_timeline_time - timer_dialogset_time - timer_graph_time - timer_sound_time;
	SCENE_LOG( TXT("*********************************************************************") );
	SCENE_LOG( TXT("Dialog edior tick") );
	SCENE_LOG( TXT("Total time: %1.3f"), timeElapsed );
	SCENE_LOG( TXT("Call gap time: %1.3f"), callGapTimer_time );
	SCENE_LOG( TXT("Player: %1.3f [%1.2f]"), timer_controllerTick_time, timer_controllerTick_time / timeElapsed );
	SCENE_LOG( TXT("Timeline: %1.3f [%1.2f]"), timer_timeline_time, timer_timeline_time / timeElapsed );
	SCENE_LOG( TXT("Dialogset: %1.3f [%1.2f]"), timer_dialogset_time, timer_dialogset_time / timeElapsed );
	SCENE_LOG( TXT("Graph: %1.3f [%1.2f]"), timer_graph_time, timer_graph_time / timeElapsed );
	SCENE_LOG( TXT("Sound: %1.3f [%1.2f]"), timer_sound_time, timer_sound_time / timeElapsed );
	SCENE_LOG( TXT("Gap: %1.3f [%1.2f]"), gapTime, gapTime / timeElapsed );
	SCENE_LOG( TXT("*********************************************************************") );
	funcCallGapTimer.ResetTimer();
#endif
}

CWorld* CEdSceneEditor::GetWorld() const
{
	return m_worldCtrl.GetWorld();
}

IStorySceneDebugger* CEdSceneEditor::GetStorySceneDebugger()
{
	return &m_debugger;
}

CActor* CEdSceneEditor::GetSceneActor( const CName& actorName )
{
	return m_controller.GetSceneActor( actorName );
}

CEntity* CEdSceneEditor::GetSceneEntity( const CName& actorName )
{
	return const_cast< CEntity* >( static_cast< const CEdSceneEditor*>( this )->GetSceneEntity( actorName ) );
}

const CEntity* CEdSceneEditor::GetSceneEntity( const CName& actorName ) const
{
	return m_controller.GetSceneEntity( actorName );
}

IControllerSetupActorsProvider* CEdSceneEditor::ResetAndGetSceneContextActorsProvider()
{
	m_actorsProvider->ResetSceneContextActors();
	return m_actorsProvider;
}

void CEdSceneEditor::ClearAllSceneDataForActors()
{
	m_actorsProvider->ClearAllSceneDataForActors();
}

void CEdSceneEditor::ProcessGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	m_controlRigPanel->OnGenerateFragments( frame );

	const CRenderFrameInfo &info = frame->GetFrameInfo();
	if( info.IsShowFlagOn( SHOW_LightsBBoxes ) )
	{		
		const CEnvCameraLightsSetupParametersAtPoint &lightParamsSetup = info.m_envParametersArea.m_cameraLightsSetup;
		const SCameraLightsModifiersSetup &mods = info.m_cameraLightsModifiersSetup;	

		const CRenderCamera &camera = info.m_camera;
		Vector cameraFwd, cameraUp, cameraRight;
		CRenderCamera::CalcCameraVectors( EulerAngles ( 0, 0, camera.GetRotation().Yaw ), cameraFwd, cameraRight, cameraUp );	

		auto helper = [ frame, info, cameraFwd, cameraUp, cameraRight ]( const CEnvCameraLightParametersAtPoint& lightParams, const SCameraLightModifiers& lightModifier )
		{
			const Float radius = lightParams.GetRadius( lightModifier );
			const Vector finalColor = lightParams.GetColor( lightModifier, Vector::ZEROS, 0.f );
			if ( radius <= 0 || (0 == finalColor.X && 0 == finalColor.Y && 0 == finalColor.Z) )
			{
				return;
			}

			Vector offset = lightParams.GetOffsetForwardRightUp( lightModifier );
			Vector lightPos = info.m_camera.GetPosition() + cameraFwd * offset.X + cameraRight * offset.Y + cameraUp * offset.Z;
			frame->AddDebugSphere( lightPos, radius, Matrix::IDENTITY, Color( finalColor ) );
			frame->AddDebugSphere( lightPos, 0.15f, Matrix::IDENTITY, Color::LIGHT_MAGENTA );
		};	

		helper( lightParamsSetup.m_gameplayLight0, mods.m_modifiersByType[ECLT_Gameplay].m_lightModifier0 );
		helper( lightParamsSetup.m_gameplayLight1, mods.m_modifiersByType[ECLT_Gameplay].m_lightModifier1 );

		helper( lightParamsSetup.m_sceneLight0, mods.m_modifiersByType[ECLT_Scene].m_lightModifier0 );
		helper( lightParamsSetup.m_sceneLight1, mods.m_modifiersByType[ECLT_Scene].m_lightModifier1 );

		helper( lightParamsSetup.m_dialogLight0, mods.m_modifiersByType[ECLT_DialogScene].m_lightModifier0 );
		helper( lightParamsSetup.m_dialogLight1, mods.m_modifiersByType[ECLT_DialogScene].m_lightModifier1 );

		helper( lightParamsSetup.m_dialogLight0, mods.m_modifiersByType[ECLT_Interior].m_lightModifier0 );
		helper( lightParamsSetup.m_dialogLight1, mods.m_modifiersByType[ECLT_Interior].m_lightModifier1 );
	}
	if( info.IsShowFlagOn( SHOW_SoundListener ) )
	{
		StorySceneCameraState state = m_controller.GetCameraState();
		frame->AddDebugSphere( state.m_position, 0.1f, Matrix::IDENTITY, Color::CYAN );
		frame->AddDebug3DArrow(state.m_position, state.m_localToWorld.V[1].Normalized3(), 0.25f, 0.02f,0.04, 0.05f, Color::CYAN);
	}
}

Bool CEdSceneEditor::ProcessCalculateCamera( IViewport* view, CRenderCamera &camera ) const
{
	if ( !m_camera.CalculateCamera() )
	{
		return m_controller.CalculateCamera( view, camera );
	}

	return false;
}

Bool CEdSceneEditor::ProcessViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data, Bool& moveViewportCam )
{
	moveViewportCam = m_camera.CanMoveViewportCamera();

	if ( key == IK_Tab && action == IACT_Press )
	{
		if ( RIM_IS_KEY_DOWN( IK_Shift ) )
		{
			m_controlRigPanel->HideSkeletonIK();
			m_controlRigPanel->ShowSkeletonFK();
		}
		else
		{
			m_controlRigPanel->HideSkeletonFK();
			m_controlRigPanel->ShowSkeletonIK();
		}
	}
	else if ( !RIM_IS_KEY_DOWN( IK_Tab ) )
	{
		m_controlRigPanel->HideSkeletonFK();
		m_controlRigPanel->HideSkeletonIK();
	}

	if ( key == IK_K && action == IACT_Press )
	{
		wxCommandEvent nullEvent;
		OnNewKeyframe( nullEvent );
		return true;
	}
	else if ( key == IK_C && action == IACT_Press )
	{
		return CreateCustomCameraFromView();
	}

	if ( RIM_IS_KEY_DOWN( IK_Ctrl ) )
	{
		if ( key == IK_Up && action == IACT_Press )
		{
			m_timeline->GoToNextCameraEvent();
			return true;
		}
		else if ( key == IK_Down && action == IACT_Press )
		{
			m_timeline->GoToPrevCameraEvent();
			return true;
		}
		if ( key == IK_Left && action == IACT_Press )
		{
			m_timeline->GoToPrevFrame();
			return true;
		}
		if ( key == IK_Right && action == IACT_Press )
		{
			m_timeline->GoToNextFrame();
			return true;
		}
	}

	return false;
}

void CEdSceneEditor::OnToggleDoF( wxCommandEvent& commandEvent )
{
	Config::cvAllowDOF.Set( !Config::cvAllowDOF.Get() );
}

void CEdSceneEditor::RelinkAllEventsForElement( CStorySceneElement* element )
{
	// TODO
}

CStorySceneEvent* CEdSceneEditor::FindSelectedEvent( const CClass* c, const CName& actorId )
{
	CStorySceneEvent* selectedEvent( nullptr );

	TDynArray< CStorySceneEvent* > selectedEvts;
	m_timeline->GetSelectedEvents( selectedEvts );

	Bool found = false;
	for ( Uint32 k=0; k<selectedEvts.Size(); ++k )
	{
		CStorySceneEvent* e = selectedEvts[ k ];
		if ( e->GetSubject() == actorId )
		{
			if ( found )
			{
				SCENE_WARN( TXT("FindSelectedEvent - More than one CStorySceneEvent for actor %s at the same time!"), actorId.AsChar() );
			}

			found = true;
			selectedEvent = e;
		}
	}
	return selectedEvent;
}

CStorySceneEvent* CEdSceneEditor::FindEditorEvent( const CClass* c, const CName& actorId, const Float* time )
{
	const Float eventTime = time ? *time : m_controller.GetSectionTime();

	CStorySceneEvent* selectedEvent( nullptr );

	Bool found = false;
	TDynArray< CStorySceneEvent* > evts;
	m_controller.FindEventsByTime( c, eventTime, evts );

	TDynArray< CStorySceneEvent* > selectedEvts;
	m_timeline->GetSelectedEvents( selectedEvts );

	for ( Uint32 k=0; k<evts.Size(); ++k )
	{
		CStorySceneEvent* e = evts[ k ];
		if ( e->GetSubject() == actorId && selectedEvts.Exist( e ) )
		{
			if ( found )
			{
				SCENE_WARN( TXT("FindEditorEvent - More than one CStorySceneEvent for actor %s at the same time!"), actorId.AsChar() );
			}

			found = true;
			selectedEvent = e;
		}
	}

	for ( Uint32 k=0; k<evts.Size(); ++k )
	{
		CStorySceneEvent* e = evts[ k ];
		if ( e->GetSubject() == actorId )
		{
			if ( found )
			{
				SCENE_WARN( TXT("FindEditorEvent - More than one CStorySceneEvent for actor %s at the same time!"), actorId.AsChar() );
			}

			found = true;
			selectedEvent = e;
		}
	}

	return selectedEvent;
}

void CEdSceneEditor::UpdateLightPresets()
{
	// clear old presets
	m_lightPresets.Clear();

	// load presets
	C2dArray* lightPresets = Cast< C2dArray >( GDepot->LoadResource( DIALOG_EDITOR_LIGHT_PRESETS_CSV ) );	
	if ( lightPresets )
	{
		for ( Uint32 i = 0; i < lightPresets->GetNumberOfRows(); ++i )
		{
			LightPreset lp;
			Uint32 tmpI;
			Float tmpF;

			lp.m_presetName = lightPresets->GetValue( TXT("PresetName"), i );
			if( lp.m_presetName.Empty() )
			{
				continue;
			}

			lp.m_lightID = lightPresets->GetValue( TXT("LightID"), i );

			C2dArray::ConvertValue<Uint32>( lightPresets->GetValue( TXT("Type"), i ), tmpI );
			lp.m_type = (ELightType)tmpI;
			
			C2dArray::ConvertValue<Uint32>( lightPresets->GetValue( TXT("Color"), i ), tmpI );
			lp.m_color = Color( tmpI );

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Radius"), i ), tmpF );
			lp.m_radius = tmpF;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Brightness"), i ), tmpF );
			lp.m_brightness = tmpF;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("InnerAngle"), i ), tmpF );
			lp.m_innerAngle = tmpF;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("OuterAngle"), i ), tmpF );
			lp.m_outerAngle = tmpF;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Softness"), i ), tmpF );
			lp.m_softness = tmpF;

			C2dArray::ConvertValue<Uint32>( lightPresets->GetValue( TXT("ShadowCastingMode"), i ), tmpI );
			lp.m_shadowCastingMode = (ELightShadowCastingMode)tmpI;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("ShadowFadeDistance"), i ), tmpF );
			lp.m_shadowFadeDistance = tmpF;

			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("ShadowFadeRange"), i ), tmpF );
			lp.m_shadowFadeRange = tmpF;

			Vector pos;
			EulerAngles rot;
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("PosX"), i ), pos.X );
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("PosY"), i ), pos.Y );
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("PosZ"), i ), pos.Z );
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Yaw"), i ), rot.Yaw );
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Pitch"), i ), rot.Pitch );
			C2dArray::ConvertValue<Float>( lightPresets->GetValue( TXT("Roll"), i ), rot.Roll );
			lp.m_transform = EngineTransform( pos, rot );

			m_lightPresets.PushBack( lp );
		}
	}

	// add preset menu items in code
	wxMenuBar* menu = GetOriginalFrame()->GetMenuBar();
	if( menu )
	{
		Int32 id = menu->FindMenu( TXT( "Tool" ) );
		if( id != wxNOT_FOUND )
		{
			wxMenu* toolMenu = menu->GetMenu( id );
			if( toolMenu )
			{
				Int32 pos = toolMenu->FindItem( TXT( "Light Presets" ) );
				if( pos != wxNOT_FOUND )
				{
					// find light preset item
					// NOTE: for some reason the toolMenu->FindItem( TXT( "Light Presets" ) ) doesn't work... but looking through all the children does the trick					
					wxMenuItem* lightPresetItem = nullptr;
					for( Uint32 i=0; i<toolMenu->GetMenuItemCount(); ++i )
					{
						String label = String( toolMenu->FindItemByPosition( i )->GetItemLabel().c_str() );
						if( label == TXT( "Light Presets" ) )
						{
							lightPresetItem = toolMenu->FindItemByPosition( i );
						}
					}

					if( lightPresetItem && lightPresetItem->GetSubMenu() )
					{
						// remove old preset items
						wxMenu* subMenu = lightPresetItem->GetSubMenu();
						while( subMenu->GetMenuItemCount() > 2 )		// 1st is "create new preset", 2nd is separator, all others are preset menu items
						{
							wxMenuItem* oldPresetItem = subMenu->GetMenuItems()[ subMenu->GetMenuItemCount() - 1 ];
							subMenu->Delete( oldPresetItem );
						}

						// create new preset items
						THashMap< String, Bool > uniquePresetMap;
						Uint32 iPresetIndex = wxID_LIGHT_PRESET_LOAD_FIRST;
						for( Uint32 i=0; i<m_lightPresets.Size(); ++i )
						{
							// use a temporary hash map to make sure the presets only gets added once
							if( uniquePresetMap.FindPtr( m_lightPresets[ i ].m_presetName ) != nullptr )
							{
								continue;
							}
							uniquePresetMap.Insert( m_lightPresets[ i ].m_presetName, true );

							// add new preset menu item
							wxMenuItem* presetItem = new wxMenuItem();
							presetItem->SetItemLabel( m_lightPresets[ i ].m_presetName.AsChar() );
							presetItem->SetKind( wxITEM_NORMAL );							
							presetItem->SetId( iPresetIndex++ );
							subMenu->Append( presetItem );

							// check that we haven't added too many presets
							if( iPresetIndex > wxID_LIGHT_PRESET_LOAD_LAST )
							{
								WARN_EDITOR( TXT( "Too many light presets. Remove unused ones." ) );
								break;
							}
						}
					}
				}

				// Add recording menu
				toolMenu->Append( wxID_STORYSCENEEDITOR_RECORD, wxT("Record...") );
			}
		}
	}
}

CEdSceneHelperEntity* CEdSceneEditor::CreateHelperEntityForEvent( CStorySceneEvent* e )
{
	if ( !e )
	{
		return nullptr;
	}

	CEdSceneHelperEntity* helper = m_helperEntitiesCtrl.FindHelperById( e->GetGUID() );
	if( helper )
	{
		return helper;
	}

	if ( CStorySceneEventLookAt* le = Cast< CStorySceneEventLookAt >( e ) )
	{		
		if ( le->UsesStaticTarget() )
		{			
			if( CActor* actor = m_controller.GetSceneActor( le->GetSubject() ) )
			{
				SSceneHelperReferenceFrameSettings frameSettings( SSceneHelperReferenceFrameSettings::RF_ActorFrame );
				frameSettings.m_parentActor = actor;
				helper = m_helperEntitiesCtrl.CreateHelper( le->GetGUID(), le->GetTargetVectorRef() );
				helper->SetFrameSettings( frameSettings );			
			}						
		}
	}
	else if ( CStorySceneEventGameplayLookAt * le = Cast< CStorySceneEventGameplayLookAt >( e ) )
	{
		if ( le->UsesStaticTarget() )
		{
			m_helperEntitiesCtrl.CreateHelper( le->GetGUID(), le->GetTargetVectorRef() );
		}
	}
	else if ( CStorySceneEventLookAtDuration* le = Cast< CStorySceneEventLookAtDuration >( e ) )
	{
		if ( le->UsesStaticTarget() && le->UsesTwoTargets() )
		{		
			helper = m_helperEntitiesCtrl.CreateHelperForDurationLookatEvent( le->GetGUID(), le->GetEyesTargetRef(), le->GetBodyTargetRef() );
		}
		else if ( le->UsesStaticTarget() && !m_helperEntitiesCtrl.Exist( le->GetGUID() ) )
		{
			helper = m_helperEntitiesCtrl.CreateHelper( le->GetGUID(), le->GetBodyTargetRef() );
		}
	}
	else if ( CStorySceneEventOverridePlacement* oe = Cast< CStorySceneEventOverridePlacement >( e ) )
	{		
		helper = m_helperEntitiesCtrl.CreateHelper( oe->GetGUID(), oe->GetTransformRef() );
	}
 	else if ( CStorySceneEventWorldPropPlacement* sp = Cast<CStorySceneEventWorldPropPlacement>( e ) )
	{
		helper = m_helperEntitiesCtrl.CreateHelper( sp->GetGUID(), sp->GetTransformRef() );
	}
	else if ( CStorySceneEventScenePropPlacement* sp = Cast<CStorySceneEventScenePropPlacement>( e ) )
	{
		helper = m_helperEntitiesCtrl.CreateHelper( sp->GetGUID(), sp->GetTransformRef() );
	}
	else if ( CStorySceneEventLightProperties* le = Cast< CStorySceneEventLightProperties >( e ) )
	{
		helper = m_helperEntitiesCtrl.CreateHelperForLightEvent( le->GetGUID(), le->GetTransformRef() );
	}
	else if ( CStorySceneEventAttachPropToSlot* le = Cast< CStorySceneEventAttachPropToSlot >( e ) )
	{
		helper = m_helperEntitiesCtrl.CreateHelper( le->GetGUID(), le->GetTransformRef() );
	}

	return helper;
}

void CEdSceneEditor::UpdateHelperEntityForEvent( CStorySceneEvent* e )
{
	CEdSceneHelperEntity* helper = m_helperEntitiesCtrl.FindHelperById( e->GetGUID() );
	if( helper )
	{
		Bool selectHelper = UpdateHelperEntityForEvent( e, helper );
	}
}

Bool CEdSceneEditor::UpdateHelperEntityForEvent( CStorySceneEvent* e, CEdSceneHelperEntity* helper )
{
	if( CStorySceneEventLightProperties* le = Cast< CStorySceneEventLightProperties >( e ) )
	{
		CEdSceneHelperEntityForLightEvent* lHelper = static_cast< CEdSceneHelperEntityForLightEvent* >( helper );
		lHelper->SetLightId( le->GetSubject() );
		if( le->IsAttached() )
		{					
			if( CEntity* actor = m_controller.GetSceneEntity( le->GetAttachmentActor() ) )
			{
				SSceneHelperReferenceFrameSettings frameSettings( SSceneHelperReferenceFrameSettings::RF_BoneFrame );
				frameSettings.m_attachmentFlags = le->GetAttachmentFlags();
				frameSettings.m_boneName = le->GetAttachmentBone();
				frameSettings.m_parentComp = actor->GetRootAnimatedComponent();			
				helper->SetFrameSettings( frameSettings );						
			}											
		}
		else if ( le->UseGlobalCoords() )
		{
			helper->SetFrameSettings( SSceneHelperReferenceFrameSettings( SSceneHelperReferenceFrameSettings::RF_WorldFrame ) );
		}
		else
		{
			helper->SetFrameSettings( SSceneHelperReferenceFrameSettings() );
		}
	}
	if( CStorySceneEventAttachPropToSlot* ate = Cast< CStorySceneEventAttachPropToSlot >( e ) )
	{	
		CActor* actor = m_controller.GetSceneActor( ate->GetAttachmentActor() );	
		if( actor && ate->GetSlotName() && ate->IsActivation() && ate->SnapAtStart() )
		{
			SSceneHelperReferenceFrameSettings frameSettings( SSceneHelperReferenceFrameSettings::RF_SlotFrame );
			frameSettings.m_slotName = ate->GetSlotName();
			frameSettings.m_slotOwner = actor;			
			helper->SetFrameSettings( frameSettings );																				
		}
		else
		{
			return false;
		}
	}
	return true;
}

Bool CEdSceneEditor::GetVoiceDataPositions( CStorySceneLine* forLine, TDynArray< TPair< Float, Float > >* maxAmpPos, Float* voiceStartPos /*= NULL */, Float* voiceEndPos /*= NULL*/ )
{
	EdLipsyncCreator::SLineAnaResult result;

	const String& currentLocale = SLocalizationManager::GetInstance().GetCurrentLocale();

	if( !forLine || !SEdLipsyncCreator::GetInstance().AnalyzeLineDataSync(  forLine->GetVoiceFileName(), currentLocale.ToLower(), forLine->CalculateDuration( currentLocale ), result ) )
	{
		return false;
	}
	if( maxAmpPos ) 
	{
		*maxAmpPos = result.expressionPoints ;
	}
	voiceStartPos ? *voiceStartPos = result.voiceStartTime : 0;
	voiceEndPos ? *voiceEndPos = result.voiceEndTime : 0;
	return true;
}

void CEdSceneEditor::GoToEvent( const CStorySceneEvent* e )
{
	if ( e )
	{
		Float t = 0.f;

		if ( const DialogTimelineItems::CTimelineItemEvent* item = m_timeline->FindItemEvent( e ) )
		{
			t = item->CalcGoToThisEventTime();
		}
		else
		{
			Float d = 0.f;
			m_controller.GetEventAbsTime( e, t, d );
		}

		m_controlRequest.RequestTime( t );
	}
}

void CEdSceneEditor::SelectItemWithAnimation( const CStorySceneEvent* e )
{
	m_timeline->SelectOnlyOneItem( e );
	m_controlRequest.RequestRefreshTimeline();
}

CStorySceneEvent* CEdSceneEditor::CreateAndSelectEventAndItemWithAnimation( CClass* c, const CName& actorId )
{
	CStorySceneEvent* e = m_timeline->CreateEvent( c, m_controller.GetSectionTime(), actorId );
	if ( e )
	{
		SelectItemWithAnimation( e );
	}

	return e;
}

void CEdSceneEditor::SetFCurveDataToPlot( const TDynArray< String >& tracks, const TDynArray< TDynArray< TPair< Vector2, Vector > > >& data, Bool setFocus )
{
	m_curveEditor->SetDataToPlot( tracks, data );

	if ( setFocus )
	{
		// TODO
	}
}

void CEdSceneEditor::ResetFCurvePanel()
{
	m_curveEditor->RemoveAllCurves();
}

Bool CEdSceneEditor::SetTreeAnimationSelection( CStorySceneEvent* e )
{
	if ( e->GetClass()->IsA< CStorySceneEventAnimation >() || e->GetClass()->IsA< CStorySceneEventChangePose >() )
	{
		CStorySceneEventAnimClip* animEvt = static_cast< CStorySceneEventAnimClip* >( e );
		m_animTreeBody->SelectAnimation( animEvt->GetAnimationName() );
		return true;
	}
	else if ( e->GetClass()->IsA< CStorySceneEventMimicsAnim >() || e->GetClass()->IsA< CStorySceneEventMimics >() )
	{
		CStorySceneEventAnimClip* animEvt = static_cast< CStorySceneEventAnimClip* >( e );
		m_animTreeMimics->SelectAnimation( animEvt->GetAnimationName() );
		return true;
	}

	return false;
}

Bool CEdSceneEditor::IsAnyAnimTreeOpen() const
{
	return IsDetailPageOpened( PAGE_BODY_ANIMS ) || IsDetailPageOpened( PAGE_MIMICS_ANIMS );
}

void CEdSceneEditor::ResetBodyControlRigObject()
{
	m_controlRigPanel->SetData( nullptr, nullptr );
	m_controlRigPanel->Enable( false );
}

void CEdSceneEditor::SetBodyControlRigObject( CStorySceneEventPoseKey* e, CActor* a )
{
	if ( e && a )
	{
		m_controlRigPanel->SetData( e, a );
		m_controlRigPanel->Enable( true );
	}
	else
	{
		ResetBodyControlRigObject();
	}
}

void CEdSceneEditor::ResetMimicControlRigObject()
{
	m_mimicsControlRigEvt = nullptr;
	m_mimicsControlRig->SetData( nullptr, 0 );
	m_mimicsControlRig->Enable( false );
}

void CEdSceneEditor::SetMimicControlRigObject( CStorySceneEventPoseKey* e )
{
	m_mimicsControlRigBuffer.Resize( 148 );
	m_mimicsControlRigEvt = e;

	m_mimicsControlRig->Enable( true );
	m_mimicsControlRig->SetData( m_mimicsControlRigBuffer.Data(), m_mimicsControlRigBuffer.DataSize() );
}

void CEdSceneEditor::GetMimicIdleData( TDynArray<CName>& data, Float& poseWeight, CName actor ) const
{
	data.Resize( 4 );
	m_controller.GetCurrentActorAnimationMimicState( actor, data[0], data[1], data[2], data[3], poseWeight );
}

void CEdSceneEditor::OnPatToolControlsChanging()
{
	if ( m_mimicsControlRigAutoRefresh )
	{
		RefreshMimicKeyEventData();

		m_controlRequest.RequestRefresh();
	}
}

void CEdSceneEditor::OnPatToolControlsPostChanged()
{
	RefreshMimicKeyEventData();

	m_controlRequest.RequestRefresh();
}

void CEdSceneEditor::OnMimicsControlRigPanelAutoRefresh( wxCommandEvent& event )
{
	m_mimicsControlRigAutoRefresh = event.IsChecked();
}

void CEdSceneEditor::OnMimicsControlRigPanelRefresh( wxCommandEvent& event )
{
	m_controlRequest.RequestRefresh();
}

void CEdSceneEditor::OnMimicsControlRigPanelWeightRefresh( wxCommandEvent& event )
{
	m_controlRequest.RequestRefresh();
}

void CEdSceneEditor::OnDetailsTabsPageChanged( wxAuiNotebookEvent& event )
{
	if ( event.GetSelection() == PAGE_BODY_ANIMS )
	{
		RefreshBodyAnimTree();
		FreezeMimicsAnimTree();
	}
	else if ( event.GetSelection() == PAGE_MIMICS_ANIMS )
	{
		RefreshMimicsAnimTree();
		FreezeBodyAnimTree();
	}
	else
	{
		FreezeBodyAnimTree();
		FreezeMimicsAnimTree();
	}

	// TODO: Use this method for other pages.
	Int32 selectedPage = event.GetSelection();
	if( selectedPage != wxNOT_FOUND )
	{
		String pageText = m_detailsTabs->GetPageText( selectedPage );
		if( pageText == L"Section" )
		{
			RefreshMainControlPanel();
		}
	}
}

void CEdSceneEditor::RefreshMainControlPanel()
{
	m_mainControlPanel->UpdateUI();
}

void CEdSceneEditor::RefreshBodyAnimTree()
{
	const CName actor = m_timeline->GetSelectedEntityName();
	if ( m_animTreeBodySelectedEntity != actor )
	{
		if ( CEntity* e = GetSceneEntity( actor ) )
		{
			if ( CAnimatedComponent* ac = e->GetRootAnimatedComponent() )
			{
				m_animTreeBody->CloneAndUseAnimatedComponent( ac );
				m_animTreeBodySelectedEntity = actor;
			}
		}
	}
	else if ( !actor )
	{
		m_animTreeBody->UnloadEntity();
		m_animTreeBodySelectedEntity = actor;
	}

	m_animTreeBody->Unpause();
}

void CEdSceneEditor::RefreshMimicsAnimTree()
{
	const CName actor = m_timeline->GetSelectedEntityName();
	if ( m_animTreeMimicsSelectedEntity != actor )
	{
		if ( CEntity* e = GetSceneEntity( actor ) )
		{
			if ( IActorInterface* a = e->QueryActorInterface() )
			{
				if ( CMimicComponent* m = a->GetMimicComponent() )
				{
					m_animTreeMimics->CloneAndUseAnimatedComponent( m );
					m_animTreeMimicsSelectedEntity = actor;
				}
			}
		}
	}
	else if ( !actor )
	{
		m_animTreeMimics->UnloadEntity();
		m_animTreeMimicsSelectedEntity = actor;
	}

	m_animTreeMimics->Unpause();
}

void CEdSceneEditor::FreezeBodyAnimTree()
{
	m_animTreeBody->Pause();
}

void CEdSceneEditor::FreezeMimicsAnimTree()
{
	m_animTreeMimics->Pause();
}

void CEdSceneEditor::RefreshMimicKeyEventData()
{
	SCENE_ASSERT( m_mimicsControlRigEvt );

	if ( m_mimicsControlRigEvt )
	{
		m_mimicsControlRigEvt->SetTrackValuesRaw( m_controller.GetPlayer(), m_mimicsControlRigBuffer );
	}
}

void CEdSceneEditor::RemoveEvent( CStorySceneEvent* e )
{
	m_helperEntitiesCtrl.DestroyHelpers( e->GetGUID() );

	m_controller.GetCurrentSection()->RemoveEvent( e->GetGUID() );

	m_controlRequest.RequestRebuild();
	m_controlRequest.RequestRefreshTimeline();
}

void CEdSceneEditor::RunLaterOnceEx( CEdSceneRunnable* runnable )
{
	m_runnables.PushBack( runnable );
}

Float CEdSceneEditor::GetAnimationDurationFromEvent( const CStorySceneEventAnimClip* animClip )
{
	if( Cast< CStorySceneEventMimicsAnim >( animClip ) )
	{
		return GetMimicAnimationDuration( animClip->GetActor(), animClip->GetAnimationName() );
	}
	else if( Cast< CStorySceneEventCameraAnim >( animClip ) )
	{
		return GetCameraAnimationDuration( animClip->GetAnimationName() );
	}
	else
	{
		return GetBodyAnimationDuration( animClip->GetActor(), animClip->GetAnimationName() );
	}
}

Float CEdSceneEditor::GetBodyAnimationDuration( const CName& actorVoicetag, const CName& animationName )
{
	CEntity* actor = GetSceneEntity( actorVoicetag );
	if ( actor && actor->GetRootAnimatedComponent() && actor->GetRootAnimatedComponent()->GetAnimationContainer() )
	{
		const CSkeletalAnimationSetEntry* anim = actor->GetRootAnimatedComponent()->GetAnimationContainer()->FindAnimation( animationName );
		if ( anim )
		{
			return anim->GetDuration();
		}
	}
	return 1.f;
}

Float CEdSceneEditor::GetMimicAnimationDuration( const CName& actorVoicetag, const CName& animationName )
{
	CEntity* ent = GetSceneEntity( actorVoicetag );
	if ( IActorInterface* actor = ent->QueryActorInterface() )
	{
		if( actor->GetMimicComponent() && actor->GetMimicComponent()->GetAnimationContainer() )
		{
			const CSkeletalAnimationSetEntry* anim = actor->GetMimicComponent()->GetAnimationContainer()->FindAnimation( animationName );
			if ( anim )
			{
				return anim->GetDuration();
			}
		}
	}
	return 1.f;
}

Float CEdSceneEditor::GetCameraAnimationDuration( const CName& animationName )
{
	if ( CAnimatedComponent* ac = GetCameraComponent() )
	{
		if ( ac->GetAnimationContainer() )
		{
			const CSkeletalAnimationSetEntry* anim = ac->GetAnimationContainer()->FindAnimation( animationName );
			if ( anim )
			{
				return anim->GetDuration();
			}
		}
	}
	return 1.f;
}

const CActor* CEdSceneEditor::AsSceneActor( const CEntity* e ) const
{
	return m_controller.AsSceneActor( e );
}

const CEntity* CEdSceneEditor::AsSceneProp( const CEntity* e ) const
{
	return m_controller.AsSceneProp( e );
}

const CEntity* CEdSceneEditor::AsSceneLight( const CEntity* e ) const
{
	return m_controller.AsSceneLight( e );
}

String CEdSceneEditor::GetEntityIdStr( const CEntity* e ) const
{
	if ( const CActor* actor = Cast< CActor >( AsSceneActor( e ) ) )
	{
		return actor->GetVoiceTag().AsString();
	}
	else
	{
		return e->GetName();
	}
}

CStorySceneEvent* CEdSceneEditor::FindPrevEventOfClass( const CStorySceneEvent* evt, const CClass* cls )
{
	return const_cast< CStorySceneEvent* >( static_cast< const CEdSceneEditor*>( this )->FindPrevEventOfClass( evt, cls ) );
}

const CStorySceneEvent* CEdSceneEditor::FindPrevEventOfClass( const CStorySceneEvent* evt, const CClass* cls ) const
{
	if( CStorySceneElement* el = evt->GetSceneElement() )
	{
		if( const CStorySceneSection* sec = el->GetSection() )
		{
			auto IsBefore = []( const CStorySceneEvent* evt1, Uint32 evt1ElIndex, const CStorySceneEvent* evt2, Uint32 evt2ElIndex )->Bool
			{
				if( evt1ElIndex != evt2ElIndex )
				{
					return evt1ElIndex < evt2ElIndex;
				}
				else
				{
					return evt1->GetStartPosition() < evt2->GetStartPosition();
				}
			};

			const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
			const CStorySceneSectionVariantId sectionVariantid = sec->GetVariantUsedByLocale( currentLocaleId );
			
			const TDynArray< CGUID >& evGuids = sec->GetEvents( sectionVariantid );
			const TDynArray< CStorySceneElement* >& elems = sec->GetElements();
			const Uint32 elIndex = elems.GetIndex( el );
			
			Uint32 resElIndex = elems.Size();
			const CStorySceneEvent* res = nullptr;

			ASSERT( elIndex != -1 );

			for ( auto evGuid : evGuids )
			{
				const CStorySceneEvent* currEv = sec->GetEvent( evGuid );
				CStorySceneElement* iterElem = currEv->GetSceneElement();
				Uint32 iterElIndex = elems.GetIndex( iterElem );

				if( currEv->GetSubject() == evt->GetSubject() && currEv->GetClass() == cls &&
					IsBefore( currEv, iterElIndex, evt, elIndex ) && ( !res || IsBefore( res, resElIndex , currEv, iterElIndex ) ) )
				{
					resElIndex = iterElIndex;
					res = currEv;
				}
			}
			return res;
		}
	}

	return nullptr;
}

/*
Gets Red User privileges obtained at Scene Editor startup.
*/
const CRedUserPrivileges& CEdSceneEditor::GetRedUserPrivileges() const
{
	return m_redUserPrivileges;
}

//*******************************************************************************************************************

int CEdStringDbConnectionTimer::CONNECTION_CHECK_TIME = 200000;

void CEdStringDbConnectionTimer::Notify()
{
	if( SLocalizationManager::GetInstance().IsConnected() )
	{	
		if(SLocalizationManager::GetInstance().IsSQLConnectionValid() == false)
		{
			if(m_notified == false)
			{
				m_notified = true;
				GFeedback->ShowWarn(TXT("Lost connection to String sql database"));
			}
		}		
		else
		{
			m_notified = false;
		}
	}
}

void CEdSceneEditor::OnMenuDumpAnimations( wxCommandEvent& event )
{
	String defaultDir = wxEmptyString;
	String wildCard = TXT("TXT files (*.txt)|*.txt");
	wxFileDialog loadFileDialog( this, wxT("Seve txt"), defaultDir.AsChar(), wxT( "" ), wildCard.AsChar(), wxFD_SAVE );
	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		String loadPath = loadFileDialog.GetPath().wc_str();
		FILE* f = fopen( UNICODE_TO_ANSI( loadPath.AsChar() ), "w" );
		if( f )
		{
			for( Uint32 iSection = 0, numSections = m_storyScene->GetNumberOfSections(); iSection < numSections; ++iSection )
			{
				const CStorySceneSection* sec = m_storyScene->GetSection( iSection );
				const TDynArray< CStorySceneEvent* >& events = sec->GetEventsFromAllVariants();
				for( Uint32 iEvent = 0, numEvents = events.Size(); iEvent < numEvents; ++iEvent )
				{
					CStorySceneEvent* ev = events[iEvent];
					if( ev->GetClass()->IsA< CStorySceneEventAnimation >() )
					{
						CStorySceneEventAnimClip* animEvt = static_cast< CStorySceneEventAnimClip* >( ev );
						String animname = animEvt->GetAnimationName().AsString();
						fprintf( f, "%s\n", UNICODE_TO_ANSI( animname.AsChar() ) );
					}
					if( ev->GetClass()->IsA< CStorySceneEventMimicsAnim >() )
					{
						CStorySceneEventMimicsAnim* animEvt = static_cast< CStorySceneEventMimicsAnim* >( ev );
						String animname = animEvt->GetAnimationName().AsString();
						fprintf( f, "%s\n", UNICODE_TO_ANSI( animname.AsChar() ) );
					}
				}
			}
			fclose( f );
		}
	}
}

void CEdSceneEditor::OnRefreshVoiceovers( wxCommandEvent& event )
{
	// Ask for network path
	String path( TXT( "\\\\cdprs-id574\\w3_speech" ) );

	if( ! InputBox( this, TXT( "Source location" ), TXT( "Enter VO location:" ), path ) )
	{
		return;
	}

	if( ! path.EndsWith( TXT( "\\" ) ) )
	{
		path +=  TXT( "\\" );
	}

	String language = SLocalizationManager::GetInstance().GetCurrentLocale().ToLower();
	String pathWithLanguage	= path + language;

	String destPath( GFileManager->GetDataDirectory() + TXT( "speech\\" ) + language );

	TDynArray< LocalizedStringEntry > localizedStrings;
	m_storyScene->GetLocalizedStrings( localizedStrings );

	// create list of files to copy
	typedef TPair< String, String > Item; // m_first - src path, m_second - dst path
	TDynArray< Item > items;
	for( LocalizedStringEntry& locString : localizedStrings )
	{			
		if( !locString.m_voiceoverName.Empty() )
		{
			items.PushBack( Item( pathWithLanguage + TXT("\\audio\\") + locString.m_voiceoverName + TXT(".wav"), destPath + TXT("\\audio\\") + locString.m_voiceoverName + TXT(".wav") ) );
			items.PushBack( Item( pathWithLanguage + TXT("\\audio\\") + locString.m_voiceoverName + TXT(".ogg"), destPath + TXT("\\audio\\") + locString.m_voiceoverName + TXT(".ogg") ) );
			items.PushBack( Item( pathWithLanguage + TXT("\\lipsync\\") + locString.m_voiceoverName + TXT(".re"), destPath + TXT("\\lipsync\\") + locString.m_voiceoverName + TXT(".re") ) );
		}
	}

	// copy files
	Int32 failed = 0;
	String failedFiles;
	for( const Item& item : items )
	{
		if( !GFileManager->FileExist( item.m_second.AsChar() ) )
		{
			const Bool fileCopied = GFileManager->CopyFile( item.m_first.AsChar(), item.m_second.AsChar(), true );
			if( !fileCopied )
			{
				++failed;
				failedFiles += item.m_first + TXT("\n");
			}
		}
	}

	if ( failed > 0)
	{
		GFeedback->ShowWarn( String::Printf( TXT(" Failed downloading %d files\n %s"), failed, failedFiles.AsChar() ).AsChar() );
	}		 		
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
