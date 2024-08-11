/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "../../common/engine/abstractMarkerSystem.h"

class CSticker;
class CStickersSystem;

class CEdStickersPanel : public wxPanel, public IEdEventListener, public IMarkerSystemListener
{
	DECLARE_EVENT_TABLE();

public:	
	CEdStickersPanel(wxWindow* parent);
	~CEdStickersPanel();

	// wxWidgets callback functions
	void OnShowAddNewStickerWindow		( wxCommandEvent& event );
	void OnShowEditStickerWindow		( wxCommandEvent& event );

	void OnDeleteSelectedSticker		( wxCommandEvent& event );
	void OnSingleClickSticker			( wxCommandEvent& event );
	void OnDoubleClickSticker			( wxCommandEvent& event );
	void OnUpdateAutoSyncTime			( wxCommandEvent& event );
	void OnEnableAutoSyncStickers		( wxCommandEvent& event );
	void OnShowFilterPanel				( wxCommandEvent& event );
	void OnRefreshStickers				( wxCommandEvent& event );

	// m_filtersPanel
	void OnCheckShowOnMap				( wxCommandEvent& event );
	void OnClickFilter					( wxCommandEvent& event );
	void OnSelectFilter					( wxCommandEvent& event );
	void OnSelectAllFilters				( wxCommandEvent& event );

	// m_addStickerWindow
	void OnClickSetStickerOnMap			( wxCommandEvent& event );
	void OnClickAddSticker				( wxCommandEvent& event );
	void OnClickAddStickerCancel		( wxCommandEvent& event );

	// m_showModifyStickerWindow
	void OnClickSetStickerCategory		( wxCommandEvent& event );
	void OnClickModifySticker			( wxCommandEvent& event );
	void OnClickCloseStickerWindow		( wxCommandEvent& event );

	// event listener
	void DispatchEditorEvent			( const CName& name, IEdEventData* data );
	void ProcessMessage( enum EMarkerSystemMessage message, enum EMarkerSystemType systemType, IMarkerSystemInterface* system );

private:
	void FillStickersWindow	();
	void FillFilterPanel	();

	void InternalProcessMessage();

	void OnSearchButtonClicked( wxCommandEvent& event );
	void OnSearchEnterClicked( wxCommandEvent& event );

private:
	wxDialog*		m_addStickerWindow;
	wxDialog*		m_showModifyStickerWindow;

	// main panel
	wxToolBar*		m_mainToolbar;
	wxSpinCtrl*		m_autoSyncTime;
	wxCheckBox*		m_autoSyncEnable;
	wxPanel*		m_mainPanel;
	wxPanel*		m_filtersPanel;
	wxListBox*		m_stickersList;
	wxToolBar*		m_stickerToolBar;
	wxPanel*		m_connectionPanel;

	// searching
	wxTextCtrl*		m_searchLine;			//!< 
	wxButton*		m_searchButton;			//!< 

	// m_filtersPanel
	wxCheckBox*		m_filterShowOnMap;
	wxCheckListBox* m_filterCategoriesList;
	wxButton*		m_filterButton;
	wxButton*		m_filterResetButton;
	wxCheckBox*		m_filterSelectAll;

	// m_addStickerWindow
	wxTextCtrl*		m_newStickerTitle;
	wxTextCtrl*		m_newStickerDescription;
	wxChoice*		m_newStickerCategory;
	wxBitmapButton*	m_newStickerEntity;
	wxButton*		m_newStickerAdd;
	wxButton*		m_newStickerCancel;

	// m_showModifyStickerWindow
	wxTextCtrl*		m_stickerTitle;
	wxTextCtrl*		m_stickerDescription;
	wxChoice*		m_stickerCategory;
	wxButton*		m_editStickerButton;
	wxButton*		m_cancelStickerButton;

	// others
	CSticker*			m_stickerOpenToEdit;
	TDynArray<CEntity*>	m_oldStickerEntities;

	// logic
	CStickersSystem*	m_attachedMarkerSystem;

	Red::Threads::CMutex			m_synchronizationMutex;		//!<
	TQueue< EMarkerSystemMessage >	m_messages;

};

#endif // NO_MARKER_SYSTEMS
