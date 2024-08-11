/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "navigationHistory.h"

#include "events/eventGoto.h"

wxIMPLEMENT_CLASS( CSSNavigationHistory, wxListCtrl );

CSSNavigationHistory::CSSNavigationHistory( wxWindow* parent )
:	wxListCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES )
,	m_selectedItem( -1 )
{
	InsertColumn( Col_File, wxT( "File" ) );
	InsertColumn( Col_Line, wxT( "Line" ) );
	InsertColumn( Col_Snippet, wxT( "Context" ) );

	Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CSSNavigationHistory::OnItemActivated, this );
}

CSSNavigationHistory::~CSSNavigationHistory()
{

}

void CSSNavigationHistory::OnAdd( CNavigationHistoryEvent& event )
{
	// Delete any history that comes after the previously selected historical event
	long itemToDelete = GetNextItem( m_selectedItem );
	while( itemToDelete != -1 )
	{
		long nextItemToDelete = GetNextItem( m_selectedItem );
		DeleteItem( itemToDelete );
		itemToDelete = nextItemToDelete;
	}

	// Add the new historical event
	long row = InsertItem( GetItemCount(), event.GetFile() );

	wxString lineNo;
	lineNo << event.GetLine();

	SetItem( row, Col_Line, lineNo );
	SetItem( row, Col_Snippet, event.GetSnippet() );

	SetColumnWidth( Col_File, wxLIST_AUTOSIZE );
	SetColumnWidth( Col_Line, wxLIST_AUTOSIZE );
	SetColumnWidth( Col_Snippet, wxLIST_AUTOSIZE );

	m_selectedItem = row;
}

void CSSNavigationHistory::OnItemActivated( wxListEvent& event )
{
	long row = event.GetIndex();

	Goto( row );

	m_selectedItem = row;
}

void CSSNavigationHistory::GotoPrevious()
{
	long row = GetNextItem( m_selectedItem, wxLIST_NEXT_ABOVE );

	if( row != -1 )
	{
		Goto( row );
		m_selectedItem = row;
	}
}

void CSSNavigationHistory::GotoNext()
{
	long row = GetNextItem( m_selectedItem, wxLIST_NEXT_BELOW );

	if( row != -1 )
	{
		Goto( row );
		m_selectedItem = row;
	}
}

bool CSSNavigationHistory::CanGotoPrevious() const
{
	return GetNextItem( m_selectedItem, wxLIST_NEXT_ABOVE ) != -1;
}

bool CSSNavigationHistory::CanGotoNext() const
{
	return GetNextItem( m_selectedItem, wxLIST_NEXT_BELOW ) != -1;
}

void CSSNavigationHistory::Goto( long item )
{
	wxString file = GetItemText( item, Col_File );
	wxString line = GetItemText( item, Col_Line );

	long lineNumber;
	if( line.ToLong( &lineNumber ) )
	{
		CGotoEvent* newEvent = new CGotoEvent( file, lineNumber );
		QueueEvent( newEvent );
	}
}
