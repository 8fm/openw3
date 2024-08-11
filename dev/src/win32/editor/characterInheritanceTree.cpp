#include "build.h"
#include "characterResourceContainer.h"
#include "characterInheritanceTree.h"

enum eEventIds
{
	EventID_DeleteNode = wxID_HIGHEST,
	EventID_DeleteSubtree,

	EventID_SC_CheckOutNode,
	EventID_SC_CheckOutSubtree,

	EventID_Max
};

CEdCInheritanceTreeItemData::CEdCInheritanceTreeItemData( CCharacter* character )
	: m_character( character )
{

}

CEdCInheritanceTreeItemData::CEdCInheritanceTreeItemData( const CEdCInheritanceTreeItemData* other )
{
	m_character = other->m_character;
}

wxIMPLEMENT_DYNAMIC_CLASS( CEdCharacterInheritanceTree, wxTreeCtrl )

CEdCharacterInheritanceTree::CEdCharacterInheritanceTree()
	: m_popupMenu( nullptr )
{
	Bind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdCharacterInheritanceTree::OnTreeDrag, this );
	Bind( wxEVT_COMMAND_TREE_END_DRAG, &CEdCharacterInheritanceTree::OnTreeDrop, this );

	Bind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdCharacterInheritanceTree::OnRightClick, this );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnDeleteNode, this, EventID_DeleteNode );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnDeleteSubtree, this, EventID_DeleteSubtree );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnCheckOutNode, this, EventID_SC_CheckOutNode );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnCheckOutSubtree, this, EventID_SC_CheckOutSubtree );
}

CEdCharacterInheritanceTree::~CEdCharacterInheritanceTree()
{
	Unbind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdCharacterInheritanceTree::OnTreeDrag, this );
	Unbind( wxEVT_COMMAND_TREE_END_DRAG, &CEdCharacterInheritanceTree::OnTreeDrop, this );

	Unbind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdCharacterInheritanceTree::OnRightClick, this );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnDeleteNode, this, EventID_DeleteNode );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnDeleteSubtree, this, EventID_DeleteSubtree );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnCheckOutNode, this, EventID_SC_CheckOutNode );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterInheritanceTree::OnCheckOutSubtree, this, EventID_SC_CheckOutSubtree );

	if ( m_popupMenu )
	{
		delete m_popupMenu;
	}
}

void CEdCharacterInheritanceTree::Initialize( IEdCharacterDBFolderManagement* sc, Bool readOnlyMode )
{
	m_readOnlyMode = readOnlyMode;

	m_fm = sc;

	wxIcon icon;
	icon.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( TXT( "IMG_CHARACTER" ) ) ); 

	wxImageList* treeImages = new wxImageList( 16, 16, true, 1 );

	m_icon = treeImages->Add( icon );

	AssignImageList( treeImages );
}

void CEdCharacterInheritanceTree::OnRightClick( wxTreeEvent& event )
{
	if ( m_readOnlyMode )
	{
		return;
	}

	if( m_popupMenu )
	{
		delete m_popupMenu;
	}

	m_popupMenu = new wxMenu();

	m_popupMenuItem = event.GetItem();
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should exists because we get event from it" );

	PopulateMenuOptions( m_popupMenuItem, m_popupMenu );

	PopupMenu( m_popupMenu );
}

void CEdCharacterInheritanceTree::PopulateMenuOptions( const wxTreeItemId& item, wxMenu* menu )
{
	CEdCInheritanceTreeItemData* itemData = GetItemData( item );

	menu->Append( EventID_DeleteNode, wxT( "Delete node" ) );
	if ( HasChildren( item ) )
	{
		menu->Append( EventID_DeleteSubtree, wxT( "Delete subtree" ) );
	}

	menu->AppendSeparator();

	menu->Append( EventID_SC_CheckOutNode, wxT( "Check out node" ) );

	if ( HasChildren( item ) )
	{
		menu->Append( EventID_SC_CheckOutSubtree, wxT( "Check out subtree" ) );
	}
}

void CEdCharacterInheritanceTree::OnTreeDrag( wxTreeEvent& event )
{
	if ( m_readOnlyMode )
	{
		event.Veto();
		return;
	}
	m_draggedItem = event.GetItem();
	m_draggedItemOldParent = GetItemParent( m_draggedItem );

	event.Allow();

	RED_FATAL_ASSERT( m_draggedItem.IsOk(), "Item has to be ok because we have event from it" );
}

CCharacter* CEdCharacterInheritanceTree::GetItemCharacter( const wxTreeItemId& item )
{
	RED_FATAL_ASSERT( item.IsOk(), "can't get character from invalid node" );
	CEdCInheritanceTreeItemData* data = GetItemData( item );
	RED_FATAL_ASSERT( data, "Every node should have valida data" );
	return data->GetCharacter();
}

void CEdCharacterInheritanceTree::OnTreeDrop( wxTreeEvent& event )
{
	RED_FATAL_ASSERT( !m_readOnlyMode, "Can't drag node in read only mode" );
	wxTreeItemId destItem = event.GetItem();
	if ( !destItem.IsOk() || !m_draggedItem.IsOk() || !m_draggedItemOldParent.IsOk() )
	{
		return;
	}

	CCharacter* parent = GetItemCharacter( destItem );
	
	CCharacter* character = GetItemCharacter( m_draggedItem );
	CEdCInheritanceTreeItemData* characterData = GetItemData( m_draggedItem );

	CCharacter* characterOldParent = GetItemCharacter( m_draggedItemOldParent );

	const CCharacter* parentsParent = parent;
	// Can't add parent to its child
	while ( parentsParent )
	{
		if ( parentsParent == character )
		{
			return;
		}
		parentsParent = parentsParent->GetParentCharacter();
	}

	if ( !CheckOutSubTree( m_draggedItem ) )
	{
		return;
	}

	character->SetParentCharacter( parent );

	CEdCInheritanceTreeItemData* newItemData = new CEdCInheritanceTreeItemData( character );
	wxTreeItemId newItem = AppendItem( destItem, character->GetName().AsChar(), m_icon, m_icon, newItemData );
	
	m_itemMap.Erase( character->GetGUID() );
	m_itemMap.Insert( character->GetGUID(), newItemData );

	CopyChildren( m_draggedItem, newItem );

	SelectItem( newItem );

	Delete( m_draggedItem );

	m_draggedItem.Unset();
	m_draggedItemOldParent.Unset();
}

void CEdCharacterInheritanceTree::CopyChildren( const wxTreeItemId& sourceItem, const wxTreeItemId& destItem )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId sourceChild = GetFirstChild( sourceItem, cookie );

	while( sourceChild.IsOk() )
	{
		CCharacter* childCharacter = GetItemCharacter( sourceChild );
		RED_FATAL_ASSERT( childCharacter, "Every child item should have not null character" );

		CEdCInheritanceTreeItemData* newItemData = new CEdCInheritanceTreeItemData( childCharacter );
		wxTreeItemId destChild = AppendItem( destItem, childCharacter->GetName().AsChar(), m_icon, m_icon, newItemData );

		m_fm->CharacterModified( childCharacter );

		m_itemMap.Erase( childCharacter->GetGUID() );
		m_itemMap.Insert( childCharacter->GetGUID(), newItemData );

		RED_FATAL_ASSERT( childCharacter->GetParent() == GetItemCharacter( destItem ), "Parent of the character should already be right" );

		CopyChildren( sourceChild, destChild );

		sourceChild = GetNextChild( sourceItem, cookie );
	}
}

void CEdCharacterInheritanceTree::OnDeleteNode( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Item has to be reight, we have event from it" );

	wxTreeItemId parent = GetItemParent( m_popupMenuItem );

	RemoveNode( m_popupMenuItem );

	if ( GetRootItem() != parent )
	{
		SelectItem( parent );
	}
}

void CEdCharacterInheritanceTree::OnDeleteSubtree( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Item has to be reight, we have event from it" );

	wxTreeItemId parent = GetItemParent( m_popupMenuItem );

	RemoveSubTree( m_popupMenuItem );

	if ( GetRootItem() != parent )
	{
		SelectItem( parent );
	}
}

void CEdCharacterInheritanceTree::OnCheckOutNode( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Item has to be reight, we have event from it" );
	
	CheckOutNode( m_popupMenuItem );
}

void CEdCharacterInheritanceTree::OnCheckOutSubtree( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Item has to be reight, we have event from it" );

	CheckOutSubTree( m_popupMenuItem );
}

Bool CEdCharacterInheritanceTree::SelectionChanged( const wxTreeItemId& item )
{
	m_popupMenuItem = item;
	return m_popupMenuItem.IsOk();
}

wxTreeItemId CEdCharacterInheritanceTree::AddTreeRoot()
{
	if( !GetRootItem().IsOk() )
	{
		AddRoot	( wxT( "NONE" ), m_icon, m_icon, new CEdCInheritanceTreeItemData( ( CCharacter* ) nullptr ) );
	}
	RED_FATAL_ASSERT( GetRootItem().IsOk(), "Root item shoud be right, we have just created it" );
	return GetRootItem();
}

void CEdCharacterInheritanceTree::PopulateTree( CEdCharacterResourceContainer& resContainer )
{
	m_resContainer = &resContainer;

	UpdateTree();
}

void CEdCharacterInheritanceTree::UpdateTree()
{
	DeleteAllItems();
	m_itemMap.Clear();

	wxTreeItemId rootItem = AddTreeRoot();

	TDynArray< THandle< CCharacterResource > >& resources = m_resContainer->GetResources();
	for ( auto resIt = resources.Begin(); resIt != resources.End(); ++resIt )
	{
		CCharacterResource* resource = (*resIt).Get();
		RED_FATAL_ASSERT( resource, "Container should store only valid resources" );
		TDynArray< CCharacter* >& characters = resource->GetCharactersData();
		for ( auto charIt = characters.Begin(); charIt != characters.End(); ++charIt )
		{
			CCharacter* character = *charIt;
			RED_FATAL_ASSERT( character, "Resource should store only valid characters" );
			CEdCInheritanceTreeItemData* itemData = new CEdCInheritanceTreeItemData( character );
			m_itemMap.Insert( character->GetGUID(), itemData );
		}
	}

	for ( auto it = m_itemMap.Begin(); it != m_itemMap.End(); ++it )
	{
		CEdCInheritanceTreeItemData* itemData = it->m_second;
		AddCharacterToTheTree( itemData );
	}
}

wxTreeItemId CEdCharacterInheritanceTree::AddCharacterToTheTree( CEdCInheritanceTreeItemData* characterData )
{
	if ( characterData->GetId().IsOk() )
	{
		return characterData->GetId();
	}
	CCharacter* character = characterData->GetCharacter();

	const CCharacter* parent = character->GetParentCharacter();

	wxTreeItemId parentId;

	if ( parent == nullptr )
	{
		parentId = GetRootItem();
	}
	else
	{
		CEdCInheritanceTreeItemData** parentNode = m_itemMap.FindPtr( parent->GetGUID() );
		RED_FATAL_ASSERT( parentNode, "Parent node should be in map" );
		RED_FATAL_ASSERT( *parentNode, "Parent node should have valid data" );
		parentId = AddCharacterToTheTree( *parentNode );
	}

	RED_FATAL_ASSERT( parentId.IsOk(), "Parent node should already be created" );
	
	return wxTreeCtrl::AppendItem( parentId, character->GetName().AsChar(), m_icon, m_icon, characterData );
}

CEdCInheritanceTreeItemData* CEdCharacterInheritanceTree::GetItemData( const wxTreeItemId& item )
{
	RED_FATAL_ASSERT( item.IsOk(), "Can't get item from invalid node" );
	CEdCInheritanceTreeItemData* itemData = static_cast< CEdCInheritanceTreeItemData* >( wxTreeCtrl::GetItemData( item ) );
	RED_FATAL_ASSERT( itemData, "Every node should have valid data" );
	return itemData;
}

void CEdCharacterInheritanceTree::GatherCharactersForSelectedNode( TDynArray< CCharacter* >& characters )
{
	wxTreeItemId selection = GetSelection();
	if ( selection.IsOk() )
	{
		GatherAllCharacters( characters, selection );
	}
}

void CEdCharacterInheritanceTree::GatherAllCharacters( TDynArray< CCharacter* >& characters, wxTreeItemId item )
{
	CEdCInheritanceTreeItemData* itemData = GetItemData( item );
	characters.PushBack( itemData->GetCharacter() );
	
	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );

	while( child.IsOk() )
	{
		GatherAllCharacters( characters, child );

		child = GetNextChild( item, cookie );
	}
}


void CEdCharacterInheritanceTree::SelectCharacterItem( CCharacter* character )
{
	if ( !character )
	{
		// unselect current node
		wxTreeItemId selection = GetSelection();
		if ( selection.IsOk() )
		{
			SelectItem( selection, false );
		}
	}
	else
	{
		CEdCInheritanceTreeItemData** itemData = m_itemMap.FindPtr( character->GetGUID() );
		RED_FATAL_ASSERT( itemData, "Character should be in map" );
		RED_FATAL_ASSERT( *itemData, "CHaracter should have valid node data" );
			
		if ( ( *itemData )->GetId().IsOk() )
		{
			SelectItem( ( *itemData )->GetId() );
		}
	}
}

CCharacter* CEdCharacterInheritanceTree::GetSelectedCharacter()
{
	if ( m_popupMenuItem.IsOk() )
	{
		CEdCInheritanceTreeItemData* itemData = GetItemData( m_popupMenuItem );
		return itemData->GetCharacter();
	}
	return nullptr;
}

Bool CEdCharacterInheritanceTree::CheckOutNode( const wxTreeItemId& item )
{
	CEdCInheritanceTreeItemData* itemData = GetItemData( item );
	RED_FATAL_ASSERT( itemData, "Can't check out invalid node" );

	return m_fm->CheckOutCharacter( itemData->GetCharacter(), true );
}

Bool CEdCharacterInheritanceTree::CheckOutInheritanceTree( CCharacter* character, const CProperty* prop )
{
	CEdCInheritanceTreeItemData** itemData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( itemData, "Character should be in map" );
	RED_FATAL_ASSERT( *itemData, "CHaracter should have valida data" );

	return CheckOutSubTree( ( *itemData )->GetId(), prop );
}

Bool CEdCharacterInheritanceTree::CheckOutSubTree( const wxTreeItemId& item, const CProperty* prop )
{
	TDynArray< CCharacter* >  failedToCheckOut;

	CCharacter* character = GetItemCharacter( item );

	if ( !m_fm->CheckOutCharacter( character, false ) )
	{
		failedToCheckOut.PushBack( character );
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );

	while( child.IsOk() )
	{
		CCharacter* childCharacter = GetItemCharacter( child );

		if ( !prop || childCharacter->IsInherited( prop ) )
		{
			if ( !m_fm->CheckOutCharacter( childCharacter, false ) )
			{
				failedToCheckOut.PushBack( childCharacter );
			}
		}
		child = GetNextChild( item, cookie );
	}
	if ( !failedToCheckOut.Empty() )
	{
		String msg( TXT("Can't check out resources for following characters: \n") );
		for ( auto it = failedToCheckOut.Begin(); it != failedToCheckOut.End(); ++it )
		{
			msg = String::Printf( TXT("%s\n %s"), (*it)->GetName());
		}

		wxMessageBox( msg.AsChar(), wxT( "Not checked out" ), wxOK, this );

		return false;
	}
	return true;
}

void CEdCharacterInheritanceTree::RemoveSubTree( const wxTreeItemId& item )
{
	RED_FATAL_ASSERT( item.IsOk(), "Can't remove invalid node" );

	if ( !CheckOutSubTree( item ) )
	{
		return;
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );

	while( child.IsOk() )
	{
		RemoveSubTree( child );
		child = GetNextChild( item, cookie );
	}

	CEdCInheritanceTreeItemData* itemData = GetItemData( item );
	m_itemMap.Erase( itemData->GetCharacter()->GetGUID() );

	Delete( item );
}

void CEdCharacterInheritanceTree::RemoveNode( const wxTreeItemId& item )
{
	RED_FATAL_ASSERT( item.IsOk(), "Can't remove invalid node" );
	// need to check out whole subtree because of inheritance
	if ( !CheckOutSubTree( item ) )
	{
		return;
	}

	CEdCInheritanceTreeItemData* itemData = GetItemData( item );
	CCharacter* character = itemData->GetCharacter();

	if ( !m_fm->DeleteCharacter( character ) )
	{
		return;
	}

	UpdateInheritanceForRemovedCharacter( character );
}

void CEdCharacterInheritanceTree::UpdateInheritanceForRemovedCharacter( CCharacter* character )
{
	CEdCInheritanceTreeItemData** itemData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( itemData, "Character should be in map" );
	RED_FATAL_ASSERT( *itemData, "Character should have valid data" );

	wxTreeItemId item = ( *itemData )->GetId();

	wxTreeItemId parent = GetItemParent( item );
	CEdCInheritanceTreeItemData* parentData = GetItemData( parent );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );
	while( child.IsOk() )
	{
		CCharacter* childCharacter = GetItemCharacter( child );
		childCharacter->SetParentCharacter( parentData->GetCharacter() );
		child = GetNextChild( item, cookie );
		m_fm->CharacterModified( childCharacter );
	}

	CopyChildren( item, parent );

	m_itemMap.Erase( ( *itemData )->GetCharacter()->GetGUID() );

	Delete( item );
}

void CEdCharacterInheritanceTree::AddCharacter( CCharacter* character )
{
	const CCharacter* parent = character->GetParentCharacter();
	wxTreeItemId parentId;
	if ( parent )
	{
		CEdCInheritanceTreeItemData** parentDataPtr = m_itemMap.FindPtr( parent->GetGUID() );
		RED_FATAL_ASSERT( parentDataPtr, "Character should be in map" );
		RED_FATAL_ASSERT( *parentDataPtr, "Character should have valid data" );

		parentId = ( *parentDataPtr )->GetId();
	}
	else
	{
		parentId = GetRootItem();
	}


	CEdCInheritanceTreeItemData* newItemData = new CEdCInheritanceTreeItemData( character );
	wxTreeItemId newItem = AppendItem( parentId, character->GetName().AsChar(), m_icon, m_icon, newItemData );
	m_itemMap.Insert( character->GetGUID(), newItemData );
}

void CEdCharacterInheritanceTree::RenameCharacter( CCharacter* character )
{
	CEdCInheritanceTreeItemData** dataPtr = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( dataPtr, "Character should be in map" );
	RED_FATAL_ASSERT( *dataPtr, "Character should have valid data" );

	SetItemText( ( *dataPtr )->GetId(), character->GetName().AsChar() );
}

void CEdCharacterInheritanceTree::GetInheritanceTree( CCharacter* parent, const CProperty* prop, TDynArray< CCharacter* >& editingCharacters )
{
	CEdCInheritanceTreeItemData** parentDataPtr = m_itemMap.FindPtr( parent->GetGUID() );
	RED_FATAL_ASSERT( parentDataPtr, "Character should be in map" );
	RED_FATAL_ASSERT( *parentDataPtr, "Character should have valid data" );

	wxTreeItemId parentId = ( *parentDataPtr )->GetId();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( parentId, cookie );
	while( child.IsOk() )
	{
		CCharacter* childCharacter = GetItemCharacter( child );
		if ( childCharacter->IsInherited( prop ) )
		{
			editingCharacters.PushBack( childCharacter );
			GetInheritanceTree( childCharacter, prop, editingCharacters );
		}

		child = GetNextChild( parentId, cookie );
	}
}

