/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_CHOOSE_ITEM_OK, wxEVT_USER_FIRST + 1 )
	DECLARE_EVENT_TYPE( wxEVT_CHOOSE_ITEM_CANCEL, wxEVT_USER_FIRST + 2 )
END_DECLARE_EVENT_TYPES()

class CEdChooseItemDialog : public wxDialog
{
	DECLARE_EVENT_TABLE()

protected:
	CName				m_item; // Edited value

	wxListBox*			m_categoriesList;
	wxListBox*			m_itemsList;
	wxTextCtrl*			m_itemDescription;

public:
	//! Get edited item
	RED_INLINE const CName& GetItem() const { return m_item; }

public:
	CEdChooseItemDialog( wxWindow* parent, CName currentItem );
	~CEdChooseItemDialog();
	
protected:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnAny( wxCommandEvent& event );
	void OnNoneItem( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnCategorySelected( wxCommandEvent& event );
	void OnItemSelected( wxCommandEvent& event );
	void OnItemDoubleClicked( wxCommandEvent& event );

protected:
	void FillItemsList( CName category );
	void FillDescriptionBox( CName item );
};