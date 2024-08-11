/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class CEdLightEntityCollector : public wxFrame
{
	DECLARE_EVENT_TABLE();

	TDynArray< CEntity* >				m_lightEntities;

	// Widgets
	wxListBox*						m_lightEntitiesList;

	void ScanLayer( CLayer* layer, CWorld* world );
	void ScanLayerGroup( CLayerGroup* layerGroup, CWorld* world );
	void Refresh();
	void Focus();
	void RefreshProperties( int index );

	// Event handlers
	void OnLightEntitiesDoubleClick( wxCommandEvent& event );
	void OnRefreshClicked( wxCommandEvent& event );
	void OnApplyClicked( wxCommandEvent& event );
	void OnRemoveFromListClicked( wxCommandEvent& event );
	void OnCloseClicked( wxCommandEvent& event );
	void OnItemSelected( wxCommandEvent& event );

public:
	CEdLightEntityCollector( wxWindow* parent );
	~CEdLightEntityCollector();
};
