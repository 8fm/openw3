/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communityDebugger.h"
#include "communityDebuggerEditor.h"
#include "../../common/game/communityErrorReport.h"
#include "../../common/game/communitySystem.h"

// Event table
BEGIN_EVENT_TABLE( CCommunityDebuggerEditor, wxSmartLayoutPanel )
END_EVENT_TABLE()

CCommunityDebuggerEditor::CCommunityDebuggerEditor( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TXT("CommunityDebugger"), true )
	, m_commDbg( NULL )
	, m_idStartTool( XRCID("startTool") )
	, m_idStopTool( XRCID("stopTool") )
{
	// Get GUI elements
	m_toolBar = XRCCTRL( *this, "toolBar", wxToolBar );

	m_toolBar->Connect( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnToolBar ), 0, this );
	m_toolBar->EnableTool( m_idStartTool, true );
	m_toolBar->EnableTool( m_idStopTool, false );

	// Set timer
	m_timer = new CEdTimer();
	m_timer->Connect( wxEVT_TIMER, wxCommandEventHandler( CCommunityDebuggerEditor::OnTimer ), NULL, this );
	m_timer->Stop();

	m_panelMain = XRCCTRL( *this, "panelMain", wxPanel);
	wxBoxSizer *sizer  = new wxBoxSizer( wxHORIZONTAL );
	wxBoxSizer *sizer1 = new wxBoxSizer( wxHORIZONTAL );
	wxBoxSizer *sizer2 = new wxBoxSizer( wxHORIZONTAL );
	wxBoxSizer *sizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_agentsShortInfoListBox = new wxSimpleHtmlListBox( m_panelMain, wxID_ANY );
	m_agentInfoWindow = new wxHtmlWindow( m_panelMain );
	m_actionPointsShortInfoListBox = new wxSimpleHtmlListBox( m_panelMain, wxID_ANY );
	m_actionPointInfoWindow = new wxHtmlWindow( m_panelMain );
	m_despawnPlacesShortInfoListBox = new wxSimpleHtmlListBox( m_panelMain, wxID_ANY );
	m_storyPhasesShortInfoListBox = new wxSimpleHtmlListBox( m_panelMain, wxID_ANY );

	m_agentInfoWindow->SetWindowStyle( wxBORDER_SUNKEN );
	m_actionPointInfoWindow->SetWindowStyle( wxBORDER_SUNKEN );

	GetParent()->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CCommunityDebuggerEditor::OnClose ), 0, this);
	m_agentsShortInfoListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnAgentsListBoxSelected ), NULL, this );
	m_actionPointsShortInfoListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnActionPointsListBoxSelected ), NULL, this );
	m_despawnPlacesShortInfoListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnDespawnPlacesListBoxSelected ), NULL, this );
	m_storyPhasesShortInfoListBox->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnStoryPhasesListBoxSelected ), NULL, this );

	m_agentsText = new wxStaticText( m_panelMain, wxID_ANY, TXT("Agents") );
	wxBoxSizer *agentsTextSizer = new wxBoxSizer( wxVERTICAL );
	agentsTextSizer->Add( m_agentsText, 0, wxALIGN_CENTRE, 0 );

	m_actionPointsText = new wxStaticText( m_panelMain, wxID_ANY, TXT("Action Points") );
	wxBoxSizer *actionPointsTextSizer = new wxBoxSizer( wxVERTICAL );
	actionPointsTextSizer->Add( m_actionPointsText, 0, wxALIGN_CENTRE, 0 );

	m_despawnPlacesText = new wxStaticText( m_panelMain, wxID_ANY, TXT("Despawn places") );
	wxBoxSizer *despawnPlacesTextSizer = new wxBoxSizer( wxVERTICAL );
	despawnPlacesTextSizer->Add( m_despawnPlacesText, 0, wxALIGN_CENTRE, 0 );

	m_storyPhasesText = new wxStaticText( m_panelMain, wxID_ANY, TXT("Story phases") );
	wxBoxSizer *storyPhasesTextSizer = new wxBoxSizer( wxVERTICAL );
	storyPhasesTextSizer->Add( m_storyPhasesText, 0, wxALIGN_CENTRE, 0 );

	sizer->Add( m_agentsShortInfoListBox, 1, wxEXPAND, 0 );
	sizer->Add( m_agentInfoWindow, 1, wxEXPAND, 0 );
	sizer1->Add( m_actionPointsShortInfoListBox, 1, wxEXPAND, 0 );
	sizer1->Add( m_actionPointInfoWindow, 1, wxEXPAND, 0 );
	sizer2->Add( m_despawnPlacesShortInfoListBox, 1, wxEXPAND, 0 );
	sizer3->Add( m_storyPhasesShortInfoListBox, 1, wxEXPAND, 0 );
	
	m_panelMain->GetSizer()->Add( agentsTextSizer, 0, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( sizer, 1, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( actionPointsTextSizer, 0, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( sizer1, 1, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( despawnPlacesTextSizer, 0, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( sizer2, 1, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( storyPhasesTextSizer, 0, wxEXPAND, 0 );
	m_panelMain->GetSizer()->Add( sizer3, 1, wxEXPAND, 0 );

	// create menu
	m_csDebugMenu = new wxMenu();
	m_csDebugMenu->Append( 16, TXT("Show/Hide agents"), wxEmptyString, true );
	Connect( 16, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuShowHideAgents ), NULL, this );
	m_csDebugMenu->Append( 17, TXT("Show/Hide action points"), wxEmptyString, true );
	Connect( 17, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuShowHideActionPoints ), NULL, this );
	m_csDebugMenu->Append( 18, TXT("Show/Hide despawn places"), wxEmptyString, true );
	Connect( 18, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuShowHideDespawnPlaces ), NULL, this );
	m_csDebugMenu->Append( 19, TXT("Show/Hide story phases"), wxEmptyString, true );
	Connect( 19, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuShowHideStoryPhases ), NULL, this );
	GetMenuBar()->Append( m_csDebugMenu, TXT("Debugger") );

	wxMenu *debugInfoMenu = new wxMenu();
	debugInfoMenu->Append( 20, TXT("Switch general log"), wxEmptyString, true );
	Connect( 20, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuSwitchLogGeneral ), NULL, this );
	debugInfoMenu->Append( 21, TXT("Switch log - action points"), wxEmptyString, true );
	Connect( 21, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuSwitchLogAP ), NULL, this );
	debugInfoMenu->Append( 22, TXT("Switch log - spawn"), wxEmptyString, true );
	Connect( 22, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuSwitchLogSpawn ), NULL, this );
	debugInfoMenu->Append( 23, TXT("Switch log - agent stubs"), wxEmptyString, true );
	Connect( 23, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuSwitchLogStub ), NULL, this );
	debugInfoMenu->Append( 24, TXT("Switch log - story phase change"), wxEmptyString, true );
	Connect( 24, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CCommunityDebuggerEditor::OnMenuSwitchLogStoryPhase ), NULL, this );
	GetMenuBar()->Append( debugInfoMenu, TXT("Debug info") );

	m_commDbg = new CCommunityDebugger();
	
	// Update and finalize layout
	Layout();
	m_panelMain->Layout();
	Show();
	LoadOptionsFromConfig();
}

CCommunityDebuggerEditor::~CCommunityDebuggerEditor()
{
	SaveOptionsToConfig();

	delete m_timer;
	delete m_commDbg;
}

void CCommunityDebuggerEditor::OnToolBar( wxCommandEvent &event )
{
	const int refreshFactor = 400; // milliseconds
	const int idSelected = event.GetId();

	if ( idSelected == m_idStartTool )
	{
		if ( m_commDbg->Init() )
		{
			m_timer->Start( refreshFactor );
			m_toolBar->EnableTool( m_idStartTool, false );
			m_toolBar->EnableTool( m_idStopTool, true );
		}
		else
		{
			wxMessageBox( TXT("Cannot initialize community debugger"), wxT("Warning"),wxOK | wxCENTRE | wxICON_WARNING );
		}
	}
	else if ( idSelected == m_idStopTool )
	{
		m_timer->Stop();
		m_toolBar->EnableTool( m_idStartTool, true );
		m_toolBar->EnableTool( m_idStopTool, false );
	}
}

void CCommunityDebuggerEditor::OnClose( wxCloseEvent &event )
{
	SaveOptionsToConfig();

	wxSmartLayoutPanel::OnClose( event );
}

void CCommunityDebuggerEditor::OnTimer( wxCommandEvent &event )
{
	if ( !GGame->IsActive() )
	{
		m_commDbg->DeInit();
		m_timer->Stop();
		SCSErrRep::GetInstance().SwitchLogDebug( false );
		m_toolBar->EnableTool( m_idStartTool, true );
		m_toolBar->EnableTool( m_idStopTool, false );
	}
	else
	{
		m_commDbg->Update();
		UpdateGUIData();
	}
}

void CCommunityDebuggerEditor::SaveOptionsToConfig()
{
	SaveLayout( TXT("/Frames/CommunityDebugger") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/CommunityDebugger") );

	bool showHideFlag1 = m_storyPhasesShortInfoListBox->IsShown();
	bool showHideFlag2 = m_despawnPlacesShortInfoListBox->IsShown();
	bool showHideFlag3 = m_actionPointsShortInfoListBox->IsShown();
	bool showHideFlag4 = m_agentsShortInfoListBox->IsShown();

	config.Write( TXT("ShowHideFlag1"), showHideFlag1 ? 1 : 0 );
	config.Write( TXT("ShowHideFlag2"), showHideFlag2 ? 1 : 0 );
	config.Write( TXT("ShowHideFlag3"), showHideFlag3 ? 1 : 0 );
	config.Write( TXT("ShowHideFlag4"), showHideFlag4 ? 1 : 0 );
}

void CCommunityDebuggerEditor::LoadOptionsFromConfig()
{
	LoadLayout( TXT("/Frames/CommunityDebugger") );

	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Frames/CommunityDebugger") );

	bool showHideFlag1 = config.Read( TXT("ShowHideFlag1"), 1 ) == 1 ? true : false;
	bool showHideFlag2 = config.Read( TXT("ShowHideFlag2"), 1 ) == 1 ? true : false;
	bool showHideFlag3 = config.Read( TXT("ShowHideFlag3"), 1 ) == 1 ? true : false;
	bool showHideFlag4 = config.Read( TXT("ShowHideFlag4"), 1 ) == 1 ? true : false;
	bool showHideFlag5 = config.Read( TXT("ShowHideFlag5"), 1 ) == 1 ? true : false;

	m_storyPhasesShortInfoListBox->Show( showHideFlag1 );
	m_storyPhasesText->Show( showHideFlag1 );

	m_despawnPlacesShortInfoListBox->Show( showHideFlag2 );
	m_despawnPlacesText->Show( showHideFlag2 );

	m_actionPointsShortInfoListBox->Show( showHideFlag4 );
	m_actionPointInfoWindow->Show( showHideFlag4 );
	m_actionPointsText->Show( showHideFlag4 );

	m_agentsShortInfoListBox->Show( showHideFlag5 );
	m_agentInfoWindow->Show( showHideFlag5 );
	m_agentsText->Show( showHideFlag5 );	

	m_csDebugMenu->Check( 20, showHideFlag1 );
	m_csDebugMenu->Check( 19, showHideFlag2 );
	m_csDebugMenu->Check( 18, showHideFlag3 );
	m_csDebugMenu->Check( 17, showHideFlag4 );
	m_csDebugMenu->Check( 16, showHideFlag5 );

	m_panelMain->Layout();
}

void CCommunityDebuggerEditor::UpdateGUIData()
{
	GetAgentsData();
	GetAgentData();
	GetActionPointsData();
	GetActionPointData();
	GetDespawnPlacesData();
	GetStoryPhasesData();

	UpdateListBox( m_agentsShortInfoListBox, m_agentsData );
	UpdateWindow( m_agentInfoWindow, m_agentData );
	UpdateListBox( m_actionPointsShortInfoListBox, m_actionPointsData );
	UpdateWindow( m_actionPointInfoWindow, m_actionPointData );
	UpdateListBox( m_despawnPlacesShortInfoListBox, m_despawnPlacesData );
	UpdateListBox( m_storyPhasesShortInfoListBox, m_storyPhasesData );
}

void CCommunityDebuggerEditor::GetAgentsData()
{
	m_agentsData.Clear();
	for ( TDynArray< String >::const_iterator ci = m_commDbg->GetAgentsShortInfo().Begin();
		ci != m_commDbg->GetAgentsShortInfo().End();
		++ci )
	{
		m_agentsData.Add( ci->AsChar() );
	}
}

void CCommunityDebuggerEditor::GetAgentData()
{
	m_agentData.Clear();
	m_agentData = m_commDbg->GetCurrentAgentInfo().AsChar();
}

void CCommunityDebuggerEditor::GetActionPointsData()
{
	m_actionPointsData.Clear();
	for ( TDynArray< String >::const_iterator ci = m_commDbg->GetActionPointsShortInfo().Begin();
		  ci != m_commDbg->GetActionPointsShortInfo().End();
		  ++ci )
	{
		m_actionPointsData.Add( ci->AsChar() );
	}
}

void CCommunityDebuggerEditor::GetActionPointData()
{
	m_actionPointData.Clear();
	m_actionPointData = m_commDbg->GetCurrentActionPointInfo().AsChar();
}

void CCommunityDebuggerEditor::GetDespawnPlacesData()
{
	m_despawnPlacesData.Clear();
	for ( TDynArray< String >::const_iterator ci = m_commDbg->GetDespawnPlacesShortInfo().Begin();
		  ci != m_commDbg->GetDespawnPlacesShortInfo().End();
		  ++ci )
	{
		m_despawnPlacesData.Add( ci->AsChar() );
	}
}

void CCommunityDebuggerEditor::GetStoryPhasesData()
{
	m_storyPhasesData.Clear();
	for ( TDynArray< String >::const_iterator ci = m_commDbg->GetStoryPhasesShortInfo().Begin();
		  ci != m_commDbg->GetStoryPhasesShortInfo().End();
		  ++ci )
	{
		m_storyPhasesData.Add( ci->AsChar() );
	}
}

void CCommunityDebuggerEditor::UpdateListBox( wxSimpleHtmlListBox *listBox, const wxArrayString &data )
{
	const wxArrayString &dataCurrent = listBox->GetStrings();
	if ( dataCurrent != data )
	{
		int oldSelection = listBox->GetSelection(); // remember old selection and then try to restore it
		listBox->Clear();
		listBox->Append( data );
		if ( oldSelection < static_cast< int >( listBox->GetCount() ) )
		{
			listBox->SetSelection( oldSelection );
		}
	}
}

void CCommunityDebuggerEditor::UpdateWindow( wxHtmlWindow *htmlWindow, const wxString &data )
{
	const wxString &dataCurrent = htmlWindow->GetOpenedPage();
	if ( dataCurrent != data )
	{
		htmlWindow->SetPage( data );
	}
}

void CCommunityDebuggerEditor::OnAgentsListBoxSelected( wxCommandEvent &event )
{
	int index = m_agentsShortInfoListBox->GetSelection();
	if ( index == wxNOT_FOUND ) return;

	m_commDbg->SetCurrentAgent( (Uint32)index );
}

void CCommunityDebuggerEditor::OnActionPointsListBoxSelected( wxCommandEvent &event )
{
	int index = m_actionPointsShortInfoListBox->GetSelection();
	if ( index == wxNOT_FOUND ) return;

	m_commDbg->SetCurrentActionPoint( (Uint32)index );
}

void CCommunityDebuggerEditor::OnDespawnPlacesListBoxSelected( wxCommandEvent &event )
{
	int index = m_despawnPlacesShortInfoListBox->GetSelection();
	if ( index == wxNOT_FOUND ) return;

	m_commDbg->SetCurrentDespawnPlace( (Uint32)index );
}

void CCommunityDebuggerEditor::OnStoryPhasesListBoxSelected( wxCommandEvent &event )
{
	int index = m_storyPhasesShortInfoListBox->GetSelection();
	if ( index == wxNOT_FOUND ) return;

	m_commDbg->SetCurrentStoryPhase( (Uint32)index );
}

void CCommunityDebuggerEditor::OnMenuShowHideActionPoints( wxCommandEvent &event )
{
	bool showHideFlag = !m_actionPointsShortInfoListBox->IsShown();
	m_actionPointsShortInfoListBox->Show( showHideFlag );
	m_actionPointInfoWindow->Show( showHideFlag );
	m_actionPointsText->Show( showHideFlag );
	m_panelMain->Layout();
}

void CCommunityDebuggerEditor::OnMenuShowHideAgents( wxCommandEvent &event )
{
	bool showHideFlag = !m_agentsShortInfoListBox->IsShown();
	m_agentsShortInfoListBox->Show( showHideFlag );
	m_agentInfoWindow->Show( showHideFlag );
	m_agentsText->Show( showHideFlag );
	m_panelMain->Layout();
}

void CCommunityDebuggerEditor::OnMenuShowHideDespawnPlaces( wxCommandEvent &event )
{
	bool showHideFlag = !m_despawnPlacesShortInfoListBox->IsShown();
	m_despawnPlacesShortInfoListBox->Show( showHideFlag );
	m_despawnPlacesText->Show( showHideFlag );
	m_panelMain->Layout();
}

void CCommunityDebuggerEditor::OnMenuShowHideStoryPhases( wxCommandEvent &event )
{
	bool showHideFlag = !m_storyPhasesShortInfoListBox->IsShown();
	m_storyPhasesShortInfoListBox->Show( showHideFlag );
	m_storyPhasesText->Show( showHideFlag );
	m_panelMain->Layout();
}

void CCommunityDebuggerEditor::OnMenuSwitchLogGeneral( wxCommandEvent &event )
{
	SCSErrRep::GetInstance().SwitchLogDebug( event.IsChecked() );
}

void CCommunityDebuggerEditor::OnMenuSwitchLogAP( wxCommandEvent &event )
{
	SCSErrRep::GetInstance().SwitchLogActionPoint( event.IsChecked() );
}

void CCommunityDebuggerEditor::OnMenuSwitchLogSpawn( wxCommandEvent &event )
{
	SCSErrRep::GetInstance().SwitchLogSpawn( event.IsChecked() );
}

void CCommunityDebuggerEditor::OnMenuSwitchLogStub( wxCommandEvent &event )
{
	SCSErrRep::GetInstance().SwitchLogStub( event.IsChecked() );
}

void CCommunityDebuggerEditor::OnMenuSwitchLogStoryPhase( wxCommandEvent &event )
{
	SCSErrRep::GetInstance().SwitchLogStoryPhase( event.IsChecked() );
}
