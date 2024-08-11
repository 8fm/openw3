#include "build.h"

#include "characterFolderTree.h"
#include "characterResourceContainer.h"
#include "../../common/game/characterResource.h"
#include "../../common/core/depot.h"
#include "../../common/core/dataError.h"

#include "res/images/folder.xpm"
#include "res/images/folder_locked.xpm"
#include "res/images/item.xpm"
#include "res/images/item_locked.xpm"


//-----------------------------------------------------------------------------------
enum eEventIds
{
	EventID_CreateDirectory = wxID_HIGHEST,
	EventID_CreateResource,
	EventID_CreateCharacter,
	EventID_DeleteCharacter,

	EventID_SC_CheckOut,
	EventID_SC_Save,
	EventID_SC_SaveAndCheckin,
	EventID_SC_Revert,
	EventID_SC_Delete,

	EventID_RenameResource,
	EventID_RenameCharacter,

	EventID_Max
};

CEdCFolderTreeItemDataBase::CEdCFolderTreeItemDataBase()
{
	m_state = State_Unmodified;
}

CEdCFolderTreeItemDataBase::CEdCFolderTreeItemDataBase( const CEdCFolderTreeItemDataBase* other )
{
	m_state = other->m_state;
}

CEdCFolderTreeItemDataBase::EState CEdCFolderTreeItemDataBase::GetFileState( CDiskFile* file )
{
	if ( !file )
	{
		return State_Modified;
	}
	file->GetStatus();

	if( file->IsLocal() )
	{
		file->Add();
	}

	if ( file->IsModified() )
	{
		return State_Modified;
	}
	if ( file->IsLocal() || file->IsAdded() )
	{
		return CEdCFolderTreeItemDataBase::State_ReadyForSourceControl;
	}
	if ( file->IsCheckedOut() )
	{
		return CEdCFolderTreeItemDataBase::State_Locked;
	}
	if ( file->IsCheckedIn() )
	{
		return CEdCFolderTreeItemDataBase::State_Unmodified;
	}
	RED_FATAL_ASSERT( true, "Can't verify file %s state", file->GetFileName().AsChar() );
	return CEdCFolderTreeItemDataBase::State_Unmodified;
}

CEdCFolderTreeItemDataDirBase::CEdCFolderTreeItemDataDirBase( CDirectory* directory )
{
	m_directory = directory;
}

CEdCFolderTreeItemDataDirBase::CEdCFolderTreeItemDataDirBase( const CEdCFolderTreeItemDataDirBase* other )
{
	m_directory = other->m_directory;
}

void CEdCFolderTreeItemDataDirBase::PopulateMenuOptions( wxTreeItemId item, wxMenu* menu )
{
	menu->Append( EventID_CreateDirectory, wxT( "Create folder" ) );
	menu->Append( EventID_CreateResource, wxT( "Create resource" ) );

	menu->AppendSeparator();

	switch ( GetState() )
	{
	case State_Locked:
		break;
	case State_Unmodified: 
		menu->Append( EventID_SC_CheckOut, wxT( "Check out" ) );
		break;
	case State_ReadyForSourceControl:
		menu->Append( EventID_SC_SaveAndCheckin, wxT( "Submit" ) );
		menu->Append( EventID_SC_Revert, wxT( "Revert" ) );
		break;
	case State_Modified:
		menu->Append( EventID_SC_Save, wxT( "Save" ) );
		menu->Append( EventID_SC_SaveAndCheckin, wxT( "Save and Check in" ) );
		menu->Append( EventID_SC_Revert, wxT( "Revert" ) );
		break;
	}
}

CEdCFolderTreeItemDataRoot::CEdCFolderTreeItemDataRoot( CDirectory* directory ) 
	: CEdCFolderTreeItemDataDirBase( directory)
{
}

CEdCFolderTreeItemDataRoot::CEdCFolderTreeItemDataRoot( const CEdCFolderTreeItemDataRoot* other )
	: CEdCFolderTreeItemDataDirBase( other )
{
}

CEdCFolderTreeItemDataDir::CEdCFolderTreeItemDataDir( CDirectory* directory )
	: CEdCFolderTreeItemDataDirBase( directory)
{
	m_dirName = directory->GetName();
}

CEdCFolderTreeItemDataDir::CEdCFolderTreeItemDataDir( String& fileName )
	: CEdCFolderTreeItemDataDirBase( ( CDirectory* ) nullptr )
{
	m_dirName = fileName;
}

CEdCFolderTreeItemDataDir::CEdCFolderTreeItemDataDir( const CEdCFolderTreeItemDataDir* other )
	: CEdCFolderTreeItemDataDirBase( other )
{
	m_dirName = other->m_dirName;
}


void CEdCFolderTreeItemDataDir::PopulateMenuOptions( wxTreeItemId item, wxMenu* menu )
{
	CEdCFolderTreeItemDataDirBase::PopulateMenuOptions( item, menu );

	menu->AppendSeparator();

	menu->Append( EventID_SC_Delete, wxT( "Delete subtree and Remove from Source Control" ) );
}	

CEdCFolderTreeItemDataFile::CEdCFolderTreeItemDataFile( CCharacterResource* resource )
{
	m_resource = resource;
	RED_FATAL_ASSERT( m_resource->GetFile(), "Resource doesn't have a file, give a file name" );

	UpdateName();
	
	UpdateState();
}

CEdCFolderTreeItemDataFile::CEdCFolderTreeItemDataFile( CCharacterResource* resource, String& fileName )
{
	m_resource = resource;
	CDiskFile* file = resource->GetFile();
	RED_FATAL_ASSERT( !resource->GetFile() || resource->GetFile()->GetFileName() == fileName.LeftString( fileName.Size() - Red::System::StringLength( CCharacterResource::GetFileExtension() ) - 2), "Invalid constructor used - doubled file name" );
	m_fileName = fileName;
	UpdateState();
}

void CEdCFolderTreeItemDataFile::UpdateName()
{
	if ( m_resource->GetFile() )
	{
		String name = m_resource->GetFile()->GetFileName();

		m_fileName = name.LeftString( name.Size() - Red::System::StringLength( CCharacterResource::GetFileExtension() ) - 2);
	}
}

CEdCFolderTreeItemDataFile::CEdCFolderTreeItemDataFile( const CEdCFolderTreeItemDataFile* other )
{
	m_resource = other->m_resource;
}

Bool CEdCFolderTreeItemDataFile::Rename( const String& newName ) 
{
	CDiskFile* file = m_resource->GetFile();
	if ( file )
	{
		if ( !file->Rename( newName, m_resource->GetExtension() ) )
		{
			return false;
		}
		else
		{
			UpdateName();
		}
	}
	else
	{
		m_fileName = newName;
	}
	return true;
}

void CEdCFolderTreeItemDataFile::PopulateMenuOptions( wxTreeItemId item, wxMenu* menu )
{
	menu->Append( EventID_CreateCharacter, wxT( "Create character" ) );

	menu->AppendSeparator();

	menu->Append( EventID_RenameResource, wxT( "Rename file" ) );
	
	menu->AppendSeparator();


	Bool checkedOut = m_resource->GetFile() && m_resource->GetFile()->IsCheckedOut();

	if ( checkedOut )
	{
		menu->Append( EventID_SC_Revert, wxT( "Revert" ) );
	}

	switch ( GetState() )
	{
	case State_Locked:
		break;
	case State_Unmodified: 
		menu->Append( EventID_SC_CheckOut, wxT( "Check out" ) );
		break;
	case State_ReadyForSourceControl:
		menu->Append( EventID_SC_SaveAndCheckin, wxT( "Submit" ) );
		break;
	case State_Modified:
		menu->Append( EventID_SC_Save, wxT( "Save" ) );
		menu->Append( EventID_SC_SaveAndCheckin, wxT( "Save and Check in" ) );
		break;
	}

	menu->AppendSeparator();
	menu->Append( EventID_SC_Delete, wxT( "Delete and Remove from Source Control" ) );
}

CEdCFolderTreeItemDataCharacter::CEdCFolderTreeItemDataCharacter( CCharacter* characterData )
{
	RED_FATAL_ASSERT( characterData, "Every character node has to have valid character" );
	m_characterData = characterData;
}

CEdCFolderTreeItemDataCharacter::CEdCFolderTreeItemDataCharacter( const CEdCFolderTreeItemDataCharacter* other )
{
	m_characterData = other->m_characterData;
}

void CEdCFolderTreeItemDataCharacter::PopulateMenuOptions( wxTreeItemId item, wxMenu* menu )
{
	menu->Append( EventID_RenameCharacter, wxT( "Rename character" ) );

	menu->AppendSeparator();

	switch ( GetState() )
	{
	case State_Locked:
		break;
	case State_Unmodified: 
		menu->Append( EventID_SC_CheckOut, wxT( "Check out" ) );

		menu->AppendSeparator();

		break;
	case State_ReadyForSourceControl:
		break;
	case State_Modified:
		break;
	}

	menu->Append( EventID_DeleteCharacter, wxT( "Delete" ) );
}

#define TREE_COLOUR_EDITED ( *wxRED )
#define TREE_COLOUR_EDITED_CHILD ( wxColour( 255, 128, 0 ) )		
#define TREE_COLOUR_SAVED ( *wxGREEN )
#define TREE_COLOUR_SAVED_CHILD ( wxColour( 133, 255, 133 ) )
#define TREE_COLOUR_UNMODIFIED ( *wxBLACK )
#define TREE_COLOUR_LOCKED ( *wxBLUE )
#define TREE_COLOUR_LOCKED_CHILD ( wxColour( 133, 133, 255 ) )


//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

wxDEFINE_EVENT( wXEVT_TREE_CHARACTER_NAME_CHANGED, wxEvent );

wxIMPLEMENT_DYNAMIC_CLASS( CEdCharacterFolderTree, wxTreeCtrl )

CEdCharacterFolderTree::CEdCharacterFolderTree()
	: m_popupMenu( nullptr )
{
	Bind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdCharacterFolderTree::OnTreeDrag, this );
	Bind( wxEVT_COMMAND_TREE_END_DRAG, &CEdCharacterFolderTree::OnTreeDrop, this );
	Bind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdCharacterFolderTree::OnRightClick, this );

	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateDirectory, this, EventID_CreateDirectory );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateResource, this, EventID_CreateResource );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateCharacter, this, EventID_CreateCharacter );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnDeleteCharacter, this, EventID_DeleteCharacter );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnDelete, this, EventID_SC_Delete );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCheckOut, this, EventID_SC_CheckOut );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnSave, this, EventID_SC_Save );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnSaveAndCheckIn, this, EventID_SC_SaveAndCheckin );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRevert, this, EventID_SC_Revert );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRenameResource, this, EventID_RenameResource );
	Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRenameCharacter, this, EventID_RenameCharacter );
}

CEdCharacterFolderTree::~CEdCharacterFolderTree()
{
	if ( m_popupMenu )
	{
		delete( m_popupMenu );
	}

	Unbind( wxEVT_COMMAND_TREE_BEGIN_DRAG, &CEdCharacterFolderTree::OnTreeDrag, this );
	Unbind( wxEVT_COMMAND_TREE_END_DRAG, &CEdCharacterFolderTree::OnTreeDrop, this );
	Unbind( wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &CEdCharacterFolderTree::OnRightClick, this );

	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateDirectory, this, EventID_CreateDirectory );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateResource, this, EventID_CreateResource );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCreateCharacter, this, EventID_CreateCharacter );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnDeleteCharacter, this, EventID_DeleteCharacter );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnDelete, this, EventID_SC_Delete );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnCheckOut, this, EventID_SC_CheckOut );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnSave, this, EventID_SC_Save );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnSaveAndCheckIn, this, EventID_SC_SaveAndCheckin );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRevert, this, EventID_SC_Revert );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRenameResource, this, EventID_RenameResource );
	Unbind( wxEVT_COMMAND_MENU_SELECTED, &CEdCharacterFolderTree::OnRenameCharacter, this, EventID_RenameCharacter );
}

void CEdCharacterFolderTree::Initialize( CEdCharacterResourceContainer& resourceContainer, IEdCharacterDBInheritanceManagement* im, Bool readOnlyMode )
{
	m_readOnlyMode = readOnlyMode;
	m_im = im;
	m_resourceContainer = &resourceContainer;

	wxIcon groupIcon( folder_xpm );
	wxIcon itemIcon( item_xpm );

	wxImageList* treeImages = new wxImageList( 16, 16, true, 6 );

	m_dirIcon = treeImages->Add( groupIcon );
	m_resIcon = treeImages->Add( groupIcon );
	m_itemIcon = treeImages->Add( itemIcon );

	AssignImageList( treeImages );
}

void CEdCharacterFolderTree::OnTreeDrag( wxTreeEvent& event )
{
	if ( m_readOnlyMode )
	{
		event.Veto();
		return;
	}
	m_draggedItem = event.GetItem();
	m_draggedItemOldParent = GetItemParent( m_draggedItem );

	event.Allow();

	RED_FATAL_ASSERT( m_draggedItem.IsOk(), "Can't dragg invalid node" );
}

void CEdCharacterFolderTree::OnTreeDrop( wxTreeEvent& event )
{
	RED_FATAL_ASSERT( !m_readOnlyMode, "Dragging node only in not read only node" );

	wxTreeItemId destItem = event.GetItem();
	if ( !destItem.IsOk() || !m_draggedItem.IsOk() || !m_draggedItemOldParent.IsOk() )
	{
		return;
	}
	CEdCFolderTreeItemDataBase* draggedItemData = GetItemData( m_draggedItem );

	CEdCFolderTreeItemDataBase* newParentData = GetItemData( destItem );

	wxTreeItemId itemId;

	switch ( newParentData->GetNodeType() )
	{
	case FTNodeType_Root:
	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDirBase* newParentDirData = static_cast< CEdCFolderTreeItemDataDirBase* >( newParentData );
			if ( draggedItemData->GetNodeType() == FTNodeType_File )
			{
				CEdCFolderTreeItemDataFile* draggedFileData = static_cast< CEdCFolderTreeItemDataFile* >( draggedItemData );
				itemId = MoveFile( newParentDirData, draggedFileData );
			}
			else
			{
				return;
			}
		}
		break;
	case FTNodeType_File:
		{
			if ( draggedItemData->GetNodeType() != FTNodeType_Character )
			{
				return;
			}
			CEdCFolderTreeItemDataCharacter* draggedCharItemData = static_cast< CEdCFolderTreeItemDataCharacter* >( draggedItemData );
			CEdCFolderTreeItemDataFile* newParentFileData = static_cast< CEdCFolderTreeItemDataFile* >( newParentData );
			itemId = MoveCharacter( newParentFileData, draggedCharItemData, m_draggedItemOldParent );
		}		
		break;
	case FTNodeType_Character:
		return;
	default:
		break;
	}
	if ( itemId.IsOk() )
	{
		SelectItem ( itemId );
	}

	m_draggedItem.Unset();
	m_draggedItemOldParent.Unset();
}

wxTreeItemId CEdCharacterFolderTree::MoveFile( CEdCFolderTreeItemDataDirBase* newParentData, CEdCFolderTreeItemDataFile* itemData )
{
	RED_FATAL_ASSERT( itemData, "Can't move invalid file node" );
	RED_FATAL_ASSERT( newParentData, "Can't move node to invalid directory node" );

	if ( !CheckOutFile( itemData->GetId(), true ) )
	{
		return wxTreeItemId();
	}
	CDirectory* directory = newParentData->GetDirectory();
	CCharacterResource* resource = itemData->GetResource();
	CDiskFile* file = resource->GetFile();
	if ( file )
	{
		if ( !resource->SaveAs( directory, itemData->GetName() ) )
		{
			return wxTreeItemId();
		}
		file->Delete();
	}

	CEdCFolderTreeItemDataFile* newItemData = new CEdCFolderTreeItemDataFile( resource );

	wxTreeItemId itemId = wxTreeCtrl::AppendItem( newParentData->GetId(), newItemData->GetName(), m_resIcon, m_resIcon, newItemData );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( itemData->GetId(), cookie );

	while( child.IsOk() )
	{
		CEdCFolderTreeItemDataBase* childItemData = GetItemData( child );
		RED_FATAL_ASSERT( itemData->GetNodeType() == FTNodeType_Character, "File node should has Character nodes as children" );
		{
			CEdCFolderTreeItemDataCharacter* childCharItemData = static_cast< CEdCFolderTreeItemDataCharacter* >( childItemData );
			CEdCFolderTreeItemDataCharacter* newCharItemData = new CEdCFolderTreeItemDataCharacter( childCharItemData );
			wxTreeItemId childItemId = wxTreeCtrl::AppendItem( itemId, newCharItemData->GetName(), m_itemIcon, m_itemIcon, newCharItemData );
			if ( newCharItemData->GetState() == CEdCFolderTreeItemDataBase::State_Modified )
			{
				SetItemTextColour( newCharItemData->GetId(), TREE_COLOUR_EDITED_CHILD );
			}
		}

		child = GetNextChild( itemData->GetId(), cookie );
	}
	UpdateColours( newItemData );

	Delete( itemData->GetId() );

	
	return itemId;
}

wxTreeItemId CEdCharacterFolderTree::MoveCharacter( CEdCFolderTreeItemDataFile* newParentData, CEdCFolderTreeItemDataCharacter* itemData, const wxTreeItemId& oldParent )
{
	RED_FATAL_ASSERT( itemData, "Can't move invalid character node" );
	RED_FATAL_ASSERT( newParentData, "Can't move to invalid file node" );

	if ( !CheckOutCharacter( itemData->GetCharacterData(), true ) )
	{
		return wxTreeItemId();
	}
	
	if ( !CheckOutFile( newParentData->GetId(), true ) )
	{
		return wxTreeItemId();
	}

	CCharacter* character = itemData->GetCharacterData();

	MarkItemModified( itemData->GetId() );
	m_itemMap.Erase( character->GetGUID() );
	Delete( itemData->GetId() );

	CEdCFolderTreeItemDataCharacter* newItemData = new CEdCFolderTreeItemDataCharacter( character );
	m_itemMap.Insert( character->GetGUID(), newItemData );

	wxTreeItemId itemId = wxTreeCtrl::AppendItem( newParentData->GetId(), newItemData->GetName(), m_itemIcon, m_itemIcon, newItemData );

	CCharacterResource* resource = newParentData->GetResource();
	RED_FATAL_ASSERT( resource, "Every resource node should have valid recource" );
	character->SetParent( resource );
	resource->GetCharactersData().PushBack( character );

	CEdCFolderTreeItemDataBase* oldParentData = GetItemData( oldParent );
	RED_FATAL_ASSERT( oldParentData, "Old paren't has invalid data" );
	RED_FATAL_ASSERT( oldParentData->GetNodeType() == FTNodeType_File, "Parent has to be file node" );
	CEdCFolderTreeItemDataFile* oldParentFileData = static_cast< CEdCFolderTreeItemDataFile* >( oldParentData );
	RED_FATAL_ASSERT( oldParentFileData, "Parent has to be file node" );
	CCharacterResource* oldResource = oldParentFileData->GetResource();
	RED_FATAL_ASSERT( oldResource, "Every file node should has valid resource" );
	oldResource->GetCharactersData().Remove( character );

	MarkItemModified( itemId );
	SelectItem( itemId );

	return itemId;
}

wxTreeItemId CEdCharacterFolderTree::AddTreeRoot( CDirectory* rootDirectory )
{
	if( !GetRootItem().IsOk() )
	{
		AddRoot( wxT( "Character DB" ), m_dirIcon, m_dirIcon, new CEdCFolderTreeItemDataRoot( rootDirectory ) );
	}

	wxTreeItemId rootItem = GetRootItem();
	wxTreeItemData* rootData = GetItemData( rootItem );
	RED_FATAL_ASSERT( GetRootItem().IsOk(), "Root item has to be right - we have already created it" );
	return GetRootItem();
}

void CEdCharacterFolderTree::ExpandPath( CCharacterResource* characterResource )
{
	if ( !characterResource )
	{
		SelectItem( GetRootItem() );
	}
	else
	{
		FindAndSelectResourceNode( characterResource, GetRootItem() );
	}
}

Bool CEdCharacterFolderTree::IsModified()
{
	CEdCFolderTreeItemDataBase* itemData = GetItemData( GetRootItem() );
	return itemData->GetState() == CEdCFolderTreeItemDataBase::State_Modified;
}

Bool CEdCharacterFolderTree::FindAndSelectResourceNode( CCharacterResource* characterResource, const wxTreeItemId& curItem )
{	
	RED_FATAL_ASSERT( characterResource, "Can't find node with invalid resource" );
	RED_FATAL_ASSERT( curItem.IsOk(), "Can't find resource in invalid tree" );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( curItem, cookie );

	while( child.IsOk() )
	{
		CEdCFolderTreeItemDataBase* itemData = GetItemData( child );
		if ( itemData->GetNodeType() == FTNodeType_File )
		{
			CEdCFolderTreeItemDataFile* fileItemData = static_cast< CEdCFolderTreeItemDataFile* >( itemData );
			RED_FATAL_ASSERT( fileItemData->GetResource(), "File node should always have a resource" );
			if ( characterResource == fileItemData->GetResource() )
			{
				SelectItem( child );
				return true;
			}
		}
		else if ( itemData->GetNodeType() == FTNodeType_Root || itemData->GetNodeType() == FTNodeType_Dir )
		{
			if ( FindAndSelectResourceNode( characterResource, child ) )
			{
				return true;
			}
		}

		child = GetNextChild( curItem, cookie );
	}

	return false;
}

void CEdCharacterFolderTree::PopulateTree( CDirectory* rootDirectory )
{
	AddTreeRoot( rootDirectory );

	PopulateTreeDirs( rootDirectory, GetItemData( GetRootItem() ) );

	SelectItem( GetRootItem() );
}

void CEdCharacterFolderTree::PopulateTreeDirs( CDirectory* directory, wxTreeItemData* treeItem )
{
	for ( CDirectory* childDir : directory->GetDirectories() )
	{
		CEdCFolderTreeItemDataDir* itemData = new CEdCFolderTreeItemDataDir( childDir );
		wxTreeItemId itemId = wxTreeCtrl::AppendItem( treeItem->GetId(), itemData->GetName(), m_dirIcon, m_dirIcon, itemData );

		PopulateTreeDirs( childDir, itemData );
	}

	for ( CDiskFile* file : directory->GetFiles() )
	{
		CResource* resource = GDepot->LoadResource( file->GetDepotPath() );
		if( resource && resource->IsA< CCharacterResource >() )
		{
			CCharacterResource* characterResource = static_cast< CCharacterResource* >( resource );
			m_resourceContainer->AddResource( characterResource );

			CDiskFile* file = characterResource->GetFile();
			RED_FATAL_ASSERT( file, "Resource is read from disk - it has to have file" );

			CEdCFolderTreeItemDataFile* itemData = new CEdCFolderTreeItemDataFile( characterResource );
			wxTreeItemId resNode = wxTreeCtrl::AppendItem( treeItem->GetId(), itemData->GetName(), m_resIcon, m_resIcon, itemData );

			PopulateTreeRes( itemData, characterResource );
		}
		else
		{
			DATA_HALT( DES_Tiny, resource, TXT( "Character DB" ), TXT( "Invalid Resource in Charachet DB - Please move it out of the Character DB root directory" ) );
		}
	}
}

void CEdCharacterFolderTree::PopulateTreeRes( CEdCFolderTreeItemDataFile* itemData, CCharacterResource* characterResource )
{
	TDynArray< CCharacter* >& characterData = characterResource->GetCharactersData();
	for ( auto it = characterData.Begin(); it != characterData.End(); ++it )
	{
		CEdCFolderTreeItemDataCharacter* treeItemCharacter = new CEdCFolderTreeItemDataCharacter( *it );
		m_itemMap.Insert( (*it)->GetGUID(), treeItemCharacter );
		wxTreeCtrl::AppendItem( itemData->GetId(), treeItemCharacter->GetName(), m_itemIcon, m_itemIcon, treeItemCharacter );
	}
	
	UpdateColours( itemData );
}

void CEdCharacterFolderTree::UpdateTree( const wxTreeItemId& item )
{
	CEdCFolderTreeItemDataBase* itemData = GetItemData( item );

	DeleteChildren( item );

	switch ( itemData->GetNodeType() )
	{
	case FTNodeType_Root:
		{
			CEdCFolderTreeItemDataRoot* data = static_cast< CEdCFolderTreeItemDataRoot* >( itemData );
			PopulateTreeDirs( data->GetDirectory(), data );
		}
		break;
	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDir* data = static_cast< CEdCFolderTreeItemDataDir* >( itemData );
			PopulateTreeDirs( data->GetDirectory(), data );
		}
		break;
	case FTNodeType_File:
		{
			CEdCFolderTreeItemDataFile* data = static_cast< CEdCFolderTreeItemDataFile* >( itemData );
			PopulateTreeRes( data, data->GetResource() );
		}
		break; 

	case FTNodeType_Character:
		{
		}
		break;
	}
}

CEdCFolderTreeItemDataBase* CEdCharacterFolderTree::GetItemData( const wxTreeItemId& item )
{
	CEdCFolderTreeItemDataBase* itemData = static_cast< CEdCFolderTreeItemDataBase* >( wxTreeCtrl::GetItemData( item ) );
	RED_FATAL_ASSERT( itemData, "Every node should have valid data" );
	return itemData;
}

CCharacterResource* CEdCharacterFolderTree::GetParentResource( const wxTreeItemId& item )
{
	wxTreeItemId itemParent = GetItemParent( item );
	CEdCFolderTreeItemDataBase* itemParentBase = GetItemData( itemParent );
	RED_FATAL_ASSERT( itemParentBase->GetNodeType() == FTNodeType_File, "Character parent is not a resource in folder tree" );
	CEdCFolderTreeItemDataFile* itemParentFile = static_cast< CEdCFolderTreeItemDataFile* >( itemParentBase );
	return itemParentFile->GetResource();
}

CDirectory* CEdCharacterFolderTree::GetDirectory( const wxTreeItemId& item )
{
	CEdCFolderTreeItemDataBase* baseData = GetItemData( item );

	RED_FATAL_ASSERT( baseData->GetNodeType() == FTNodeType_Root ||  baseData->GetNodeType() == FTNodeType_Dir, "Can't get directory for this node type" );

	CEdCFolderTreeItemDataDirBase* dirBaseData = static_cast< CEdCFolderTreeItemDataDirBase* >( baseData );
	return  dirBaseData->GetDirectory();
}

CDirectory* CEdCharacterFolderTree::GetParentDirectory( const wxTreeItemId& item )
{
	wxTreeItemId itemParent = GetItemParent( item );
	CEdCFolderTreeItemDataBase* itemParentBase = GetItemData( itemParent );
	RED_FATAL_ASSERT( itemParentBase->GetNodeType() == FTNodeType_Dir || itemParentBase->GetNodeType() == FTNodeType_Root, "Character parent is not a directory or root" );
	CEdCFolderTreeItemDataDirBase* itemParentDirBase = static_cast< CEdCFolderTreeItemDataDirBase* >( itemParentBase );
	return itemParentDirBase->GetDirectory();
}


void CEdCharacterFolderTree::GatherCharactersForSelectedNode( TDynArray< CCharacter* >& characters )
{
	wxTreeItemId selection = GetSelection();
	if ( selection.IsOk() )
	{
		GatherAllCharacters( characters, selection );
	}
}

void CEdCharacterFolderTree::GatherAllCharacters( TDynArray< CCharacter* >& characters, wxTreeItemId item )
{

	CEdCFolderTreeItemDataBase* dataBase = GetItemData( item );
	switch ( dataBase->GetNodeType() )
	{
	case FTNodeType_Root:
	case FTNodeType_Dir:
	case FTNodeType_File:
		{
			wxTreeItemIdValue cookie;
			wxTreeItemId child = GetFirstChild( item, cookie );

			while( child.IsOk() )
			{
				GatherAllCharacters( characters, child );

				child = GetNextChild( item, cookie );
			}
		}
		break; 

	case FTNodeType_Character:
		{
			CEdCFolderTreeItemDataCharacter* dataChar = static_cast< CEdCFolderTreeItemDataCharacter* >( dataBase );
			characters.PushBack( dataChar->GetCharacterData() );
		}
		break;
	}
}

Bool CEdCharacterFolderTree::SelectionChanged( const wxTreeItemId& item )
{
	m_popupMenuItem = item;
	return m_popupMenuItem.IsOk();
}

CCharacter* CEdCharacterFolderTree::GetSelectedCharacter()
{
	if ( m_popupMenuItem.IsOk() )
	{
		CEdCFolderTreeItemDataBase* itemData = GetItemData( m_popupMenuItem );
		if ( itemData->GetNodeType() == FTNodeType_Character )
		{
			CEdCFolderTreeItemDataCharacter* charItemData = static_cast< CEdCFolderTreeItemDataCharacter* > ( itemData );
			return charItemData->GetCharacterData();
		}
	}
	return nullptr;
}

void CEdCharacterFolderTree::SelectCharacterItem( CCharacter* character )
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
		CEdCFolderTreeItemDataCharacter** itemData = m_itemMap.FindPtr( character->GetGUID() );
		RED_FATAL_ASSERT( itemData, "Character should be in map" );
		RED_FATAL_ASSERT( *itemData, "Character should have valid node data" );


		if ( ( *itemData )->GetId().IsOk() )
		{
			SelectItem( ( *itemData )->GetId() );
		}
	}
}

void CEdCharacterFolderTree::OnRightClick( wxTreeEvent& event )
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
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );
		
	CEdCFolderTreeItemDataBase* itemData = GetItemData( m_popupMenuItem );
	itemData->PopulateMenuOptions( m_popupMenuItem, m_popupMenu );

	PopupMenu( m_popupMenu );
}

wxTreeItemId CEdCharacterFolderTree::CreateDirectory( const wxTreeItemId& item, CDirectory* newDir )
{
	RED_FATAL_ASSERT( newDir, "Can't create directory node without valid directory" );
	RED_FATAL_ASSERT( item.IsOk(), "Need to have valid node to add directory node" );
	
	CEdCFolderTreeItemDataDir* itemData = new CEdCFolderTreeItemDataDir( newDir );
	wxTreeItemId itemId = wxTreeCtrl::AppendItem( item, itemData->GetName(), m_dirIcon, m_dirIcon, itemData );
	return itemId;
}

wxTreeItemId CEdCharacterFolderTree::CreateResource( const wxTreeItemId& item, CCharacterResource* resource, const String& name  )
{
	CEdCFolderTreeItemDataFile* itemData = new CEdCFolderTreeItemDataFile( resource, String( name ) );
	wxTreeItemId itemId = wxTreeCtrl::AppendItem( item, itemData->GetName(), m_resIcon, m_resIcon, itemData );
	UpdateColours( itemData );
	return itemId;
}

wxTreeItemId CEdCharacterFolderTree::CreateCharacter( const wxTreeItemId& item, const String& name )
{
	CEdCFolderTreeItemDataBase* baseData = GetItemData( item );

	RED_FATAL_ASSERT( GetItemData( item )->GetNodeType() == FTNodeType_File, "Creating character allowed only on file node type" );
	CEdCFolderTreeItemDataFile* fileData = static_cast< CEdCFolderTreeItemDataFile* >( baseData );
	CCharacterResource* resource = fileData->GetResource();

	if ( !CheckOutFile( item, true ) )
	{
		return wxTreeItemId();
	}

	CCharacter* resData = ::CreateObject< CCharacter >( resource );
	resData->SetName( CName( name ) );
	resource->GetCharactersData().PushBack( resData );

	CEdCFolderTreeItemDataCharacter* itemData = new CEdCFolderTreeItemDataCharacter( resData );
	m_itemMap.Insert( resData->GetGUID(), itemData );

	wxTreeItemId itemId = wxTreeCtrl::AppendItem( item, itemData->GetName(), m_itemIcon, m_itemIcon, itemData );

	MarkItemModified( itemId );

	m_im->AddCharacter( resData );

	return itemId;
}

Bool CEdCharacterFolderTree::SaveItem( const wxTreeItemId& item, Bool submit, TDynArray< CCharacterResource* >& failedToSaveItems )
{
	CEdCFolderTreeItemDataBase* itemData = static_cast< CEdCFolderTreeItemDataBase* >( GetItemData( item ) );

	if ( itemData->GetState() != CEdCFolderTreeItemDataDirBase::State_Modified 
		&& ( itemData->GetState() != CEdCFolderTreeItemDataDirBase::State_ReadyForSourceControl || !submit ) )
	{
		return true;
	}

	switch ( itemData->GetNodeType() )
	{
	case FTNodeType_Root:
		{
			CEdCFolderTreeItemDataRoot* rootItemData = static_cast< CEdCFolderTreeItemDataRoot* > ( itemData );
			SaveDirectory( rootItemData, submit, failedToSaveItems );
		}
		break;

	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDir* dirItemData = static_cast< CEdCFolderTreeItemDataDir* > ( itemData );
			SaveDirectory( dirItemData, submit, failedToSaveItems );
		}
		break;

	case FTNodeType_File:
		{
			CEdCFolderTreeItemDataFile* fileItemData = static_cast< CEdCFolderTreeItemDataFile* > ( itemData );

			RED_FATAL_ASSERT( GetChildrenCount( item ) == fileItemData->GetResource()->GetCharactersData().Size(), "Node should have one child per character" );

			CCharacterResource* resource = fileItemData->GetResource();
			CDirectory* dir = GetParentDirectory( item );
			if ( !SaveResource( resource, fileItemData->GetName(), dir, submit ) )
			{
				failedToSaveItems.PushBack( resource );
			}
			else
			{
				UpdateColours( fileItemData );
			}
		}
		break;

	case FTNodeType_Character:
		{
			RED_FATAL_ASSERT( true, "Can't save single character");
		}
	}
	return failedToSaveItems.Empty();
}

Bool CEdCharacterFolderTree::SaveDirectory( CEdCFolderTreeItemDataDirBase* nodeData, Bool submit, TDynArray< CCharacterResource* >& failedToSaveItems )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( nodeData->GetId(), cookie );

	while( child.IsOk() )
	{
		SaveItem( child, submit, failedToSaveItems );

		child = GetNextChild( nodeData->GetId(), cookie );
	}

	return failedToSaveItems.Empty();
}

Bool CEdCharacterFolderTree::SaveResource( CCharacterResource* resource, const Char* name, CDirectory* dir, Bool submit )
{
	CDiskFile* file = resource->GetFile();
	if( file )
	{
		resource->Save();
	}
	else
	{
		RED_FATAL_ASSERT( name,  "No filename specified" );
		if( !name )
		{
			return false;
		}

		String filename = String::Printf( TXT( "%s.%s" ), name, CCharacterResource::GetFileExtension() );

		if ( !resource->SaveAs( dir, name ) )
		{
			return false;
		}
	}

	file = resource->GetFile();

	if( file )
	{
		// update source control status
		file->GetStatus();

		// Add it to source control if it isn't already
		if( file->IsLocal() )
		{
			file->Add();
		}
	}
	if ( submit )
	{
		return file->Submit();
	}

	return true;
}

Bool CEdCharacterFolderTree::RevertItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToRevertItems )
{
	CEdCFolderTreeItemDataBase* itemData = static_cast< CEdCFolderTreeItemDataBase* >( GetItemData( item ) );

	if ( itemData->GetState() == CEdCFolderTreeItemDataDirBase::State_Unmodified )
	{
		return true;
	}

	switch ( itemData->GetNodeType() )
	{
	case FTNodeType_Root:
		{
			CEdCFolderTreeItemDataRoot* rootItemData = static_cast< CEdCFolderTreeItemDataRoot* > ( itemData );
			RevertDirectory( rootItemData, failedToRevertItems );
		}
		break;

	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDir* dirItemData = static_cast< CEdCFolderTreeItemDataDir* > ( itemData );
			RevertDirectory( dirItemData, failedToRevertItems );
		}
		break;

	case FTNodeType_File:
		{
			CEdCFolderTreeItemDataFile* fileItemData = static_cast< CEdCFolderTreeItemDataFile* > ( itemData );

			RED_FATAL_ASSERT( GetChildrenCount( item ) == fileItemData->GetResource()->GetCharactersData().Size(), "Node should have one child per character" );

			CCharacterResource* resource = fileItemData->GetResource();

			if ( resource->GetFile()->IsCheckedOut() )
			{
				if ( !resource->GetFile()->Revert() )
				{
					failedToRevertItems.PushBack( resource );
				}
				else
				{
					UpdateTree( item );
				}
			}
		}
		break;

	case FTNodeType_Character:
		{
			RED_FATAL_ASSERT( true, "Can't revert single character" );
		}
	}
	return failedToRevertItems.Empty();
}

Bool CEdCharacterFolderTree::RevertDirectory( CEdCFolderTreeItemDataDirBase* nodeData, TDynArray< CCharacterResource* >& failedToRevertItems )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( nodeData->GetId(), cookie );

	while( child.IsOk() )
	{
		RevertItem( child, failedToRevertItems );

		child = GetNextChild( nodeData->GetId(), cookie );
	}

	return failedToRevertItems.Empty();
}

Bool CEdCharacterFolderTree::DeleteItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToDeleteItems )
{
	CEdCFolderTreeItemDataBase* itemData = static_cast< CEdCFolderTreeItemDataBase* >( GetItemData( item ) );

	switch ( itemData->GetNodeType() )
	{
	case FTNodeType_Root:
		{
			CEdCFolderTreeItemDataRoot* rootItemData = static_cast< CEdCFolderTreeItemDataRoot* > ( itemData );
			DeleteDirectoryContent( rootItemData, failedToDeleteItems, false );
		}
		break;

	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDir* dirItemData = static_cast< CEdCFolderTreeItemDataDir* > ( itemData );
			DeleteDirectoryContent( dirItemData, failedToDeleteItems, true );
		}
		break;

	case FTNodeType_File:
		{
			CEdCFolderTreeItemDataFile* fileItemData = static_cast< CEdCFolderTreeItemDataFile* > ( itemData );

			RED_FATAL_ASSERT( GetChildrenCount( item ) == fileItemData->GetResource()->GetCharactersData().Size(), "Node should have on child per character" );

			CCharacterResource* resource = fileItemData->GetResource();

			if ( resource->GetFile() && !resource->GetFile()->Delete() )
			{
				failedToDeleteItems.PushBack( resource );
			}
			else
			{
				m_resourceContainer->RemoveResource( resource );

				wxTreeItemId parent = GetItemParent( item );

				Delete( item );

				UpdateDirState( parent );		
				
			}
		}
		break;

	case FTNodeType_Character:
		{
			RED_FATAL_ASSERT( true, "Can't remove from SC single character" );
		}
		break;
	}

	return failedToDeleteItems.Empty();
}

Bool CEdCharacterFolderTree::DeleteDirectoryContent( CEdCFolderTreeItemDataDirBase* itemData, TDynArray< CCharacterResource* >& failedToDeleteItems, Bool deleteNode )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( itemData->GetId(), cookie );

	while( child.IsOk() )
	{
		DeleteItem( child, failedToDeleteItems );

		child = GetNextChild( itemData->GetId(), cookie );
	}

	if ( deleteNode )
	{
		CDirectory* dir = itemData->GetDirectory();
		RED_FATAL_ASSERT( dir, "Every directory node should have valid directory" );
		CDirectory* parentDir = dir->GetParent();
		if ( parentDir->DeleteChildDirectory( dir ) )
		{
			wxTreeItemId parent = GetItemParent( itemData->GetId() );

			Delete( itemData->GetId() );

			UpdateDirState( parent );		
		}
	}

	return failedToDeleteItems.Empty();
}

void CEdCharacterFolderTree::MarkModified( CCharacter* character )
{
	if ( !character )
	{
		return;
	}
	CEdCFolderTreeItemDataCharacter** characterNodeData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( characterNodeData, "Character should be in map" );
	RED_FATAL_ASSERT( *characterNodeData, "Character should have valid node data" );


	MarkItemModified( ( *characterNodeData )->GetId() );
}

void CEdCharacterFolderTree::MarkItemModified( const wxTreeItemId& item )
{
	RED_FATAL_ASSERT( item.IsOk(), "Can't mark invalid node" );

	CEdCFolderTreeItemDataBase* itemData = GetItemData( item );

	if( itemData->GetState() != CEdCFolderTreeItemDataBase::State_Modified )
	{
		itemData->SetState( CEdCFolderTreeItemDataBase::State_Modified );

		if ( itemData->GetNodeType() == FTNodeType_Character )
		{
			SetItemTextColour( item, TREE_COLOUR_EDITED_CHILD );
		}
		else
		{
			SetItemTextColour( item, TREE_COLOUR_EDITED );
		}

		wxTreeItemId parentItem = GetItemParent( item );
		if ( parentItem.IsOk() )
		{
			MarkItemModified( parentItem );
		}
	}	
}

void CEdCharacterFolderTree::GetColour( CEdCFolderTreeItemDataBase::EState state, wxColour& colour, wxColour& childColour )
{
	colour = TREE_COLOUR_UNMODIFIED;
	childColour = TREE_COLOUR_UNMODIFIED;

	switch ( state )
	{
	case CEdCFolderTreeItemDataBase::State_Unmodified:
		colour = TREE_COLOUR_UNMODIFIED;
		childColour = TREE_COLOUR_UNMODIFIED;
		break;

	case CEdCFolderTreeItemDataBase::State_Locked:
		colour = TREE_COLOUR_LOCKED;
		childColour = TREE_COLOUR_LOCKED_CHILD;
		break;

	case CEdCFolderTreeItemDataBase::State_ReadyForSourceControl:
		colour = TREE_COLOUR_SAVED;
		childColour = TREE_COLOUR_SAVED_CHILD;
		break;

	case CEdCFolderTreeItemDataBase::State_Modified:
		colour = TREE_COLOUR_EDITED;
		childColour = TREE_COLOUR_EDITED_CHILD;
		break;

	default:
		break;
	}
}

void CEdCharacterFolderTree::UpdateColours( CEdCFolderTreeItemDataFile* itemData )
{
	itemData->UpdateState();

	wxColour colour;
	wxColour childColour;

	GetColour( itemData->GetState(), colour, childColour );
	
	SetItemTextColour( itemData->GetId(), colour );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( itemData->GetId(), cookie );

	// change children state
	while( child.IsOk() )
	{
		SetItemTextColour( child, childColour );

		CEdCFolderTreeItemDataBase* childData = static_cast< CEdCFolderTreeItemDataBase* >( GetItemData( child ) );
		RED_FATAL_ASSERT( childData->GetNodeType() == FTNodeType_Character, "Resource can only have character type child" );
		childData->SetState( itemData->GetState() );

		child = GetNextChild( itemData->GetId(), cookie );
	}

	// update parent state
	wxTreeItemId parent = GetItemParent( itemData->GetId() );
	UpdateDirState( parent );
}

CEdCFolderTreeItemDataBase::EState CEdCharacterFolderTree::GetChildrenHighestState( const wxTreeItemId& item )
{
	CEdCFolderTreeItemDataBase::EState itemNewState = CEdCFolderTreeItemDataBase::State_Unmodified;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );

	Bool childernAreLocked = true;

	while( child.IsOk() )
	{
		CEdCFolderTreeItemDataBase* childData = static_cast< CEdCFolderTreeItemDataBase* >( GetItemData( child ) );

		// directory node state is the highest state of its children
		if ( childData->GetState() > itemNewState )
		{
			itemNewState = childData->GetState();
		}
		childernAreLocked &= ( childData->GetState() == CEdCFolderTreeItemDataBase::State_Locked );
		child = GetNextChild( item, cookie );
	}

	if ( GetChildrenCount( item ) > 0 && childernAreLocked )
	{
		RED_FATAL_ASSERT( itemNewState == CEdCFolderTreeItemDataBase::State_Unmodified, "If all children are locked new state should be unmodified" );
		itemNewState = CEdCFolderTreeItemDataBase::State_Locked;
	}

	return itemNewState;
}

void CEdCharacterFolderTree::UpdateDirState( wxTreeItemId item )
{
	Bool needUpdate = true;
	while( item.IsOk() && needUpdate )
	{
		CEdCFolderTreeItemDataBase* itemData = GetItemData( item );
		RED_FATAL_ASSERT( itemData->GetNodeType() == FTNodeType_Root || itemData->GetNodeType() == FTNodeType_Dir, "Resource can only have root or directory type child" );

		CEdCFolderTreeItemDataBase::EState itemNewState = GetChildrenHighestState( item );
		
		if ( itemNewState == itemData->GetState() )
		{
			needUpdate = false;
		}
		else
		{
			wxColour colour;
			wxColour childColour;

			GetColour( itemNewState, colour, childColour );

			SetItemTextColour( item, colour );

			itemData->SetState( itemNewState );
		}

		item = GetItemParent( item );
	}
}

void CEdCharacterFolderTree::OnCreateDirectory( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	CDirectory* dir = GetDirectory( m_popupMenuItem );
	RED_FATAL_ASSERT( dir, "Create directory option is allowed only on directory node type" );

	String name( TXT("new folder") );
	String message = TXT("Directory name");
	CDirectory* newDir = nullptr;

	while ( !newDir )
	{
		bool ok = InputBox( this, TXT("Directory name"), message, name );
		if ( !ok )
		{
			return;
		}

		if ( dir->FindLocalDirectory( name.AsChar() ) )
		{
			message = String::Printf( TXT("Directory with a name %s already exists. Give a different name."), name.AsChar() );
			continue;
		}

		newDir = dir->CreateNewDirectory( name );
		newDir->CreateOnDisk();
	}

	wxTreeItemId newItem = CreateDirectory( m_popupMenuItem, newDir );
	SelectItem( newItem );
}

void CEdCharacterFolderTree::OnCreateResource( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	CDirectory* directory = GetDirectory( m_popupMenuItem );
	RED_FATAL_ASSERT( directory, "Creating resource allowed only on directory type node, which always have valid directory" );

	String name( TXT("New file") );

	if ( !GetUserFileName(directory, name, false) )
	{
		return;
	}

	CCharacterResource* resource = m_resourceContainer->CreateResouce();
	wxTreeItemId newItem = CreateResource( m_popupMenuItem, resource, name );
	SelectItem( newItem );
}

void CEdCharacterFolderTree::OnCreateCharacter( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	String name( TXT("Character") );

	if ( !GetUserCharacterName( name, false ) )
	{
		return;
	}

	wxTreeItemId newItem = CreateCharacter( m_popupMenuItem, name );
	if ( newItem.IsOk() )
	{
		SelectItem( newItem );
	}
}

void CEdCharacterFolderTree::OnDelete( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	String confirmationMessage = String( TXT( "Are you sure you want to delete all items?" ) );

	if( wxMessageBox( confirmationMessage.AsChar(), wxT( "Delete" ), wxYES_NO, this ) == wxYES )
	{
		wxTreeItemId parentItem = GetItemParent( m_popupMenuItem );

		TDynArray< CCharacter* > activeCharacters;

		GatherCharactersForSelectedNode( activeCharacters );

		for ( auto it = activeCharacters.Begin(); it != activeCharacters.End(); ++it )
		{
			CCharacter* character = *it;
			if ( !m_im->CheckOutInheritanceTree( character ) )
			{
				return;
			}
		}

		for ( auto it = activeCharacters.Begin(); it != activeCharacters.End(); ++it )
		{
			CCharacter* character = *it;
			m_itemMap.Erase( character->GetGUID() );
			m_im->UpdateInheritanceForRemovedCharacter( character );
		}

		TDynArray< CCharacterResource* > failedToDeleteItems;

		if ( !DeleteItem( m_popupMenuItem, failedToDeleteItems ) )
		{
			String msg( TXT("Can't delete resources:\n") );
			for ( auto it = failedToDeleteItems.Begin(); it != failedToDeleteItems.End(); ++it )
			{
				msg = String::Printf( TXT("%s\n %s"), (*it)->GetDepotPath());
			}

			wxMessageBox( msg.AsChar(), wxT( "Not deleted" ), wxOK, this );
		}
		else
		{
			SelectItem( parentItem );
			m_popupMenuItem = parentItem;
		}
		m_im->UpdateTree();
	}
}

void CEdCharacterFolderTree::OnDeleteCharacter( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );
	wxTreeItemId parentItem = GetItemParent( m_popupMenuItem );

	CEdCFolderTreeItemDataBase* itemData = GetItemData( m_popupMenuItem );
	RED_FATAL_ASSERT( itemData->GetNodeType() == FTNodeType_Character, "Deleting character allowed only on character type node" );

	CEdCFolderTreeItemDataCharacter* charItemData = static_cast< CEdCFolderTreeItemDataCharacter* > ( itemData );
	CCharacter* character = charItemData->GetCharacterData();
	if ( !m_im->CheckOutInheritanceTree( character ) )
	{
		return;
	}
	DeleteCharacter( character );
	m_im->UpdateInheritanceForRemovedCharacter( character );

	SelectItem( parentItem );
	m_popupMenuItem = parentItem;
}

void CEdCharacterFolderTree::OnCheckOut( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	CheckOutItem( m_popupMenuItem );
}

void CEdCharacterFolderTree::OnSave( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	SaveOrShowFaileMessage( m_popupMenuItem, false );
}

void CEdCharacterFolderTree::OnSaveAndCheckIn( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	SaveOrShowFaileMessage( m_popupMenuItem, true );
}

Bool CEdCharacterFolderTree::SaveOrShowFaileMessage( const wxTreeItemId& item, Bool submit)
{
	RED_FATAL_ASSERT( item.IsOk(), "Menu item should be right, we got an event from it" );

	TDynArray< CCharacterResource* > failedToSaveItems;

	if ( !SaveItem( item, submit, failedToSaveItems ) )
	{
		String msg( TXT("Can't save resources:\n") );
		for ( auto it = failedToSaveItems.Begin(); it != failedToSaveItems.End(); ++it )
		{
			msg = String::Printf( TXT("%s\n %s"), msg, (*it)->GetDepotPath());
		}

		wxMessageBox( msg.AsChar(), wxT( "Not saved" ), wxOK, this );

		return false;
	}
	return true;
}

void CEdCharacterFolderTree::OnRevert( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we get an event from it" );

	String confirmationMessage = String( TXT( "Are you sure you want to revert all items?" ) );

	if( wxMessageBox( confirmationMessage.AsChar(), wxT( "Revert" ), wxYES_NO, this ) == wxYES )
	{
		TDynArray< CCharacterResource* > failedToRevertItems;

		if ( !RevertItem( m_popupMenuItem, failedToRevertItems ) )
		{
			String msg( TXT("Can't revert resources:\n") );
			for ( auto it = failedToRevertItems.Begin(); it != failedToRevertItems.End(); ++it )
			{
				msg = String::Printf( TXT("%s\n %s"), (*it)->GetDepotPath());
			}

			wxMessageBox( msg.AsChar(), wxT( "Not saved" ), wxOK, this );
		}
	}
	m_im->UpdateTree();
}


Bool CEdCharacterFolderTree::GetUserFileName( CDirectory* dir, String& name, Bool acceptOldName )
{
	RED_FATAL_ASSERT( dir, "Can't get file from non-existing directory" );

	String message = TXT("Resource name");
	String newName = name;

	Bool validFileName = false;
	while ( !validFileName )
	{
		bool ok = InputBox( this, TXT("Resource name"), message, newName );
		if ( !ok )
		{
			return false;
		}
		if ( acceptOldName && name == newName)
		{
			return false;
		}

		String fileName = String::Printf( TXT("%s.%s"), newName.AsChar(), CCharacterResource::GetFileExtension() );
		if ( dir->FindLocalFile( fileName ) )
		{
			message = String::Printf( TXT("File with a name %s already exists. Give a different name."), newName.AsChar() );
			continue;
		}

		name = newName;
		validFileName = true;
	}
	return true;
}

Bool CEdCharacterFolderTree::GetUserCharacterName( String& name, Bool acceptOldName )
{
	String message = TXT("Character name");
	String newName = name;

	Bool validName = false;
	while ( !validName )
	{
		bool ok = InputBox( this, TXT("Character name"), message, newName );
		if ( !ok )
		{
			return false;
		}

		if ( acceptOldName && name == newName)
		{
			return false;
		}

		if ( m_resourceContainer->FindFirstCharacterWithName( CName( newName ) ) )
		{
			message = String::Printf( TXT("Character with a name %s already exists. Give a different name."), newName.AsChar() );
			continue;
		}

		name = newName;
		validName = true;
	}
	return true;
}

void CEdCharacterFolderTree::OnRenameCharacter( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	CEdCFolderTreeItemDataBase* dataBase = GetItemData( m_popupMenuItem );
	RED_FATAL_ASSERT ( dataBase->GetNodeType() == FTNodeType_Character , "Rename character can be done only on character node");
	CEdCFolderTreeItemDataCharacter* characterNodeData = static_cast< CEdCFolderTreeItemDataCharacter* >( dataBase );
	RED_FATAL_ASSERT( characterNodeData, "Character type node should have character node data" );
	CCharacter* character = characterNodeData->GetCharacterData();
	RED_FATAL_ASSERT( character, "Every character node should has valid character" );
	String name = character->GetName().AsString();

	if ( !GetUserCharacterName( name, true ) )
	{
		return;
	}

	RenameCharacter( characterNodeData, name );
}

void CEdCharacterFolderTree::OnRenameResource( wxEvent& event )
{
	RED_FATAL_ASSERT( m_popupMenuItem.IsOk(), "Menu item should be right, we got an event from it" );

	CEdCFolderTreeItemDataBase* dataBase = GetItemData( m_popupMenuItem );
	RED_FATAL_ASSERT ( dataBase->GetNodeType() == FTNodeType_File, "Renaming resource is allowed only on resource type node" );
	CEdCFolderTreeItemDataFile* fileNodeData = static_cast< CEdCFolderTreeItemDataFile* >( dataBase );
	String name = fileNodeData->GetName();

	CDirectory* directory = GetParentDirectory( fileNodeData->GetId() );
	RED_FATAL_ASSERT( directory, "Every directory node should have valid directory" );

	if ( !GetUserFileName( directory, name, true ) )
	{
		return;
	}

	RenameResource( fileNodeData, name );
}


Bool CEdCharacterFolderTree::CheckOutItem( const wxTreeItemId& item )
{
	TDynArray< CCharacterResource* > failedToCheckOutItems;

	if ( !CheckOutItem( m_popupMenuItem, failedToCheckOutItems ) )
	{
		String msg( TXT("Can't check out resources:\n") );
		for ( auto it = failedToCheckOutItems.Begin(); it != failedToCheckOutItems.End(); ++it )
		{
			msg = String::Printf( TXT("%s\n %s"), (*it)->GetDepotPath());
		}

		wxMessageBox( msg.AsChar(), wxT( "Not checked out" ), wxOK, this );
		return false;
	}
	return true;
}

Bool CEdCharacterFolderTree::CheckOutItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToSaveItems )
{
	CEdCFolderTreeItemDataBase* baseData = GetItemData( item );
	
	switch ( baseData->GetNodeType() )
	{
	case FTNodeType_Root:
	case FTNodeType_Dir:
		{
			CEdCFolderTreeItemDataDirBase* dirBaseData = static_cast< CEdCFolderTreeItemDataDirBase* >( baseData );
			
			return CheckOutDirectory( dirBaseData, failedToSaveItems );
		}

	case FTNodeType_File:
		return CheckOutFile( item, failedToSaveItems );

	case FTNodeType_Character:
		return CheckOutFile( GetItemParent( item ), failedToSaveItems );
	}
	return false;
}

Bool CEdCharacterFolderTree::CheckOutDirectory( CEdCFolderTreeItemDataDirBase* dirBaseData, TDynArray< CCharacterResource* >& failedToSaveItems )
{
	CDirectory* dir = dirBaseData->GetDirectory();
	RED_FATAL_ASSERT( dir, "Every directory base node should have valid directory" );

	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( dirBaseData->GetId(), cookie );

	while( child.IsOk() )
	{
		CheckOutItem( child, failedToSaveItems );

		child = GetNextChild( dirBaseData->GetId(), cookie );
	}

	return failedToSaveItems.Empty();
}

Bool CEdCharacterFolderTree::CheckOutFile( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToSaveItems )
{
	CEdCFolderTreeItemDataBase* baseData = GetItemData( item );
	RED_FATAL_ASSERT( baseData->GetNodeType() == FTNodeType_File, "Node is not a resource node, can't check out file" );
	CEdCFolderTreeItemDataFile* fileItemData = static_cast< CEdCFolderTreeItemDataFile* > ( baseData );

	CCharacterResource* resource = fileItemData->GetResource();
	RED_FATAL_ASSERT( resource, "Every file node should have valid resource" );
	if ( resource->CanModify() )
	{
		return true;
	}
	RED_FATAL_ASSERT( resource && resource->GetFile(), "Check out option for item without file" );

	if ( resource->GetFile()->CheckOut() )
	{
		UpdateColours( fileItemData );

		return true;
	}
	failedToSaveItems.PushBack( resource );

	return false;
}

Bool CEdCharacterFolderTree::CheckOutFile( const wxTreeItemId& item, Bool failedMessage )
{
	TDynArray< CCharacterResource* > failedToSaveItems;
	if ( !CheckOutFile( item, failedToSaveItems ) )
	{
		if ( failedMessage )
		{
			String msg = String::Printf( TXT("Can't check out file \"%s\""), failedToSaveItems[0]->GetDepotPath() );
			wxMessageBox( msg.AsChar(), wxT( "Failed" ), wxOK, this );
		}
		return false;
	}
	return true;
}

Bool CEdCharacterFolderTree::CheckOutCharacter( CCharacter* character, Bool failedMessage )
{
	CEdCFolderTreeItemDataCharacter** characterNodeData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( characterNodeData, "Character should be in map" );
	RED_FATAL_ASSERT( *characterNodeData, "Character should have valid node data" );

	wxTreeItemId itemParent = GetItemParent( ( *characterNodeData )->GetId() );

	return CheckOutFile( itemParent, failedMessage );
}

Bool CEdCharacterFolderTree::RenameResource( CEdCFolderTreeItemDataFile* fileItemData, const String& newName )
{
	if ( !fileItemData->Rename( newName ) )
	{
		wxMessageBox( TXT("Renaming file failed."), wxT( "Failed" ), wxOK, this );
		return false;
	}

	SetItemText( fileItemData->GetId(), newName.AsChar() );
	return true;
}

void CEdCharacterFolderTree::RenameCharacter( CEdCFolderTreeItemDataCharacter* characterNodeData, const String& newName )
{
	RED_FATAL_ASSERT( characterNodeData, "Can't rename character based on invalid node data" );

	wxTreeItemId itemParent = GetItemParent( characterNodeData->GetId() );
	CEdCFolderTreeItemDataBase* itemParentBase = GetItemData( itemParent );
	RED_FATAL_ASSERT( itemParentBase->GetNodeType() == FTNodeType_File, "Character parent is not a resource in folder tree" );
	CEdCFolderTreeItemDataFile* itemParentFile = static_cast< CEdCFolderTreeItemDataFile* >( itemParentBase );
	RED_FATAL_ASSERT( itemParentFile, "File node should have file node data" );

	if ( !CheckOutFile( itemParentFile->GetId(), true ) )
	{
		return;
	}

	CCharacter* character = characterNodeData->GetCharacterData();
	character->SetName( CName( newName ) );
	SetItemText( characterNodeData->GetId(), newName.AsChar() );
	MarkItemModified( characterNodeData->GetId() );

	GetEventHandler()->ProcessEvent( wxCommandEvent( wXEVT_TREE_CHARACTER_NAME_CHANGED, GetId() ) );

	m_im->RenameCharacter( character );
}

Bool CEdCharacterFolderTree::DeleteCharacter( CCharacter* character )
{
	CEdCFolderTreeItemDataCharacter** characterNodeData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( characterNodeData, "Character should be in map" );

	return DeleteCharacter( *characterNodeData );
}

Bool CEdCharacterFolderTree::DeleteCharacter( CEdCFolderTreeItemDataCharacter* characterNodeData )
{
	RED_FATAL_ASSERT( characterNodeData, "Can't delete character based on invalid data" );
	CCharacterResource* resource = GetParentResource( characterNodeData->GetId() );
	RED_FATAL_ASSERT( resource, "Character should has File node as a parent" );
	RED_FATAL_ASSERT( resource->CanModify(), "Resource has to be checked out before deleting character from it" );

	CCharacter* character = characterNodeData->GetCharacterData();
	RED_FATAL_ASSERT( character, "Every character node should has valid character" );

	resource->GetCharactersData().Remove( character );

	m_itemMap.Erase( character->GetGUID() );

	MarkItemModified( characterNodeData->GetId() );

	Delete( characterNodeData->GetId() );

	return true;
}

void CEdCharacterFolderTree::CharacterModified( CCharacter* character )
{
	CEdCFolderTreeItemDataCharacter** characterNodeData = m_itemMap.FindPtr( character->GetGUID() );
	RED_FATAL_ASSERT( characterNodeData, "Character should be in map" );
	RED_FATAL_ASSERT( *characterNodeData, "Character should has valid data" );

	MarkItemModified( ( *characterNodeData )->GetId() );
}