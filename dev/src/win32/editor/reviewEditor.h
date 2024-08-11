/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "../../common/engine/abstractMarkerSystem.h"

enum EReviewMarkerColumn
{
	RMC_Name,
	RMC_Priority,
	RMC_State,
	RMC_Type,
	RMC_Date,

	RMC_Count
};

class CReviewSystem;
class CReviewFlag;
class CReviewFlagComment;

class CEdReviewPanel : public wxPanel, public IEdEventListener, public IMarkerSystemListener
{
	DECLARE_EVENT_TABLE();

public:	
	CEdReviewPanel(wxWindow* parent);
	~CEdReviewPanel();

	// wxWidgets callback functions
	void OnShowFlagWindow		( wxCommandEvent& event );
	void OnShowAddNewFlagWindow	( wxCommandEvent& event );
	void OnShowModifyFlagWindow	( wxCommandEvent& event );
	void OnShowFilterPanel		( wxCommandEvent& event );
	void OnShowSettingsPanel	( wxCommandEvent& event );
	void OnRefreshFlag			( wxCommandEvent& event );
	void OnEnableAutoSyncFlag	( wxCommandEvent& event );
	void OnUpdateAutoSyncTime	( wxCommandEvent& event );

	// grid callbacks
	void OnSelectRowInGrid( wxListEvent& event );
	void OnShowColumnContextMenu( wxListEvent& event );
	void OnShowRowContextMenu( wxListEvent& event );
	void OnSortByColumn( wxListEvent& event );
	void OnColumnPopupClick( wxCommandEvent& event );
	void OnRowPopupClick( wxCommandEvent& event );

	// settings callback funtions
	void OnTTPProjectSelecetd( wxCommandEvent& event );
	void OnTTPMilestoneSelected( wxCommandEvent& event );

	// filters callback functions
	void OnCheckShowOnMap		( wxCommandEvent& event );
	void OnCheckShowClosedFlag	( wxCommandEvent& event );
	void OnCheckStateFilter		( wxCommandEvent& event );
	void OnCheckTypeFilter		( wxCommandEvent& event );
	void OnChooseSearchCategory ( wxCommandEvent& event );
	void OnClickFilter			( wxCommandEvent& event );
	void OnClickClearFilter		( wxCommandEvent& event );

	// callback functions for flag list
	void OnDoubleClickFlagInList( wxCommandEvent& event );

	// callback function for m_showFlagWindow
	void OnClickEditFlagInShow		( wxCommandEvent& event );
	void OnClickCloseShowWindow		( wxCommandEvent& event );

	void ClearSelection();

	void OnClickShowFullScreen		( wxCommandEvent& event );
	void OnClickCommentInList		( wxCommandEvent& event );
	void OnDoubleClickCommentInList	( wxCommandEvent& event );

	// callback functions for m_showFullScreenWindow
	void OnClickCloseFullScreen	( wxCommandEvent& event );

	// callback function for m_addNewFlagWindow
	void OnAddNewFlag			( wxCommandEvent& event );
	void OnCancelButtonClick	( wxCommandEvent& event );
	void OnClickSetFlagPos		( wxCommandEvent& event );

	// callback function for m_modifyFlagWindow
	void OnModifyFlag			( wxCommandEvent& event );
	void OnModifyCancel			( wxCommandEvent& event );
	void OnKeepStateChanged		( wxCommandEvent& event );
	void OnMakeScreenChanged	( wxCommandEvent& event );
	void OnKeepPriorityChanged	( wxCommandEvent& event );

	// event listener
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system );

private:
	// flags functions
	void FillFlagsWindow();
	void FillFilterPanel();
	void FillScreenMiniViewer(CReviewFlagComment& comment);

	void OnSearchButtonClicked( wxCommandEvent& event );
	void OnSearchEnterClicked( wxCommandEvent& event );

	Int32 GetSelectedItemIndex();

	void InternalProcessMessage();

private:
	CReviewFlag*	m_flagOpenToEdit;

	wxDialog*		m_showFlagWindow;
	wxDialog*		m_addNewFlagWindow;
	wxDialog*		m_modifyFlagWindow;
	wxDialog*		m_showFullScreenWindow;
	wxToolBar*		m_mainToolbar;
	wxSpinCtrl*		m_autoSyncTime;
	wxCheckBox*		m_autoSyncEnable;
	wxPanel*		m_mainPanel;
	wxPanel*		m_filtersPanel;
	wxPanel*		m_settingsPanel;
	wxListCtrl*		m_flagList;
	wxToolBar*		m_flagToolBar;
	wxPanel*		m_connectionPanel;
	wxStaticText*	m_connectionInfo;
	wxMenu*			m_columnContextMenu;
	wxMenu*			m_rowContextMenu;

	// searching
	wxChoice*		m_searchCategory;
	wxTextCtrl*		m_searchLine;			//!< 
	wxButton*		m_searchButton;			//!< 

	// settings
	wxChoice*		m_projectName;
	wxChoice*		m_milestoneName;

	// filters
	wxCheckBox*		m_filterShowOnMap;
	wxCheckBox*		m_filterDownloadClosedFlags;
	wxCheckListBox* m_filterStatesList;
	wxCheckListBox* m_filterTypesList;
	wxButton*		m_filterButton;
	wxButton*		m_filterResetButton;

	// m_showFlagWindow
	wxStaticText*	m_showSummary;
	wxStaticText*	m_showPriority;
	wxStaticText*	m_showState;
	wxStaticText*	m_showType;
	wxStaticText*	m_showAuthor;
	wxStaticText*	m_showCreatedDate;
	wxListBox*		m_commentsList;
	wxStaticText*	m_showCommentAuthor;
	wxStaticText*	m_showCommentState;
	wxStaticText*	m_showCommentPriority;
	wxStaticText*	m_showTestTrackNumber;
	wxTextCtrl*		m_showCommentDescription;
	wxBitmapButton* m_showFullScreenButton;
	wxBitmapButton* m_fullScreenButton;


	// m_addNewFlagWindow
	wxTextCtrl*		m_newFlagSummary;
	wxTextCtrl*		m_newFlagDescription;
	wxChoice*		m_newFlagPriority;
	wxChoice*		m_newFlagType;
	wxCheckBox*		m_editNewScreenAfterAdding;

	// m_modifyFlagWindow
	wxTextCtrl*		m_modifyFlagComment;
	wxChoice*		m_modifyFlagPriority;
	wxChoice*		m_modifyFlagState;
	wxCheckBox*		m_editScreenAfterAdding;
	wxCheckBox*		m_editKeepState;
	wxCheckBox*		m_editKeepPriority;
	wxCheckBox*		m_makeScreen;

	wxBitmap*		m_screen;

	// logic
	CReviewSystem*	m_attachedMarkerSystem;

	Red::Threads::CMutex	m_synchronizationMutex;		//!<
	TQueue< EMarkerSystemMessage > m_messages;
};

#endif // NO_MARKER_SYSTEMS
