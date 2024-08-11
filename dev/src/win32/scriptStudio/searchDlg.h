/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#ifndef __SEARCH_DIALOG_H__
#define __SEARCH_DIALOG_H__

#include "searchResults.h"
class SolutionDir;

#define MAX_SEARCH_QUERY_HISTORY ( 10 )

/// Input box
class CSSSearchDialog : public wxDialog 
{
	wxDECLARE_CLASS( CSSSearchDialog );
	wxDECLARE_EVENT_TABLE();

	enum ESearchInFilesMode
	{
		SearchInFilesMode_Current	= 0,
		SearchInFilesMode_Opened	= 1,
		SearchInFilesMode_All		= 2,
	};

protected:
	typedef std::list< wxString > TSearchQueries;

	Solution*		m_solution;

	wxComboBox*		m_txtToFind;
	wxCheckBox*		m_chkMatchCase;
	wxCheckBox*		m_chkUseRegExp;
	wxRadioBox*		m_rdbSearchMode;
	wxRadioBox*		m_rdbFilesMode;
	wxButton*		m_btnFindNext;
	wxButton*		m_btnFindPrev;
	wxButton*		m_btnFindAll;
	TSearchQueries	m_previousSearchQueries;

	wxPanel*		m_pnlReplace1;
	wxComboBox*		m_txtToReplace;
	wxPanel*		m_pnlReplace2;
	wxButton*		m_btnReplaceNext;
	wxButton*		m_btnReplacePrev;
	wxButton*		m_btnReplaceAll;
	TSearchQueries	m_previousReplaceQueries;

public:
	CSSSearchDialog( wxWindow* parent, Solution* solution );

	void SetTextToFind( const wxString& findWhat );
	void SetFindAll( bool findAll );
	void FindNext();
	void FindPrev();

	void RecordHistory( TSearchQueries& history, wxString newHistory );
	void SaveHistory( wxFileConfig& config );
	void SaveHistory( wxFileConfig& config, const wxString& path, TSearchQueries& history );
	void LoadHistory( wxFileConfig& config );
	void LoadHistory( wxFileConfig& config, const wxString& path, TSearchQueries& history );

protected:
	void OnFindNext( wxCommandEvent& event );
	void OnFindPrev( wxCommandEvent& event );
	void OnFindAll( wxCommandEvent& event );
	void OnReplaceNext( wxCommandEvent& event );
	void OnReplacePrev( wxCommandEvent& event );
	void OnReplaceAll( wxCommandEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnClose( wxCloseEvent& event );

	int CollectFlags();
	void OnShow( wxShowEvent& event );
	void FillHistory( wxComboBox* queryBox, TSearchQueries& history );


	inline bool IsWordCharacter( char ch ) const;

	void FindInFile( const SolutionFilePtr& file, int flags, wxString& textToFind, vector<CSSSearchResults::CSearchEntry*>& results, bool breakOnFirstMatch = false ) const;
	void FindInDirectory( SolutionDir* dir, int flags, wxString& textToFind, vector<CSSSearchResults::CSearchEntry*>& results ) const;

	void ReplaceInFile( const SolutionFilePtr& file, int flags, wxString& textToFind, wxString& textToReplace, vector< CSSSearchResults::CSearchEntry* >& results ) const;
	void ReplaceInDirectory( SolutionDir* dir, int flags, wxString& textToFind, wxString& textToReplace, vector< CSSSearchResults::CSearchEntry* >& results ) const;
};

inline bool CSSSearchDialog::IsWordCharacter( char ch ) const
{
	return ( ch >= ( 'A' ) && ch <= ( 'Z' ) ) || ( ch >= ( 'a' ) && ch <= ( 'z' ) )  || ( ch >= ( '0' ) && ch <= ( '9' ) )  || ( ch == ( '_' ) );
}

#endif // __SEARCH_DIALOG_H__
