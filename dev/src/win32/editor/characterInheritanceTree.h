#pragma once

#include "../../../../games/r6/r6GameResource.h"
#include "../../../../common/game/characterResource.h"
#include "../../../../common/game/character.h"
#include "characterDBEdInterfaces.h"

// Data that is stored in every tree node
class CEdCInheritanceTreeItemData : public wxTreeItemData
{
protected:
	CCharacter* m_character;

public:
	CEdCInheritanceTreeItemData( CCharacter* character );

	CEdCInheritanceTreeItemData( const CEdCInheritanceTreeItemData* other );

	RED_INLINE CCharacter* GetCharacter() { return m_character; }
};

// Tree that shows and manages operation related to character inheritance  
class CEdCharacterInheritanceTree : public wxTreeCtrl, public IEdCharacterDBInheritanceManagement
{
	DECLARE_DYNAMIC_CLASS( CEdCharacterInheritanceTree )

private:
	// tree node icon
	Int32 m_icon;
	// map : character guid -> tree node data
	THashMap< CGUID, CEdCInheritanceTreeItemData* > m_itemMap;

	// resources that are shown by tree
	CEdCharacterResourceContainer* m_resContainer;

	// interface to perform operation on files etc
	IEdCharacterDBFolderManagement* m_fm;

	// When on tree can't change data
	Bool m_readOnlyMode;

	// drag&drop info
	wxTreeItemId m_draggedItem;
	wxTreeItemId m_draggedItemOldParent;

	// context menu data
	wxMenu* m_popupMenu;
	wxTreeItemId m_popupMenuItem;

public:
	CEdCharacterInheritanceTree();
	~CEdCharacterInheritanceTree();
	void Initialize( IEdCharacterDBFolderManagement* sc, Bool readOnlyMode = false );

	// Creates tree
	void PopulateTree( CEdCharacterResourceContainer& resContainer );

	// Inserts into array selected node character with all character from subtree
	void GatherCharactersForSelectedNode( TDynArray< CCharacter* >& characters );
	// Selects character
	void SelectCharacterItem( CCharacter* character );
	// Returns selected character
	CCharacter* GetSelectedCharacter();
	// Updates info about selected node
	Bool SelectionChanged( const wxTreeItemId& item );

	// IEdCharacterDBInheritanceManagement implementation
	void UpdateInheritanceForRemovedCharacter( CCharacter* character );
	Bool CheckOutInheritanceTree( CCharacter* character, const CProperty* prop = nullptr );
	void AddCharacter( CCharacter* character );
	void RenameCharacter( CCharacter* character );
	void UpdateTree();
	void GetInheritanceTree( CCharacter* parent, const CProperty* property, TDynArray< CCharacter* >& editingCharacters );

private:
	// Creates context menu
	void PopulateMenuOptions( const wxTreeItemId& item, wxMenu* menu );
	// Creates tree root
	wxTreeItemId AddTreeRoot();
	// Helper method to get noda data
	CEdCInheritanceTreeItemData* GetItemData( const wxTreeItemId& item );
	// Helper method to get character from node
	CCharacter* GetItemCharacter( const wxTreeItemId& item );

	// Adds character node
	wxTreeItemId AddCharacterToTheTree( CEdCInheritanceTreeItemData* characterData );

	// Copy children from one node to another
	void CopyChildren( const wxTreeItemId& sourceItem, const wxTreeItemId& destItem );

	// Puts on the list all characters from the subtree
	void GatherAllCharacters( TDynArray< CCharacter* >& characters, wxTreeItemId item );

	// Removes subtree
	void RemoveSubTree( const wxTreeItemId& item );
	// Removes single node
	void RemoveNode( const wxTreeItemId& item );

	// Checks out single node
	Bool CheckOutNode( const wxTreeItemId& item );
	// Checks out whole subtree
	Bool CheckOutSubTree( const wxTreeItemId& item, const CProperty* prop = nullptr );

	// Handles tree actions
	void OnRightClick( wxTreeEvent& event );
	void OnTreeDrag( wxTreeEvent& event );
	void OnTreeDrop( wxTreeEvent& event );
	void OnDeleteNode( wxEvent& event );
	void OnDeleteSubtree( wxEvent& event );
	void OnCheckOutNode( wxEvent& event );
	void OnCheckOutSubtree( wxEvent& event );
};

