/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdEmptyEntityCollector : public wxFrame
{
	DECLARE_EVENT_TABLE();

	THashSet< CGUID >					m_entitiesInStreaming;
	THashMap< CGUID, CEntity* >			m_foundEntities;
	TDynArray< CEntity* >				m_emptyEntities;

	// Widgets
	wxCheckListBox*						m_emptyEntitiesList;

	void ScanLayer( CLayer* layer, Bool showTagged );
	void ScanLayerGroup( CLayerGroup* layerGroup, Bool showTagged );
	void Refresh();
	void Focus();
	void DeleteSelected();
	void SelectAll();
	void SelectNone();
	void ExportList( Uint32 parts, const wxString& fileName );
	void ImportList( const wxString& fileName );

	// Event handlers
	void OnEmptyEntitiesDoubleClick( wxCommandEvent& event );
	void OnRefreshClicked( wxCommandEvent& event );
	void OnFocusClicked( wxCommandEvent& event );
	void OnDeleteClicked( wxCommandEvent& event );
	void OnSelectAllClicked( wxCommandEvent& event );
	void OnSelectNoneClicked( wxCommandEvent& event );
	void OnCloseClicked( wxCommandEvent& event );
	void OnExportClicked( wxCommandEvent& event );
	void OnImportClicked( wxCommandEvent& event );

public:
	CEdEmptyEntityCollector( wxWindow* parent );
	~CEdEmptyEntityCollector();
};
