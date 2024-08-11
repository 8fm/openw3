#pragma once

class CEdScriptCompilationErrorsDialog : public wxDialog
{
public:
	CEdScriptCompilationErrorsDialog( wxWindow* parent, const CScriptCompilationMessages& errorCollector );
	~CEdScriptCompilationErrorsDialog();

	ECompileScriptsReturnValue GetUserChoice() const { return m_userChoice; }

private:
	void OnItemSelected( wxListEvent& event );
	void OnRecompile(wxCommandEvent& event);
	void OnSkipCompile(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	void Fill( const TDynArray< CScriptCompilationMessages::SContext >& contexts, const wxString& typeStr, const wxColour& colour );
	
	ECompileScriptsReturnValue m_userChoice;
	wxListCtrl* m_errorCtrl;
};
