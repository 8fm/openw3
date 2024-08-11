/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

class CEdChangeMaterialFlagsDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdChangeMaterialFlagsDlg( wxWindow* parent, CContextMenuDir* contextMenuDir );
	~CEdChangeMaterialFlagsDlg();

private:
	void OnBatch( wxCommandEvent& event );
	void OnFlagsChanged( wxCommandEvent& event );
	void ChangeMaterialFlags( CMaterialGraph* materialGraph );

private:
	wxCheckListBox*				m_materialFlags;
	wxButton*					m_applyButton;

	CContextMenuDir				m_contextMenuDir;

	wxString					m_activeLabel;
	const wxString				m_inactiveLabel;
	const Uint32				m_materialFlagCount;

	TStaticArray< String, 5 >	m_materialFlagNames;
};