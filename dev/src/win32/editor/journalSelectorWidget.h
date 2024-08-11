#pragma once

class CEdJournalTree;

wxDECLARE_EVENT( wxEVT_PATH_SELECTED, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_PATH_CLEARED, wxCommandEvent );

class CJournalSelectorWidget : public wxTextCtrl
{
	wxDECLARE_CLASS( CJournalSelectorWidget );

public:
	CJournalSelectorWidget( wxWindow* parent, wxWindowID id, Uint32 journalFlags, const CClass* typeSelectable = NULL );
	~CJournalSelectorWidget();

	const THandle< CJournalPath > GetPath() const { return m_path; }
	THandle< CJournalPath > GetPath() { return m_path; }
	void SetPath( THandle< CJournalPath > path );

private:
	void OnItemSelected( wxTreeEvent& event );
	void OnFocus( wxFocusEvent& event );
	void OnFocusLost( wxFocusEvent& event );

public:
	void OnItemCleared();


private:
	THandle< CJournalPath > m_path;

	CEdJournalTree* m_selectorWidget;
	const CClass* m_typeSelectable;
	Uint32 m_flags;
};
