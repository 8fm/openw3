/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

wxDECLARE_EVENT( wxEVT_TAGEDITOR_OK, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_TAGEDITOR_CANCEL, wxCommandEvent );

/// Editor of tags
class CEdTagEditor : public wxSmartLayoutDialog
{
	wxDECLARE_EVENT_TABLE();

protected:
	TagList						m_tagList;		//!< List of tags to edit
	wxTreeListCtrl*				m_worldTags;	//!< List of world tags
	wxListBox*					m_localTags;	//!< List of local tags
	wxTextCtrl*					m_filter;		//!< Search filter
	TDynArray< CName >			m_tags;			//!< Local tags
	CEdTimer					m_filterTimer;	//!< Filter update timer
    TSet< String >              m_historyTags;  //!< Tags used somewhere in the history

public:
	//! Get edited tag list
	RED_INLINE const TDynArray< CName >& GetTags() const { return m_tags; }

public:
	CEdTagEditor( wxWindow* parent, const TagList& currentTagList );
	~CEdTagEditor();

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();

protected:
	void UpdateWorldTagList();
	void UpdateLocalTagList();
	void SelectLocalTag( const CName& tag );

protected:
	void OnAddNewTag( wxCommandEvent& event );
	void OnSetTag( wxCommandEvent& event );
	void OnClearTag( wxCommandEvent& event );
	void OnEditTag( wxCommandEvent& event );
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnActivateTag( wxTreeEvent& event );
	void OnTimer( wxTimerEvent& event );
	void OnFilterChanged( wxCommandEvent& event );
};
