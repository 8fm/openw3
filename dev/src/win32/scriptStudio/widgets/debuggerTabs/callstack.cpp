/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "callstack.h"

#include "wx/clipbrd.h"

#include "../../app.h"
#include "../../debuggerHelper.h"
#include "../../events/eventCallstackFrameSelected.h"


wxIMPLEMENT_CLASS( CSSCallstackDebuggerTab, CSSDebuggerTabBase );

CSSCallstackDebuggerTab::CSSCallstackDebuggerTab( wxAuiNotebook* parent )
:	CSSDebuggerTabBase( parent )
{
	wxBoxSizer* windowSizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( windowSizer );

	m_listView = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT );

	windowSizer->Add( m_listView, 1, wxEXPAND, 0 );
	
	// Create image list
	wxImageList* images = new wxImageList( 14, 14, true, Icon_Max );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_CALLSTACK_HEAD" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_CALLSTACK_LOCAL" ) ) );
	m_listView->SetImageList( images, wxIMAGE_LIST_SMALL );

	m_listView->Bind( wxEVT_CONTEXT_MENU, &CSSCallstackDebuggerTab::OnContextMenu, this );
	m_listView->Bind( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, &CSSCallstackDebuggerTab::OnItemActivated, this );
	m_listView->Bind( wxEVT_COMMAND_LIST_DELETE_ITEM, &CSSCallstackDebuggerTab::OnItemDeleted, this );
}

CSSCallstackDebuggerTab::~CSSCallstackDebuggerTab()
{

}

void CSSCallstackDebuggerTab::Connect( const wxChar* )
{

}

void CSSCallstackDebuggerTab::DebuggingStopped()
{
	m_listView->ClearAll();
	m_listView->Enable( false );
}

void CSSCallstackDebuggerTab::Refresh()
{

}

bool CSSCallstackDebuggerTab::Paste( const wxString& )
{
	return false;
}

void CSSCallstackDebuggerTab::OnContextMenu( wxContextMenuEvent& )
{
	wxMenu menu;
	menu.Append( emid_CopyAll, "Copy all" );
	menu.Append( emid_CopySelection, "Copy selection" );

	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSCallstackDebuggerTab::OnCopyAll, this, emid_CopyAll );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSCallstackDebuggerTab::OnCopySelection, this, emid_CopySelection );

	if( m_listView->GetSelectedItemCount() == 0 )
	{
		menu.Enable( emid_CopySelection, false );
	}

	PopupMenu( &menu );
}

void CSSCallstackDebuggerTab::CopyToClipboard()
{
	CopySelection();
}

void CSSCallstackDebuggerTab::OnCopyAll( const wxCommandEvent& )
{
	CopyAll();
}

void CSSCallstackDebuggerTab::OnCopySelection( const wxCommandEvent& )
{
	CopySelection();
}

void CSSCallstackDebuggerTab::CopyAll()
{
	int count = m_listView->GetItemCount();

	wxString value = Extract( 0, count );
	PushToClipboard( value );
}

void CSSCallstackDebuggerTab::CopySelection()
{
	int selectedCount = m_listView->GetSelectedItemCount();
	int firstItem = m_listView->GetFirstSelected();

	wxString value = Extract( firstItem, selectedCount );
	PushToClipboard( value );
}

wxString CSSCallstackDebuggerTab::Extract( int startItem, int count )
{
	wxString returnValue;

	for( int i = 0; i < count; ++i )
	{
		int item = startItem + i;

		if( i > 0 )
		{
			returnValue += wxT( "\n" );
		}

		returnValue += m_listView->GetItemText( item, Col_Location );
	}

	return returnValue;
}

void CSSCallstackDebuggerTab::PushToClipboard( const wxString& value )
{
	if ( !value.empty() && wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( value ) );
		wxTheClipboard->Close();
	}
}

wxString CSSCallstackDebuggerTab::FormatFrame( const SFrame& frame ) const
{
	wxString formattedText;

	formattedText = frame.file + wxT( "!" ) + frame.function + wxT( "(" );

	for( Red::System::Uint32 i = 0; i < frame.numParameters; ++i )
	{
		if( i > 0 )
		{
			formattedText += wxT( ", " );
		}

		const SProperty& property = frame.parameters[ i ];

		formattedText += property.name;
		formattedText += wxT( "=" );
		formattedText += property.value;
	}

	formattedText += wxString::Format( wxT( ") Line %u" ), frame.line );

	return formattedText;
}

wxString CSSCallstackDebuggerTab::FormatFrame( const SNativeFrame& frame ) const
{
	return frame.location + wxT( "!" ) + frame.symbol;
}

void CSSCallstackDebuggerTab::AppendFrame( const wxString& location, const wxColour& colour, const LocationInfo* info )
{
	int index = m_listView->InsertItem( m_listView->GetItemCount(), wxEmptyString, Icon_Invalid );
	m_listView->SetItem( index, Col_Location, location );
	m_listView->SetItemTextColour( index, colour );
	m_listView->SetItemColumnImage( index, Col_Icon, Icon_Invalid );
	m_listView->SetItemPtrData( index, reinterpret_cast< wxUIntPtr >( info ) );
}

void CSSCallstackDebuggerTab::OnCallstackEvent( CCallstackUpdateEvent& event )
{
	m_activeStackFrame = 0;

	m_listView->Freeze();
	m_listView->ClearAll();

	// Create list layout
	m_listView->InsertColumn( Col_Icon, wxEmptyString, wxLIST_FORMAT_CENTER, 20 );
	m_listView->InsertColumn( Col_Location, wxT( "Location" ), wxLIST_FORMAT_LEFT, GetClientSize().x - 20 );

	const wxColour frameColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
	const Red::System::Uint32 numFrames = event.GetNumberOfFrames();
	const SFrame* frames = event.GetFrames();

	for( Red::System::Uint32 i = 0; i < numFrames; ++i )
	{
		wxString entry = FormatFrame( frames[ i ] );

		LocationInfo* info = new LocationInfo;
		info->m_file = frames[ i ].file;
		info->m_line = frames[ i ].line;

		AppendFrame( entry, frameColour, info );
	}

	const Red::System::Uint32 numNativeFrames = event.GetNumberOfNativeFrames();
	if( numNativeFrames > 0 )
	{
		const wxColour nativeFrameColour( 128, 128, 128 );

		AppendFrame( wxT( "[Frames below contains callstack from native code]" ), nativeFrameColour );

		const SNativeFrame* nativeFrames = event.GetNativeFrames();
		for ( Red::System::Uint32 i = 0; i < numNativeFrames; ++i )
		{
			wxString item = FormatFrame( nativeFrames[ i ] );
			AppendFrame( item, nativeFrameColour );
		}
	}

	if( numFrames > 0 )
	{
		// Frame 0 always has the "head" icon, select it as the entry point
		m_listView->SetItemColumnImage( 0, Col_Icon, Icon_Head );

		ActivateStackFrame( 0 );
	}

	m_listView->Enable( true );

	// Refresh
	m_listView->Thaw();
	m_listView->Refresh();
}

void CSSCallstackDebuggerTab::ActivateStackFrame( int newActiveStackFrame )
{
	int oldActiveStackFrame = m_activeStackFrame;
	m_activeStackFrame = newActiveStackFrame;

	// Don't erase the head icon
	if( oldActiveStackFrame > 0 )
	{
		m_listView->SetItemColumnImage( oldActiveStackFrame, Col_Icon, Icon_Invalid );
	}

	if( newActiveStackFrame > 0 )
	{
		m_listView->SetItemColumnImage( newActiveStackFrame, Col_Icon, Icon_Local );
	}

	const LocationInfo* info = reinterpret_cast< const LocationInfo* >( m_listView->GetItemData( newActiveStackFrame ) );

	if( info )
	{
		CCallstackFrameSelectedEvent* callstackEvent = new CCallstackFrameSelectedEvent( newActiveStackFrame, info->m_file, info->m_line );
		QueueEvent( callstackEvent );
	}
}

void CSSCallstackDebuggerTab::OnItemActivated( wxListEvent& event )
{
	ActivateStackFrame( event.GetIndex() );
}

void CSSCallstackDebuggerTab::OnItemDeleted( wxListEvent& event )
{
	const LocationInfo* info = reinterpret_cast< const LocationInfo* >( m_listView->GetItemData( event.GetIndex() ) );

	delete info;

	m_listView->SetItemPtrData( event.GetIndex(), 0 );
}
