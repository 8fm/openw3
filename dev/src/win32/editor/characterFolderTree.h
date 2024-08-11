#pragma once

#include "../../../../games/r6/r6GameResource.h"
#include "../../../../common/game/characterResource.h"
#include "../../../../common/game/character.h"
#include "characterDBEdInterfaces.h"

#include "../../common/core/directory.h"

// Node types
enum EFTNodeType
{
	FTNodeType_Root,
	FTNodeType_Dir,
	FTNodeType_File,
	FTNodeType_Character,
};

// Data that is stored in tree node
class CEdCFolderTreeItemDataBase : public wxTreeItemData
{
public:
	// Source control state
	enum EState
	{
		State_Locked = 0,
		State_Unmodified,
		State_ReadyForSourceControl,
		State_Modified,
	};
	
protected:
	EState m_state;

public:
	CEdCFolderTreeItemDataBase();

	CEdCFolderTreeItemDataBase( const CEdCFolderTreeItemDataBase* other );

	static EState GetFileState( CDiskFile* file );

	virtual const Char* GetName() const = 0;
	virtual EFTNodeType GetNodeType() const = 0;
	RED_INLINE EState GetState() const { return m_state; }
	RED_INLINE void SetState( EState state ) { m_state = state; }

	virtual void PopulateMenuOptions( wxTreeItemId item, wxMenu* menu ) = 0;

};

class CEdCFolderTreeItemDataDirBase : public CEdCFolderTreeItemDataBase
{
protected:
	CDirectory* m_directory;

public:
	CEdCFolderTreeItemDataDirBase( CDirectory* directory );
	
	CEdCFolderTreeItemDataDirBase( const CEdCFolderTreeItemDataDirBase* other );

	RED_INLINE CDirectory* GetDirectory() { return m_directory; };

	virtual void PopulateMenuOptions( wxTreeItemId item, wxMenu* menu );
};

class CEdCFolderTreeItemDataRoot : public CEdCFolderTreeItemDataDirBase
{

public:
	CEdCFolderTreeItemDataRoot( CDirectory* directory );

	CEdCFolderTreeItemDataRoot( const CEdCFolderTreeItemDataRoot* other );

	RED_INLINE virtual const Char* GetName() const { return m_directory->GetName().AsChar(); }

	RED_INLINE virtual EFTNodeType GetNodeType() const { return FTNodeType_Root; }
};

class CEdCFolderTreeItemDataDir : public CEdCFolderTreeItemDataDirBase
{
protected:
	String m_dirName;

public:
	CEdCFolderTreeItemDataDir( CDirectory* directory );
	CEdCFolderTreeItemDataDir( String& fileName );

	CEdCFolderTreeItemDataDir( const CEdCFolderTreeItemDataDir* other );

	RED_INLINE const Char* GetName() const { return m_dirName.AsChar(); }

	RED_INLINE virtual EFTNodeType GetNodeType() const { return FTNodeType_Dir; }

	virtual void PopulateMenuOptions( wxTreeItemId item, wxMenu* menu );
};

wxDECLARE_EVENT( wXEVT_TREE_CHARACTER_NAME_CHANGED, wxEvent );

class CEdCFolderTreeItemDataFile : public CEdCFolderTreeItemDataBase
{
protected:
	CCharacterResource* m_resource;
	String m_fileName;

public:
	CEdCFolderTreeItemDataFile( CCharacterResource* resource );
	CEdCFolderTreeItemDataFile( CCharacterResource* resource, String& fileName );

	CEdCFolderTreeItemDataFile( const CEdCFolderTreeItemDataFile* other );

	RED_INLINE const Char* GetName() const { return m_fileName.AsChar(); }

	Bool Rename( const String& newName );

	RED_INLINE virtual EFTNodeType GetNodeType() const { return FTNodeType_File; }

	RED_INLINE CCharacterResource* GetResource() { return m_resource; }

	RED_INLINE void UpdateState() { SetState( CEdCFolderTreeItemDataBase::GetFileState( m_resource->GetFile() ) ); }

	virtual void PopulateMenuOptions( wxTreeItemId item, wxMenu* menu );

private:
	void UpdateName();
};

class CEdCFolderTreeItemDataCharacter : public CEdCFolderTreeItemDataBase
{
	CCharacter* m_characterData;

public:
	CEdCFolderTreeItemDataCharacter( CCharacter* characterData );

	CEdCFolderTreeItemDataCharacter( const CEdCFolderTreeItemDataCharacter* other );

	RED_INLINE const Char* GetName() const { return m_characterData->GetName().AsChar(); }

	RED_INLINE virtual EFTNodeType GetNodeType() const { return FTNodeType_Character; }

	RED_INLINE CCharacter* GetCharacterData() { return m_characterData; }

	virtual void PopulateMenuOptions( wxTreeItemId item, wxMenu* menu );
};

//------------------------------------------------------------------------------------------------------------------------------------
// Tree that shows folder structure
class CEdCharacterFolderTree : public wxTreeCtrl, public IEdCharacterDBFolderManagement
{
	DECLARE_DYNAMIC_CLASS( CEdCharacterFolderTree )
private:
	// icons
	Int32 m_dirIcon;
	Int32 m_resIcon;
	Int32 m_itemIcon;

	// context menu data
	wxMenu* m_popupMenu;
	wxTreeItemId m_popupMenuItem;

	// drag&drop data
	wxTreeItemId m_draggedItem;
	wxTreeItemId m_draggedItemOldParent;

	// In this mode tree can't change resources
	Bool m_readOnlyMode;

	// Resource container
	CEdCharacterResourceContainer* m_resourceContainer;

	// map: character guid -> tree node data
	THashMap< CGUID, CEdCFolderTreeItemDataCharacter* > m_itemMap;

	// Interfaces to perform operation related to inheritance
	IEdCharacterDBInheritanceManagement* m_im;

public:
	CEdCharacterFolderTree();
	virtual ~CEdCharacterFolderTree();

	// Initializes tree
	void Initialize( CEdCharacterResourceContainer& resourceContainer, IEdCharacterDBInheritanceManagement* im, Bool readOnlyMode = false );

	// Creates tree root
	wxTreeItemId AddTreeRoot( CDirectory* rootDirectory );

	// Creates tree
	void PopulateTree( CDirectory* rootDirectory );

	// Fins and selects resource on tree
	void ExpandPath( CCharacterResource* characterResource );

	// Puts all characters from subtree on list
	void GatherCharactersForSelectedNode( TDynArray< CCharacter* >& characters );
	
	// Returns selected character if any
	CCharacter* GetSelectedCharacter();
	// Selects character
	void SelectCharacterItem( CCharacter* character );
	// Updates selected node data
	Bool SelectionChanged( const wxTreeItemId& item );

	// IEbCharacterDBSourceControl
	virtual Bool CheckOutCharacter( CCharacter* character, Bool failedMessage );
	virtual Bool DeleteCharacter( CCharacter* character );
	virtual void CharacterModified( CCharacter* character );

	// Saves subtree or shows fail massage
	Bool SaveOrShowFaileMessage( const wxTreeItemId& item, Bool submit );

	// Marks node modified and updates tree state
	void MarkModified( CCharacter* character );

	// Checks if tree is modified
	Bool IsModified();
	
private:
	// Creates nodes
	wxTreeItemId CreateDirectory( const wxTreeItemId& item, CDirectory* newDir );
	wxTreeItemId CreateResource( const wxTreeItemId& item, CCharacterResource* resource, const String& name );
	wxTreeItemId CreateCharacter( const wxTreeItemId& item, const String& name );

	// Deletes nodes
	Bool DeleteCharacter( CEdCFolderTreeItemDataCharacter* characterNodeData );
	Bool DeleteDirectoryContent( CEdCFolderTreeItemDataDirBase* itemData, TDynArray< CCharacterResource* >& failedToDeleteItems, Bool deleteNode );
	Bool DeleteItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToDeleteItems );

	// Selects resource node
	Bool FindAndSelectResourceNode( CCharacterResource* characterResource, const wxTreeItemId& curItem );

	// Puts all characters from subtree on list
	void GatherAllCharacters( TDynArray< CCharacter* >& characters, wxTreeItemId item );

	// Creates tree
	void PopulateTreeRes( CEdCFolderTreeItemDataFile* itemData, CCharacterResource* characterResource );
	void PopulateTreeDirs( CDirectory* directory, wxTreeItemData* treeItem );

	// Recreate tree
	void UpdateTree( const wxTreeItemId& item );

	// Saves nodes data
	Bool SaveItem( const wxTreeItemId& item, Bool submit, TDynArray< CCharacterResource* >& failedToSaveItems );
	Bool SaveDirectory( CEdCFolderTreeItemDataDirBase* nodeData, Bool submit, TDynArray< CCharacterResource* >& failedToSaveItems );
	Bool SaveResource( CCharacterResource* resource, const Char* name, CDirectory* dir, Bool submit );

	// Revers nodes data
	Bool RevertItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToRevertItems );
	Bool RevertDirectory( CEdCFolderTreeItemDataDirBase* nodeData, TDynArray< CCharacterResource* >& failedToRevertItems );

	// Move file to different directory
	wxTreeItemId MoveFile( CEdCFolderTreeItemDataDirBase* newParentData, CEdCFolderTreeItemDataFile* itemData );
	// Move character to different resource
	wxTreeItemId MoveCharacter( CEdCFolderTreeItemDataFile* newParentData, CEdCFolderTreeItemDataCharacter* itemData, const wxTreeItemId& oldParent );

	// Renames nodes items
	void RenameCharacter( CEdCFolderTreeItemDataCharacter* characterNodeData, const String& newName );
	Bool RenameResource( CEdCFolderTreeItemDataFile* fileItemData, const String& newName );
	Bool GetUserCharacterName( String& name, Bool acceptOldName );
	Bool GetUserFileName( CDirectory* dir, String& name, Bool acceptOldName );

	// CHeck outs items
	Bool CheckOutItem( const wxTreeItemId& item );
	Bool CheckOutItem( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToSaveItems );
	Bool CheckOutFile( const wxTreeItemId& item, TDynArray< CCharacterResource* >& failedToSaveItems );
	Bool CheckOutFile( const wxTreeItemId& item, Bool failedMessage );
	Bool CheckOutDirectory( CEdCFolderTreeItemDataDirBase* dirBaseData, TDynArray< CCharacterResource* >& failedToSaveItems );


	// Handles tree events
	void OnRightClick( wxTreeEvent& event );
	void OnCreateDirectory( wxEvent& event );
	void OnCreateResource( wxEvent& event );
	void OnCreateCharacter( wxEvent& event );
	void OnDeleteCharacter( wxEvent& event );
	void OnDelete( wxEvent& event );
	void OnCheckOut( wxEvent& event );
	void OnSave( wxEvent& event );
	void OnRevert( wxEvent& event );
	void OnRenameCharacter( wxEvent& event );
	void OnRenameResource( wxEvent& event );
	void OnSaveAndCheckIn( wxEvent& event );
	void OnTreeDrag( wxTreeEvent& event );
	void OnTreeDrop( wxTreeEvent& event );

	// Marks nodes modified
	void MarkItemModified( const wxTreeItemId& item );

	// Helper function to get right node colour based on node state
	void GetColour( CEdCFolderTreeItemDataBase::EState state, wxColour& colour, wxColour& childColour );
	// Updated file node colour and whole subtree
	void UpdateColours( CEdCFolderTreeItemDataFile* itemData );
	// Updates dir state based on its children states
	void UpdateDirState( wxTreeItemId item );
	// Helper function to get the highest state of children nodes 
	CEdCFolderTreeItemDataBase::EState GetChildrenHighestState( const wxTreeItemId& item );
	
	// Helper functions to get node data
	CEdCFolderTreeItemDataBase* GetItemData( const wxTreeItemId& item );
	CCharacterResource* GetParentResource( const wxTreeItemId& item );
	CDirectory* GetDirectory( const wxTreeItemId& item );
	CDirectory* GetParentDirectory( const wxTreeItemId& item );
};
