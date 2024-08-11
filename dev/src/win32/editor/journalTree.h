#pragma once
#include "..\..\games\r4\r4GameResource.h"

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

class CEdJournalTreeItemData : public wxTreeItemData
{
public:
	CJournalBase* m_entry;

	enum eState
	{
		State_Any = -1,
		State_Unmodified = 0,
		State_Modified,
		State_ReadyForSourceControl
	};

	eState m_state;
	CJournalResource* m_resource;

	String m_filename;
	CDirectory* m_directory;

	CEdJournalTreeItemData( CJournalBase* entry, CJournalResource* resource = NULL, CDirectory* directory = NULL );
	CEdJournalTreeItemData( const CEdJournalTreeItemData* other );
};

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

struct SJournalItemHandle
{
	virtual Bool IsValid() = 0;

	CJournalResource* m_resource;
};

struct SJournalwxTreeItem : public SJournalItemHandle
{
	SJournalwxTreeItem( const wxTreeItemId& item )
	:	m_itemId( item )
	{
	}

	virtual Bool IsValid() { return m_itemId.IsOk(); }

	wxTreeItemId m_itemId;
};

class CJournalTree
{
public:
	CJournalTree();
	virtual ~CJournalTree();

	void PopulateTreeSection( SJournalItemHandle* rootHandle, const TDynArray< CDirectory* >& sectionDirectories );

protected:
	virtual SJournalItemHandle* AddItemAppend( SJournalItemHandle* parentHandle, CJournalBase* entry, CJournalResource* journalResource, CDirectory* sectionDirectory ) = 0;
	virtual void Sort( SJournalItemHandle* parentHandle ) = 0;

	// For levels that consist of files, the number is the total number of all items at this level, not for this specific parent (so it is always greater than the actual number required)
	// for all other levels (Journal Containers) the number is accurate
	virtual void MaximumNumberOfChildEntries( SJournalItemHandle* /*parentItem*/, Uint32 /*number*/ ) {};

private:
	typedef THashMap< CGUID, SJournalItemHandle* > TJournalItemHandleMap;

	void PopulateTreeSubStep( TDynArray< CJournalResource* >& childResources, TJournalItemHandleMap& groupHandles );
	void PopulateTreeContainers( SJournalItemHandle* parentHandle, CJournalBase* entry, CJournalResource* resource = NULL );

private:
	TDynArray< SJournalItemHandle* > m_handles;
};

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

class CEdJournalTree : public wxTreeCtrl, public CJournalTree
{
	// NB: due to an ugly wxMSW hack you _must_ use DECLARE_DYNAMIC_CLASS()
	//     if you want your overloaded OnCompareItems() to be called.
	//     OTOH, if you don't want it you may omit the next line - this will
	//     make default (alphabetical) sorting much faster under wxMSW.
	DECLARE_DYNAMIC_CLASS( CEdJournalTree )

public:

	enum eTreeCategory
	{
		TreeCategory_Quest = 0,
		TreeCategory_Character,
		TreeCategory_Glossary,
		TreeCategory_Tutorial,
		TreeCategory_Items,
		TreeCategory_Creatures,
		TreeCategory_StoryBook,
		TreeCategory_Places,

		TreeCategory_Max
	};

public:
	CEdJournalTree();
	virtual ~CEdJournalTree();

	void Initialize();

	void MarkItemLocked( const wxTreeItemId& item, Bool locked );

	wxTreeItemId AddCategoryRoot( eTreeCategory category );

	wxTreeItemId GetCategoryRootItem( eTreeCategory category ) const;
	void PopulateTreeSection( eTreeCategory category );
	void GetResourceItem( const wxTreeItemId& item /* in */, wxTreeItemId& resourceItem /* out */, CEdJournalTreeItemData** resourceItemData /* out */ ) const;

	void CollectEntries( THashMap< CGUID, TDynArray< CJournalBase* > >& entries );
	void CollectEntries( THashMap< CName, TDynArray< CJournalBase* > >& entries );
	void CollectEntriesRecursively( const wxTreeItemId& item, THashMap< CGUID, TDynArray< CJournalBase* > >& journalEntries );
	void CollectEntriesRecursively( const wxTreeItemId& item, THashMap< CName, TDynArray< CJournalBase* > >& journalEntries );
	void GenerateMissingUniqueScriptTags();
	void GenerateMissingUniqueScriptTagsRecursively( const wxTreeItemId& item );

	// Wrap the tree functions
	wxTreeItemId AddItemPrepend( const wxTreeItemId& parentItem, const wxString& text, CEdJournalTreeItemData* data = NULL );
	wxTreeItemId AddItemAppend( const wxTreeItemId& parentItem, const wxString& text, CEdJournalTreeItemData* data = NULL );
	wxTreeItemId AddItemInsert( const wxTreeItemId& parentItem, const wxTreeItemId& idBefore, const wxString& text, CEdJournalTreeItemData* data = NULL );
	void DeleteItem( const wxTreeItemId& item );

	virtual void DeleteAllItems();

	void ExpandPath( const THandle< CJournalPath > path );

	Bool MarkItemModified( const wxTreeItemId& item, Bool force );
	Bool MarkItemSaved( const wxTreeItemId& item, Bool force );
	Bool MarkItemUnmodified( const wxTreeItemId& item, Bool force );

	void PopulateDuplicatedTreeContainers( const wxTreeItemId& entryItem, CJournalBase* entry, Bool entryAlreadyAdded );

private:
	void MarkChildItems( const wxTreeItemId& item, CEdJournalTreeItemData::eState oldState, CEdJournalTreeItemData::eState newState, const wxColour& colour );

public:
	RED_INLINE static Bool IsDirectoryDefined()
	{
		ASSERT( GGame, TXT( "Can't access game resource as there's no game" ) );
		CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );

		if( gameResource )
		{
			const String& basePath = gameResource->GetJournalPath();

			if( !basePath.Empty() )
			{
				return true;
			}
		}

		return false;
	}

protected:
	static TDynArray< CDirectory* > GetDirectories( const Char* path );

public:
	// !!! ALL DIRECTORY NAMES MUST BE LOWERCASE !!!
	static TDynArray< CDirectory* > GetQuestsDirectories()		{ return GetDirectories( TXT( "quests" ) ); }
	static TDynArray< CDirectory* > GetCharactersDirectories()	{ return GetDirectories( TXT( "characters" ) ); }
	static TDynArray< CDirectory* > GetGlossaryDirectories()	{ return GetDirectories( TXT( "glossary" ) ); }
	static TDynArray< CDirectory* > GetTutorialDirectories()	{ return GetDirectories( TXT( "tutorial" ) ); }
	static TDynArray< CDirectory* > GetItemsDirectories()		{ return GetDirectories( TXT( "items" ) ); }
	static TDynArray< CDirectory* > GetCreaturesDirectories()	{ return GetDirectories( TXT( "bestiary" ) ); }
	static TDynArray< CDirectory* > GetStoryBookDirectories()	{ return GetDirectories( TXT( "storybook" ) ); }
	static TDynArray< CDirectory* > GetPlacesDirectories()		{ return GetDirectories( TXT( "places" ) ); }

private:
	
	Int32 GetIcon( const wxTreeItemId& parentItem, CEdJournalTreeItemData* data ) const;

	virtual SJournalItemHandle* AddItemAppend( SJournalItemHandle* parentHandle, CJournalBase* entry, CJournalResource* journalResource, CDirectory* sectionDirectory );
	virtual void Sort( SJournalItemHandle* parentHandle );


	virtual int OnCompareItems( const wxTreeItemId& a, const wxTreeItemId& b );

	struct SCategoryData
	{
		const Char*					name;
		TDynArray< CDirectory* >	dirs;
		CClass*						rttiClass;

		CJournalBase* root;
		wxTreeItemId item;

		SCategoryData()
		:	name( NULL ),
			rttiClass( NULL ),
			root( NULL )
		{
		}

		SCategoryData( const Char* catName, const TDynArray< CDirectory* >& catDirs, CClass* catRttiClass )
		:	name( catName ),
			dirs( catDirs ),
			rttiClass( catRttiClass ),
			root( NULL )
		{
		}
	};

	struct SItemClassData
	{
		Int32 m_iconIndexUnlocked;
		Int32 m_iconIndexLocked;

		SItemClassData( Int32 iconIndexUnlocked = -1, Int32 iconIndexLocked = -1 )
		:	m_iconIndexUnlocked( iconIndexUnlocked ),
			m_iconIndexLocked( iconIndexUnlocked )
		{
		}
	};

	THashMap< CGUID, wxTreeItemId > m_itemMap;
	TStaticArray< SCategoryData, TreeCategory_Max > m_categories;
	THashMap< const CClass*, SItemClassData > m_itemClassDataMap;
};
