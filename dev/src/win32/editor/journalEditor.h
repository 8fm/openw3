#pragma once

#include "../../games/r4/journalQuest.h"

class CEdStringSelector;

class CEdJournalTree;
class CEdJournalTreeItemData;

class CClass;
class CDirectory;

wxDECLARE_EVENT( wxEVT_JOURNAL_CHOICE_POPULATED, wxCommandEvent );

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

struct SEdWidgetData
{
public:
	enum eDataType
	{
		DT_PropertyName = 0,
		DT_WidgetPtr
	};

	eDataType dataType;

	CName propertyName;
	wxWindow* widget;

	SEdWidgetData( const CName& name )
	:	propertyName( name ),
		widget( NULL ),
		dataType( DT_PropertyName )
	{
	}

	SEdWidgetData( wxWindow* associatedWidget )
	:	widget( associatedWidget ),
		dataType( DT_WidgetPtr )
	{
	}
};

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

struct SLinkedJournalTypeDescriptor
{
	const CClass*				m_baseClass;
	const CClass*				m_linkedClass;
	TDynArray< CDirectory* >	m_resourceDirectories;

	SLinkedJournalTypeDescriptor( const CClass* baseClass, const CClass* linkedClass, const TDynArray< CDirectory* > & resourceDirectories )
		: m_baseClass( baseClass )
		, m_linkedClass( linkedClass )
		, m_resourceDirectories( resourceDirectories )
	{}

	SLinkedJournalTypeDescriptor( const CClass* baseClass, const CClass* linkedClass )
		: m_baseClass( baseClass )
		, m_linkedClass( linkedClass )
	{}
};

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

class CEdJournalEditor : public wxSmartLayoutPanel, public IEdEventListener, public ILocalizableObject
{
private:
	typedef TDynArray< wxWindow* > TWidgetGroup;

public:
	enum eError
	{
		Error_None = 0,
		Error_NoFilename,
		Error_DuplicateFilename,

		Error_Max
	};

	CEdJournalEditor( wxWindow* parent, THandle< CJournalPath > defaultPath = NULL );
	~CEdJournalEditor();

private:
	void OnCloseWindow( wxCloseEvent& event );
	void OnTreeRightClick( wxEvent& event );

	void OnCreateNewQuestGroup( wxEvent& event );
	void OnCreateNewQuest( wxEvent& event );
	void OnCreateNewQuestPhase( wxEvent& event );
	void OnCreateNewQuestObjective( wxEvent& event );
	void OnCreateNewQuestMapPin( wxEvent& event );
	void OnCreateNewQuestItemTag( wxEvent& event );
	void OnCreateNewQuestMonsterTag( wxEvent& event );
	void OnCreateNewQuestDescription( wxEvent& event );

	void OnCreateNewCharacterGroup( wxEvent& event );
	void OnCreateNewCharacter( wxEvent& event );
	void OnCreateNewCharacterDescription( wxEvent& event );

	void OnCreateNewGlossaryGroup( wxEvent& event );
	void OnCreateNewGlossaryEntry( wxEvent& event );
	void OnCreateNewGlossaryDescription( wxEvent& event );

	void OnCreateNewTutorialGroup( wxEvent& event );
	void OnCreateNewTutorialEntry( wxEvent& event );

	void OnCreateNewItemGroup( wxEvent& event );
	void OnCreateNewItemSubGroup( wxEvent& event );
	void OnCreateNewItemEntry( wxEvent& event );
	void OnCreateNewItemComponent( wxEvent& event );

	void OnCreateNewCreatureGroup( wxEvent& event );
	void OnCreateNewCreatureVirtualGroup( wxEvent& event );
	void OnCreateNewCreature( wxEvent& event );
	void OnCreateNewCreatureDescription( wxEvent& event );
	void OnCreateNewCreatureHuntingClue( wxEvent& event );
	void OnCreateNewCreatureGameplayHint( wxEvent& event );
	void OnCreateNewCreatureVitalSpot( wxEvent& event );

	void OnCreateNewStoryBookChapter( wxEvent& event );
	void OnCreateNewStoryBookPage( wxEvent& event );
	void OnCreateNewStoryBookPageDescription( wxEvent& event );

	void OnCreateNewPlaceGroup( wxEvent& event );
	void OnCreateNewPlace( wxEvent& event );
	void OnCreateNewPlaceDescription( wxEvent& event );

	void OnPopupDuplicateQuestObjective( wxEvent& event );
	void OnPopupDuplicateQuestMapPin( wxEvent& event );

	void OnPopupSortChildren( wxEvent& event );

	void OnSelectItemDialog( wxCommandEvent& event );
	void OnSelectItemDialogOK( wxCommandEvent& event );

	void OnSelectItemsDialog( wxCommandEvent& event );
	void OnSelectItemsDialogOK( wxCommandEvent& event );

	void OnSelectStringDialog( wxCommandEvent& event );
	void OnSelectStringDialogOK( wxCommandEvent& event );
	void OnSelectStringDialogCancel( wxEvent& event );

	void OnUseObject( wxCommandEvent& event );

	void OnClearPath( wxCommandEvent& event );

	void OnPopupSCCheckOut( wxEvent& event );
	void OnPopupSCSaveItem( wxEvent& event );
	void OnPopupSCSaveAndCheckIn( wxEvent& event );
	void OnPopupSCRevert( wxEvent& event );
	void OnPopupSCDeleteItem( wxEvent& event );

	void OnPopupDeleteItem( wxEvent& event );

	void OnFilenameChange( wxCommandEvent& event );

	void OnLocalizedStringTextChange( wxCommandEvent& event );
	void OnStringChange( wxCommandEvent& event );
	void OnUintChange( wxCommandEvent& event );
	void OnFloatChange( wxCommandEvent& event );
	void OnChoiceChange( wxCommandEvent& event );
	void OnCNameChanged( wxCommandEvent& event );
    void OnCheckBoxChange( wxCommandEvent& event );
	void OnPathChanged( wxCommandEvent& event );
	void OnPathCleared( wxCommandEvent& event );
	void OnNameArrayChange( wxCommandEvent& event );

	void OnNameChange( wxCommandEvent& event );
	void OnObjectHandleChange( wxCommandEvent& event );

	void OnQuestObjectiveTypeChange( wxCommandEvent& event );

	void PopulateTree();

	void OnTreeDrag( wxTreeEvent& event );
	void OnTreeDrop( wxTreeEvent& event );

	void OnSaveAll( wxEvent& event );
	void OnCheckInAll( wxEvent& event );
	void OnSelectTargetDLC( wxEvent& event );

	void OnCategorySelectionChanged( wxTreeEvent& event );

	void OnCollapseAll( wxEvent& event );
	void OnExpandAll( wxEvent& event );

	void OnResetWindowSize( wxEvent& event );

	void OnCheckDuplicatedUniqueScriptTags( wxEvent& event );
	void OnCheckDuplicatedGUIDs( wxEvent& event );
	void OnGenerateMissingUniqueScriptTags( wxEvent& event );

	//! IEdEventListener interface
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	//! ILocalizableObject interface
	virtual void GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings );

private:
	void VerifyParent( wxTreeItemId& parentItem, CJournalBase* journalEntry ) const;

	wxTreeItemId CreateNewResourceItem( wxTreeItemId parentItem, CJournalBase* journalEntry, CDirectory* directory );
	wxTreeItemId CreateNewSubItem( wxTreeItemId parentItem, CJournalContainerEntry* journalEntry, const Char* baseName = NULL );
	wxTreeItemId CreateNewItem( wxTreeItemId parentItem, CJournalBase* child, CJournalResource* resource = NULL, CDirectory* directory = NULL, const Char* baseName = NULL );

	wxStaticText* CreateLabel( wxSizer* sizer, TWidgetGroup& widgetGroup, const wxString& text );
	wxTextCtrl* CreateTextWidget( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& toolTip, Bool isDescription = false, const wxValidator& validator = wxDefaultValidator );
	wxChoice* CreateChoiceWidget( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& toolTip, const wxArrayString& choices );
	wxButton* CreateSelectButton( wxGridBagSizer* sizer, TWidgetGroup& widgetGroup, wxTextCtrl* linkedControl, Int32 eventID, const wxString& label, const wxBitmap* bitmap = NULL );
    wxCheckBox* CreateCheckBox( wxSizer* sizer, TWidgetGroup& widgetGroup, wxWindowID windowID, const CName& name, const wxString& toolTip );

	void ShowSubsectionWidgets( const TWidgetGroup& widgetGroup, const wxTreeItemId& item );
	void HideSubsectionWidgets( const CClass* widgetGroupKey );

	void CategorySelectionChanged( const wxTreeItemId& item );

	void MarkItemUnmodified( const wxTreeItemId& item, Bool force = false );
	void MarkItemModified( const wxTreeItemId& item, Bool force = false );
	void MarkItemSaved( const wxTreeItemId& item, Bool force = false );

	Bool CanModifyItem( const wxTreeItemId& item ) const;

	void CheckOutItem( const wxTreeItemId& item );
	void RevertItem( const wxTreeItemId& item );

	void SaveAll();
	void CheckinAll();
	void SaveAllLocalizedStrings();

	eError SaveItem( const wxTreeItemId& item );
	eError SaveItem( CEdJournalTreeItemData* itemData );
	eError DeleteItem( const wxTreeItemId& item );
	void GetResourceChildren( const wxTreeItemId& item, TDynArray< CEdJournalTreeItemData* >& children ) const;

	void PopulateMenuCreationOptions( const wxTreeItemId& item, wxMenu* menu ) const;
	void PopulateCommonMenuOptions( const wxTreeItemId& item, wxMenu* menu ) const;

	inline wxWindowID GetID( Int32 id ) const { return id; }

	template< typename T >
	void SetProperty( wxCommandEvent& event, const T& propertyValue );

	template< typename T >
	void SetProperty( SEdWidgetData* propData, const T& propertyValue );

	template< typename T >
	void GetProperty( CJournalBase* entry, wxWindow* widget, T& outValue );

	template< typename T >
	T* GetPropertyPointer( CJournalBase* entry, wxWindow* widget );

	CProperty* GetProperty( CJournalBase* entry, wxWindow* widget );

	SEdWidgetData* GetWidgetDataFromEvent( wxCommandEvent& event );
	void SetWidgetValueFromProperty( CJournalBase* entry, wxWindow* widget );

	Uint32 CalculateOrderFront( const wxTreeItemId& parentItem ) const;
	Uint32 CalculateOrder( const wxTreeItemId& startItem ) const;
	Uint32 CalculateOrder( Uint32 a, Uint32 b ) const;

	void CopyChildren( const wxTreeItemId& sourceParentItem, const wxTreeItemId& destParentItem );

	void DisplayError( eError error, const wxTreeItemId& item );

	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	wxTextCtrl* GetWidget( wxCommandEvent& event );
	void CreateWidgets( const CClass* journalEntryClass, Bool needFile );
	void AddDataToWidget( wxWindow* widget, const CName& name );

private:
	wxPanel* m_subContainer;
	wxSplitterWindow* m_splitter;

	CEdJournalTree* m_Tree;
	THandle< class CJournalInitialEntriesResource > m_initalEntriesResource[ EJCT_Count ];

	wxTreeItemId m_popupMenuItem;
	wxTreeItemId m_draggedItem;
	wxMenu* m_treePopupMenu;

	wxWindowID m_assignedWindowStartingID;

	TDynArray< CJournalResource* > m_questGroups;
	TDynArray< CJournalResource* > m_quests;

	TDynArray< wxTreeItemId > m_modifiedMenuItems;
	TDynArray< wxTreeItemId > m_CheckedOutMenuItems;

	TDynArray< SEdWidgetData* > m_widgetData;

	THashMap< const CClass* , SLinkedJournalTypeDescriptor >	m_linkedJournalTypes;

	THashMap< const CClass*, TWidgetGroup > m_controls;
	const CClass* m_previousGroup;

	CEdStringSelector* m_stringSelectorTool;

	String m_targetDLC;

	Uint32 GetTargetDLCDirectoryIndexFrom( const TDynArray< CDirectory* >& directories ) const;
	CDirectory* GetTargetDLCDirectoryFrom( const TDynArray< CDirectory* >& directories ) const;

	// Organise the changes to localised strings by their parent entry (guid)
	typedef TDynArray< LocalizedStringEntry > TLocalizedStringEntryArray;
	THashMap< CGUID, TLocalizedStringEntryArray > m_localizedStringChanges;
	CGUID m_localizedStringsToSave;

	// Custom functionality
	private:

	void OnHuntingClueCategorySelected( wxCommandEvent& event );
	void DoHuntingClueCategorySelected( const CName& categoryName );
	wxChoice* m_huntingClueChoiceWidget;

	void OnChangeQuestTypeProperty( wxCommandEvent& event );
	void OnChangeActiveProperty( wxCommandEvent& event );
	void OnUpdateActiveProperty( CJournalBase* entry, wxWindow* widget, Bool& boolVal );
	void OnOpenInitialEntriesResource();
	void OnCloseInitialEntriesResource();
	void SaveInitialEntriesResource();
	Bool IsQuestTypePropertyClass( const CJournalBase* entry );
	Bool IsQuestTypePropertyWidgetData( const SEdWidgetData* propData );
	Bool IsActivePropertyClass( const CJournalBase* entry );
	Bool IsActivePropertyWidgetData( const SEdWidgetData* propData );
	Bool IsParentResourceSavedToFile( const CJournalBase* entry );

	void CollectLinkedResourcedForType( const CClass* entryType, TDynArray< String > & outFilenames );

	void CountQuests( Uint32 contentType, Uint32& regularQuestCount, Uint32& monsterHuntCount, Uint32& treasureHuntCount );
		
	void InitTargetDLC();
	void SetTargetDLC( const String& targetDLC );
};
