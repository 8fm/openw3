/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

class CEdChangeDrawableFlagsDlg : public wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdChangeDrawableFlagsDlg( wxWindow* parent, CContextMenuDir* contextMenuDir );
	~CEdChangeDrawableFlagsDlg();

	void FlagValueEnabled( wxCommandEvent& event );

private:
	void OnBatch( wxCommandEvent& event );
	void ChangeDrawableFlags( CEntityTemplate* entityTemplate );

private:
	struct FlagCheckbox
	{
		String m_name;
		wxCheckBox* m_valCBox;
	};

	THashMap< wxCheckBox*, FlagCheckbox > m_drawableFlags;

	CBitField*					m_bitField;
	CContextMenuDir				m_contextMenuDir;
	THashMap< String, Uint32 >	m_mapCheckBoxToBit;
};