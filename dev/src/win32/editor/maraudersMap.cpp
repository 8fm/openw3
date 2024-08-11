/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/deniedAreaComponent.h"
#include "../../common/game/communitySystem.h"
#include "../../common/game/actionPointComponent.h"
#include "../../common/game/storySceneVoicetagMapping.h"
#include "../../common/game/nodeStorage.h"
#include "../../common/game/communityArea.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storyScenePlayer.h"
#include "../../common/game/doorComponent.h"
#include "callbackData.h"
#include "assetBrowser.h"
#include "maraudersMapCanvas.h"
#include "maraudersMap.h"
#include "maraudersMapItems.h"
#include "maraudersMapAIDebugCanvas.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/stickerComponent.h"
#include "../../common/engine/meshTypeComponent.h"


// Event table
BEGIN_EVENT_TABLE( CMaraudersMap, wxSmartLayoutPanel )
END_EVENT_TABLE()

CMaraudersMap::CMaraudersMap( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("MaraudersMap"), true )
	, m_idAutoStartTool( XRCID("autoStartTool") )
	, m_idStartTool( XRCID("startTool") )
	, m_idPauseTool( XRCID("pauseTool") )
	, m_idStopTool( XRCID("stopTool") )
	, m_idGotoTool( XRCID("gotoTool") )
	, m_idTimeScaleUpTool( XRCID("timeScaleUpTool") )
	, m_idTimeScaleDownTool( XRCID("timeScaleDownTool") )
	, m_idSplitterTool( XRCID("splitterTool") )
	, m_lastKnownSashPos( 1 )
	, m_layersLabel( NULL )
	, m_globalInfoLabel( NULL )
	, m_descriptionLabel( NULL )
	, m_windowMenu( NULL )
	, m_lookAtLabel( NULL )
	, m_lookAtWindow( NULL )
	, m_lookAtSizer( NULL )
	, m_isDescriptionPanelVisible( true )
	, m_isLayersPanelVisible( false )
	, m_isGlobalInfoPanelVisible( false )
	, m_isFindPanelVisible( false )
	, m_isLookAtPanelVisible( false )
	, m_gameStartTime( 0.0f )
{
	// Get GUI elements
	m_toolBar = XRCCTRL( *this, "toolBar", wxToolBar );
	m_toolBar->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnToolBar ), 0, this );
	m_toolBar->EnableTool( m_idStartTool, true );
	m_toolBar->EnableTool( m_idPauseTool, false );
	m_toolBar->EnableTool( m_idStopTool, false );

	m_comboBoxToolbar = XRCCTRL( *this, "comboBoxToolbar", wxComboBox ); // combo box on toolbar
	m_comboBoxToolbar->SetWindowStyle( m_comboBoxToolbar->GetWindowStyle() | wxTE_PROCESS_ENTER );
	m_comboBoxToolbar->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CMaraudersMap::OnToolbarComboBoxEnter ), NULL, this );

	GetParent()->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CMaraudersMap::OnClose ), 0, this );
	GetParent()->Connect( wxEVT_SHOW, wxShowEventHandler( CMaraudersMap::OnShow ), 0, this );

	// Menu
	m_menuBar = GetMenuBar();
	Connect( XRCID( "menuItemExit" ), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnExit ), NULL, this );
	Connect( XRCID( "menuItemHelp" ), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnHelp ), NULL, this );

	// Menu Filters
	wxMenu *filtersMenu = m_menuBar->GetMenu( 1 );
	Int32 i = 0;
	for ( i = 0; i < FILTER_SIZE; ++i )
	{
		filtersMenu->Append( i, GetFriendlyFilterName( (EFilters)i ).AsChar(), TXT(""), true );
		Connect( i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFilters ), NULL, this );
	}

	// Menu Filters Type
	m_filtersTypeMenu = new wxMenu();
	m_filtersTypeMenuItemProcessing = i++;
	m_filtersTypeMenuItemVisibility = i++;
	m_filtersTypeMenuItemSelecting = i++;
	m_filtersTypeMenuItemFinding = i++;
	m_filtersTypeMenu->Append( m_filtersTypeMenuItemProcessing, TXT("Processing filter"), TXT(""), true );
	Connect( m_filtersTypeMenuItemProcessing, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFiltersType ), NULL, this );
	m_filtersTypeMenu->Append( m_filtersTypeMenuItemVisibility, TXT("Visibility filter"), TXT(""), true );
	Connect( m_filtersTypeMenuItemVisibility, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFiltersType ), NULL, this );
	m_filtersTypeMenu->Append( m_filtersTypeMenuItemSelecting, TXT("Selecting filter"), TXT(""), true );
	Connect( m_filtersTypeMenuItemSelecting, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFiltersType ), NULL, this );
	m_filtersTypeMenu->Append( m_filtersTypeMenuItemFinding, TXT("Finding filter"), TXT(""), true );
	Connect( m_filtersTypeMenuItemFinding, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFiltersType ), NULL, this );
	m_menuBar->Insert( 2, m_filtersTypeMenu, TXT("Filters type") );
	
	// Menu window
	m_windowMenuItemGlobalInfoId = i++;
	m_windowMenuItemLayersId = i++;
	m_windowMenuItemDescriptionId = i++;
	m_windowMenuItemFindId = i++;
	m_windowMenuItemLookAtId = i++;
	m_windowMenu = new wxMenu();
	m_windowMenu->Append( m_windowMenuItemGlobalInfoId, TXT("Show/hide global info panel"), TXT(""), true );
	Connect( m_windowMenuItemGlobalInfoId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnWindowShowHide ), NULL, this );
	m_windowMenu->Append( m_windowMenuItemLayersId, TXT("Show/hide layers panel"), TXT(""), true );
	Connect( m_windowMenuItemLayersId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnWindowShowHide ), NULL, this );
	m_windowMenu->Append( m_windowMenuItemFindId, TXT("Show/hide find panel"), TXT(""), true );
	Connect( m_windowMenuItemFindId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnWindowShowHide ), NULL, this );
	m_windowMenu->Append( m_windowMenuItemDescriptionId, TXT("Show/hide description panel"), TXT(""), true );
	Connect( m_windowMenuItemDescriptionId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnWindowShowHide ), NULL, this );
	m_windowMenu->Append( m_windowMenuItemLookAtId, TXT("Show/hide look at panel"), TXT(""), true );
	Connect( m_windowMenuItemLookAtId, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnWindowShowHide ), NULL, this );
	m_menuBar->Insert( 3, m_windowMenu, TXT("Window") );

	// Menu tools
	m_toolsMenu = new wxMenu();
	m_savedPlayerPosMenu = new wxMenu();
	m_toolsMenu->Append( i, TXT("Teleport player"), TXT("Teleports player in game world") );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnPlayerTeleport ), NULL, this );
	m_toolsMenu->Append( i, TXT("Teleport camera"), TXT("Teleports camera in editor of free camera in game") );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnCameraTeleport ), NULL, this );
	m_toolsMenu->Append( i, TXT("Goto map"), TXT("") );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnGotoMap ), NULL, this );
	m_toolsMenu->Append( i, TXT("Goto camera"), TXT("") );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnGotoCamera ), NULL, this );
	m_toolsMenu->Append( i, TXT("Follow player"), TXT(""), true );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFollowPlayer ), NULL, this );
	m_toolsMenu->Append( i, TXT("Save player position"), TXT(""), false );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnSavePlayerPos ), NULL, this );
	m_toolsMenu->Append( i, TXT("Restore player position"), m_savedPlayerPosMenu );
	i++;
	m_toolsMenu->Append( i, TXT("Clear saved player positions"), TXT(""), false );
	Connect( i++, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnClearPlayerPos ), NULL, this );

	m_menuBar->Insert( 4, m_toolsMenu, TXT("Tools") );

	// Toolbar
	m_choiceRefreshRate = XRCCTRL( *this, "choiceRefreshRate", wxChoice );
	m_choiceRefreshRate->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CMaraudersMap::OnChoiceRefreshRate ), NULL, this );
	m_textCtrlTime = XRCCTRL( *this, "textCtrlTime", wxTextCtrl );
	m_textCtrlTime->SetWindowStyleFlag( m_textCtrlTime->GetWindowStyleFlag() | wxTE_PROCESS_ENTER );
	m_textCtrlTime->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CMaraudersMap::OnTextCtrlGameTimeChange ), NULL, this );
	m_textCtrlWorldPosition = XRCCTRL( *this, "m_textCtrlWorldPosition", wxTextCtrl );

	// Set timer
	m_timer = new CEdTimer();
	m_timer->Connect( wxEVT_TIMER, wxCommandEventHandler( CMaraudersMap::OnTimer ), NULL, this );
	m_timer->Stop();

	m_splitterMain		= XRCCTRL( *this, "splitterMain", wxSplitterWindow );
	m_panelMain			= XRCCTRL( *this, "panelMain", wxPanel );
	m_panelSide			= XRCCTRL( *this, "panelSide", wxPanel );
	m_toolsNotebook		= XRCCTRL( *this, "toolsNotebook", wxNotebook );


	// Panel Main
	{
		wxBoxSizer *sizerMain = new wxBoxSizer( wxVERTICAL );
		m_canvas = new CMaraudersMapCanvas( m_panelMain );
		sizerMain->Add( m_canvas, 1, wxEXPAND, 0 );
		m_panelMain->SetSizer( sizerMain );
	}

	// Panel Side
	{
		wxBoxSizer *sizerSide = new wxBoxSizer( wxVERTICAL );

		// Description panel
		wxPanel* descriptionPanel = new wxPanel( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		m_toolsNotebook->AddPage( descriptionPanel, TXT( "Description" ) );
		m_descriptionLabel = new wxStaticText( descriptionPanel, wxID_ANY, TXT("Description") );
		m_descriptionWindow = new wxHtmlWindow( descriptionPanel );
		m_descriptionWindow->SetWindowStyle( wxBORDER_SUNKEN );
		m_descriptionWindow->SetWindowStyle( m_descriptionWindow->GetWindowStyle() | wxHSCROLL | wxVSCROLL );
		m_descriptionSizer = new wxBoxSizer( wxVERTICAL );
		m_descriptionSizer->Add( m_descriptionLabel, 0, wxALIGN_CENTER, 0 );
		m_descriptionSizer->Add( m_descriptionWindow, 1, wxEXPAND, 0 );
		m_descriptionWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( CMaraudersMap::OnDescriptionWinLinkClicked ), NULL, this );
		descriptionPanel->SetSizer( m_descriptionSizer );

		// Layers panel
		wxPanel* layersPanel = new wxPanel( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		m_toolsNotebook->AddPage( layersPanel, TXT( "Layers" ) );
		m_layersLabel = new wxStaticText( layersPanel, wxID_ANY, TXT("Layers") );
		m_layersWindow = new wxCheckListBox( layersPanel, wxID_ANY );
		m_layersWindow->SetWindowStyle( wxLB_OWNERDRAW );
		m_layersWindow->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( CMaraudersMap::OnCheckListBoxLayers ), NULL, this );
		m_layersWindow->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CMaraudersMap::OnLayersWindowRightMouseClick ), NULL, this );
		m_layersSizer = new wxBoxSizer( wxVERTICAL );
		m_layersSizer->Add( m_layersLabel, 0, wxALIGN_CENTER, 0 );
		m_layersSizer->Add( m_layersWindow, 1, wxEXPAND, 0 );
		layersPanel->SetSizer( m_layersSizer );

		// Look at panel
		wxPanel* lookAtsPanel = new wxPanel( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		m_toolsNotebook->AddPage( lookAtsPanel, TXT( "Look Ats" ) );
		m_lookAtLabel = new wxStaticText( lookAtsPanel, wxID_ANY, TXT("Look Ats") );
		m_lookAtWindow = new wxHtmlWindow( lookAtsPanel );
		m_lookAtWindow->SetWindowStyle( wxBORDER_SUNKEN );
		m_lookAtWindow->SetWindowStyle( m_descriptionWindow->GetWindowStyle() | wxHSCROLL | wxVSCROLL );
		m_lookAtSizer = new wxBoxSizer( wxVERTICAL );
		m_lookAtSizer->Add( m_lookAtLabel, 0, wxALIGN_CENTER, 0 );
		m_lookAtSizer->Add( m_lookAtWindow, 1, wxEXPAND, 0 );
		lookAtsPanel->SetSizer( m_lookAtSizer );

		// Find panel
		wxPanel* itemsFinderPanel = new wxPanel( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		m_toolsNotebook->AddPage( itemsFinderPanel, TXT( "Items finder" ) );
		m_findLabel = new wxStaticText( itemsFinderPanel, wxID_ANY, TXT("Items finder") );
		m_findUseTags = new wxCheckBox( itemsFinderPanel, wxID_ANY, TXT("Tag search") );
		m_findOnEnterSearch = new wxCheckBox( itemsFinderPanel, wxID_ANY, TXT("On Enter search") );
		m_findOnEnterSearch->SetValue( true );
		m_findTextInput = new wxTextCtrl( itemsFinderPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
		m_findTextInput->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CMaraudersMap::OnFindTextInput ), NULL, this );
		m_findTextInput->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CMaraudersMap::OnFindTextEnter ), NULL, this );
		m_findItemsList = new wxListBox( itemsFinderPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, 0, wxLB_HSCROLL );
		m_findItemsList->SetWindowStyle( m_findItemsList->GetWindowStyle() | wxLB_HSCROLL | wxHSCROLL | wxVSCROLL );
		m_findItemsList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CMaraudersMap::OnFindItemsSelected ), NULL, this );
		m_findItemsList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CMaraudersMap::OnFindItemsDoubleclicked ), NULL, this );
		m_findSizer = new wxBoxSizer( wxVERTICAL );
		wxBoxSizer *findCheckBoxesSizer = new wxBoxSizer( wxHORIZONTAL );
		findCheckBoxesSizer->Add( m_findUseTags, 0, wxALIGN_LEFT, 0 );
		findCheckBoxesSizer->Add( m_findOnEnterSearch, 0, wxALIGN_LEFT, 0 );
		m_findSizer->Add( m_findLabel, 0, wxALIGN_CENTER, 0 );
		m_findSizer->Add( findCheckBoxesSizer );
		m_findSizer->Add( m_findTextInput, 0, wxEXPAND, 0 );
		m_findSizer->Add( m_findItemsList, 1, wxEXPAND, 0 );
		itemsFinderPanel->SetSizer( m_findSizer );

		// AI Debugger
		wxSplitterWindow* aiDebuggerPanel = new wxSplitterWindow( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		aiDebuggerPanel->SetMinimumPaneSize( 50 );
		m_toolsNotebook->AddPage( aiDebuggerPanel, TXT( "AI debugger" ) );

		m_aiEventDescription = new wxHtmlWindow( aiDebuggerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		wxPanel* aiDebuggerTrackPanel = new wxPanel( aiDebuggerPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		aiDebuggerPanel->SplitVertically( m_aiEventDescription, aiDebuggerTrackPanel, 200 );

		wxBoxSizer* tracksSizer = new wxBoxSizer( wxVERTICAL );
		m_aiDebugCanvas = new CAIHistoryDebugCanvas( aiDebuggerTrackPanel, this );
		tracksSizer->Add( m_aiDebugCanvas, 1, wxEXPAND, 0 );
		aiDebuggerTrackPanel->SetSizer( tracksSizer );

		// Global info panel
		wxPanel* globalInfoPanel = new wxPanel( m_toolsNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize );
		m_toolsNotebook->AddPage( globalInfoPanel, TXT( "Global info" ) );
		m_globalInfoMappinsCheckBox = new wxCheckBox( globalInfoPanel, wxID_ANY, TXT("Map pins") );
		m_globalInfoMappinsCheckBox->SetValue( true );
		m_globalInfoLabel = new wxStaticText( globalInfoPanel, wxID_ANY, TXT("Global Info") );
		m_globalInfoWindow = new wxHtmlWindow( globalInfoPanel );
		m_globalInfoWindow->SetWindowStyle( wxBORDER_SUNKEN );
		m_globalInfoWindow->SetWindowStyle( m_globalInfoWindow->GetWindowStyle() | wxHSCROLL | wxVSCROLL );
		wxBoxSizer *globalInfoCheckBoxesSizer = new wxBoxSizer( wxHORIZONTAL );
		globalInfoCheckBoxesSizer->Add( m_globalInfoMappinsCheckBox, 0, wxALIGN_LEFT, 0 );
		m_globalInfoSizer = new wxBoxSizer( wxVERTICAL );
		m_globalInfoSizer->Add( m_globalInfoLabel, 0, wxALIGN_CENTER, 0 );
		m_globalInfoSizer->Add( globalInfoCheckBoxesSizer );
		m_globalInfoSizer->Add( m_globalInfoWindow, 1, wxEXPAND, 0 );
		m_globalInfoWindow->Connect( wxEVT_COMMAND_HTML_LINK_CLICKED, wxHtmlLinkEventHandler( CMaraudersMap::OnDescriptionWinLinkClickedGlobalInfo ), NULL, this );
		globalInfoPanel->SetSizer( m_globalInfoSizer );
	}

	// Hidden layers
	m_layersHidden.PushBack( TXT("lores terrain") );
	m_layersHidden.PushBack( TXT("hires terrain") );
	m_layersHidden.PushBack( TXT("paths") );
	m_layersHidden.PushBack( TXT("skybox") );

	// Register listeners
	SEvents::GetInstance().RegisterListener( CNAME( GameStarted ), this );
	SEvents::GetInstance().RegisterListener( CNAME( GameEnded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WorldUnloaded ), this );

	// Set the icon
	wxIcon icon;
	icon.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MARMAP") ) );
	SetIcon( icon );

	// Update and finalize layout
	Layout();
	m_panelMain->Layout();
	Show();
	LoadOptionsFromConfig();
}

CMaraudersMap::~CMaraudersMap()
{
	SaveOptionsToConfig();

	m_timer->Stop();
	delete m_timer;
}

void CMaraudersMap::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( GameStarted ) )
	{
		if ( m_toolBar->GetToolState( m_idAutoStartTool ) )
		{
			// disable control toolbar
			m_toolBar->EnableTool( m_idStartTool, false );
			m_toolBar->EnableTool( m_idStopTool,  false );
			m_toolBar->EnableTool( m_idPauseTool, false );

			Init();
			m_gameStartTime = (Float)GGame->GetEngineTime();
			m_timer->Start( GetRefreshFactor() );
		}
	}
	else if ( name == CNAME( GameEnded ) )
	{
		m_timer->Stop();
		m_canvas->Deselect();
		m_canvas->RemoveInvalidItems();

		if ( !m_toolBar->GetToolState( m_idAutoStartTool ) )
		{
			// reset control toolbar
			m_toolBar->EnableTool( m_idStartTool, true );
			m_toolBar->EnableTool( m_idStopTool,  false );
			m_toolBar->EnableTool( m_idPauseTool, false );
		}
	}
	else if ( name == CNAME( WorldUnloaded ) )
	{
		m_timer->Stop();
		m_canvas->Reset();
	}
}

void CMaraudersMap::OnToolBar( wxCommandEvent &event )
{
	const int idSelected = event.GetId();

	// autostart 
	if ( idSelected == m_idAutoStartTool )
	{
		if ( m_toolBar->GetToolState( m_idAutoStartTool ) )
		{
			m_toolBar->EnableTool( m_idStartTool, false );
			m_toolBar->EnableTool( m_idStopTool,  false );
			m_toolBar->EnableTool( m_idPauseTool, false );
		}
		else
		{
			// is running
			if ( m_timer->IsRunning() )
			{
				m_toolBar->EnableTool( m_idStartTool, false );
				m_toolBar->EnableTool( m_idStopTool,  true );
				m_toolBar->EnableTool( m_idPauseTool, true );
			}

			// when is stopped
			else
			{
				m_toolBar->EnableTool( m_idStartTool, true );
				m_toolBar->EnableTool( m_idStopTool,  false );
				m_toolBar->EnableTool( m_idPauseTool, false );
			}
		}
	}

	// start
	else if ( idSelected == m_idStartTool )
	{
		m_toolBar->EnableTool( m_idStartTool, false );
		m_toolBar->EnableTool( m_idStopTool,  true );
		m_toolBar->EnableTool( m_idPauseTool, true );
		m_timer->Start( GetRefreshFactor() );

		// Initialize static objects (that will not change during game)
		if ( GGame->GetActiveWorld() )
		{
			Init();
		}
	}

	// pause
	else if ( idSelected == m_idPauseTool )
	{
		if ( m_timer->IsRunning() )
		{
			m_timer->Stop();
		}

		// resume
		else
		{
			m_timer->Start( GetRefreshFactor() );
		}
	}

	// stop
	else if ( idSelected == m_idStopTool )
	{
		m_timer->Stop();
		m_toolBar->EnableTool( m_idStartTool, true );
		m_toolBar->EnableTool( m_idStopTool,  false );
		m_toolBar->EnableTool( m_idPauseTool, false );
	}

	// time scale up
	else if ( idSelected == m_idTimeScaleUpTool )
	{
		Float timeScale = GGame->GetTimeScale() + 0.5f;
		if ( timeScale <= 5.0f )
		{
			GGame->SetOrRemoveTimeScale( timeScale, CNAME( MaraudersTimeScale ), 0x7FFFFFFF );
		}
	}

	// time scale down
	else if ( idSelected == m_idTimeScaleDownTool )
	{
		Float timeScale = GGame->GetTimeScale() - 0.5f;
		if ( timeScale > 0.0f )
		{
			GGame->SetOrRemoveTimeScale( timeScale, CNAME( MaraudersTimeScale ), 0x7FFFFFFF );
		}
	}

	// goto
	else if ( idSelected == m_idGotoTool )
	{
		GotoMapFromComboBox();
	}

	// splitter
	else if ( idSelected == m_idSplitterTool )
	{
		Bool isSplit = m_splitterMain->IsSplit();
		if ( isSplit )
		{
			m_lastKnownSashPos = m_splitterMain->GetSashPosition();
			m_splitterMain->Unsplit( m_panelSide );
		}
		else
		{
			m_splitterMain->SplitVertically( m_panelSide, m_panelMain, m_splitterMain->GetSashPosition() );
			m_splitterMain->SetSashPosition( m_lastKnownSashPos );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// on close window
void CMaraudersMap::OnClose( wxCloseEvent &event )
{
	SaveOptionsToConfig();

	m_canvas->Deselect();

	// remove all items from the canvas
	m_canvas->Reset();

	m_timer->Stop();

	// reset control toolbar
	if ( !m_toolBar->GetToolState( m_idAutoStartTool ) )
	{
		m_toolBar->EnableTool( m_idStartTool, true );
		m_toolBar->EnableTool( m_idStopTool,  false );
		m_toolBar->EnableTool( m_idPauseTool, false );
	}

	wxSmartLayoutPanel::OnClose( event );
}

void CMaraudersMap::OnChoiceRefreshRate( wxCommandEvent &event )
{
	if ( m_timer->IsRunning() )
	{
		m_timer->Start( GetRefreshFactor() );
	}
}

void CMaraudersMap::OnTextCtrlGameTimeChange( wxCommandEvent &event )
{
	String newTimeStr = m_textCtrlTime->GetValue();
	CTokenizer tokens( newTimeStr, TXT(":") );
	if ( tokens.GetNumTokens() != 3 )
	{
		m_canvas->AddLogInfo( TXT("Cannot change time - bad time format (should be HH:MM:SS)") );
		return;
	}
	Uint32 days  = GGame->GetTimeManager()->GetTime().Days();
	Int32 hours = 0;
	Int32 mins  = 0;
	Int32 secs  = 0;
	String strHours = tokens.GetToken( 0 ).AsChar();
	String strMins = tokens.GetToken( 1 ).AsChar();
	String strSecs = tokens.GetToken( 2 ).AsChar();
	Bool hoursPassed = FromString( strHours, hours );
	Bool minsPassed = FromString( strMins, mins );
	Bool secsPassed = FromString( strSecs, secs );
	if ( !hoursPassed || !minsPassed || !secsPassed )
	{
		m_canvas->AddLogInfo( TXT("Cannot change time - bad time format (should be HH:MM:SS)") );
		return;
	}
	GGame->GetTimeManager()->SetTime( GameTime(days, hours, mins, secs), false );
	m_canvas->AddLogInfo( TXT("Time changed.") );
}

void CMaraudersMap::OnCheckListBoxLayers( wxCommandEvent &event )
{
	int i = event.GetSelection();
	if ( i >= 0 )
	{
		if ( m_layersWindow->IsChecked( (Uint32)i ) )
		{
			m_canvas->ShowItem( i+1 );
		}
		else
		{
			m_canvas->HideItem( i+1 );
		}
	}
}

void CMaraudersMap::OnDescriptionWinLinkClickedGlobalInfo( wxHtmlLinkEvent &event )
{
}

void CMaraudersMap::OnToolbarComboBoxEnter( wxCommandEvent &event )
{
	if ( m_comboBoxToolbar->GetValue() != wxEmptyString )
	{
		m_comboBoxToolbar->Append( m_comboBoxToolbar->GetValue() );
	}
}

void CMaraudersMap::OnLayersWindowRightMouseClick( wxMouseEvent &event )
{
	wxPoint menuPos = event.GetPosition();
	wxMenu menu;
	menu.Append( 1, TXT("Select all") );
	menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnLayersWindowSelectAll ), NULL, this );
	menu.Append( 2, TXT("Deselect all") );
	menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnLayersWindowDeselectAll ), NULL, this );

	PopupMenu( &menu, menuPos );
}

void CMaraudersMap::OnLayersWindowDeselectAll( wxCommandEvent &event )
{
	const Uint32 layersCount = m_layersWindow->GetCount();
	for ( Uint32 i = 0; i < layersCount; ++i )
	{
		m_layersWindow->Check( i, false );
		m_canvas->HideItem( i+1 );
	}
}

void CMaraudersMap::OnLayersWindowSelectAll( wxCommandEvent &event )
{
	const Uint32 layersCount = m_layersWindow->GetCount();
	for ( Uint32 i = 0; i < layersCount; ++i )
	{
		m_layersWindow->Check( i, true );
		m_canvas->ShowItem( i+1 );
	}
}

void CMaraudersMap::OnDescriptionWinLinkClicked( wxHtmlLinkEvent &event )
{
	// Get the link
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	if( href.StartsWith( wxT("goto:") ) )
	{
		String position = href.AfterFirst(':').wc_str();
		m_comboBoxToolbar->SetValue( position.AsChar() );
	}
	else if( href.StartsWith( wxT("gotoandopenres:") ) )
	{
		String resPath = href.AfterFirst(':').wc_str();
		wxTheFrame->GetAssetBrowser()->SelectAndOpenFile( resPath );
	}
	else if( href.StartsWith( wxT("gotores:") ) )
	{
		String resPath = href.AfterFirst(':').wc_str();
		wxTheFrame->GetAssetBrowser()->SelectFile( resPath );
	}
	else if( href.StartsWith( wxT("goto_maritem_ap:") ) )
	{
		String apIDStr = href.AfterFirst(':').wc_str();
		TActionPointID apID;
		apID.FromString( apIDStr );
		m_canvas->GotoActionPointItem( apID );
	}
	else if( href.StartsWith( wxT("deactivate_communities:") ) )
	{
		String communityDepotPath = href.AfterFirst(':').wc_str();
		GCommonGame->GetSystem< CCommunitySystem >()->DebugDeactivateAllCommunitiesButThis( communityDepotPath );
	}
	else if( href.StartsWith( wxT("only_one_stub_mode:") ) )
	{
		String stubIdxStr = href.AfterFirst(':').wc_str();
		Int32 stubIdx = -1;
		FromString( stubIdxStr, stubIdx );
		GCommonGame->GetSystem< CCommunitySystem >()->DebugSetOnlyOneStubIdx( stubIdx );
	}
}

void CMaraudersMap::OnFindTextInput( wxCommandEvent &event )
{
	if ( !m_findOnEnterSearch->IsChecked() )
	{
		UpdateFindPanel( false );
	}
}

void CMaraudersMap::OnFindTextEnter( wxCommandEvent &event )
{
	UpdateFindPanel( true );
}

void CMaraudersMap::OnFindItemsSelected( wxCommandEvent &event )
{
	CMaraudersMapItemBase *item = GetSelectedItemOnFindListBox();
	if ( item )
	{
		m_canvas->SelectItem( item );
	}
}

void CMaraudersMap::OnFindItemsDoubleclicked( wxCommandEvent &event )
{
	CMaraudersMapItemBase *item = GetSelectedItemOnFindListBox();
	if ( item )
	{
		m_canvas->GotoItem( item );
	}
}

CMaraudersMapItemBase* CMaraudersMap::GetSelectedItemOnFindListBox()
{
	Int32 nSel = m_findItemsList->GetSelection();
	if ( nSel < 0 || nSel >= (Int32)m_findItemsIndexes.Size() ) return NULL;

	Int32 nItem = m_findItemsIndexes[ nSel ];
	TDynArray< CMaraudersMapItemBase* > &items = m_canvas->GetItems();
	if ( nItem < (Int32)items.Size() )
	{
		return items[ nItem ];
	}

	return NULL;
}

void CMaraudersMap::UpdateFindPanel( Bool forceUpdate /* = false */ )
{
	Bool isTagBaseSearch = m_findUseTags->IsChecked();

	String searchWord = m_findTextInput->GetValue();
	if ( forceUpdate || searchWord.GetLength() >= 3 )
	{
		// prepare 'searchTags'
		TagList searchTags;
		TDynArray< String > searchWordsTmp = searchWord.Split( TXT(" ") );
		for ( TDynArray< String >::iterator i = searchWordsTmp.Begin(); i != searchWordsTmp.End(); ++i )
		{
			searchTags.AddTag( CName( *i ) );
		}

		// prepare 'searchWords'
		searchWord.Trim();
		searchWord.MakeLower();
		TDynArray< String > searchWords = searchWord.Split( TXT(" ") );

		m_findItemsList->Clear();
		m_findItemsAll.Clear();
		m_findItemsIndexes.Clear();

		TDynArray< CMaraudersMapItemBase* > &items = m_canvas->GetItems();
		const Uint32 itemsSize = items.Size();
		for ( Uint32 i = 0; i < itemsSize; ++i )
		{
			if ( IsFilterEnabled( (EFilters)items[ i ]->GetTypeID(), FILTERS_TYPE_FINDING ) )
			{
				String itemDesc = items[ i ]->GetShortDescription();
				Bool match = false;

				// different types of match
				if ( isTagBaseSearch )
				{
					const TagList *tagList = items[ i ]->GetTags();
					if ( tagList )
					{
						match = TagList::MatchAny( searchTags, *tagList );
					}
				}
				else
				{
					match = itemDesc.ToLower().MatchAll( searchWords );
				}
				//if ( itemDesc.FindSubstring( searchWord ) != -1 )

				if ( match )
				{
					m_findItemsAll.Add( itemDesc.AsChar() );
					m_findItemsIndexes.PushBack( i );
				}
			}
		}

		m_findItemsList->Append( m_findItemsAll );
	}
	else
	{
		m_findItemsList->Clear();
	}
}

void CMaraudersMap::OnTimer( wxCommandEvent &event )
{
	if ( GGame->IsActive() )
	{
		// Add player
		if( IsFilterEnabled( FILTER_PLAYER, FILTERS_TYPE_PROCESSING ) ) 
		{
			CMaraudersMapItemPlayer *item = new CMaraudersMapItemPlayer();
			item->SetTypeID( FILTER_PLAYER );
			item->SetLogger( m_canvas );
			if ( !m_canvas->AddItem( item ) )
			{
				delete item;
			}
		}

		// Add NPCs
		if ( IsFilterEnabled( FILTER_NPCS, FILTERS_TYPE_PROCESSING ) )
		{
			CCommonGame::ActorIterator actorIter;
			for ( CCommonGame::ActorIterator npc = CCommonGame::ActorIterator(); npc; ++npc )
			{
				if( (*npc)->IsA<CNewNPC>() )
				{
					CMaraudersMapItemNPC *item = new CMaraudersMapItemNPC( Cast<CNewNPC>(*npc) );
					item->SetTypeID( FILTER_NPCS );
					item->SetLogger( m_canvas );
					if ( !m_canvas->AddItem( item ) )
					{
						delete item;
					}
				}
			}
		}

		// Add stubs
		if ( IsFilterEnabled( FILTER_AGENT_STUBS, FILTERS_TYPE_PROCESSING ) )
		{
			TDynArray< SAgentStub* > &agentStubs = GCommonGame->GetSystem< CCommunitySystem >()->GetAgentsStubs();
			for ( TDynArray< SAgentStub* >::const_iterator agentStub = agentStubs.Begin(); agentStub != agentStubs.End(); ++agentStub )
			{
				if ( ! m_agentsStubs.Exist( *agentStub ) )
				{
					CMaraudersMapItemAgentStub *item = new CMaraudersMapItemAgentStub( *agentStub );
					item->SetTypeID( FILTER_AGENT_STUBS );
					item->SetLogger( m_canvas );
					if ( !m_canvas->AddItem( item ) )
					{
						delete item;
					}
				}
			}
			m_agentsStubs = agentStubs;
		}

		// Add denied areas
		if ( IsFilterEnabled( FILTER_DENIED_AREAS, FILTERS_TYPE_PROCESSING ) )
		{
			const TDynArray< THandle< CStoryScenePlayer > >& scenePlrs = GCommonGame->GetSystem< CStorySceneSystem >()->GetScenePlayers();
			m_deniedAreas.Clear();
			for ( TDynArray<THandle< CStoryScenePlayer >>::const_iterator plrs = scenePlrs.Begin(); plrs!=scenePlrs.End(); ++plrs )
			{
				if ( plrs->Get() == NULL )
				{
					continue;
				}
				const CStoryScenePlayer* player = (*plrs).Get();
				TDynArray< CDeniedAreaComponent* > deniedAreas;
				CollectEntityComponents( (CEntity*)player, deniedAreas );
				const Uint32 deniedAreasCount = deniedAreas.Size();
				for ( Uint32 i=0; i<deniedAreasCount; ++i )
				{
					if ( !m_deniedAreas.Exist( deniedAreas[i] ) )
					{
						CMaraudersMapItemDeniedArea * item = new CMaraudersMapItemDeniedArea( deniedAreas[i] );
						item->SetTypeID( FILTER_DENIED_AREAS );
						item->SetLogger( m_canvas );
						if ( !m_canvas->AddItem( item ) )
						{
							delete item;
						}
					}
				}
				m_deniedAreas.PushBack( deniedAreas );
			}	
		}

		// Add community areas
		if ( IsFilterEnabled( FILTER_COMMUNITY_AREAS, FILTERS_TYPE_PROCESSING ) )
		{
			//m_communityAreas.Clear();
			TDynArray< CCommunityArea* > communityAreas;
			CollectAllEntities< CCommunityArea >( GGame->GetActiveWorld(), communityAreas );
			const Uint32 communityAreasCount = communityAreas.Size();
			for ( Uint32 i=0; i < communityAreasCount; ++i )
			{
				if ( !m_communityAreas.Exist( communityAreas[i] ) )
				{
					CMaraudersMapItemCommunityArea *item = new CMaraudersMapItemCommunityArea( communityAreas[i] );
					item->SetTypeID( FILTER_COMMUNITY_AREAS );
					item->SetLogger( m_canvas );
					if ( !m_canvas->AddItem( item ) )
					{
						delete item;
					}
				}
			}
		}

		ToolbarSetCurrentGameplayTime();
	}

	// fill global info window
	if ( m_globalInfoWindow->IsShown() )
	{
		FillGlobalInfoWindow();
	}

	// fill description window
	if ( const CMaraudersMapItemBase *item = m_canvas->GetSelectedItem() )
	{
		SetCurrentDescription( item->GetFullDescription().AsChar() );

		if ( m_lookAtWindow->IsShown() )
		{
			String info;
			item->GetLookAtInfo( info );
			SetLookAtInfo( info.AsChar() );
		}
	}

	// refresh AI
	m_aiDebugCanvas->Refresh();

	// Set current mouse position
	m_textCtrlWorldPosition->SetValue( String::Printf( TXT("%.1f, %.1f"), m_canvas->GetLastMouseWorldPos().X, m_canvas->GetLastMouseWorldPos().Y ).AsChar() );

	m_canvas->Repaint();
}

const CMaraudersMapItemBase* CMaraudersMap::GetSelectedItem()
{
	return m_canvas->GetSelectedItem();
}

void CMaraudersMap::DisplayAIEventDescription( const SAIEvent& event )
{
	static Char* resultStr[] = { TXT( "InProgress" ),
		TXT( "EAIR_Success" ),
		TXT( "EAIR_Failure" ),
		TXT( "EAIR_Exception" ),
		TXT( "EAIR_Interrupted" ) };

	String htmlStr = String::Printf(
		TXT("<b>Result</b>: %s<br> \
			<b>Start time</b>: %.3f<br> \
			<b>End time</b>: %.3f<br> \
			<b>Name</b>: %s<br> \
			<b>Description</b>:<br>%s<br>"),
			resultStr[ event.m_result ], 
			event.m_startTime, 
			event.m_endTime,
			event.m_name.AsChar(),
			event.m_description.AsChar() );
	
	m_aiEventDescription->Freeze();
	m_aiEventDescription->SetPage( htmlStr.AsChar() );
	m_aiEventDescription->Scroll( 0, 0 );
	m_aiEventDescription->Thaw();
}

void CMaraudersMap::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/MaraudersMap") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/MaraudersMap") );

	bool splitterFlag = m_splitterMain->IsSplit();
	int sashPos = m_splitterMain->GetSashPosition();
	Vector zoomedRegionCanvas = m_canvas->GetZoomedRegion();
	int refreshRatePos = m_choiceRefreshRate->GetSelection();

	config.Write( TXT("SplitterEnabled"), splitterFlag ? 1 : 0 );
	config.Write( TXT("SashPos"), sashPos );
	config.Write( TXT("CanvasZoomedRegion1X"), zoomedRegionCanvas.X );
	config.Write( TXT("CanvasZoomedRegion2Y"), zoomedRegionCanvas.Y );
	config.Write( TXT("CanvasZoomedRegion2X"), zoomedRegionCanvas.Z );
	config.Write( TXT("CanvasZoomedRegion1Y"), zoomedRegionCanvas.W );
	config.Write( TXT("RefreshRatePos"), refreshRatePos );
	
	// Filters
	for ( Int32 i = 0; i < FILTERS_TYPE_SIZE; ++i )
	{
		EFiltersType filtersType = (EFiltersType)i;
		config.Write( GetFriendlyFilterName( FILTER_ENCOUNTER_ITEMS,	filtersType ), IsFilterEnabled( FILTER_ENCOUNTER_ITEMS,		filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_ACTION_POINTS_ITEMS,filtersType ), IsFilterEnabled( FILTER_ACTION_POINTS_ITEMS,	filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_MESH_ITEMS,			filtersType ), IsFilterEnabled( FILTER_MESH_ITEMS,			filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_WAYPOINT_ITEMS,		filtersType ), IsFilterEnabled( FILTER_WAYPOINT_ITEMS,		filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_STICKER_ITEMS,		filtersType ), IsFilterEnabled( FILTER_STICKER_ITEMS,		filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_PLAYER,				filtersType ), IsFilterEnabled( FILTER_PLAYER,				filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_NPCS,				filtersType ), IsFilterEnabled( FILTER_NPCS,				filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_AGENT_STUBS,		filtersType ), IsFilterEnabled( FILTER_AGENT_STUBS,			filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_DENIED_AREAS,		filtersType ), IsFilterEnabled( FILTER_DENIED_AREAS,		filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_DOOR_ITEMS,			filtersType ), IsFilterEnabled( FILTER_DOOR_ITEMS,			filtersType ) );
		config.Write( GetFriendlyFilterName( FILTER_COMMUNITY_AREAS,	filtersType ), IsFilterEnabled( FILTER_COMMUNITY_AREAS,		filtersType ) );
	}

	config.Write( TXT("FiltersType"), (Int32)m_filtersTypeCurrent );

	// Windows
	config.Write( TXT("GlobalInfoPanelVisible"), m_isGlobalInfoPanelVisible );
	config.Write( TXT("LayersPanelVisible"), m_isLayersPanelVisible );
	config.Write( TXT("DescriptionPanelVisible"), m_isDescriptionPanelVisible );
	config.Write( TXT("FindPanelVisible"), m_isFindPanelVisible );
	config.Write( TXT("LookAtPanelVisible"), m_isLookAtPanelVisible );

	// Clear old saved player positions
	for ( Int32 i = 0; i < 100; ++i )
	{
		String tmp;
		String entryName = TXT("SavedPlayerPosName_") + ToString( i );
		Bool result = config.Read( entryName, &tmp );
		if ( result )
		{
			config.DeleteEntry( entryName, false );
		}
		else
		{
			break;
		}
	}

	// Saved player positions
	Int32 savedPlayerPosIdx = 0;
	for ( THashMap< String, Vector >::const_iterator ci = m_savedPlayerPos.Begin(); ci != m_savedPlayerPos.End(); ++ci, ++savedPlayerPosIdx )
	{
		config.Write( TXT("SavedPlayerPosName_") + ToString( savedPlayerPosIdx ), ci->m_first );
		config.Write( TXT("SavedPlayerPosValue_") + ToString( savedPlayerPosIdx ), ToString( ci->m_second ) );
	}
}

void CMaraudersMap::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/MaraudersMap") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/MaraudersMap") );

	bool splitterFlag = config.Read( TXT("SplitterEnabled"), 1 ) == 1 ? true : false;
	m_lastKnownSashPos = config.Read( TXT("SashPos"), 10 );
	Float zoomedRegionCanvas1X = config.Read( TXT("CanvasZoomedRegion1X"), 1.0f );
	Float zoomedRegionCanvas2Y = config.Read( TXT("CanvasZoomedRegion2Y"), 1.0f );
	Float zoomedRegionCanvas2X = config.Read( TXT("CanvasZoomedRegion2X"), 1.0f );
	Float zoomedRegionCanvas1Y = config.Read( TXT("CanvasZoomedRegion1Y"), 1.0f );
	int refreshRatePos = config.Read( TXT("RefreshRatePos"), 3 );

	if ( splitterFlag )
	{
		m_splitterMain->SplitVertically( m_panelSide, m_panelMain, m_splitterMain->GetSashPosition() );
	}
	else
	{
		m_splitterMain->Unsplit( m_panelSide );
	}

	m_splitterMain->SetSashPosition( m_lastKnownSashPos );

	m_canvas->SetZoomedRegion( Vector( zoomedRegionCanvas1X, zoomedRegionCanvas1Y, 0 ),
		Vector( zoomedRegionCanvas2X, zoomedRegionCanvas2Y, 0 ) );

	m_choiceRefreshRate->SetSelection( refreshRatePos );

	// Filters
	for ( Int32 i = 0; i < FILTERS_TYPE_SIZE; ++i )
	{
		EFiltersType filtersType = (EFiltersType)i;
		SwitchFilter( FILTER_ENCOUNTER_ITEMS,		filtersType, config.Read( GetFriendlyFilterName( FILTER_ENCOUNTER_ITEMS, filtersType ),		true ) != 0 );
		SwitchFilter( FILTER_ACTION_POINTS_ITEMS,	filtersType, config.Read( GetFriendlyFilterName( FILTER_ACTION_POINTS_ITEMS, filtersType ),	true ) != 0 );
		SwitchFilter( FILTER_MESH_ITEMS,			filtersType, config.Read( GetFriendlyFilterName( FILTER_MESH_ITEMS, filtersType ),			false ) != 0 );
		SwitchFilter( FILTER_WAYPOINT_ITEMS,		filtersType, config.Read( GetFriendlyFilterName( FILTER_WAYPOINT_ITEMS, filtersType ),		true ) != 0 );
		SwitchFilter( FILTER_STICKER_ITEMS,			filtersType, config.Read( GetFriendlyFilterName( FILTER_STICKER_ITEMS, filtersType ),		true ) != 0 );
		SwitchFilter( FILTER_PLAYER,				filtersType, config.Read( GetFriendlyFilterName( FILTER_PLAYER, filtersType ),				true ) != 0 );
		SwitchFilter( FILTER_NPCS,					filtersType, config.Read( GetFriendlyFilterName( FILTER_NPCS, filtersType ),				true ) != 0 );
		SwitchFilter( FILTER_AGENT_STUBS,			filtersType, config.Read( GetFriendlyFilterName( FILTER_AGENT_STUBS, filtersType ),			true ) != 0 );
		SwitchFilter( FILTER_DENIED_AREAS,			filtersType, config.Read( GetFriendlyFilterName( FILTER_DENIED_AREAS, filtersType ),		true ) != 0 );
		SwitchFilter( FILTER_DOOR_ITEMS,			filtersType, config.Read( GetFriendlyFilterName( FILTER_DOOR_ITEMS, filtersType ),			true ) != 0 );
		SwitchFilter( FILTER_COMMUNITY_AREAS,		filtersType, config.Read( GetFriendlyFilterName( FILTER_COMMUNITY_AREAS, filtersType ),		true ) != 0 );
	}

	Int32 filtersType = config.Read( TXT("FiltersType"), 0 );
	m_filtersTypeCurrent = (EFiltersType)filtersType;
	
	UpdateFiltersTypeMenu();
	UpdateFiltersMenu();

	// Windows
	m_isGlobalInfoPanelVisible = config.Read( TXT("GlobalInfoPanelVisible"), 1 ) == 1;
	m_isLayersPanelVisible = config.Read( TXT("LayersPanelVisible"), 1 ) == 1;
	m_isDescriptionPanelVisible = config.Read( TXT("DescriptionPanelVisible"), 1 ) == 1;
	m_isFindPanelVisible = config.Read( TXT("FindPanelVisible"), 1 ) == 1;
	m_isLookAtPanelVisible = config.Read( TXT("LookAtPanelVisible"), 1 ) == 1;
	m_menuBar->Check( m_windowMenuItemGlobalInfoId, m_isGlobalInfoPanelVisible );
	m_menuBar->Check( m_windowMenuItemLayersId, m_isLayersPanelVisible );
	m_menuBar->Check( m_windowMenuItemDescriptionId, m_isDescriptionPanelVisible );
	m_menuBar->Check( m_windowMenuItemFindId, m_isFindPanelVisible );
	m_menuBar->Check( m_windowMenuItemLookAtId, m_isLookAtPanelVisible );

	// Saved player positions
	const Int32 MAX_SAVED_PLAYER_POS = 10;
	ClearPlayerPos();
	for ( Int32 savedPlayerPosIdx = 0, menuIdx = 0; savedPlayerPosIdx < MAX_SAVED_PLAYER_POS; ++savedPlayerPosIdx, ++menuIdx )
	{
		String playerPosName;
		String playerPosValue;
		Bool result = config.Read( TXT("SavedPlayerPosName_") + ToString( savedPlayerPosIdx ), &playerPosName );
		if ( result )
		{
			config.Read( TXT("SavedPlayerPosValue_") + ToString( savedPlayerPosIdx ), &playerPosValue );
			Vector playerPosVec;
			if ( FromString( playerPosValue, playerPosVec ) )
			{
				m_savedPlayerPos.Insert( playerPosName, playerPosVec );
				m_savedPlayerPosMenu->Append( 50 + menuIdx, playerPosName.AsChar(), TXT("") );
				Connect( 50 + menuIdx, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CMaraudersMap::OnRestorePlayerPos ), new TCallbackData<String>(playerPosName), this );
			}
		}
		else
		{
			break;
		}
	}

	UpdateSidePanel();
	m_panelMain->Layout();
}
//////////////////////////////////////////////////////////////////////////
//
// on show window
void CMaraudersMap::OnShow( wxShowEvent& event )
{
	if ( event.IsShown() )
	{
		// auto-start
		if ( m_toolBar->GetToolState( m_idAutoStartTool ) && !m_timer->IsRunning() )
		{
			// disable control toolbar
			m_toolBar->EnableTool( m_idStartTool, false );
			m_toolBar->EnableTool( m_idStopTool,  false );
			m_toolBar->EnableTool( m_idPauseTool, false );

			Init();
			m_timer->Start( GetRefreshFactor() );
		}
	}
}

void CMaraudersMap::OnExit( wxCommandEvent &event )
{
	SaveOptionsToConfig();
	Close();
}

void CMaraudersMap::OnHelp( wxCommandEvent &event )
{
	wxDialog *dialog = new wxDialog( this, -1, TXT("Help") );
	wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
	wxHtmlWindow *htmlWindow = new wxHtmlWindow( dialog );

	wxString helpText =
		TXT("<h1>Quick help</h1><br>")
		TXT("<h2>Shortcuts</h2><br>")
		TXT("CTRL + HOME - moves map to (0,0) position<br>")
		TXT("ALT + HOME - centers map<br>")
		TXT("HOME - moves map to the player position<br>")
		TXT("Right Mouse Move - moves map<br>")
		TXT("Mouse Scroll - zooms map<br>")
		TXT("Mouse Scroll + SHIFT - zooms map faster<br>")
		TXT("<br>")
		TXT("<h2>Actors drag</h2>")
		TXT("You can move (teleport) NPCs by selecting them and dragging mouse with pressed SHIFT key.")
		TXT("<h2>Filters</h2>")
		TXT("Filters can help you in managing marauder's map items. There are three types of filters. ")
		TXT("Each type has its own set of filters.")
		TXT("<h3>Filters types</h3>")
		TXT("<ul>")
		TXT("<li>Processing - reading static items and updating dynamic items</li>")
		TXT("<li>Visibility - drawing items</li>")
		TXT("<li>Selecting  - selecting items by left mouse click</li>")
		TXT("</ul>")
		TXT("")
		TXT("")
		TXT("")
		TXT("");
	htmlWindow->SetPage( helpText );

	sizer->Add( htmlWindow, 1, wxEXPAND, 0 );
	dialog->SetSizer( sizer );
	dialog->Show();
}

void CMaraudersMap::OnFilters( wxCommandEvent &event )
{
	EFilters filter = (EFilters)event.GetId();
	SwitchFilter( filter, m_filtersTypeCurrent, event.IsChecked() );
	UpdateFiltersMenu();
}

void CMaraudersMap::OnFiltersType( wxCommandEvent &event )
{
	Int32 clickedMenuItem = event.GetId();

	if ( clickedMenuItem == m_filtersTypeMenuItemProcessing )
	{
		m_filtersTypeCurrent = FILTERS_TYPE_PROCESSING;
	}
	else if ( clickedMenuItem == m_filtersTypeMenuItemVisibility )
	{
		m_filtersTypeCurrent = FILTERS_TYPE_VISIBILITY;
	}
	else if ( clickedMenuItem == m_filtersTypeMenuItemSelecting )
	{
		m_filtersTypeCurrent = FILTERS_TYPE_SELECTING;
	}
	else if ( clickedMenuItem == m_filtersTypeMenuItemFinding )
	{
		m_filtersTypeCurrent = FILTERS_TYPE_FINDING;
	}
	else
	{
		ASSERT( !TXT("Unknown filters type menu item selected" ) );
	}

	UpdateFiltersTypeMenu();
	UpdateFiltersMenu();
}

void CMaraudersMap::UpdateFiltersMenu()
{
	wxMenu *filtersMenu = m_menuBar->GetMenu( 1 );
	for ( Int32 i = 0; i < FILTER_SIZE; ++i )
	{
		filtersMenu->Check( i, IsFilterEnabled( (EFilters)i, m_filtersTypeCurrent ) );
	}
}

void CMaraudersMap::UpdateFiltersTypeMenu()
{
	m_filtersTypeMenu->Check( m_filtersTypeMenuItemProcessing, false );
	m_filtersTypeMenu->Check( m_filtersTypeMenuItemVisibility, false );
	m_filtersTypeMenu->Check( m_filtersTypeMenuItemSelecting, false );
	m_filtersTypeMenu->Check( m_filtersTypeMenuItemFinding, false );

	switch ( m_filtersTypeCurrent )
	{
	case FILTERS_TYPE_PROCESSING:
		m_filtersTypeMenu->Check( m_filtersTypeMenuItemProcessing, true );
		break;
	case FILTERS_TYPE_VISIBILITY:
		m_filtersTypeMenu->Check( m_filtersTypeMenuItemVisibility, true );
		break;
	case FILTERS_TYPE_SELECTING:
		m_filtersTypeMenu->Check( m_filtersTypeMenuItemSelecting, true );
		break;
	case FILTERS_TYPE_FINDING:
		m_filtersTypeMenu->Check( m_filtersTypeMenuItemFinding, true );
		break;
	default:
		ASSERT( !TXT("Unknown filters type menu item" ) );
	}
}

void CMaraudersMap::OnWindowShowHide( wxCommandEvent &event )
{
	Int32 eventId = event.GetId();

	if ( eventId == m_windowMenuItemGlobalInfoId )
	{
		m_isGlobalInfoPanelVisible = !m_isGlobalInfoPanelVisible;
		m_menuBar->Check( m_windowMenuItemGlobalInfoId, m_isGlobalInfoPanelVisible );	
	}
	else if ( eventId == m_windowMenuItemLayersId )
	{
		m_isLayersPanelVisible = !m_isLayersPanelVisible;
		m_menuBar->Check( m_windowMenuItemLayersId, m_isLayersPanelVisible );	
	}
	else if ( eventId == m_windowMenuItemDescriptionId )
	{
		m_isDescriptionPanelVisible = !m_isDescriptionPanelVisible;
		m_menuBar->Check( m_windowMenuItemDescriptionId, m_isDescriptionPanelVisible );
	}
	else if ( eventId == m_windowMenuItemFindId )
	{
		m_isFindPanelVisible = !m_isFindPanelVisible;
		m_menuBar->Check( m_windowMenuItemFindId, m_isFindPanelVisible );
	}
	else if ( eventId == m_windowMenuItemLookAtId )
	{
		m_isLookAtPanelVisible = !m_isLookAtPanelVisible;
		m_menuBar->Check( m_windowMenuItemLookAtId, m_isLookAtPanelVisible );
	}

	UpdateSidePanel();
}

void CMaraudersMap::OnPlayerTeleport( wxCommandEvent &event )
{
	m_canvas->TeleportPlayer();
}

void CMaraudersMap::OnCameraTeleport( wxCommandEvent &event )
{
	m_canvas->TeleportCamera();
}

void CMaraudersMap::OnGotoMap( wxCommandEvent &event )
{
	GotoMapFromComboBox();
}

void CMaraudersMap::OnGotoCamera( wxCommandEvent &event )
{
	GotoCameraFromComboBox();
}

void CMaraudersMap::OnFollowPlayer( wxCommandEvent &event )
{
	m_canvas->SetFollowingPlayer( event.IsChecked() );
}

void CMaraudersMap::OnSavePlayerPos( wxCommandEvent &event )
{
	SavePlayerPos();
}

void CMaraudersMap::OnRestorePlayerPos( wxCommandEvent &event )
{
	TCallbackData< String > *callbackData = (TCallbackData< String >*)event.m_callbackUserData;
	if ( callbackData )
	{
		RestorePlayerPos( callbackData->GetData() );
	}
}

void CMaraudersMap::OnClearPlayerPos( wxCommandEvent &event )
{
	ClearPlayerPos();
}

void CMaraudersMap::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	m_canvas->OnViewportGenerateFragments( view, frame );

	const CMaraudersMapItemBase* selectedItem = GetSelectedItem();
	if ( selectedItem )
	{
		selectedItem->OnViewportGenerateFragments( view, frame );
	}
}

void CMaraudersMap::UpdateSidePanel()
{
	m_globalInfoSizer->Show( m_isGlobalInfoPanelVisible );

	m_layersSizer->Show( m_isLayersPanelVisible );

	m_findSizer->Show( m_isFindPanelVisible );

	m_descriptionSizer->Show( m_isDescriptionPanelVisible );

	m_lookAtSizer->Show( m_isLookAtPanelVisible );

	m_panelSide->Layout();
}

void CMaraudersMap::Init()
{
	m_canvas->Deselect();
	m_descriptionWindow->SetPage( wxEmptyString );
	m_globalInfoWindow->SetPage( wxEmptyString );
	m_lookAtWindow->SetPage( wxEmptyString );
	m_layersWindow->Clear();
	m_layers.Clear();
	m_agentsStubs.Clear();
	m_canvas->Reset();	

	GFeedback->BeginTask( TXT("Initializing the Marauder's Map"), true );

	CLayerGroup *layerGroup = GGame->GetActiveWorld()->GetWorldLayers();
	TDynArray< CLayerInfo * > layers;
	layerGroup->GetLayers( layers, false, true );

	// Calculate the size of the data to read for progress bar
	Int32 entitiesToLoadSize = 0; // the total number of entities
	Int32 entityLoadingNum = 0;   // the number of entity that is currently loading
	for ( TDynArray< CLayerInfo * >::const_iterator layerInfo = layers.Begin(); layerInfo != layers.End(); ++layerInfo )
	{
		if ( (*layerInfo)->IsLoaded() )
		{
			CLayer* layer = (*layerInfo)->GetLayer();
			TDynArray< CEntity* > entities;
			layer->GetEntities( entities );
			entitiesToLoadSize += entities.Size();
		}
	}

	Int32 layerNum = 0;
	Uint32 ecount = 0;		// entities counter
	Uint32 ccount = 0;		// attached components counter

	for ( TDynArray< CLayerInfo * >::const_iterator layerInfo = layers.Begin(); layerInfo != layers.End(); ++layerInfo )
	{
		ASSERT( *layerInfo );
		{
			// Layers
			if ( *layerInfo )
			{
				m_layers.PushBack( *layerInfo );
				++layerNum;
			}

			// Items
			if ( (*layerInfo)->IsLoaded() )
			{
				CLayer *layer = (*layerInfo)->GetLayer();
				const LayerEntitiesArray& entities = layer->GetEntities();
				ecount += entities.Size();
				ccount += (Uint32)layer->GetNumAttachedComponents();

				for ( auto entity = entities.Begin(); entity != entities.End(); ++entity, ++entityLoadingNum )
				{
					if ( GFeedback->IsTaskCanceled() )
					{
						break;
					}

					GFeedback->UpdateTaskInfo( TXT("Reading entity: %s"), (*entity)->GetName().AsChar() );
					GFeedback->UpdateTaskProgress( entityLoadingNum, entitiesToLoadSize );

					// Encounter items
					if ( IsFilterEnabled( FILTER_ENCOUNTER_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						if ( CEncounter *encounter = Cast< CEncounter >( *entity ) )
						{
							CMaraudersMapItemEncounter *item = new CMaraudersMapItemEncounter( encounter );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_ENCOUNTER_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item );
							continue;
						}
					}

					// Action points items
					if ( IsFilterEnabled( FILTER_ACTION_POINTS_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						const TDynArray<CComponent*>& comps = (*entity)->GetComponents();
						Bool found = false;
						const Uint32 compsCount = comps.Size();
						for ( Uint32 a = 0; a < compsCount; ++a )
						{
							if ( comps[a]->IsA<CActionPointComponent>() )
							{
								found = true;
								break;
							}
						}
						
						if ( found )
						{
							CMaraudersMapItemActionPoint *item = new CMaraudersMapItemActionPoint( *entity );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_ACTION_POINTS_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item );
							continue;
						}
					}

					// Door items
					if ( IsFilterEnabled( FILTER_DOOR_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						const TDynArray<CComponent*>& comps = (*entity)->GetComponents();
						Int32 idx = -1;
						const Uint32 compsCount = comps.Size();
						for ( Uint32 a = 0; a < compsCount; ++a )
						{
							if ( comps[a]->IsA<CDoorComponent>() )
							{
								idx = a;
								break;
							}
						}

						if ( idx != -1 )
						{
							CMaraudersMapItemDoor *item = new CMaraudersMapItemDoor( static_cast<CDoorComponent*>( comps[ idx ] ) );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_DOOR_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item );
							continue;
						}
					}

					// Mesh items
					if ( IsFilterEnabled( FILTER_MESH_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						if ( (*entity)->FindComponent< CMeshTypeComponent >() != nullptr )
						{
							//LOG_EDITOR( TXT("RYTHON: LOADING MESH ITEM: %s"), (*entity)->GetFriendlyName().AsChar() );
							CMaraudersMapItemMesh *item = new CMaraudersMapItemMesh( *entity );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_MESH_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item, false );
							continue;
						}
					}

					// Waypoint items (waypoint: entity with only one component - the WayPoint component)
					if ( IsFilterEnabled( FILTER_WAYPOINT_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						const TDynArray<CComponent*>& comps = (*entity)->GetComponents();
						Bool found = false;
						const Uint32 compsCount = comps.Size();
						for ( Uint32 a = 0; a < compsCount; ++a )
						{
							if ( comps[a]->IsA<CWayPointComponent>() )
							{
								found = true;
								break;
							}
						}

						if ( found && comps.Size() == 1 )
						{
							CMaraudersMapItemWayPoint *item = new CMaraudersMapItemWayPoint( *entity );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_WAYPOINT_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item );
							continue;
						}
					}

					// Sticker items (sticker: entity with only one component - the sticker component)
					if ( IsFilterEnabled( FILTER_STICKER_ITEMS, FILTERS_TYPE_PROCESSING ) )
					{
						const TDynArray<CComponent*>& comps = (*entity)->GetComponents();
						Bool found = false;
						const Uint32 compsCount = comps.Size();
						for ( Uint32 a = 0; a < compsCount; ++a )
						{
							if ( comps[a]->IsA<CStickerComponent>() )
							{
								found = true;
								break;
							}
						}

						if ( found && comps.Size() == 1 )
						{
							CMaraudersMapItemSticker *item = new CMaraudersMapItemSticker( *entity );
							item->SetLayerID( layerNum );
							item->SetTypeID( FILTER_STICKER_ITEMS );
							item->SetLogger( m_canvas );
							m_canvas->AddItem( item );
							continue;
						}
					}
				}
			}

			if ( GFeedback->IsTaskCanceled() )
			{
				break;
				GFeedback->EndTask();
			}
		}
	}

	// populating layers list
	CTimeCounter tm;
	Double start = tm.GetTimePeriodMS();
	Uint32 lcount = 0, llcount = 0;

	const Uint32 layersCount = m_layers.Size();
	for ( Uint32 i = 0; i < layersCount; ++i )
	{
		String layerPath;
		Bool streamingTile = false;

		// compute layer name
		CLayerGroup* parentGroup = m_layers[i]->GetLayerGroup();
		while ( parentGroup )
		{
			if ( parentGroup->GetName() != TXT( "streaming_tiles" ) )
			{
				if ( parentGroup->GetParentGroup() )
					layerPath = parentGroup->GetName() + TXT( "/" ) + layerPath;
				parentGroup = parentGroup->GetParentGroup();
			}
			else
			{
				streamingTile = true;
				parentGroup = nullptr;
			}
		}

		// add layer to list
		if ( !streamingTile )
		{
			layerPath += m_layers[i]->GetShortName(); 
			++lcount;

			// update layer status
			Uint32 idx;
			if ( m_layers[i]->IsLoaded() )
			{
				++llcount;
				layerPath += String::Printf( TXT( "     -    entities: %i components: %i" ), m_layers[i]->GetLayer()->GetEntities().Size(), m_layers[i]->GetLayer()->GetNumAttachedComponents() );
				idx = m_layersWindow->Append( layerPath.AsChar() );
				m_layersWindow->GetItem( idx )->SetTextColour( wxColour( 0, 128, 0 ) );
			}
			else
			{
				idx = m_layersWindow->Append( layerPath.AsChar() );
			}

			// set as checked
			m_layersWindow->Check( idx );
		}
	}
	Double end = tm.GetTimePeriodMS();
	LOG_EDITOR( TXT("adding layers %0.3fms %i"), (Float)(end-start), lcount );

	wxString str;
	str.Printf( "Layers: %i loaded: %i entities: %i components: %i", lcount, llcount, ecount, ccount );
	m_layersLabel->SetLabel( str );

	GFeedback->UpdateTaskInfo( TXT("The Marauder's Map initialized!") );
	GFeedback->UpdateTaskProgress( entitiesToLoadSize, entitiesToLoadSize );

	GFeedback->EndTask();

	// center on player at start-up
	if ( const CPlayer *player = GCommonGame->GetPlayer() )
	{
		m_canvas->Center( player->GetWorldPosition() );
	}

	m_canvas->Repaint( true );
}

int CMaraudersMap::GetRefreshFactor()
{
	long refreshRate = 0;
	m_choiceRefreshRate->GetStringSelection().ToLong( &refreshRate );
	const int &refreshFactor = static_cast< int >( refreshRate ); // milliseconds
	return refreshFactor;
}

void CMaraudersMap::SetGlobalInfoDescription( const wxString &text )
{
	int oldScrollPos = m_globalInfoWindow->GetScrollPos( wxVERTICAL );

	m_globalInfoWindow->Freeze();
	m_globalInfoWindow->SetPage( text );
	m_globalInfoWindow->Scroll( 0, oldScrollPos );
	m_globalInfoWindow->Thaw();
}

void CMaraudersMap::FillGlobalInfoWindow()
{
	wxString text;

	// Info about Map Pins
	if ( m_globalInfoMappinsCheckBox->IsChecked() )
	{
		// fill map pins tags
#if 0 // GFx 3
		const TDynArray< CName >& pinTagsList = GWitcherGame->GetHudInstance()->m_mapPinManager.m_trackedMapPinsTags;
#else
		const TDynArray< CName >& pinTagsList = TDynArray< CName >();
#endif

		String mappinsText;
		mappinsText = TXT("<h1>Map pins</h1>");
		mappinsText += TXT("Current map pin tags:<br>");
		for ( Uint32 i = 0; i < pinTagsList.Size(); ++i )
		{
			mappinsText += pinTagsList[i].AsString();
			if ( i + 1 < pinTagsList.Size() )
			{
				mappinsText += TXT(" ; ");
			}
		}
		mappinsText += TXT("<br>");

		// mark map pins on the world
		{
			if ( GGame->IsActive() )
			{
				TagList pinTags( pinTagsList );
				TDynArray< CEntity* > entities;
				GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( pinTags, entities );
				mappinsText += String::Printf( TXT("Tagged entities found: %d<br>"), entities.Size() );
				m_mappinsPositions.Clear();
				const Uint32 entitiesCount = entities.Size();
				for ( Uint32 i = 0; i < entitiesCount; ++i )
				{
					mappinsText += entities[i]->GetName() + TXT("<br>");
					m_mappinsPositions.PushBack( entities[i]->GetWorldPosition() );
				}

				// characters
				Int32 npcsFound = 0;
				String npcsText;
				const CCommonGame::TNPCCharacters &npcs = GCommonGame->GetNPCCharacters();
				for ( CCommonGame::TNPCCharacters::const_iterator npc = npcs.Begin(); npc != npcs.End(); ++npc )
				{
					const Uint32 pinTagsCount = pinTagsList.Size();
					for ( Uint32 i = 0; i < pinTagsCount; ++i )
					{
						if ( (*npc)->GetTags().HasTag( pinTagsList[i] ) )
						{
							npcsText += (*npc)->GetName() + TXT("<br>");
							m_mappinsPositions.PushBack( (*npc)->GetWorldPosition() );
							++npcsFound;
							break;
						}
					}
				}
				mappinsText += String::Printf( TXT("NPCs found: %d<br>"), npcsFound );
				//mappinsText += npcsText;

				// stubs
				Int32 questStubsFound;
				Int32 merchantStubsFound;
				String stubsInfo;
				GCommonGame->GetSystem< CCommunitySystem >()->GetMappinTrackedStubsInfo( questStubsFound, merchantStubsFound, stubsInfo );
				mappinsText += String::Printf( TXT("Quest stubs found: %d<br>"), questStubsFound );
				mappinsText += String::Printf( TXT("Merchant stubs found: %d<br>"), merchantStubsFound );
				mappinsText += stubsInfo + TXT("<br>");
			}
		}

		text.Append( mappinsText.AsChar() );
	}

	SetGlobalInfoDescription( text );
}

void CMaraudersMap::SetCurrentDescription( const wxString &text )
{
	const int oldScrollPos = m_descriptionWindow->GetScrollPos( wxVERTICAL );

	//if ( m_descWinCurrText != text)
	{
		//m_descWinCurrText = text;
		m_descriptionWindow->Freeze();
		m_descriptionWindow->SetPage( text );
		m_descriptionWindow->Scroll( 0, oldScrollPos );
		m_descriptionWindow->Thaw();
	}
}

void CMaraudersMap::SetLookAtInfo( const wxString &text )
{
	const int oldScrollPos = m_lookAtWindow->GetScrollPos( wxVERTICAL );

	m_lookAtWindow->Freeze();
	m_lookAtWindow->SetPage( text );
	m_lookAtWindow->Scroll( 0, oldScrollPos );
	m_lookAtWindow->Thaw();
}

void CMaraudersMap::GotoMapFromComboBox()
{
	Vector gotoVec;
	if ( GetGotoWorldPositionVector( gotoVec ) )
	{
		GotoMap( gotoVec );
	}
	else
	{
		m_canvas->AddLogInfo( TXT("Bad world position format. It should be 'XXX YYY'") );
	}
}

void CMaraudersMap::GotoCameraFromComboBox()
{
	Vector gotoVec;
	if ( GetGotoWorldPositionVector( gotoVec ) )
	{
		GotoCamera( gotoVec );
	}
	else
	{
		m_canvas->AddLogInfo( TXT("Bad world position format. It should be 'XXX YYY'") );
	}
}

Bool CMaraudersMap::GetGotoWorldPositionVector( Vector &vec )
{
	String text = m_comboBoxToolbar->GetValue();
	CTokenizer tokenizer( text, TXT(" ") );
	Float gotoVecValues[2];

	m_canvas->ClearWaypoint();

	if ( tokenizer.GetNumTokens() < 2 ) return false;

	Bool isOk = true;
	for( Uint32 i = 0; i < 2; ++i )
	{
		String tmp = tokenizer.GetToken( i );
		const Char* str = tmp.AsChar();
		if ( !GParseFloat( str, gotoVecValues[ i ] ) )
		{
			isOk = false;
		}
	}

	if ( isOk )
	{
		vec = Vector( gotoVecValues[0], gotoVecValues[1], 3.0f );
	}

	return isOk;
}

void CMaraudersMap::GotoMap( Vector &gotoVec )
{
	m_canvas->SetWaypoint( gotoVec );
	m_canvas->GotoWaypoint();
}

void CMaraudersMap::GotoCamera( Vector &gotoVec )
{
	if ( GGame->IsActive() && GGame->IsFreeCameraEnabled() )
	{
		GGame->SetFreeCameraWorldPosition( gotoVec );
	}
	else if ( !GGame->IsActive() )
	{
		wxTheFrame->GetWorldEditPanel()->SetCameraPosition( gotoVec );
	}
}

void CMaraudersMap::SavePlayerPos()
{
	String saveName;
	CPlayer *player = GCommonGame ? GCommonGame->GetPlayer() : NULL;
	if ( player && InputBox( this, TXT("Saving player position"), TXT("Position save name"), saveName, false ) )
	{
		Vector playerPos = player->GetWorldPosition();
		m_savedPlayerPos.Insert( saveName, playerPos );
	}
}

void CMaraudersMap::RestorePlayerPos( const String &savedPlayerPosName )
{
	Vector *savedPlayerPosVec = m_savedPlayerPos.FindPtr( savedPlayerPosName );

	if ( savedPlayerPosVec )
	{
		CPlayer *player = GCommonGame->GetPlayer();
		if ( player )
		{
			player->Teleport( *savedPlayerPosVec, player->GetWorldRotation() );
		}
	}
}

void CMaraudersMap::ClearPlayerPos()
{
	ClearSavedPlayerPosMenu();
	m_savedPlayerPos.Clear();
}

void CMaraudersMap::ClearSavedPlayerPosMenu()
{
	if ( m_savedPlayerPosMenu )
	{	
		while ( m_savedPlayerPosMenu->GetMenuItemCount() > 0 )
		{
			wxMenuItem *menuItem = m_savedPlayerPosMenu->FindItemByPosition( 0 );
			m_savedPlayerPosMenu->Delete( menuItem );
		}	
	}
}

String CMaraudersMap::GetFriendlyFilterName( EFilters filter, EFiltersType filterType )
{
	String filterTypeFriendlyName;
	switch( filterType )
	{
	case FILTERS_TYPE_PROCESSING:
		filterTypeFriendlyName = TXT("Processing_");
		break;
	case FILTERS_TYPE_VISIBILITY:
		filterTypeFriendlyName = TXT("Visibility_");
		break;
	case FILTERS_TYPE_SELECTING:
		filterTypeFriendlyName = TXT("Selecting_");
		break;
	case FILTERS_TYPE_FINDING:
		filterTypeFriendlyName = TXT("Finding_");
		break;
	default:
		filterTypeFriendlyName = TXT("Unknown_");
		ASSERT( !TXT("Unknown filter type") );
	}

	return filterTypeFriendlyName + GetFriendlyFilterName( filter );
}

String CMaraudersMap::GetFriendlyFilterName( EFilters filter )
{
	switch( filter )
	{
	case FILTER_ENCOUNTER_ITEMS:
		return TXT("Encounters");
	case FILTER_ACTION_POINTS_ITEMS:
		return TXT("Action points");
	case FILTER_MESH_ITEMS:
		return TXT("Meshes");
	case FILTER_WAYPOINT_ITEMS:
		return TXT("Waypoints");
	case FILTER_STICKER_ITEMS:
		return TXT("Stickers");
	case FILTER_PLAYER:
		return TXT("Player");
	case FILTER_NPCS:
		return TXT("NPCs");
	case FILTER_AGENT_STUBS:
		return TXT("Agent stubs");
	case FILTER_DENIED_AREAS:
		return TXT("Denied areas");
	case FILTER_DOOR_ITEMS:
		return TXT("Doors");
	case FILTER_COMMUNITY_AREAS:
		return TXT("Community areas");
	default:
		return TXT("Unknown filter");
	}
}

Bool CMaraudersMap::IsFilterEnabled( EFilters filter, EFiltersType filterType )
{
	switch( filterType )
	{
	case FILTERS_TYPE_PROCESSING:
		return m_filtersEnabledProcessing.Find( filter ) == m_filtersEnabledProcessing.End();	
	case FILTERS_TYPE_VISIBILITY:
		return m_filtersEnabledVisibility.Find( filter ) == m_filtersEnabledVisibility.End();	
	case FILTERS_TYPE_SELECTING:
		return m_filtersEnabledSelecting.Find( filter ) == m_filtersEnabledSelecting.End();
	case FILTERS_TYPE_FINDING:
		return m_filtersEnabledFinding.Find( filter ) == m_filtersEnabledFinding.End();
	default:
		ASSERT( !TXT("Unknown filter type") );
		return m_filtersEnabledProcessing.Find( filter ) == m_filtersEnabledProcessing.End();
	}
}

void CMaraudersMap::SwitchFilter( EFilters filter, EFiltersType filterType, Bool enable )
{
	switch( filterType )
	{
	case FILTERS_TYPE_PROCESSING:
		if ( enable ) m_filtersEnabledProcessing.Erase( filter ); else m_filtersEnabledProcessing.Insert( filter );
		break;
	case FILTERS_TYPE_VISIBILITY:
		if ( enable )
		{
			m_filtersEnabledVisibility.Erase( filter );
			m_canvas->ActivateVisibilityFilter( (Int32)filter );
		}
		else
		{
			m_filtersEnabledVisibility.Insert( filter );
			m_canvas->DeactivateVisibilityFilter( (Int32)filter );
		}
		break;
	case FILTERS_TYPE_SELECTING:
		if ( enable )
		{
			m_filtersEnabledSelecting.Erase( filter );
			m_canvas->ActivateSelectingFilter( (Int32)filter );
		}
		else
		{
			m_filtersEnabledSelecting.Insert( filter );
			m_canvas->DeactivateSelectingFilter( (Int32)filter );
		}
		break;
	case FILTERS_TYPE_FINDING:
		if ( enable )
		{
			m_filtersEnabledFinding.Erase( filter );
		}
		else
		{
			m_filtersEnabledFinding.Insert( filter );
		}
		break;
	default:
		if ( enable ) m_filtersEnabledProcessing.Erase( filter ); else m_filtersEnabledProcessing.Insert( filter );
		ASSERT( !TXT("Unknown filter type") );
	}
}

Bool CMaraudersMap::IsLayerHidden( const CLayerInfo *layerInfo ) const
{
	String layerPath;

	layerInfo->GetHierarchyPath( layerPath, true );

	return layerPath.MatchAny( m_layersHidden );
}

void CMaraudersMap::ToolbarSetCurrentGameplayTime()
{
	wxWindow *focused = wxWindow::FindFocus();
	if ( focused != m_textCtrlTime )
	{
		GameTime currentGameDayTime = GGame->GetTimeManager()->GetTime() % GameTime::DAY;
		String strCurrentGameDayTime = String::Printf( TXT("%2d:%2d:%2d"),
			currentGameDayTime.Hours(), currentGameDayTime.Minutes(), currentGameDayTime.Seconds() );
		m_textCtrlTime->SetValue( strCurrentGameDayTime.AsChar() );
	}
}
