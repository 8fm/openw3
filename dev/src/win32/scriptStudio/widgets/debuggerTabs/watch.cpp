/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "watch.h"

#include "../../events/eventCallstackFrameSelected.h"
#include "../../app.h"
#include "wx/clipbrd.h"

wxIMPLEMENT_CLASS( CSSWatchDebuggerTab, CSSVariablesTabBase );

CSSWatchDebuggerTab::CSSWatchDebuggerTab( wxAuiNotebook* parent )
:	CSSVariablesTabBase( parent )
{
	m_tree->Bind( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, &CSSWatchDebuggerTab::OnStartLabelEdit, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_END_LABEL_EDIT, &CSSWatchDebuggerTab::OnEndLabelEdit, this );
	m_tree->Bind( wxEVT_RIGHT_UP, &CSSWatchDebuggerTab::OnRightClick, this );

	m_tree->Bind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CSSWatchDebuggerTab::OnTreeDrag, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_END_DRAG, &CSSWatchDebuggerTab::OnTreeDrop, this );

	Bind( wxEVT_COMMAND_TEXT_COPY, &CSSWatchDebuggerTab::OnCopy, this );
	Bind( wxEVT_COMMAND_TEXT_PASTE, &CSSWatchDebuggerTab::OnPaste, this );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CSSWatchDebuggerTab::OnMenuItemSelected, this );

	CSSWatchDebuggerDropTarget* target = new CSSWatchDebuggerDropTarget
	(
		[ this ]( const wxString& droppedText )
		{
			wxString path = droppedText;
			wxTreeItemId item = CreateWatch( path );

			// Don't "move" the text from the editor window, we just want a copy
			return false;
		}
	);

	m_tree->SetDropTarget( target );

	wxBitmap cursorBitmap = wxTheSSApp->LoadBitmap( wxT( "IMG_CURSOR_DRAGCOPY" ) );
	wxImage cursorImage = cursorBitmap.ConvertToImage();
	m_dragCursor = new wxCursor( cursorImage );
}

CSSWatchDebuggerTab::~CSSWatchDebuggerTab()
{
	m_tree->SetCursor( wxNullCursor );
	delete m_dragCursor;

	m_tree->SetDropTarget( nullptr );
}

Red::System::Uint32 CSSWatchDebuggerTab::GetItemExpansionStamp()
{
	return WATCH_EXPANSION_STAMP;
}

void CSSWatchDebuggerTab::Refresh()
{
	wxTreeItemIdValue cookie;
	wxTreeItemId root = m_tree->GetRootItem();
	wxTreeItemId watchItem = m_tree->GetFirstChild( root, cookie );

	while( watchItem.IsOk() )
	{
		RefreshItem( watchItem );

		watchItem = m_tree->GetNextChild( root, cookie );
	}
}

void CSSWatchDebuggerTab::OnStackFrameSelected( CCallstackFrameSelectedEvent& event )
{
	m_currentStackFrameIndex = event.GetFrame();

	Refresh();

	event.Skip();
}

void CSSWatchDebuggerTab::OnStartLabelEdit( wxTreeEvent& event )
{
	event.Skip();
}

void CSSWatchDebuggerTab::OnEndLabelEdit( wxTreeEvent& event )
{
	if( event.GetInt() == Col_Name )
	{
		wxString path = event.GetLabel();
		path.Trim();

		if( !( path.IsEmpty() ) )
		{
			wxTreeItemId item = event.GetItem();

			CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( item ) );

			// Only refresh if it's changed
			if( data->path != path )
			{
				data->path = path;

				RefreshItem( item );
			}
		}
	}
	else
	{
		event.Skip();
	}
}

void CSSWatchDebuggerTab::OnUserCreateNewItem()
{
	wxString path = wxEmptyString;
	wxTreeItemId childItem = CreateWatch( path, false );
	m_tree->EditLabel( childItem );
}

void CSSWatchDebuggerTab::OnUserStartEditItemName( const wxTreeItemId& item )
{
	wxTreeItemId root = m_tree->GetRootItem();

	if( m_tree->GetItemParent( item ) == root )
	{
		m_tree->EditLabel( item );
	}
}

wxTreeItemId CSSWatchDebuggerTab::CreateWatch( wxString& path, bool refresh )
{
	wxTreeItemId root = m_tree->GetRootItem();
	wxTreeItemId item = ForceCreateNewItem( root, path, path );

	m_tree->SetItemTextColour( item, UNVERIFIED_COLOUR );

	if( refresh )
	{
		RefreshItem( item );
	}

	return item;
}

bool CSSWatchDebuggerTab::Paste( const wxString& text )
{
	if( !m_tree->InsertIntoEditLabel( text ) )
	{
		wxString path = text;
		wxTreeItemId item = CreateWatch( path );

		m_tree->ScrollTo( item );
	}

	return true;
}

void CSSWatchDebuggerTab::OnPaste( wxClipboardTextEvent& )
{
	if ( wxTheClipboard->Open() )
	{
		wxTextDataObject data;
		if( wxTheClipboard->GetData( data ) )
		{
			Paste( data.GetText() );
		}
		wxTheClipboard->Close();
	}
}

void CSSWatchDebuggerTab::OnCopy( wxClipboardTextEvent& )
{
	CopyToClipboard();
}

void CSSWatchDebuggerTab::OnKeyDown( wxTreeEvent& event )  
{
	if( event.GetKeyCode() == WXK_DELETE )
	{
		wxTreeItemId selectedItem = m_tree->GetSelection();

		wxTreeItemId rootItem = m_tree->GetRootItem();

		wxTreeItemId parentItem = m_tree->GetItemParent( selectedItem );

		if( parentItem == rootItem )
		{
			m_tree->Delete( selectedItem );

			return;
		}
	}

	CSSVariablesTabBase::OnKeyDown( event );
}

void CSSWatchDebuggerTab::OnRightClick( wxMouseEvent& event )
{
	wxMenu menu;

	menu.Append( wxID_NEW, wxT( "Add Watch" ) );

	if( m_tree->GetSelection().IsOk() )
	{
		menu.Append( wxID_REFRESH );
		menu.AppendSeparator();
		menu.Append( wxID_COPY );
	}

	if ( wxTheClipboard->Open() )
	{
		if( wxTheClipboard->IsSupported( wxDataFormat( wxDF_TEXT ) ) )
		{
			menu.Append( wxID_PASTE );
		}

		wxTheClipboard->Close();
	}

	PopupMenu( &menu );
}

void CSSWatchDebuggerTab::OnMenuItemSelected( wxCommandEvent& event )
{
	switch( event.GetId() )
	{
	case wxID_NEW:
		{
			wxString path = wxEmptyString;
			wxTreeItemId childItem = CreateWatch( path, false );
			m_tree->EditLabel( childItem );
		}
		break;

	case wxID_REFRESH:
		RefreshItem( m_tree->GetSelection() );
		break;

	case wxID_COPY:
		CopyToClipboard();
		break;

	case wxID_PASTE:
		{
			wxClipboardTextEvent dummy;
			OnPaste( dummy );
		}
		break;
	}
}

void CSSWatchDebuggerTab::OnTreeDrag( wxTreeEvent& event )
{
	m_draggedItem = event.GetItem();

	if( m_draggedItem.IsOk() )
	{
		event.Allow();
		m_tree->SetCursor( *m_dragCursor );
	}
}

void CSSWatchDebuggerTab::OnTreeDrop( wxTreeEvent& event )
{
	int x = event.GetPoint().x;
	int y = event.GetPoint().y;

	if( x > 0 && y > 0 && x < GetClientSize().GetX() && y < GetClientSize().GetY() )
	{
		if( m_draggedItem.IsOk() )
		{
			CSSVariablesTabItemData* existingData = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( m_draggedItem ) );

			wxTreeItemId newItem = CreateWatch( existingData->path );
			m_tree->ScrollTo( newItem );
		}
	}

	m_draggedItem.Unset();

	m_tree->SetCursor( wxNullCursor );
}

CSSWatchDebuggerTab::CSSWatchDebuggerDropTarget::CSSWatchDebuggerDropTarget( TCallback callback )
:	m_callback( callback )
{

}

CSSWatchDebuggerTab::CSSWatchDebuggerDropTarget::~CSSWatchDebuggerDropTarget()
{

}

bool CSSWatchDebuggerTab::CSSWatchDebuggerDropTarget::OnDropText( wxCoord, wxCoord, const wxString& data )
{
	return m_callback( data );
}
