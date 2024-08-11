/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

class CEdChangeLightChannelsDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdChangeLightChannelsDlg( wxWindow* parent, CContextMenuDir* contextMenuDir );
	~CEdChangeLightChannelsDlg();

private:
	void OnBatch( wxCommandEvent& event );
	void ChangeLightChannels( CEntityTemplate* entityTemplate );

private:
	wxCheckListBox*				m_lightChannels;

	CWorld*						m_world;
	CBitField*					m_bitField;
	CContextMenuDir				m_contextMenuDir;
	TDynArray< CDirectory* >	m_selectedDirectories;
	THashMap< Uint32, Uint32 >	m_mapCheckBoxToBit;
};