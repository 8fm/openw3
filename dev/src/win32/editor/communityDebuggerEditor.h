/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include <wx/htmllbox.h>

class CCommunityDebugger;

class CCommunityDebuggerEditor : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

public:
	CCommunityDebuggerEditor( wxWindow* parent );
	~CCommunityDebuggerEditor();

	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

private:
	void OnTimer( wxCommandEvent &event );
	void OnToolBar( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );
	void UpdateGUIData();

	// data retrieval methods
	void GetAgentsData();
	void GetAgentData();
	void GetActionPointsData();
	void GetActionPointData();
	void GetDespawnPlacesData();
	void GetStoryPhasesData();

	// GUI methods
	void OnAgentsListBoxSelected( wxCommandEvent &event );
	void OnActionPointsListBoxSelected( wxCommandEvent &event );
	void OnDespawnPlacesListBoxSelected( wxCommandEvent &event );
	void OnStoryPhasesListBoxSelected( wxCommandEvent &event );

	// GUI Menu methods
	void OnMenuShowHideActionPoints( wxCommandEvent &event );
	void OnMenuShowHideAgents( wxCommandEvent &event );
	void OnMenuShowHideDespawnPlaces( wxCommandEvent &event );
	void OnMenuShowHideStoryPhases( wxCommandEvent &event );
	void OnMenuSwitchLogGeneral( wxCommandEvent &event );
	void OnMenuSwitchLogAP( wxCommandEvent &event );
	void OnMenuSwitchLogSpawn( wxCommandEvent &event );
	void OnMenuSwitchLogStub( wxCommandEvent &event );
	void OnMenuSwitchLogStoryPhase( wxCommandEvent &event );

	// generic methods
	void UpdateListBox( wxSimpleHtmlListBox *listBox, const wxArrayString &data );
	void UpdateWindow( wxHtmlWindow *htmlWindow, const wxString &data );

private:
	// GUI elements
	wxPanel             *m_panelMain;
	wxMenu              *m_csDebugMenu;

	wxStaticText        *m_agentsText;
	wxSimpleHtmlListBox *m_agentsShortInfoListBox;
	wxHtmlWindow        *m_agentInfoWindow;

	wxStaticText        *m_actionPointsText;
	wxSimpleHtmlListBox *m_actionPointsShortInfoListBox;
	wxHtmlWindow        *m_actionPointInfoWindow;

	wxStaticText        *m_despawnPlacesText;
	wxSimpleHtmlListBox *m_despawnPlacesShortInfoListBox;

	wxStaticText        *m_storyPhasesText;
	wxSimpleHtmlListBox *m_storyPhasesShortInfoListBox;

	wxToolBar           *m_toolBar;
	const int            m_idStartTool;
	const int            m_idStopTool;

	// GUI data

	wxArrayString m_agentsData;
	wxString      m_agentData;

	wxArrayString m_actionPointsData;
	wxString      m_actionPointData;

	wxArrayString m_communityComponentsData;

	wxArrayString m_despawnPlacesData;

	wxArrayString m_storyPhasesData;

	// General

	CEdTimer           *m_timer;
	CCommunityDebugger *m_commDbg;
};
