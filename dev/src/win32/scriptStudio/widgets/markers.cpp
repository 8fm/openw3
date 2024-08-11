/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "markers.h"

#include "../app.h"

#include "../events/eventGoto.h"
#include "../events/eventmarkers.h"

#include "../solution/slnBase.h"
#include "../solution/slnContainer.h"

wxBEGIN_EVENT_TABLE( CSSMarkers, CSSCheckListCtrl )
	EVT_LIST_ITEM_ACTIVATED( wxID_ANY, CSSMarkers::OnItemActivated )
	EVT_LIST_COL_CLICK( wxID_ANY, CSSMarkers::OnColumnClick )
	EVT_LIST_KEY_DOWN( wxID_ANY, CSSMarkers::OnKeyDown )
wxEND_EVENT_TABLE()

wxIMPLEMENT_CLASS( CSSMarkers, CSSCheckListCtrl );

CSSMarkers::CSSMarkers( wxWindow* parent, Solution* solution )
:	CSSCheckListCtrl( parent, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES )
,	m_solution( solution )
{
	InsertColumn( Col_File, wxT( "File" ) );
	InsertColumn( Col_Line, wxT( "Line" ) );
}

CSSMarkers::~CSSMarkers()
{
	for( int i = m_entries.size() - 1; i >= 0; --i )
	{
		Remove( i );
	}
}

void CSSMarkers::Set( const SolutionFilePtr& file, unsigned int line, EMarkerState state )
{
	if( state == Marker_Deleted )
	{
		Remove( file, line );
	}
	else
	{
		unsigned int index = Find( file, line );

		int row = 0;

		EChecked icon = ( state == Marker_Enabled )? Checked_On : Checked_Off;

		if( index < m_entries.size() )
		{
			row = FindItem( -1, reinterpret_cast< wxUIntPtr >( m_entries[ index ] ) );
			SetItemImage( row, icon );
		}
		else
		{
			// Store breakpoint information
			Entry* entry = new Entry;
			entry->m_file = file;
			entry->m_lineNo = line;
			entry->m_state = state;

			m_entries.push_back( entry );

			index = m_entries.size() - 1;

			row = InsertItem( GetItemCount(), icon );

			// Add breakpoint information to widget
			wxString lineNo;
			lineNo << entry->m_lineNo;

			SetItem( row, Col_File, entry->m_file->m_solutionPath.c_str() );
			SetItem( row, Col_Line, lineNo );

			SetItemPtrData( row, reinterpret_cast< wxUIntPtr >( entry ) );

			SetColumnWidth( Col_Enabled, wxLIST_AUTOSIZE );
			SetColumnWidth( Col_File, wxLIST_AUTOSIZE );
			SetColumnWidth( Col_Line, wxLIST_AUTOSIZE );
		}
	}
}

void CSSMarkers::Move( const SolutionFilePtr& file, unsigned int afterLine, int moveBy )
{
	vector< CMarkerToggledEvent* > deleteEvents;
	vector< CMarkerToggledEvent* > addEvents;

	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		if( m_entries[ i ]->m_file == file )
		{
			if( m_entries[ i ]->m_lineNo >= afterLine )
			{
				// Remove old breakpoint
				CMarkerToggledEvent* deleteEvent = CreateToggleEvent( file, m_entries[ i ]->m_lineNo, Marker_Deleted );
				deleteEvents.push_back( deleteEvent );

				// Add new breakpoint
				CMarkerToggledEvent* addEvent = CreateToggleEvent( file, m_entries[ i ]->m_lineNo + moveBy, m_entries[ i ]->m_state );
				addEvents.push_back( addEvent );
			}
		}
	}

	RED_ASSERT( deleteEvents.size() == addEvents.size() );

	for( size_t i = 0; i < deleteEvents.size(); ++i )
	{
		QueueEvent( deleteEvents[ i ] );
	}

	for( size_t i = 0; i < addEvents.size(); ++i )
	{
		QueueEvent( addEvents[ i ] );
	}
}

void CSSMarkers::RestoreAll( const SolutionFilePtr& file )
{
	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		if( file == m_entries[ i ]->m_file )
		{
			CMarkerToggledEvent* event = CreateToggleEvent( m_entries[ i ]->m_file, m_entries[ i ]->m_lineNo, m_entries[ i ]->m_state );
			QueueEvent( event );
		}
	}
}

void CSSMarkers::Remove( const SolutionFilePtr& file, unsigned int line )
{
	unsigned int index = Find( file, line );

	Remove( index );
}

void CSSMarkers::Remove( unsigned int index )
{
	if( index < m_entries.size() )
	{
		int item = FindItem( -1, reinterpret_cast< wxUIntPtr >( m_entries[ index ] ) );

		// Delete widget row
		DeleteItem( item );

		// Delete entry
		delete m_entries[ index ];

		// Swap deleted entry for item at back of array
		m_entries[ index ] = m_entries[ m_entries.size() - 1 ];

		// Remove last item from array
		m_entries.pop_back();
	}
}

void CSSMarkers::RemoveAll( const SolutionFilePtr& file )
{
	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		if( m_entries[ i ]->m_file == file )
		{
			CMarkerToggledEvent* deleteEvent = CreateToggleEvent( file, m_entries[ i ]->m_lineNo, Marker_Deleted );
			QueueEvent( deleteEvent );
		}
	}
}

void CSSMarkers::RemoveAll()
{
	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		CMarkerToggledEvent* deleteEvent = CreateToggleEvent( m_entries[ i ]->m_file, m_entries[ i ]->m_lineNo, Marker_Deleted );
		QueueEvent( deleteEvent );
	}
}

unsigned int CSSMarkers::Find( const SolutionFilePtr& file, unsigned int line )
{
	Entry entry;
	entry.m_file = file;
	entry.m_lineNo = line;

	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		if( ( *m_entries[ i ] ) == entry )
		{
			return i;
		}
	}

	return m_entries.size();
}

void CSSMarkers::OnItemActivated( wxListEvent& event )
{
	Entry* entry = reinterpret_cast< Entry* >( GetItemData( event.GetIndex() ) );

	CGotoEvent* newEvent = new CGotoEvent( entry->m_file->m_solutionPath.c_str(), static_cast< Red::System::Int32 >( entry->m_lineNo ) );
	QueueEvent( newEvent );
}

void CSSMarkers::OnStateChange( int item, EChecked state )
{
	Entry* entry = reinterpret_cast< Entry* >( GetItemData( item ) );
	entry->m_state = ( state == Checked_On )? Marker_Enabled : Marker_Disabled;

	CMarkerToggledEvent* event = CreateToggleEvent( entry->m_file, entry->m_lineNo, entry->m_state );
	QueueEvent( event );
}

void CSSMarkers::OnColumnClick( wxListEvent& event )
{
	m_sortAsc[ event.GetColumn() ] = !m_sortAsc[ event.GetColumn() ];

	SortByColumn( static_cast< EColumn >( event.GetColumn() ) );
}

void CSSMarkers::OnKeyDown( wxListEvent& event )
{
	int widgetItemIndex = event.GetIndex();

	if( widgetItemIndex >= 0 )
	{
		if( event.GetKeyCode() == WXK_DELETE )
		{
			Entry* entry = reinterpret_cast< Entry* >( GetItemData( widgetItemIndex ) );

			CMarkerToggledEvent* deleteEvent = CreateToggleEvent( entry->m_file, entry->m_lineNo, Marker_Deleted );
			QueueEvent( deleteEvent );

			// Move selection to the next item in the list
			long nextItemIndex = GetNextItem( widgetItemIndex );

			if( nextItemIndex >= 0 )
			{
				SetItemState( nextItemIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
			}
		}
		else if( event.GetKeyCode() == WXK_RETURN )
		{
			OnItemActivated( event );
		}
	}

	event.Skip();
}

void CSSMarkers::SortByColumn( EColumn column )
{
	if( column == Col_Line || column == Col_File )
	{
		SortData data;
		data.column = column;
		data.sortAsc = m_sortAsc;

		SortItems( &CSSMarkers::SortPredicate, reinterpret_cast< wxIntPtr >( &data ) );
	}
}

int wxCALLBACK CSSMarkers::SortPredicate( wxIntPtr item1, wxIntPtr item2, wxIntPtr data )
{
	Entry* a = reinterpret_cast< Entry* >( item1 );
	Entry* b = reinterpret_cast< Entry* >( item2 );
	SortData* sortData = reinterpret_cast< SortData* >( data );

	int pathComp = a->m_file->m_solutionPath.compare( b->m_file->m_solutionPath );
	int lineComp = a->m_lineNo - b->m_lineNo;

	if( !sortData->sortAsc[ Col_File ] )
	{
		pathComp = -pathComp;
	}

	if( !sortData->sortAsc[ Col_Line ] )
	{
		lineComp = -lineComp;
	}

	if( pathComp == 0 )
	{
		return lineComp;
	}

	// Path comparisons are more significant, so we need to
	// make sure they're going to return a value larger than the highest possible line number
	const int pathValueModifier = 100000;

	if( pathComp < 0 )
	{
		return pathComp - pathValueModifier;
	}
	else
	{
		return pathComp + pathValueModifier;
	}
}

void CSSMarkers::LoadConfig( const wxConfigBase& config )
{
	int count = 0;
	config.Read( wxT( "count" ), &count, 0 );

	for( int i = 0; i < count; ++i )
	{
		wxString path;
		path.Printf( wxT( "Marker%d/" ), i );

		wxString filePath = config.Read( path + wxT( "FilePath" ) );
		SolutionFilePtr solFile = m_solution->FindFile( filePath.wc_str() );

		int lineNum;
		config.Read( path + wxT( "LineNum" ), &lineNum, 0 );

		int state;
		config.Read( path + wxT( "State" ), &state, 0 );

		if ( solFile )
		{
			CMarkerToggledEvent* event = CreateToggleEvent( solFile, lineNum, ( EMarkerState ) state );
			QueueEvent( event );
		}
	}
}

void CSSMarkers::SaveConfig( wxConfigBase& config ) const
{
	// Delete the existing entries in the config file
	wxString rootpath = config.GetPath();
	config.DeleteGroup( rootpath );

	// Since we've just deleted the path, we'll need to recreate it
	config.SetPath( rootpath );

	config.Write( wxT( "count" ), static_cast< int >( m_entries.size() ) );

	for ( unsigned int i = 0; i < m_entries.size(); ++i )
	{
		wxString path;
		path.Printf( wxT( "Marker%u" ), i );
		config.SetPath( path );

		Entry* entry = m_entries[ i ];
		config.Write( wxT( "FilePath" ), entry->m_file->m_solutionPath.c_str() );
		config.Write( wxT( "LineNum" ), entry->m_lineNo );
		config.Write( wxT( "State" ), (int)entry->m_state );

		config.SetPath( wxT( ".." ) );
	}

	// Flush
	config.Flush();
}
