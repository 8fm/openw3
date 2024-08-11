/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#ifndef __OPEN_SOLUTION_FILE_H__
#define __OPEN_SOLUTION_FILE_H__

#include "solution/slnDeclarations.h"

/// Open Solution File dialog
class CSSOpenSolutionFileDialog : public wxDialog 
{
	wxDECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CSSOpenSolutionFileDialog );

protected:
	wxListCtrl*					m_listCtrlsolutionFiles;
	wxTextCtrl*					m_txtFileFilter;

	struct Cache
	{
		struct Entry
		{
			int fileIndex;
			int directoryIndex;
			wxString path;
		};

		wxSortedArrayString files;
		wxArrayString directories;
		vector< Entry > entries;
	};

	static Cache s_cache;

public:
	CSSOpenSolutionFileDialog( wxWindow* parent );
	virtual ~CSSOpenSolutionFileDialog();

	static void BuildCache( const vector< SolutionFilePtr >& allSolutionFiles );

protected:
	void OpenSolutionFile( long item );
	void FileFilterTextUpdate();

	bool OnKeyDownGeneric( wxKeyEvent& event );
	void OnKeyDownFilter( wxKeyEvent& event );
	void OnKeyDownList( wxKeyEvent& event );
	void OnShow( wxShowEvent& event );
	void OnItemActivated( wxListEvent& event );
	void OnFileFilterTextUpdate( wxCommandEvent& event );
};

#endif // __OPEN_SOLUTION_FILE_H__
