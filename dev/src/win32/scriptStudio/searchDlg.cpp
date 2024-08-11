/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "Scintilla.h"
#include "searchDlg.h"
#include "searchResults.h"
#include "documentView.h"
#include "frame.h"

#include "solution/file.h"
#include "solution/dir.h"
#include "solution/slnDeclarations.h"
#include "solution/slnContainer.h"

BEGIN_EVENT_TABLE( CSSSearchDialog, wxDialog )
	EVT_BUTTON( XRCID("m_btnFindNext"), CSSSearchDialog::OnFindNext )
	EVT_BUTTON( XRCID("m_btnFindPrev"), CSSSearchDialog::OnFindPrev )
	EVT_BUTTON( XRCID("m_btnFindAll"), CSSSearchDialog::OnFindAll )
	EVT_BUTTON( XRCID("m_btnReplaceNext"), CSSSearchDialog::OnReplaceNext )
	EVT_BUTTON( XRCID("m_btnReplacePrev"), CSSSearchDialog::OnReplacePrev )
	EVT_BUTTON( XRCID("m_btnReplaceAll"), CSSSearchDialog::OnReplaceAll )
	EVT_CHAR_HOOK( CSSSearchDialog::OnKeyDown )
	EVT_CLOSE( CSSSearchDialog::OnClose )
	EVT_SHOW( CSSSearchDialog::OnShow )
END_EVENT_TABLE()

wxIMPLEMENT_CLASS( CSSSearchDialog, wxDialog );

CSSSearchDialog::CSSSearchDialog( wxWindow* parent, Solution* solution )
:	m_solution( solution )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT( "SearchDialog" ) );

	m_txtToFind			= XRCCTRL( *this, "m_txtToFind", wxComboBox );
	m_chkMatchCase		= XRCCTRL( *this, "m_chkMatchCase", wxCheckBox );
	m_chkUseRegExp		= XRCCTRL( *this, "m_chkUseRegExp", wxCheckBox );
	m_rdbSearchMode		= XRCCTRL( *this, "m_rdbSearchMode", wxRadioBox );
	m_rdbFilesMode		= XRCCTRL( *this, "m_rdbFilesMode", wxRadioBox );
	m_btnFindNext		= XRCCTRL( *this, "m_btnFindNext", wxButton );
	m_btnFindPrev		= XRCCTRL( *this, "m_btnFindPrev", wxButton );
	m_btnFindAll		= XRCCTRL( *this, "m_btnFindAll", wxButton );

	m_pnlReplace1		= XRCCTRL( *this, "m_pnlReplace1", wxPanel );
	m_pnlReplace2		= XRCCTRL( *this, "m_pnlReplace2", wxPanel );
	m_txtToReplace		= XRCCTRL( *this, "m_txtToReplace", wxComboBox );
	m_btnReplaceNext	= XRCCTRL( *this, "m_btnReplaceNext", wxButton );
	m_btnReplacePrev	= XRCCTRL( *this, "m_btnReplacePrev", wxButton );
	m_btnReplaceAll		= XRCCTRL( *this, "m_btnReplaceAll", wxButton );

	// this is annoying, lets add an option later
	// m_txtToFind->SetValue( findWhat );

	Fit();
}

void CSSSearchDialog::OnClose( wxCloseEvent& event )
{
	event.Veto();
	Hide();
}

int CSSSearchDialog::CollectFlags()
{
	int flags = 0;
	
	if ( m_chkMatchCase->GetValue() )
		flags |= SCFIND_MATCHCASE;

	if ( m_chkUseRegExp->GetValue() )
		flags |= SCFIND_REGEXP;

	if ( m_rdbSearchMode->GetSelection() == 1 )
		flags |= SCFIND_WORDSTART;
	else
	if ( m_rdbSearchMode->GetSelection() == 2 )
		flags |= SCFIND_WHOLEWORD;

	return flags;
}

void CSSSearchDialog::OnFindNext( wxCommandEvent& event )
{
	CSSDocument* doc = wxTheFrame->GetCurrentDocument();
	if ( ! doc )
		return;

	// Make sure there is text to search for
	if ( m_txtToFind->GetValue().empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	int sel_start = doc->GetSelectionStart();
	int sel_end   = doc->GetSelectionEnd();
	int sel_pos   = Max( sel_start, sel_end );
	doc->SetSelection( sel_pos, sel_pos );
	doc->SearchAnchor();

	RecordHistory( m_previousSearchQueries, m_txtToFind->GetValue() );
	FillHistory( m_txtToFind, m_previousSearchQueries );

	if ( doc->SearchNext( CollectFlags(), m_txtToFind->GetValue() ) >= 0 )
		doc->EnsureCaretVisible();
	else
		wxMessageBox( wxT("No more occurances found"), wxT("Search results") );
}

void CSSSearchDialog::OnFindPrev( wxCommandEvent& event )
{
	CSSDocument* doc = wxTheFrame->GetCurrentDocument();
	if ( !doc )
		return;

	// Make sure there is text to search for
	if ( m_txtToFind->GetValue().empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	int sel_start = doc->GetSelectionStart();
	int sel_end   = doc->GetSelectionEnd();
	int sel_pos   = Min( sel_start, sel_end );
	doc->SetSelection( sel_pos, sel_pos );
	doc->SearchAnchor();

	RecordHistory( m_previousSearchQueries, m_txtToFind->GetValue() );
	FillHistory( m_txtToFind, m_previousSearchQueries );

	if ( doc->SearchPrev( CollectFlags(), m_txtToFind->GetValue() ) )
	{
		doc->EnsureCaretVisible();
	}
	else
	{
		wxMessageBox( wxT("No more occurrences found"), wxT("Search results") );
	}
}

void CSSSearchDialog::FindInFile( const SolutionFilePtr& file, int flags, wxString& textToFind, vector<CSSSearchResults::CSearchEntry*>& results, bool breakOnFirstMatch ) const
{
	CSSDocument* doc = file->m_document;

	// File could be a pending deletion in source control
	if( !file->ExistsOnDisk() )
	{
		return;
	}

	int numFound = 0;
	int textLen  = textToFind.Len();

	if ( doc == NULL )
	{
		// Load file
		wxString fileContent = file->GetText();
			
		// Operate on Scintilla native format (UTF8), to get proper offsets
		wxWX2MBbuf textOrgSCI = wx2stc( fileContent );
		if ( ( flags & SCFIND_MATCHCASE ) == 0 )
			fileContent.LowerCase();
		wxWX2MBbuf textSCI    = wx2stc( fileContent );
		wxWX2MBbuf searchSCI  = wx2stc( textToFind );
			
		// Init search data
		char* textStart    = textSCI.data();
		char* textEnd      = textStart + strlen( textSCI );
		char* curPos       = textStart;
		char* curLineStart = textStart;
		size_t curLineNo    = 0;
		char* lastEnter    = curPos-1;
			
		// Perform search
		while ( true )
		{
			curPos = Red::System::StringSearch( curPos, searchSCI.data() );
			if ( !curPos )
				break;

			// Match whole word
			if ( (flags & SCFIND_WHOLEWORD) != 0 )
			{
				if ( curPos > textStart && IsWordCharacter( *(curPos-1) ) )
				{
					++curPos;
					continue;
				}
				if ( curPos+textLen < textEnd && IsWordCharacter( *(curPos+textLen) ) )
				{
					++curPos;
					continue;
				}
			}
			// Match word start
			if ( (flags & SCFIND_WORDSTART) != 0 )
			{
				if ( curPos > textStart && IsWordCharacter( *(curPos-1) ) )
				{
					++curPos;
					continue;
				}
			}

			// Calculate current line number
			char* curLineEnd = Red::System::StringSearch( curPos, "\n" );
			if ( curPos > textStart )
			{
				char* newLinePos = lastEnter + 1;
				while ( true )
				{
					newLinePos = Red::System::StringSearch( newLinePos, "\n" );
					if ( newLinePos == NULL || newLinePos >= curPos )
						break;
						
					++curLineNo;
					++newLinePos;
					curLineStart = newLinePos;
				}
				lastEnter = curPos-1;
			}

			// Store result
			CSSSearchResults::CSearchEntry* entry = new CSSSearchResults::CSearchEntry();
			entry->m_file     = file;
			entry->m_lineNo   = curLineNo;
			entry->m_position = curPos - textStart;
			entry->m_length   = textLen;
			entry->m_lineText = stc2wx( textOrgSCI.data() + ( curLineStart - textStart ),
										curLineEnd ? curLineEnd-curLineStart : textEnd-curLineStart );
			entry->m_lineText.Trim( true  );
			entry->m_lineText.Trim( false );
			results.push_back( entry );

			if ( breakOnFirstMatch )
				break;

			++curPos;
			++numFound;
		}
	}
	else
	{
		int curPos = 0;
		while ( true )
		{
			curPos = doc->FindText( curPos, doc->GetLength(), textToFind, flags );

			if ( curPos < 0 )
				break;

			// Store result
			CSSSearchResults::CSearchEntry* entry = new CSSSearchResults::CSearchEntry();
			entry->m_file     = file;
			entry->m_lineNo   = doc->LineFromPosition( curPos );
			entry->m_position = curPos;
			entry->m_length   = textLen;
			entry->m_lineText = doc->GetLine( entry->m_lineNo );
			entry->m_lineText.Trim( true  );
			entry->m_lineText.Trim( false );
			results.push_back( entry );

			if ( breakOnFirstMatch )
				break;

			++curPos;
			++numFound;
		}
	}
}

void CSSSearchDialog::FindInDirectory( SolutionDir* dir, int flags, wxString& textToFind, vector<CSSSearchResults::CSearchEntry*>& results ) const
{
	vector< SolutionDir* >::const_iterator subDirCurr = dir->BeginDirectories();
	vector< SolutionDir* >::const_iterator subDirLast = dir->EndDirectories();

	for ( ; subDirCurr != subDirLast; ++subDirCurr )
	{
		FindInDirectory( *subDirCurr, flags, textToFind, results );
	}

	vector< SolutionFilePtr >::const_iterator fileCurr = dir->BeginFiles();
	vector< SolutionFilePtr >::const_iterator fileLast = dir->EndFiles();

	for ( ; fileCurr != fileLast; ++fileCurr )
	{
		FindInFile( *fileCurr, flags, textToFind, results );
	}
}
	
void CSSSearchDialog::ReplaceInFile( const SolutionFilePtr& file, int flags, wxString& textToFind, wxString& textToReplace, vector< CSSSearchResults::CSearchEntry* >& results ) const
{
	if ( file->m_document == NULL )
	{
		vector< CSSSearchResults::CSearchEntry* > tmpResults;
		FindInFile( file, flags, textToFind, tmpResults, true );
			
		if ( ! tmpResults.empty() )
		{
			CSSSearchResults::FreeResults( tmpResults );
			wxTheFrame->OpenFile( file, true );
		}
	}

	CSSDocument* doc = file->m_document;

	if ( !doc )
	{
		return;
	}

	int textToFindLen = textToFind.Len();
	int textToReplLen = textToReplace.Len();

	int numReplaced = 0;
	int curPos      = 0;
		
	while ( true )
	{
		curPos = doc->FindText( curPos, doc->GetLength(), textToFind, flags );

		if ( curPos < 0 )
		{
			break;
		}

		// Store result
		CSSSearchResults::CSearchEntry* entry = new CSSSearchResults::CSearchEntry();
		entry->m_file     = file;
		entry->m_lineNo   = doc->LineFromPosition( curPos );
		entry->m_position = curPos;
		entry->m_length   = textToReplLen;
		entry->m_lineText = doc->GetLine( entry->m_lineNo );
		entry->m_lineText.Trim( true  );
		entry->m_lineText.Trim( false );
		results.push_back( entry );

		// Perform replace
		doc->SetTargetStart( curPos );
		doc->SetTargetEnd( curPos + textToFindLen );
		doc->ReplaceTarget( textToReplace );
		curPos += textToReplLen;

		++numReplaced;
	}
}

void CSSSearchDialog::ReplaceInDirectory( SolutionDir* dir, int flags, wxString& textToFind, wxString& textToReplace, vector< CSSSearchResults::CSearchEntry* >& results ) const
{
	vector< SolutionDir* >::const_iterator subDirCurr = dir->BeginDirectories();
	vector< SolutionDir* >::const_iterator subDirLast = dir->EndDirectories();

	for ( ; subDirCurr != subDirLast; ++subDirCurr )
	{
		ReplaceInDirectory( *subDirCurr, flags, textToFind, textToReplace, results );
	}

	vector< SolutionFilePtr >::const_iterator fileCurr = dir->BeginFiles();
	vector< SolutionFilePtr >::const_iterator fileLast = dir->EndFiles();

	for ( ; fileCurr != fileLast; ++fileCurr )
	{
		ReplaceInFile( *fileCurr, flags, textToFind, textToReplace, results );
	}
}

void CSSSearchDialog::OnFindAll( wxCommandEvent& event )
{
	int flags           = CollectFlags();
	wxString textToFind = m_txtToFind->GetValue();

	// Make sure there is text to search for
	if ( textToFind.empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	RecordHistory( m_previousSearchQueries, textToFind );
	FillHistory( m_txtToFind, m_previousSearchQueries );

	if ( (flags & SCFIND_MATCHCASE) == 0 )
		textToFind.LowerCase();

	vector< CSSSearchResults::CSearchEntry* > results;
	
	switch ( m_rdbFilesMode->GetSelection() )
	{
		case SearchInFilesMode_Current:
			{
				CSSDocument* doc = wxTheFrame->GetCurrentDocument();
				if ( ! doc )
					return;

				FindInFile( doc->GetFile(), flags, textToFind, results );
			}
			break;

		case SearchInFilesMode_Opened:
			{
				vector< CSSDocument* > docs;
				wxTheFrame->GetDocuments( docs );
				for ( size_t i=0; i<docs.size(); i++ )
				{
					CSSDocument* doc = docs[i];
					FindInFile( doc->GetFile(), flags, textToFind, results );
				}
			}
			break;

		case SearchInFilesMode_All:

			// Reverse order so we look at mods first then core
			for( int i = Solution_Max - 1; i >= 0; --i )
			{
				SolutionDir* dir = m_solution->GetRoot( static_cast< ESolutionType >( i ) );

				if( dir )
				{
					FindInDirectory( dir, flags, textToFind, results );
				}
			}
			break;
	}

	wxTheFrame->ShowTab( wxTheFrame->GetSearchResults() );
	wxTheFrame->GetSearchResults()->ClearResults();

	wxTheFrame->GetSearchResults()->AddResults( results );
}

void CSSSearchDialog::OnReplaceNext( wxCommandEvent& event )
{
	CSSDocument* doc = wxTheFrame->GetCurrentDocument();
	if ( ! doc )
		return;

	// Make sure there is text to search for
	if ( m_txtToFind->GetValue().empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	RecordHistory( m_previousSearchQueries, m_txtToFind->GetValue() );
	RecordHistory( m_previousReplaceQueries, m_txtToReplace->GetValue() );
	FillHistory( m_txtToFind, m_previousSearchQueries );
	FillHistory( m_txtToReplace, m_previousReplaceQueries );

	wxString sel_text = doc->GetSelectedText();

	if ( sel_text == m_txtToFind->GetValue() )
		doc->ReplaceSelection( m_txtToReplace->GetValue() );
	
	FindNext();
}

void CSSSearchDialog::OnReplacePrev( wxCommandEvent& event )
{
	CSSDocument* doc = wxTheFrame->GetCurrentDocument();
	if ( ! doc )
		return;

	// Make sure there is text to search for
	if ( m_txtToFind->GetValue().empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	RecordHistory( m_previousSearchQueries, m_txtToFind->GetValue() );
	RecordHistory( m_previousReplaceQueries, m_txtToReplace->GetValue() );

	FillHistory( m_txtToFind, m_previousSearchQueries );
	FillHistory( m_txtToReplace, m_previousReplaceQueries );

	wxString sel_text = doc->GetSelectedText();

	if ( sel_text == m_txtToFind->GetValue() )
		doc->ReplaceSelection( m_txtToReplace->GetValue() );

	FindPrev();
}

void CSSSearchDialog::OnReplaceAll( wxCommandEvent& event )
{
	wxTheFrame->ShowTab( wxTheFrame->GetSearchResults() );
	wxTheFrame->GetSearchResults()->ClearResults();

	int flags              = CollectFlags();
	wxString textToFind    = m_txtToFind->GetValue();

	// Make sure there is text to search for
	if ( textToFind.empty() )
	{
		wxMessageBox( wxT("Please enter some value to search for"), wxT("Empty text"), MB_OK );
		m_txtToFind->SetFocus();
		return;
	}

	RecordHistory( m_previousSearchQueries, textToFind );
	FillHistory( m_txtToFind, m_previousSearchQueries );

	if ( (flags & SCFIND_MATCHCASE) == 0 )
		textToFind.LowerCase();

	wxString textToReplace = m_txtToReplace->GetValue();

	RecordHistory( m_previousReplaceQueries, textToReplace );
	FillHistory( m_txtToReplace, m_previousReplaceQueries );

	vector<CSSSearchResults::CSearchEntry*> results;

	switch ( m_rdbFilesMode->GetSelection() )
	{
		case SearchInFilesMode_Current:
			{
				CSSDocument* doc = wxTheFrame->GetCurrentDocument();
				if ( ! doc )
					return;

				ReplaceInFile( doc->GetFile(), flags, textToFind, textToReplace, results );
			}
			break;

		case SearchInFilesMode_Opened:
			{
				vector< CSSDocument* > docs;
				wxTheFrame->GetDocuments( docs );
				for ( size_t i=0; i<docs.size(); ++i )
				{
					CSSDocument* doc = docs[i];
					ReplaceInFile( doc->GetFile(), flags, textToFind, textToReplace, results );
				}
			}
			break;

		case SearchInFilesMode_All:
			// Reverse order so we look at mods first then core
			for( int i = Solution_Max - 1; i >= 0; ++i )
			{
				SolutionDir* dir = m_solution->GetRoot( static_cast< ESolutionType >( i ) );

				if( dir )
				{
					ReplaceInDirectory( dir, flags, textToFind, textToReplace, results );
				}
			}
			break;
	}

	wxTheFrame->GetSearchResults()->AddResults( results );

	wxString textResult = wxString::Format( wxT("%d occurrences replaced"), wxTheFrame->GetSearchResults()->CountResults() );
	wxMessageBox( textResult, wxT("Find & Replace results") );
}

void CSSSearchDialog::OnKeyDown( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER)
	{
		if ( wxWindow::FindFocus()->GetClassInfo()->IsKindOf( &wxButton::ms_classInfo ) )
		{
			event.Skip();
			return;
		}
		if ( event.ControlDown() )
		{
			if ( event.ShiftDown() )
			{
				wxCommandEvent dummy_event;
				OnFindAll( dummy_event );
			}
			else
			{
				wxCommandEvent dummy_event;
				OnFindPrev( dummy_event );
			}
		}
		else
		{
			wxCommandEvent dummy_event;
			OnFindNext( dummy_event );
		}
	}
	else
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		Close();
	}
	else
		event.Skip();
}

void CSSSearchDialog::RecordHistory( TSearchQueries& history, wxString newHistory )
{
	TSearchQueries::iterator historySearch = std::find( history.begin(), history.end(), newHistory );

	if( !newHistory.Trim().IsEmpty() )
	{
		if( historySearch != history.end() )
		{
			history.erase( historySearch );
		}

		history.push_front( newHistory );
	}
}

void CSSSearchDialog::LoadHistory( wxFileConfig& config, const wxString& path, TSearchQueries& history )
{
	config.SetPath( wxT( "/SearchHistory" ) );
	config.SetPath( path );

	int historyCount = 0;
	config.Read( wxT( "Count" ), &historyCount, 0 );

	for( int i = 0; i < historyCount; ++i )
	{
		wxString path;
		path << i;

		wxString historicSearch = config.Read( path, wxEmptyString );

		history.push_back( historicSearch );
	}
}

void CSSSearchDialog::SaveHistory( wxFileConfig& config, const wxString& path, TSearchQueries& history )
{
	config.SetPath( wxT( "/SearchHistory" ) );
	config.SetPath( path );

	unsigned int count = history.size();

	if( count > MAX_SEARCH_QUERY_HISTORY )
	{
		count = MAX_SEARCH_QUERY_HISTORY;
	}

	config.Write( wxT( "Count" ), count );

	unsigned int i = 0;
	for( TSearchQueries::const_iterator iter = history.begin(); iter != history.end() && i < count; ++iter )
	{
		wxString path;
		path << i;

		config.Write( path, *iter );
		++i;
	}
}

void CSSSearchDialog::SaveHistory( wxFileConfig& config )
{
	SaveHistory( config, wxT( "Search" ), m_previousSearchQueries );
	SaveHistory( config, wxT( "Replace" ), m_previousReplaceQueries );
}

void CSSSearchDialog::LoadHistory( wxFileConfig& config )
{
	LoadHistory( config, wxT( "Search" ), m_previousSearchQueries );
	LoadHistory( config, wxT( "Replace" ), m_previousReplaceQueries );
}

void CSSSearchDialog::FillHistory( wxComboBox* queryBox, TSearchQueries& history )
{
	wxString currentSearchQuery = queryBox->GetValue();

	queryBox->Clear();

	for( TSearchQueries::const_iterator iter = history.begin(); iter != history.end(); ++iter )
	{
		queryBox->Append( *iter );
	}

	queryBox->SetValue( currentSearchQuery );
}

void CSSSearchDialog::OnShow( wxShowEvent& event )
{
	FillHistory( m_txtToFind, m_previousSearchQueries );
	FillHistory( m_txtToReplace, m_previousReplaceQueries );
}

void CSSSearchDialog::SetTextToFind( const wxString& findWhat )
{
	m_txtToFind->SetValue( findWhat );
	m_txtToFind->SetFocus();
	m_txtToFind->SelectAll();
}

void CSSSearchDialog::SetFindAll( bool findAll )
{
	if ( findAll )
		m_rdbFilesMode->SetSelection( SearchInFilesMode_All );
	else
		m_rdbFilesMode->SetSelection( SearchInFilesMode_Current );
}

void CSSSearchDialog::FindNext()
{
	wxCommandEvent dummy_event;
	OnFindNext( dummy_event );
}

void CSSSearchDialog::FindPrev()
{
	wxCommandEvent dummy_event;
	OnFindPrev( dummy_event );
}
