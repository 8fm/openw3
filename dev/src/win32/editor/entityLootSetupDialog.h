#pragma once


class CEdEntityLootSetupDialog : public wxDialog
{

	CEntityTemplate*	m_entTemplate;

	CEdPropertiesPage*	m_propPage;
	wxListBox*			m_paramsList;

public:
	CEdEntityLootSetupDialog( wxWindow* parent, CEntityTemplate* entTemplate );

	void OnPropAdded			( wxCommandEvent& event );
	void OnPropRemoved			( wxCommandEvent& event );
	void OnPropModified			( wxCommandEvent& event );
	void OnListChanged			( wxCommandEvent& event );

	void OnSaveAndExit			( wxCommandEvent& event );

	void RefreshPropList();
};
