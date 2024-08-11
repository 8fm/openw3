#pragma once

wxDECLARE_EVENT( wxEVT_CHOOSE_STRING_OK, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_CHOOSE_STRING_CANCEL, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_CHOOSE_STRING_NEW, wxCommandEvent );

class CEdStringSelector : public wxSmartLayoutFrame
{
public:
	CEdStringSelector( wxWindow* parent );
	~CEdStringSelector();

private:
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

private:

	void OnCategorySelected( wxCommandEvent& event );

	void OnSearchTypeChange( wxCommandEvent& event );
	void OnSearchExecute( wxCommandEvent& event );

	void OnItemClicked( wxListEvent& event );
	void OnItemDoubleClicked( wxListEvent& event );
	void OnOK( wxCommandEvent& event );

	void OnCancel( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );

private:

	void SubmitAndClose();
	void CancelAndClose();

	long m_selectedItem;

	wxCheckListBox* m_categoryList;
	wxListCtrl* m_results;
	Bool m_searchKeys;

	TDynArray< String > m_selectedCategories;
};
