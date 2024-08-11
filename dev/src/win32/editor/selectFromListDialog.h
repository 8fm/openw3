#pragma once

class CEdSelectFromListDialog : public wxSmartLayoutDialog
{
	wxDECLARE_CLASS( CEdSelectFromListDialog );

public:
	CEdSelectFromListDialog( wxWindow* parent, const TDynArray< String >& itemList, TDynArray< Uint32 >& selectedIndicies );
	~CEdSelectFromListDialog();

private:
	void OnOK( wxCommandEvent& event );
	void OnSelectAll( wxCommandEvent& event );
	void OnItemSelected( wxCommandEvent& event );

private:
	TDynArray< Uint32 >& m_selectedIndicies;

	wxCheckListBox* m_listBox;
	wxCheckBox* m_selectAll;
};
