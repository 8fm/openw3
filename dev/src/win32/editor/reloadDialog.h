/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CReloadFileInfo;

class CEdFileReloadDialog : public wxDialog
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	DECLARE_EVENT_TABLE();

protected:
	TSortedSet< CResource*, CResource::CompareFunc >	m_allResources;

	Bool				m_ready;
	wxCollapsiblePane*	m_collPane;
	wxListCtrl*			m_listEditor;
	wxListCtrl*			m_listAll;

public:
	CEdFileReloadDialog();

	void AddResourceToReload( CResource* res );
	void AddResourceToReloadFromEditor( const CReloadFileInfo& info );
	void DoModal();

protected:
	void OnReload( wxCommandEvent& event );
	void OnSkip( wxCommandEvent& event );
	void OnPaneChanged( wxCollapsiblePaneEvent& event );
};
