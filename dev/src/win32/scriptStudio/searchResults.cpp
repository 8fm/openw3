/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "searchResults.h"

#include "documentView.h"
#include "frame.h"

#include "solution/slnContainer.h"
#include "solution/file.h"

wxBEGIN_EVENT_TABLE( CSSSearchResults, wxListCtrl )
	EVT_LIST_ITEM_ACTIVATED( wxID_ANY, CSSSearchResults::OnItemActivated )
wxEND_EVENT_TABLE();

wxIMPLEMENT_CLASS( CSSSearchResults, wxListCtrl );

CSSSearchResults::CSSSearchResults( wxWindow* parent, Solution* solution )
	: wxListCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT )
	, m_solution( solution )
{
	InsertColumn( 0, wxT( "File" ) );
	SetColumnWidth( 0, 250 );
	InsertColumn( 1, wxT( "Line" ) );
	SetColumnWidth( 1, 50 );
	InsertColumn( 2, wxT( "Context" ) );
	SetColumnWidth( 2, 1000 );
}

CSSSearchResults::~CSSSearchResults()
{
	ClearResults();
}

void CSSSearchResults::FreeResults( vector< CSearchEntry* > & results )
{
	for ( unsigned int i = 0; i < results.size(); ++i )
	{
		delete results[i];
	}
	results.clear();
}

void CSSSearchResults::ClearResults()
{
	FreeResults( m_foundLines );
	DeleteAllItems();
}

void CSSSearchResults::AddResult( CSearchEntry* entry )
{
	m_foundLines.push_back( entry );

	wxString lineNo;
	lineNo << (entry->m_lineNo + 1);

	long idx = InsertItem( GetItemCount(), entry->m_file->m_solutionPath.c_str() );
	SetItem( idx, 1, lineNo );
	SetItem( idx, 2, entry->m_lineText );
	SetItemPtrData( idx, reinterpret_cast< wxUIntPtr >( entry ) );
}

void CSSSearchResults::AddResults( const vector< CSearchEntry* > & results )
{
	for ( size_t i = 0; i < results.size(); ++i )
	{
		AddResult( results[i] );
	}
	if (results.size() > 0)
	{
		SetFocus();
	}
}

void CSSSearchResults::OnItemActivated( wxListEvent& event )
{
	CSearchEntry* entry = reinterpret_cast< CSearchEntry* >( GetItemData( event.GetIndex() ) );

	if ( ! entry->m_file->m_document && ! wxTheFrame->OpenFile( entry->m_file, false ) )
		return;

	CSSDocument* doc = entry->m_file->m_document;
	
	{
		wxTheFrame->ShowTab( entry->m_file->m_documentEx );
		doc->SetSelection( entry->m_position, entry->m_position + entry->m_length );
		doc->EnsureCaretVisible();
		doc->SetFocus();
	}
}

void CSSSearchResults::ValidateFilesInResults()
{
	SolutionFilePtr oldFile( nullptr );
	SolutionFilePtr newFile( nullptr );

	const int start = static_cast< int >( m_foundLines.size() ) - 1;
	for( int i = start; i >= 0; --i )
	{
		if ( oldFile != m_foundLines[ i ]->m_file )
		{
			oldFile = m_foundLines[ i ]->m_file;
			newFile = m_solution->FindFile( oldFile->m_solutionPath );
		}

		if ( newFile )
		{
			m_foundLines[ i ]->m_file = newFile;
		}
		else
		{
			delete m_foundLines[i];
			m_foundLines.erase( m_foundLines.begin() + i );
			DeleteItem( i );
		}
	}
}
