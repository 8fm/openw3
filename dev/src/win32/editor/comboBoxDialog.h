/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Input box
class CEdComboBoxDlg: public wxDialog 
{
protected:
	CEdChoice*		m_box;
	String			m_selectText;

public:
	CEdComboBoxDlg( wxWindow* parent, const String& title, const String &caption, const String &defaultChoice, const TDynArray<String>& choices );
	RED_INLINE const String &GetSelectText() { return m_selectText; }

protected:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
};