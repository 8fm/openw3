/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Local preview cooker
class CEdImportTextureSourceData : public wxFrame
{
	DECLARE_EVENT_TABLE();

protected:
	wxHtmlWindow*			m_stats;

	// Gay unoptimal way, but it's editor and hey - it is editor code :)
	TDynArray< String >		m_resourcesToReimport;
	TDynArray< String >		m_texturesInDepot;
	TDynArray< String >		m_texturesToDelete;

	TDynArray< String >		m_meshesInDepot;
	TDynArray< String >		m_meshesToDelete;

public:
	CEdImportTextureSourceData( wxWindow* parent );
	~CEdImportTextureSourceData();

	void RefreshLog();

protected:
	void OnStartReimport( wxCommandEvent& event );
	void OnClearDepot( wxCommandEvent& event );
	void OnClearDepotMeshes( wxCommandEvent& event );
	void OnRefresh( wxCommandEvent& event );
	void OnLinkClicked( wxHtmlLinkEvent& event );
};

