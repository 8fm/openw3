/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CItemDisplayer;

class CEdDisplayItemDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

protected:
	wxComboBox*		m_itemCategoryCombo;
	wxComboBox*		m_itemNameCombo;
	wxComboBox*		m_slotCombo;

	CItemDisplayer*		m_itemDisplayer;

public:
	CEdDisplayItemDialog( wxWindow* parent, CItemDisplayer* displayer );
	
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	
	void OnCategorySelected( wxCommandEvent& event );
};
