/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "variables.h"

#include "../../app.h"

#include <wx/clipbrd.h>

Red::System::Uint32 CSSVariablesTabBase::CSSVariablesTabItemData::idPool = 0;

const wxColour CSSVariablesTabBase::UNVERIFIED_COLOUR( 166, 174, 180 );
const wxColour CSSVariablesTabBase::MODIFIED_COLOUR( 207, 0, 0 );
const wxColour CSSVariablesTabBase::NORMAL_COLOUR( 0, 0, 0 );
const wxColour CSSVariablesTabBase::MODIFYACK_FAILED_COLOUR( 199, 120, 10 );

wxIMPLEMENT_CLASS( CSSVariablesTabBase, CSSDebuggerTabBase );

CSSVariablesTabBase::CSSVariablesTabBase( wxAuiNotebook* parent )
:	CSSDebuggerTabBase( parent )
,	m_searchSkip( 0 )
,	m_searchSkipTarget( 0 )
{
	wxBoxSizer* windowSizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( windowSizer );

	m_tree = new wxcode::wxTreeListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HIDE_ROOT | wxTR_COLUMN_LINES | wxTR_HAS_BUTTONS | wxTR_FULL_ROW_HIGHLIGHT );
	windowSizer->Add( m_tree, 1, wxEXPAND, 0 );

	m_quickSearchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxTE_RICH );
	windowSizer->Add( m_quickSearchCtrl, 0, wxEXPAND, 0 );

	m_quickSearchCtrl->Hide();

	// Create image list
	wxImageList* images = new wxImageList( 16, 16, true, 6 );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_MEMBER_PUBLIC" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_MEMBER_PRIVATE" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_OBJECT" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_STATEMACHINE" ) ) );
	images->Add( wxTheSSApp->LoadBitmap( wxT( "IMG_TREE_STATE" ) ) );
	m_tree->SetImageList( images );

	m_tree->Bind( wxEVT_COMMAND_TREE_ITEM_EXPANDING, &CSSVariablesTabBase::OnItemExpand, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &CSSVariablesTabBase::OnItemCollapse, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_KEY_DOWN, &CSSVariablesTabBase::OnKeyDown, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_DELETE_ITEM, &CSSVariablesTabBase::OnItemDeleted, this );
	m_tree->Bind( wxEVT_LEFT_DCLICK, &CSSVariablesTabBase::OnDoubleClick, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, &CSSVariablesTabBase::OnStartLabelEdit, this );
	m_tree->Bind( wxEVT_COMMAND_TREE_END_LABEL_EDIT, &CSSVariablesTabBase::OnEndLabelEdit, this );

	m_tree->Bind( wxEVT_SIZE, &CSSVariablesTabBase::OnSizeEvent, this );

	m_quickSearchCtrl->Bind( wxEVT_KEY_DOWN, &CSSVariablesTabBase::OnQuickSearchKeyDown, this );
	m_quickSearchCtrl->Bind( wxEVT_COMMAND_TEXT_UPDATED, &CSSVariablesTabBase::OnQuickSearchTextChanged, this );

	m_helper.Bind( ssEVT_LOCALS_EVENT, &CSSVariablesTabBase::OnLocalsEvent, this );
	m_helper.Bind( ssEVT_LOCALS_MODIFY_ACK_EVENT, &CSSVariablesTabBase::OnLocalsModifyAckEvent, this );

	m_tree->AddColumn( wxT( "Name" ) );
	m_tree->AddColumn( wxT( "Value" ) );

	// Create root
	wxTreeItemId root = m_tree->AddRoot( wxEmptyString );

	CSSVariablesTabItemData* data = new CSSVariablesTabItemData();
	data->path = wxEmptyString;
	m_tree->SetItemData( root, data );
	m_idsToItems[ data->id ] = root;
}

CSSVariablesTabBase::~CSSVariablesTabBase()
{
	m_tree->DeleteRoot();
}

void CSSVariablesTabBase::Connect( const wxChar* ip )
{
	m_helper.Initialize( ip );
}

void CSSVariablesTabBase::CopyToClipboard()
{
	wxString currentEditLabelTextSelection = m_tree->GetEditLabelSelection();

	if( !currentEditLabelTextSelection.IsEmpty() )
	{
		// Just copy whatever is selected in the edit label
		CopyToClipboard( currentEditLabelTextSelection );
	}
	else
	{
		// Grab the contents of the highlighted row and shove that into the clipboard
		const auto selectedItem = m_tree->GetSelection();
		if ( selectedItem == nullptr )
		{
			return;
		}

		const int columnCount = m_tree->GetColumnCount();
		wxString selectedRowText;
		for ( int column = 0; column < columnCount; ++column )
		{
			wxString columnText = m_tree->GetItemText( selectedItem, column );
			selectedRowText += columnText;
			selectedRowText += "\t";
		}

		selectedRowText.Trim();

		if ( !selectedRowText.empty() )
		{
			CopyToClipboard( selectedRowText );
		}
	}
}

void CSSVariablesTabBase::CopyToClipboard( const wxString& text )
{
	RED_FATAL_ASSERT( !text.empty(), "Can't copy empty text to the clipboard" );

	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( text ) );
		wxTheClipboard->Close();
	}
}

void CSSVariablesTabBase::DebuggingStopped()
{
}

void CSSVariablesTabBase::Clear()
{
	m_stamp = INVALID_STAMP;

	m_tree->Freeze();
	m_tree->DeleteRoot();
	m_tree->Thaw();
	m_tree->Refresh();
}

bool CSSVariablesTabBase::HasFocus() const
{
	return CSSDebuggerTabBase::HasFocus() || m_tree->HasFocus();
}

//////////////////////////////////////////////////////////////////////////

wxTreeItemId CSSVariablesTabBase::Search( const wxString& searchTerm ) const
{
	wxTreeItemId root = m_tree->GetRootItem();

	if( root.IsOk() )
	{
		if( ItemContainsSearchTerm( root, searchTerm ) )
		{
			return root;
		}

		wxTreeItemId item = Search( root, searchTerm );

		return item;
	}

	return wxTreeItemId();
}

wxTreeItemId CSSVariablesTabBase::Search( const wxTreeItemId& startingItem, const wxString& searchTerm ) const
{
	RED_FATAL_ASSERT( startingItem.IsOk(), "Invalid starting item specified for locals search" );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = m_tree->GetFirstChild( startingItem, cookie );

	if( child.IsOk() )
	{
		if( ItemContainsSearchTerm( child, searchTerm ) )
		{
			return child;
		}
		else
		{
			wxTreeItemId grandChild = Search( child, searchTerm );

			if( grandChild.IsOk() )
			{
				return grandChild;
			}
		}
	}

	wxTreeItemId sibling = m_tree->GetNextSibling( startingItem );

	if( sibling.IsOk() )
	{
		if( ItemContainsSearchTerm( sibling, searchTerm ) )
		{
			return sibling;
		}
		else
		{
			return Search( sibling, searchTerm );
		}
	}

	return wxTreeItemId();
}

bool CSSVariablesTabBase::ItemContainsSearchTerm( const wxTreeItemId& item, const wxString& searchTerm ) const
{
	wxString name = m_tree->GetItemText( item, 0 );
	name.MakeUpper();

	if( name.Find( searchTerm.Upper() ) != wxNOT_FOUND )
	{
		++m_searchSkip;

		if( m_searchSkip == m_searchSkipTarget )
		{
			return true;
		}
	}

	return false;
}

void CSSVariablesTabBase::SelectAndEnsureVisible( const wxTreeItemId& item )
{
	m_tree->EnsureVisible( item );
	m_tree->SelectItem( item );
	m_tree->SetCurrentItem( item );
}

int CSSVariablesTabBase::CalculatePage() const
{
	wxTreeItemId selected = m_tree->GetSelection();

	int height = m_tree->GetItemHeight( selected );
	wxSize size = m_tree->GetClientSize();

	return size.GetY() / height;
}

void CSSVariablesTabBase::MoveSelection( int numItems, bool up /*= false */ )
{
	wxTreeItemId (wxcode::wxTreeListCtrl::*direction)( const wxTreeItemId& ) const = ( up )? &wxcode::wxTreeListCtrl::GetPrev : &wxcode::wxTreeListCtrl::GetNext;

	wxTreeItemId selected = m_tree->GetSelection();
	for( int i = 0; i < numItems; ++i )
	{
		wxTreeItemId item = (m_tree->*direction)( selected );

		if( item.IsOk() )
		{
			selected = item;
		}
		else
		{
			break;
		}
	}

	SelectAndEnsureVisible( selected );
}

void CSSVariablesTabBase::OnSizeEvent( wxSizeEvent& event )
{
	// Setup columns
	if ( m_tree->GetColumnCount() == 2 )
	{
		int scrollbarWidth = wxSystemSettings::GetMetric( wxSYS_HSCROLL_Y );
		m_tree->SetColumnWidth( 1, ( m_tree->GetClientSize().x - m_tree->GetColumnWidth( 0 ) ) - scrollbarWidth );
	}

	event.Skip();
}

void CSSVariablesTabBase::OnKeyDown( wxTreeEvent& event )
{
	if( event.GetKeyCode() == WXK_ESCAPE )
	{
		m_quickSearchCtrl->Hide();
		m_quickSearchCtrl->Clear();
	}
	else
	{
		m_quickSearchCtrl->Show();

		switch( event.GetKeyCode() )
		{
		case WXK_LEFT:
		case WXK_RIGHT:
		case WXK_DELETE:
		case WXK_HOME:
		case WXK_END:
		case WXK_BACK:

			m_quickSearchCtrl->SetFocus();
			m_quickSearchCtrl->EmulateKeyPress( event.GetKeyEvent().GetUnicodeKey() );

			break;

		case WXK_UP:
			MoveSelection( 1, true );
			break;

		case WXK_DOWN:
			MoveSelection( 1 );
			break;

		case WXK_PAGEUP:
			{
				int pageDistance = CalculatePage();
				MoveSelection( pageDistance, true );
			}
			break;

		case WXK_PAGEDOWN:
			{
				int pageDistance = CalculatePage();
				MoveSelection( pageDistance );
			}
			break;

		default:
			m_quickSearchCtrl->WriteText( wxString( event.GetKeyEvent().GetUnicodeKey() ) );
			m_quickSearchCtrl->SetFocus();
		}
	}

	Layout();
}

void CSSVariablesTabBase::OnQuickSearchKeyDown( wxKeyEvent& event )
{
	event.Skip();

	switch( event.GetKeyCode() )
	{
	case WXK_ESCAPE:
		m_quickSearchCtrl->Hide();
		m_quickSearchCtrl->Clear();

		Layout();

		m_tree->SetFocus();
		break;

	case WXK_RETURN:
		{
			wxTreeItemId searchItem = m_tree->GetSelection();
			wxTreeItemId resultItem;

			wxString searchTerm = m_quickSearchCtrl->GetValue();
			if( searchTerm == m_previousSearchTerm )
			{
				++m_searchSkipTarget;
			}
			else
			{
				m_searchSkipTarget = 1;
				m_previousSearchTerm = searchTerm;
			}

			m_searchSkip = 0;

			if( searchItem.IsOk() )
			{
				resultItem = Search( searchItem, searchTerm );

				if( resultItem.IsOk() )
				{
					SelectAndEnsureVisible( resultItem );
					m_searchSkipTarget = 0;
				}
			}

			// If the search was unsuccessful, wrap around to the top again
			if( !resultItem.IsOk() )
			{
				resultItem = Search( m_quickSearchCtrl->GetValue() );

				if( resultItem.IsOk() )
				{
					SelectAndEnsureVisible( resultItem );
					m_searchSkipTarget = 0;
				}
			}
		}
		break;

	case WXK_UP:
		MoveSelection( 1, true );
		event.Skip( false );
		break;

	case WXK_DOWN:
		MoveSelection( 1 );
		event.Skip( false );
		break;

	case WXK_PAGEUP:
		{
			int pageDistance = CalculatePage();
			MoveSelection( pageDistance, true );
		}
		break;

	case WXK_PAGEDOWN:
		{
			int pageDistance = CalculatePage();
			MoveSelection( pageDistance );
		}
		break;
	}
}

void CSSVariablesTabBase::OnQuickSearchTextChanged( wxCommandEvent& event )
{
	wxTreeItemId item = Search( event.GetString() );

	if( item.IsOk() )
	{
		SelectAndEnsureVisible( item );
	}

	event.Skip();
}

wxTreeItemId CSSVariablesTabBase::CreateNewItem( wxString& path )
{
	wxTreeItemId root = m_tree->GetRootItem();
	return CreateNewItem( root, path );
}

wxTreeItemId CSSVariablesTabBase::CreateNewItem( const wxTreeItemId& parent, wxString& path )
{
	return CreateNewItem( parent, path, path );
}

wxTreeItemId CSSVariablesTabBase::CreateNewItem( const wxTreeItemId& parent, wxString& path, const wxString& childName )
{
	path.Trim( true );
	path.Trim( false );

	CSSVariablesTabItemData* parentData = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( parent ) );
	RED_FATAL_ASSERT( parentData, "Parent should already have had data created" );
	
	TPathToItemMap& childMap = m_childrenMap[ parentData->id ];
	TPathToItemMap::iterator iter = childMap.find( path );
	if( iter != childMap.end() )
	{
		return iter->second;
	}
	else
	{
		return ForceCreateNewItem( parent, path, childName );
	}
}

wxTreeItemId CSSVariablesTabBase::ForceCreateNewItem( const wxTreeItemId& parent, wxString& path, const wxString& childName )
{
	path.Trim( true );
	path.Trim( false );

	wxTreeItemId childItem = m_tree->AppendItem( parent, childName );
	CSSVariablesTabItemData* childData = new CSSVariablesTabItemData();
	childData->path = path;
	m_tree->SetItemData( childItem, childData );
	m_idsToItems[ childData->id ] = childItem;

	CSSVariablesTabItemData* parentData = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( parent ) );
	RED_FATAL_ASSERT( parentData, "Parent should already have had data created" );

	TPathToItemMap& childMap = m_childrenMap[ parentData->id ];
	childMap[ path ] = childItem;

	m_tree->Refresh( true );
	m_tree->Update();

	return childItem;
}

void CSSVariablesTabBase::OnItemDeleted( wxTreeEvent& event )
{
	CSSVariablesTabItemData* childData = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( event.GetItem() ) );

	if( childData )
	{
		wxTreeItemId parent = m_tree->GetItemParent( event.GetItem() );
		CSSVariablesTabItemData* parentData = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( parent ) );

		if( parentData )
		{
			TParentIdToChildPaths::iterator iter = m_childrenMap.find( parentData->id );
			if( iter != m_childrenMap.end() )
			{
				iter->second.erase( childData->path );
			}
		}

		m_childrenMap.erase( childData->id );
		m_idsToItems.erase( childData->id );

		m_tree->SetItemData( event.GetItem(), nullptr );

		delete childData;
	}
}

void CSSVariablesTabBase::GetItemChildren( const wxTreeItemId& parent, std::set< wxTreeItemId >& children ) const
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = m_tree->GetFirstChild( parent, cookie );

	while( child.IsOk() )
	{
		children.insert( child );
		child = m_tree->GetNextChild( parent, cookie );
	}
}

void CSSVariablesTabBase::SetItem( const wxTreeItemId& item, const SDebugValue& property )
{
	wxString previousValue = m_tree->GetItemText( item, Col_Value );

	if( !previousValue.IsEmpty() && previousValue != property.value )
	{
		m_tree->SetItemTextColour( item, MODIFIED_COLOUR );
	}
	else
	{
		m_tree->SetItemTextColour( item, NORMAL_COLOUR );
	}

	m_tree->SetItemText( item, Col_Value, property.value );
	m_tree->SetItemImage( item, Col_Name, property.icon );
}

void CSSVariablesTabBase::AddMembersToVar( const wxTreeItemId& parent, Red::System::Uint32 numChildren, const SDebugValueChild* children, const wxString& parentPath, std::set< wxTreeItemId >& untouchedChildren )
{
	m_tree->SetItemHasChildren( parent, numChildren > 0 );

	for( Red::System::Uint32 i = 0; i < numChildren; ++i )
	{
		const SDebugValueChild& child = children[ i ];

		wxString childPath = ( parentPath.IsEmpty() ) ? child.name : parentPath + wxT( "." ) + child.name;

		wxTreeItemId childItem = CreateNewItem( parent, childPath, child.name );

		SetItem( childItem, child );
		m_tree->SetItemHasChildren( childItem, child.isExpandable );

		CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( childItem ) );
		data->modifiable = child.isModifiable;

		// Since this child has been altered, remove it from the list (if it exists)
		untouchedChildren.erase( childItem );

		if( m_tree->IsExpanded( childItem ) )
		{
			// Request an update for the child too
			RefreshItem( childItem );
		}
	}
}

void CSSVariablesTabBase::OnLocalsEvent( CLocalsEvent& event )
{
	// Ignore any messages if the stack frame is out of date
	if( event.GetStamp() == m_stamp && event.GetStackFrameIndex() == m_currentStackFrameIndex )
	{
		const wxString path = event.GetPath();
		const wxTreeItemId parent = m_idsToItems[ event.GetItemId() ];

		m_tree->Freeze();

		const SDebugValue* parentProperty = event.GetProperty();
		SetItem( parent, *parentProperty );

		Red::System::Uint32 numChildren = event.GetNumberOfChildren();
		const SDebugValueChild* children = event.GetChildren();

		std::set< wxTreeItemId > untouchedChildren;
		GetItemChildren( parent, untouchedChildren );

		AddMembersToVar( parent, numChildren, children, path, untouchedChildren );

		// Anything left in the list must not exist in the current "state" (stack level, etc) and should be removed
		for( wxTreeItemId untouchedChild: untouchedChildren )
		{
			m_tree->Delete( untouchedChild );
		}

		int scrollbarWidth = wxSystemSettings::GetMetric( wxSYS_HSCROLL_Y );
		m_tree->SetColumnWidth( 1, ( m_tree->GetClientSize().x - m_tree->GetColumnWidth( 0 ) ) - scrollbarWidth );

		m_tree->Thaw();
		m_tree->Refresh();
	}
}

void CSSVariablesTabBase::RefreshItem( const wxTreeItemId& item )
{
	m_tree->SetItemTextColour( item, UNVERIFIED_COLOUR );

	CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( item ) );

	// Retrieve information about the path from the editor
	m_stamp = GetItemExpansionStamp();
	m_helper.RequestLocals( m_currentStackFrameIndex, m_stamp, data->path, data->id );
}

void CSSVariablesTabBase::OnItemExpand( wxTreeEvent& event )
{
	wxTreeItemId expandedItem = event.GetItem();

	RefreshItem( expandedItem );
}

void CSSVariablesTabBase::OnItemCollapse( wxTreeEvent& event )
{
	// Delete child items when collapsing stuff
	wxTreeItemId expandedItem = event.GetItem();
	CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( expandedItem ) );

	if ( expandedItem.IsOk() && expandedItem != m_tree->GetRootItem() )
	{
		m_tree->DeleteChildren( expandedItem );
	}
}

void CSSVariablesTabBase::OnDoubleClick( wxMouseEvent& event )
{
	int flags;
	wxTreeItemId item = m_tree->HitTest( event.GetPosition(), flags );

	if( flags & wxTREE_HITTEST_NOWHERE )
	{
		OnUserCreateNewItem();
	}
	else if( flags & wxTREE_HITTEST_ONITEM )
	{
		OnUserStartEditItemName( item );
	}
	else if( flags & wxcode::wxTREE_HITTEST_ONITEMCOLUMN )
	{
		OnUserStartEditItemValue( item );
	}
	else
	{
		event.Skip();
	}
}

void CSSVariablesTabBase::OnUserCreateNewItem()
{

}

void CSSVariablesTabBase::OnUserStartEditItemName( const wxTreeItemId& )
{

}

void CSSVariablesTabBase::OnUserStartEditItemValue( const wxTreeItemId& item )
{
	CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( item ) );

	if( data->modifiable )
	{
		m_tree->EditLabel( item, Col_Value );
	}
}

void CSSVariablesTabBase::OnStartLabelEdit( wxTreeEvent& event )
{
	if( event.GetInt() == Col_Value )
	{
		m_originalValue = m_tree->GetItemText( event.GetItem(), Col_Value );
	}

	event.Skip();
}

void CSSVariablesTabBase::OnEndLabelEdit( wxTreeEvent& event )
{
	if( event.GetInt() == Col_Value )
	{
		wxString value = event.GetLabel();

		if( m_originalValue != value )
		{
			const wxTreeItemId& item = event.GetItem();
			m_modifiedItem = item;

			CSSVariablesTabItemData* data = static_cast< CSSVariablesTabItemData* >( m_tree->GetItemData( item ) );

			m_helper.SetLocalsValue( m_currentStackFrameIndex, data->path, value );
		}
	}
	else
	{
		event.Skip();
	}
}

void CSSVariablesTabBase::OnLocalsModifyAckEvent( CLocalsModificationAckEvent& event )
{
	if( m_modifiedItem.IsOk() )
	{
		if( event.GetSuccess() )
		{
			RefreshItem( m_modifiedItem );
		}
		else
		{
			m_tree->SetItemTextColour( m_modifiedItem, MODIFYACK_FAILED_COLOUR );
		}

		m_modifiedItem.Unset();
	}
	else
	{
		event.Skip();
	}
}
