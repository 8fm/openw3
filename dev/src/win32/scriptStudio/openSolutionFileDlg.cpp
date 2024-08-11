/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "openSolutionFileDlg.h"

#include "events/eventGoto.h"
#include "solution/file.h"

BEGIN_EVENT_TABLE( CSSOpenSolutionFileDialog, wxDialog )
	EVT_SHOW( CSSOpenSolutionFileDialog::OnShow )
	EVT_LIST_ITEM_ACTIVATED( XRCID("listCtrlSolutionFiles"), CSSOpenSolutionFileDialog::OnItemActivated )
	EVT_TEXT( XRCID("txtFileFilter"), CSSOpenSolutionFileDialog::OnFileFilterTextUpdate )
END_EVENT_TABLE()

wxIMPLEMENT_CLASS( CSSOpenSolutionFileDialog, wxDialog );

CSSOpenSolutionFileDialog::Cache CSSOpenSolutionFileDialog::s_cache;

CSSOpenSolutionFileDialog::CSSOpenSolutionFileDialog( wxWindow* parent )
:	wxDialog()
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("OpenSolutionFileDialog") );

	m_listCtrlsolutionFiles = XRCCTRL( *this, "listCtrlSolutionFiles", wxListCtrl );
	m_txtFileFilter			= XRCCTRL( *this, "txtFileFilter", wxTextCtrl );
	
	m_txtFileFilter->Bind( wxEVT_KEY_DOWN, &CSSOpenSolutionFileDialog::OnKeyDownFilter, this );
	m_listCtrlsolutionFiles->Bind( wxEVT_CHAR, &CSSOpenSolutionFileDialog::OnKeyDownList, this );
}

CSSOpenSolutionFileDialog::~CSSOpenSolutionFileDialog()
{

}

void CSSOpenSolutionFileDialog::OpenSolutionFile( long item )
{
	unsigned int entryIndex = static_cast< unsigned int >( m_listCtrlsolutionFiles->GetItemData( item ) );

	if ( entryIndex >= s_cache.entries.size() )
	{
		return;
	}

	COpenFileEvent* event = new COpenFileEvent( s_cache.entries[ entryIndex ].path, true );
	QueueEvent( event );

	Close();
}

bool CSSOpenSolutionFileDialog::OnKeyDownGeneric( wxKeyEvent& event )
{
	if( event.GetKeyCode() == WXK_ESCAPE )
	{
		Close();

		return true;
	}
	else if( event.GetKeyCode() == WXK_RETURN )
	{
		long item = m_listCtrlsolutionFiles->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

		OpenSolutionFile( item );

		return true;
	}

	event.Skip();

	return false;
}

void CSSOpenSolutionFileDialog::OnKeyDownFilter( wxKeyEvent& event )
{
	if( !OnKeyDownGeneric( event ) )
	{
		switch( event.GetKeyCode() )
		{
		case WXK_DOWN:
		case WXK_UP:
		case WXK_PAGEUP:
		case WXK_PAGEDOWN:
			if( event.GetEventObject() != m_listCtrlsolutionFiles )
			{
				m_listCtrlsolutionFiles->SetFocus();
			}
			break;
		}
	}
}

void CSSOpenSolutionFileDialog::OnKeyDownList( wxKeyEvent& event )
{
	if( !OnKeyDownGeneric( event ) )
	{
		switch( event.GetKeyCode() )
		{
		case WXK_LEFT:
		case WXK_RIGHT:
		case WXK_DELETE:
		case WXK_HOME:
		case WXK_END:
		case WXK_BACK:

			m_txtFileFilter->SetFocus();
			m_txtFileFilter->EmulateKeyPress( event.GetUnicodeKey() );

			break;

		case WXK_DOWN:
		case WXK_UP:
		case WXK_PAGEUP:
		case WXK_PAGEDOWN:
			// Do nothing
			break;

		default:
			m_txtFileFilter->WriteText( wxString( event.GetUnicodeKey() ) );
			m_txtFileFilter->SetFocus();
		}
	}
}

void CSSOpenSolutionFileDialog::OnShow( wxShowEvent& event )
{
	if( event.IsShown() )
	{
		FileFilterTextUpdate();

		m_txtFileFilter->SetFocus();
	}

	event.Skip();
}

void CSSOpenSolutionFileDialog::OnItemActivated( wxListEvent& event )
{
	OpenSolutionFile( event.m_itemIndex );
}

void CSSOpenSolutionFileDialog::OnFileFilterTextUpdate( wxCommandEvent& event )
{
	FileFilterTextUpdate();
}

void CSSOpenSolutionFileDialog::FileFilterTextUpdate()
{
	m_listCtrlsolutionFiles->Freeze();

	m_listCtrlsolutionFiles->ClearAll();

	m_listCtrlsolutionFiles->InsertColumn( 0, wxT("File name") );
	m_listCtrlsolutionFiles->InsertColumn( 1, wxT("Directory") );

	wxString filterText = m_txtFileFilter->GetValue();

	if( filterText.Length() > 0 )
	{
		filterText.MakeLower();

		int currentItemIndex = 0;

		for( unsigned int i = 0; i < s_cache.entries.size(); ++i )
		{
			const wxString& file = s_cache.files[ s_cache.entries[ i ].fileIndex ];

			if( file.Lower().find( filterText ) != wxString::npos )
			{
				m_listCtrlsolutionFiles->InsertItem( currentItemIndex, file );
				m_listCtrlsolutionFiles->SetItem( currentItemIndex, 1, s_cache.directories[ s_cache.entries[ i ].directoryIndex ] );
				m_listCtrlsolutionFiles->SetItemPtrData( currentItemIndex, i );

				++currentItemIndex;
			}
		}
	}
	else
	{
		for( unsigned int i = 0; i < s_cache.entries.size(); ++i )
		{
			const wxString& file = s_cache.files[ s_cache.entries[ i ].fileIndex ];

			m_listCtrlsolutionFiles->InsertItem( i, file );
			m_listCtrlsolutionFiles->SetItem( i, 1, s_cache.directories[ s_cache.entries[ i ].directoryIndex ] );
			m_listCtrlsolutionFiles->SetItemPtrData( i, i );
		}
	}

	m_listCtrlsolutionFiles->SetColumnWidth( 0, wxLIST_AUTOSIZE );
	m_listCtrlsolutionFiles->SetColumnWidth( 1, wxLIST_AUTOSIZE );
	m_listCtrlsolutionFiles->Thaw();

	long topItem = m_listCtrlsolutionFiles->GetTopItem();
	m_listCtrlsolutionFiles->SetItemState( topItem, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );

}

void CSSOpenSolutionFileDialog::BuildCache( const vector< SolutionFilePtr >& allSolutionFiles )
{
	// clear out the old data
	s_cache.files.Clear();
	s_cache.directories.Clear();
	s_cache.entries.clear();

	s_cache.files.reserve( allSolutionFiles.size() );

	vector< wxFileName > paths;
	paths.reserve( allSolutionFiles.size() );

	// Create file array
	for( unsigned int i = 0; i < allSolutionFiles.size(); ++i )
	{
		const SolutionFilePtr& solutionFile = allSolutionFiles[ i ];

		wxFileName path( solutionFile->m_solutionPath.c_str() );
		paths.push_back( path );

		wxString filename = path.GetFullName();
		wxString directory = path.GetPath();

		if( s_cache.files.Index( filename ) == wxNOT_FOUND )
		{
			s_cache.files.Add( filename );
		}

		if( s_cache.directories.Index( directory ) == wxNOT_FOUND )
		{
			s_cache.directories.Add( directory );
		}
	}

	// Create entries
	s_cache.entries.reserve( allSolutionFiles.size() );
	for( unsigned int i = 0; i < paths.size(); ++i )
	{
		Cache::Entry entry;

		wxString file = paths[ i ].GetFullName();
		wxString directory = paths[ i ].GetPath();

		entry.fileIndex = s_cache.files.Index( file );
		entry.directoryIndex = s_cache.directories.Index( directory );
		entry.path = paths[ i ].GetFullPath();

		s_cache.entries.push_back( entry );
	}
}
