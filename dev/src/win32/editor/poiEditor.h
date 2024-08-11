/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "../../common/engine/abstractMarkerSystem.h"

class CPointofInterest;
class CPoiSystem;

enum EPoiColumn
{
	PC_Name,
	PC_Type,
	
	PC_Count
};

class CEdPOIPanel : public wxPanel, public IEdEventListener, public IMarkerSystemListener
{
	DECLARE_EVENT_TABLE();

public:	
	CEdPOIPanel(wxWindow* parent);
	~CEdPOIPanel();	

	void SelectPOI( const String& name );
	// wxWidgets callback functions
	void OnShowAddNewPOIWindow		( wxCommandEvent& event );
	void OnShowEditPOIWindow		( wxCommandEvent& event );

	void OnSelectRowInGrid( wxListEvent& event );

	void OnDeleteSelectedPOI		( wxCommandEvent& event );
	void OnUpdateAutoSyncTime		( wxCommandEvent& event );
	void OnEnableAutoSyncFlag		( wxCommandEvent& event );
	void OnShowFilterPanel			( wxCommandEvent& event );
	void OnRefreshPOI				( wxCommandEvent& event );
	void OnOpenBlackBox				( wxCommandEvent& event );

	// m_filtersPanel
	void OnCheckShowOnMap			( wxCommandEvent& event );
	void OnClickFilter				( wxCommandEvent& event );
	void OnSelectAllFilters			( wxCommandEvent& event );

	// m_addPOIWindow
	void OnClickSetPOIOnMap			( wxCommandEvent& event );
	void OnClickAddPOI				( wxCommandEvent& event );
	void OnClickAddPOICancel		( wxCommandEvent& event );

	// m_showModifyPOIWindow
	void OnClickSetPOICategory		( wxCommandEvent& event );
	void OnClickModifyPOI			( wxCommandEvent& event );
	void OnClickClosePOIWindow		( wxCommandEvent& event );

	// event listener
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
	void ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system );

private:
	void FillPointsWindow();
	void FillFilterPanel();
	void SetIconForCategory(Uint32 categoryId);
	void OnSearchEnterClicked( wxCommandEvent& event );
	void OnSearchButtonClicked( wxCommandEvent& event );
	void OnShowRowContextMenu( wxListEvent& event );
	void OnRowPopupClick( wxCommandEvent& event );
	void OnColumnPopupClick( wxCommandEvent& event );
	void OnSortByColumn( wxListEvent& event );
	void OnShowColumnContextMenu( wxListEvent& event );

	Int32 GetSelectedItemIndex();
	void ClearSelection();

	void InternalProcessMessage();

private:
	wxDialog*		m_addPOIWindow;
	wxDialog*		m_showModifyPOIWindow;

	// main panel
	wxToolBar*		m_mainToolbar;
	wxSpinCtrl*		m_autoSyncTime;
	wxCheckBox*		m_autoSyncEnable;
	wxPanel*		m_mainPanel;
	wxPanel*		m_filtersPanel;
	wxListCtrl*		m_pointsList;
	wxToolBar*		m_pointToolBar;
	wxPanel*		m_connectionPanel;
	wxMenu*			m_columnContextMenu;
	wxMenu*			m_rowContextMenu;

	// searching
	wxTextCtrl*		m_searchLine;			//!< 
	wxButton*		m_searchButton;			//!< 

	TDynArray< Uint32 >	m_searchingResults;		//!< 
	Int32				m_activeSelectedResult;	//!< 

	// m_filtersPanel
	wxCheckBox*		m_filterShowOnMap;
	wxCheckBox*		m_filterSelectAll;
	wxCheckListBox* m_filterCategoriesList;
	wxButton*		m_filterButton;

	// m_addPOIWindow
	wxTextCtrl*		m_newPOITitle;
	wxTextCtrl*		m_newPOIDescription;
	wxCheckBox*		m_newPOISnapToTerrain;

	// m_showModifyPOIWindow
	wxTextCtrl*		m_POITitle;
	wxTextCtrl*		m_POIDescription;
	wxCheckBox*		m_snapToTerrain;
	wxChoice*		m_POICategory;
	wxStaticBitmap* m_POICategoryIcon;
	wxButton*		m_showInBlackBoxButton;
	wxButton*		m_editPOIButton;
	wxButton*		m_cancelPOIButton;

	// others
	CPointofInterest*	m_pointOpenToEdit;
	Uint32				m_newPointCategoryId;

	// logic
	CPoiSystem*	m_attachedMarkerSystem;

	Red::Threads::CMutex	m_synchronizationMutex;		//!<
	TQueue< EMarkerSystemMessage > m_messages;
};

#endif // NO_MARKER_SYSTEMS

