/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class Solution;

/// Compilation error list
class CSSErrorList : public wxTextCtrl
{
	wxDECLARE_CLASS( CSSErrorList );
	DECLARE_EVENT_TABLE();

protected:
	wxArrayString m_warnings;
	wxArrayString m_errors;
	Solution* m_solution;

public:

	enum EType
	{
		Warning = 0,
		Error
	};

	//! Get number of printed errors
	int GetNumErrors() const { return m_errors.GetCount(); }

	//! Get number of printed warnings
	int GetNumWarnings() const { return m_warnings.GetCount(); }

	void List( EType type );

public:
	CSSErrorList( wxWindow* parent, Solution* solution );

	//! Clear list
	void Clear();

	//! Add line
	void AddLine( const wxString& message );

	void AddLine( EType type, const wxString& file, int line, const wxString& message );

protected:
	void OnLeftDClick( wxMouseEvent& event );
};
