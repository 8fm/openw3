/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef EDITORDLCLIST_H
#define EDITORDLCLIST_H

class CEdDLCListDlg : public wxDialog
{
public:
	CEdDLCListDlg( wxWindow* parent );

private:
	void FillList();
	void OnOK( wxCommandEvent& event );

	TSortedMap< Int32, CName > m_dlcIds;

	wxStaticText* m_staticText1;
	wxCheckListBox* m_dlcList;
	wxButton* m_buttonOK;
};

#endif // EDITORABOUT_H
