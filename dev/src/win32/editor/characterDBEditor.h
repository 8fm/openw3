#pragma once
#include "../../common/game/character.h"
#include "gridEditor.h"

class CEdStringSelector;

class CEdCharacterFolderTree;
class CEdCharacterInheritanceTree;
class CEdCFolderTreeItemDataFile;
class CEdCFolderTreeItemDataCharacter;
class CEdCharacterResourceContainer;

// Editor to manage characters properties
class CEdCharacterDBEditor : public wxSmartLayoutPanel
{
private:

	// Currently shown characters in grid ed
	TDynArray< CCharacter* > m_visibleCharacters;
	// Resource container
	CEdCharacterResourceContainer* m_resourceContainer;
	// Grid editor data needs to be updated
	Bool m_invalidData;

	// Grid editor
	CGridEditor* m_characterGrid;
	// Folder structure tree
	CEdCharacterFolderTree* m_folderTree;
	// Inheritance tree
	CEdCharacterInheritanceTree* m_inheritanceTree;
	// choice if grid editor shows list from folder or inheritance tree
	wxChoice* m_viewChoice;
	// notebook that have trees
	wxNotebook* m_notebook;

	// List of character curretly shown in grid editor
	TDynArray< CCharacter* > m_editingCharactersTree;
	// In read only mode user can't edit data (mainly to set up character in other tools)
	Bool m_readOnlyMode;
	// info if we currently changing slected node in one of the trees
	Bool m_changingSelection;

public:
	CEdCharacterDBEditor( wxWindow* parent, CCharacterResource* characterResource = nullptr, Bool readOnlyMode = false );
	~CEdCharacterDBEditor();

	// Checks if root directory was defined in game resource
	static Bool IsRootDirectoryDefined();
	static CDirectory* GetRootDirectory();

	// Returns selected character (mainly for choosing character from other tools in readOnly mode)
	const CCharacter* GetSelectedCharacter() const;

	RED_INLINE CEdCharacterResourceContainer* GetResourceContainer() { return m_resourceContainer; }

	// Selects character in trees
	void SelectCharacter( CGUID guid );
	
private:
	// Choice if list from folder or inheritance tree should be shown
	enum EViewChoice
	{
		VC_Folder = 0,
		VC_Inhritance
	};
	// event handlers
	void OnExpandAll( wxEvent& event );
	void OnCollapseAll( wxEvent& event );

	void OnSaveAll( wxEvent& event );
	void OnCheckInAll( wxEvent& event );

	void OnCloseWindow( wxCloseEvent& event );
	void OnGridEdit( CGridEvent& event );
	void OnGridChanged( CGridEvent& event );

	void OnGridValueChanged( wxCommandEvent& event );

	void OnNotebookPageChanged( wxCommandEvent& event );
	void OnViewChanged( wxCommandEvent& event );
	void UpdateCharacterList( EViewChoice choice );

	void OnFolderTreeNodeSelectionChanged( wxTreeEvent& event );
	void OnInheritanceTreeNodeSelectionChanged( wxTreeEvent& event );

	void OnDataChanged( wxEvent& event );
	void OnInternalIdle() override;

	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	void SetGridObject();
};
