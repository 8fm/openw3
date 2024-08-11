/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef __SCRIPT_STUDIO_SEARCH_DIALOG_H__
#define __SCRIPT_STUDIO_SEARCH_DIALOG_H__

#include "solution/slnDeclarations.h"

class CSSSearchResults : public wxListCtrl
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CSSSearchResults );

public:
	struct CSearchEntry
	{
		SolutionFilePtr	m_file;
		unsigned int	m_lineNo;
		unsigned int	m_position;
		unsigned int	m_length;

		wxString		m_lineText;
	};

protected:
	vector< CSearchEntry* >	m_foundLines;
	Solution* m_solution;

public:
	CSSSearchResults( wxWindow* parent, Solution* solution );
	~CSSSearchResults();

	static void FreeResults( vector< CSearchEntry* >& results );

	void   ClearResults();
	void   AddResult( CSearchEntry* entry );
	void   AddResults( const vector< CSearchEntry* >& results );
	size_t CountResults() const { return m_foundLines.size(); }

	void ValidateFilesInResults();

protected:
	void OnItemActivated( wxListEvent& event );
};

#endif // __SCRIPT_STUDIO_SEARCH_DIALOG_H__
