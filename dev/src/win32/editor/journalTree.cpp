#include "build.h"

#include "journalTree.h"

#include "res/images/folder.xpm"
#include "res/images/folder_locked.xpm"
#include "res/images/item.xpm"
#include "res/images/item_locked.xpm"

#include "../../games/r4/journal.h"

#include "../../common/core/depot.h"
#include "../../common/core/dataError.h"
#include "../../common/core/feedback.h"

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

// This exists to serve as a "dummy" root node on the journal tree
class CJournalTreeRoot : public CJournalBase
{
public:
	DECLARE_ENGINE_CLASS( CJournalTreeRoot, CJournalBase, 0 )

	CJournalTreeRoot() {}
	virtual ~CJournalTreeRoot() {}

	virtual Bool IsParentClass( CJournalBase* ) const { return false; }
};

BEGIN_CLASS_RTTI( CJournalTreeRoot )
	PARENT_CLASS( CJournalBase )
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CJournalTreeRoot );

//-----------------------------------------------------------------------------------

static String GetCaptionForEntry( CJournalBase* entry, CJournalResource* resource )
{
	String caption = entry->GetName();
	if ( resource != nullptr && resource->GetDepotPath().BeginsWith( TXT("dlc\\") ) )
	{
		caption = entry->GetName();
		CDirectory* dlcDir = resource->GetFile()->GetDirectory();
		while ( dlcDir->GetParent()->GetName() != TXT("dlc") )
		{
			dlcDir = dlcDir->GetParent();
		}
		caption += String::Printf( TXT(" (dlc: %s)"), dlcDir->GetName().AsChar() );
	}
	return caption;
}

//-----------------------------------------------------------------------------------

#define TREE_COLOUR_EDITED ( *wxRED )
#define TREE_COLOUR_EDITED_CHILD ( wxColour( 255, 128, 0 ) )		// Modified fileless child
#define TREE_COLOUR_SAVED ( wxColour( 0, 180, 0 ) )
#define TREE_COLOUR_SAVED_CHILD ( wxColour( 0, 180, 0 ) )
#define TREE_COLOUR_UNMODIFIED ( *wxBLACK )

CEdJournalTreeItemData::CEdJournalTreeItemData( CJournalBase* entry, CJournalResource* resource, CDirectory* directory )
:	m_entry( entry ),
	m_resource( resource ),
	m_directory( directory ),
	m_state( State_Unmodified )
{
	// Need to initialise filename member
	if( m_resource )
	{
		CDiskFile* file = m_resource->GetFile();

		if( file )
		{
			CFilePath path( file->GetFileName() );
			m_filename = path.GetFileName();
		}
	}
}

CEdJournalTreeItemData::CEdJournalTreeItemData( const CEdJournalTreeItemData* other )
{
	m_entry = other->m_entry;
	m_resource = other->m_resource;
	m_filename = other->m_filename;
	m_directory = other->m_directory;
	m_state = other->m_state;
}

//------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS( CEdJournalTree, wxTreeCtrl )

CEdJournalTree::CEdJournalTree()
{
}

CEdJournalTree::~CEdJournalTree()
{
	for( Uint32 i = 0; i < m_categories.Size(); ++i )
	{
		if( m_categories[ i ].root )
		{
			m_categories[ i ].root->RemoveFromRootSet();
			m_categories[ i ].root->Discard();
			m_categories[ i ].root = NULL;
		}
	}
}

void CEdJournalTree::Initialize()
{
	if( IsDirectoryDefined() )
	{
		wxIcon groupIcon( folder_xpm );
		wxIcon groupLockedIcon( folder_locked_xpm );
		wxIcon itemIcon( item_xpm );
		wxIcon itemLockedIcon( item_locked_xpm );

		wxImageList* treeImages = new wxImageList( 16, 16, true, 0 );

		Int32 groupIconUnlocked	= treeImages->Add( groupIcon );
		Int32 groupIconLocked		= treeImages->Add( groupLockedIcon );
		Int32 itemIconUnlocked	= treeImages->Add( itemIcon );
		Int32 itemIconLocked		= treeImages->Add( itemLockedIcon );
		AssignImageList( treeImages );

		m_itemClassDataMap.Set( ClassID< CJournalQuestRoot >(),					SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuest >(),						SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestPhase >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestObjective >(),			SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestEnemyTag >(),				SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestMapPin >(),				SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestItemTag >(),				SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalQuestDescriptionGroup >(),		SItemClassData( groupIconUnlocked,	groupIconUnlocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalQuestDescriptionEntry >(),		SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalCharacterRoot >(),				SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCharacterGroup >(),			SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCharacter >(),					SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCharacterDescription >(),		SItemClassData( groupIconUnlocked,	groupIconLocked ) );

		m_itemClassDataMap.Set( ClassID< CJournalGlossaryRoot >(),				SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalGlossaryGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalGlossary >(),					SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalGlossaryDescription >(),		SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalTutorialRoot >(),				SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalTutorialGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalTutorial >(),					SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalItemRoot >(),					SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalItemGroup >(),					SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalItemSubGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalItem >(),						SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalItemComponent >(),				SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalCreatureRoot >(),				SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureVirtualGroup >(),		SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreature >(),					SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureDescriptionGroup >(),	SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureDescriptionEntry >(),	SItemClassData( itemIconUnlocked,	itemIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureHuntingClueGroup >(),	SItemClassData( groupIconUnlocked,	groupIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureHuntingClue >(),		SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureGameplayHintGroup >(),	SItemClassData( groupIconUnlocked,	groupIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureGameplayHint >(),		SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureVitalSpotGroup >(),	SItemClassData( groupIconUnlocked,	groupIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalCreatureVitalSpotEntry >(),	SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalStoryBookRoot >(),				SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalStoryBookChapter >(),			SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalStoryBookPage >(),				SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalStoryBookPageDescription >(),	SItemClassData( itemIconUnlocked,	itemIconLocked  ) );

		m_itemClassDataMap.Set( ClassID< CJournalPlaceRoot >(),					SItemClassData( groupIconUnlocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalPlaceGroup >(),				SItemClassData( groupIconUnlocked,	groupIconLocked ) );
		m_itemClassDataMap.Set( ClassID< CJournalPlace >(),						SItemClassData( itemIconUnlocked,	itemIconLocked  ) );
		m_itemClassDataMap.Set( ClassID< CJournalPlaceDescription >(),			SItemClassData( itemIconUnlocked,	itemIconLocked	) );

		m_categories.Resize( TreeCategory_Max );
		m_categories[ TreeCategory_Quest ]		= SCategoryData( TXT( "Quests" ),		CEdJournalTree::GetQuestsDirectories(),		ClassID< CJournalQuestRoot >() );
		m_categories[ TreeCategory_Character ]	= SCategoryData( TXT( "Characters" ),	CEdJournalTree::GetCharactersDirectories(),	ClassID< CJournalCharacterRoot >() );
		m_categories[ TreeCategory_Glossary ]	= SCategoryData( TXT( "Glossary" ),		CEdJournalTree::GetGlossaryDirectories(),	ClassID< CJournalGlossaryRoot >() );
		m_categories[ TreeCategory_Tutorial ]	= SCategoryData( TXT( "Tutorials" ),	CEdJournalTree::GetTutorialDirectories(),	ClassID< CJournalTutorialRoot >() );
		m_categories[ TreeCategory_Items ]		= SCategoryData( TXT( "Items" ),		CEdJournalTree::GetItemsDirectories(),		ClassID< CJournalItemRoot >() );
		m_categories[ TreeCategory_Creatures ]	= SCategoryData( TXT( "Bestiary" ),		CEdJournalTree::GetCreaturesDirectories(),	ClassID< CJournalCreatureRoot >() );
		m_categories[ TreeCategory_StoryBook ]	= SCategoryData( TXT( "Story Book" ),	CEdJournalTree::GetStoryBookDirectories(),	ClassID< CJournalStoryBookRoot >() );
		m_categories[ TreeCategory_Places ]		= SCategoryData( TXT( "Places" ),		CEdJournalTree::GetPlacesDirectories(),		ClassID< CJournalPlaceRoot >() );
	}
	else
	{
		SetBackgroundColour( wxColour( 224, 130, 130 ) );
		CEdHelpBubble* bubble = new CEdHelpBubble( this, TXT( "The path to the journal must be defined\nin the Game Configuration Resource" ) );
		bubble->Show();
	}
}

wxTreeItemId CEdJournalTree::AddCategoryRoot( eTreeCategory category )
{
	if( !GetRootItem().IsOk() )
	{
		// Add root item
		AddRoot
		(
			wxT( "Journal" ),
			-1,
			-1,
			NULL
		);
	}

	// Break out if the journal has no proper directory defined
	if( !IsDirectoryDefined() )
	{
		return GetRootItem();
	}

	if( !m_categories[ category ].root )
	{
		m_categories[ category ].root = CreateObject< CJournalTreeRoot >( m_categories[ category ].rttiClass );
		m_categories[ category ].root->AddToRootSet();

		m_categories[ category ].item = AddItemAppend
		(
			GetRootItem(),
			m_categories[ category ].name,
			new CEdJournalTreeItemData
			(
				m_categories[ category ].root,
				NULL
			)
		);
	}

	ASSERT( m_categories[ category ].item.IsOk() );

	return m_categories[ category ].item;
}

wxTreeItemId CEdJournalTree::GetCategoryRootItem( eTreeCategory category ) const
{
	if( m_categories[ category ].item.IsOk() )
	{
		return m_categories[ category ].item;
	}

	ASSERT( GetRootItem().IsOk() );
	return GetRootItem();
}

RED_INLINE Int32 CEdJournalTree::GetIcon( const wxTreeItemId& parentItem, CEdJournalTreeItemData* data ) const
{
	Int32 icon = -1;

	if( data )
	{
		CEdJournalTreeItemData* resourceItemData = NULL;

		if( data->m_resource )
		{
			resourceItemData = data;
		}
		else
		{
			wxTreeItemId resourceItem;
			GetResourceItem( parentItem, resourceItem, &resourceItemData );
		}

		const SItemClassData& classData = *( m_itemClassDataMap.FindPtr( data->m_entry->GetClass() ) );

		icon = ( !resourceItemData || resourceItemData->m_resource->CanModify() )? classData.m_iconIndexUnlocked : classData.m_iconIndexLocked;
	}

	return icon;
}

wxTreeItemId CEdJournalTree::AddItemPrepend( const wxTreeItemId& parentItem, const wxString& text, CEdJournalTreeItemData* data )
{
	Int32 icon = GetIcon( parentItem, data );

	wxTreeItemId item = wxTreeCtrl::PrependItem( parentItem, text, icon, icon, data );

	if( data )
	{
		m_itemMap.Insert( data->m_entry->GetGUID(), item );
	}

	return item;
}

wxTreeItemId CEdJournalTree::AddItemAppend( const wxTreeItemId& parentItem, const wxString& text, CEdJournalTreeItemData* data )
{
	Int32 icon = GetIcon( parentItem, data );

	wxTreeItemId item = wxTreeCtrl::AppendItem( parentItem, text, icon, icon, data );

	if( data )
	{
		m_itemMap.Insert( data->m_entry->GetGUID(), item );
	}

	return item;
}

wxTreeItemId CEdJournalTree::AddItemInsert( const wxTreeItemId& parentItem, const wxTreeItemId& idBefore, const wxString& text, CEdJournalTreeItemData* data )
{
	Int32 icon = GetIcon( parentItem, data );

	wxTreeItemId item = wxTreeCtrl::InsertItem( parentItem, idBefore, text, icon, icon, data );

	if( data )
	{
		m_itemMap.Insert( data->m_entry->GetGUID(), item );
	}

	return item;
}

void CEdJournalTree::DeleteItem( const wxTreeItemId& item )
{
	CEdJournalTreeItemData* data = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );

	if( data )
	{
		m_itemMap.Erase( data->m_entry->GetGUID() );
	}

	wxTreeCtrl::Delete( item );
}

void CEdJournalTree::DeleteAllItems()
{
	m_itemMap.Clear();

	wxTreeCtrl:: DeleteAllItems();
}

int CEdJournalTree::OnCompareItems( const wxTreeItemId& a, const wxTreeItemId& b )
{
	CEdJournalTreeItemData* aData = static_cast< CEdJournalTreeItemData* >( GetItemData( a ) );
	CEdJournalTreeItemData* bData = static_cast< CEdJournalTreeItemData* >( GetItemData( b ) );

	if( aData->m_entry->GetOrder() > bData->m_entry->GetOrder() )
	{
		return 1;
	}
	else if( aData->m_entry->GetOrder() < bData->m_entry->GetOrder() )
	{
		return -1;
	}

	return 0;
}

void CEdJournalTree::ExpandPath( const THandle< CJournalPath > path )
{
	ASSERT( path );

	if( path->IsValid() )
	{
		const CGUID& guid = path->GetTarget()->GetGUID();

		wxTreeItemId item;

		if( m_itemMap.Find( guid, item ) )
		{
			SelectItem( item );
		}
		else
		{
			ASSERT( false && "Couldn't find specified journal item" );
		}
	}
}

void CEdJournalTree::GetResourceItem( const wxTreeItemId& item, wxTreeItemId& resourceItem, CEdJournalTreeItemData** resourceItemData ) const
{
	*resourceItemData = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );

	if( *resourceItemData )
	{
		if( ( *resourceItemData )->m_resource )
		{
			resourceItem = item;
		}
		else
		{
			GetResourceItem( GetItemParent( item ), resourceItem, resourceItemData );
		}
	}
}

void CEdJournalTree::CollectEntries( THashMap< CGUID, TDynArray< CJournalBase* > >& journalEntries )
{
	journalEntries.ClearFast();
	CollectEntriesRecursively( GetRootItem(), journalEntries );
}

void CEdJournalTree::CollectEntries( THashMap< CName, TDynArray< CJournalBase* > >& journalEntries )
{
	journalEntries.ClearFast();
	CollectEntriesRecursively( GetRootItem(), journalEntries );
}

void CEdJournalTree::CollectEntriesRecursively( const wxTreeItemId& item, THashMap< CGUID, TDynArray< CJournalBase* > >& journalEntries )
{
	CEdJournalTreeItemData* data = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	if ( data )
	{
		CJournalBase* entry = data->m_entry;
		if ( entry )
		{
			CGUID guid = data->m_entry->GetGUID();
			if ( !guid.IsZero() )
			{
				TDynArray< CJournalBase* >* entries = journalEntries.FindPtr( guid );
				if ( entries )
				{
					entries->PushBack( entry );
				}
				else
				{
					TDynArray< CJournalBase* > value;
					value.PushBack( entry );
					journalEntries.Insert( guid, value );
				}
			}
		}
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId childItem = GetFirstChild( item, cookie );
	while( childItem.IsOk() )
	{
		CollectEntriesRecursively( childItem, journalEntries );
		childItem = GetNextChild( childItem, cookie );
	}
}

void CEdJournalTree::CollectEntriesRecursively( const wxTreeItemId& item, THashMap< CName, TDynArray< CJournalBase* > >& journalEntries )
{
	CEdJournalTreeItemData* data = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	if ( data )
	{
		CJournalBase* entry = data->m_entry;
		if ( entry )
		{
			const CName& uniqueScriptTag = data->m_entry->GetUniqueScriptIdentifier();

			TDynArray< CJournalBase* >* entries = journalEntries.FindPtr( uniqueScriptTag );
			if ( entries )
			{
				entries->PushBack( entry );
			}
			else
			{
				TDynArray< CJournalBase* > value;
				value.PushBack( entry );
				journalEntries.Insert( uniqueScriptTag, value );
			}
		}
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId childItem = GetFirstChild( item, cookie );
	while( childItem.IsOk() )
	{
		CollectEntriesRecursively( childItem, journalEntries );
		childItem = GetNextChild( childItem, cookie );
	}
}

void CEdJournalTree::GenerateMissingUniqueScriptTags()
{
	RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("----------------------------------------------") );
	RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("Unique script tags generator") );
	RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("----------------------------------------------") );
	GenerateMissingUniqueScriptTagsRecursively( GetRootItem() );
	GFeedback->ShowMsg( TXT( "Info" ), TXT( "Unique script tag generation finished" ) );
}

void CEdJournalTree::GenerateMissingUniqueScriptTagsRecursively( const wxTreeItemId& item )
{
	CEdJournalTreeItemData* data = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	if ( data )
	{
		CJournalBase* entry = data->m_entry;
		if ( entry )
		{
			if ( !entry->IsA< CJournalTreeRoot >() &&
				 !entry->IsA< CJournalQuestRoot >() &&
				 !entry->IsA< CJournalCharacterRoot >() &&
				 !entry->IsA< CJournalCreatureRoot >() &&
				 !entry->IsA< CJournalGlossaryRoot >() &&
				 !entry->IsA< CJournalItemRoot >() &&
				 !entry->IsA< CJournalStoryBookRoot >() &&
				 !entry->IsA< CJournalTutorialRoot >() &&
				 !entry->IsA< CJournalPlaceRoot >() )
			{
				const CName& tag = entry->GetUniqueScriptIdentifier();
				if ( tag.Empty() )
				{
					const CGUID& guid = entry->GetGUID();
					String newTag = entry->GetName();
					if ( !guid.IsZero() )
					{
						Char guidStr[ RED_GUID_STRING_BUFFER_SIZE ];
						guid.ToString( guidStr, RED_GUID_STRING_BUFFER_SIZE );
						newTag += TXT(" ");
						newTag += guidStr;
					}
					if ( !newTag.Empty() )
					{
						CObject* parent = entry->GetParent();
						while ( parent && !parent->IsA< CResource >() )
						{
							parent = parent->GetParent();
						}
						CResource* res = Cast< CResource >( parent );
						if ( res )
						{
							CDiskFile* file = res->GetFile();
							if ( file )
							{
								file->GetStatus();
								if ( file->IsNotSynced() )
								{
									RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("File not synced: %s"), file->GetFileName().AsChar() );
								}
								else if ( !file->IsCheckedOut() )
								{
									if ( !file->SilentCheckOut() )
									{
										RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("File cannot be checked out: %s"), file->GetFileName().AsChar() );
									}
								}

								if ( file->IsCheckedOut() )
								{
									RED_LOG( RED_LOG_CHANNEL( Journal ), TXT("Missing tag generated for entry %s [%s]"), entry->GetFriendlyName().AsChar(), file->GetFileName().AsChar() );
									entry->SetUniqueScriptIdentifier( CName( newTag ) );
									file->Save();
								}
							}
						}
					}
				}
			}
		}
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId childItem = GetFirstChild( item, cookie );
	while( childItem.IsOk() )
	{
		GenerateMissingUniqueScriptTagsRecursively( childItem );
		childItem = GetNextChild( childItem, cookie );
	}
}

void CEdJournalTree::PopulateTreeSection( eTreeCategory category )
{
	if( IsDirectoryDefined() )
	{
		if( !GetRootItem().IsOk() )
		{
			// Add root item
			AddRoot
			(
				wxT( "Journal" ),
				-1,
				-1,
				NULL
			);
		}

		CJournalTree::PopulateTreeSection( new SJournalwxTreeItem( GetCategoryRootItem( category ) ), m_categories[ category ].dirs );
	}
}

SJournalItemHandle* CEdJournalTree::AddItemAppend( SJournalItemHandle* parentHandle, CJournalBase* entry, CJournalResource* journalResource, CDirectory* sectionDirectory )
{
	SJournalwxTreeItem* handle = static_cast< SJournalwxTreeItem* >( parentHandle );

	String caption = GetCaptionForEntry( entry, journalResource );
		
	wxTreeItemId newItem = AddItemAppend( handle->m_itemId, caption.AsChar(), new CEdJournalTreeItemData( entry, journalResource, sectionDirectory ) );

	return new SJournalwxTreeItem( newItem );
}

void CEdJournalTree::MarkItemLocked( const wxTreeItemId& item, Bool locked )
{

}

Bool CEdJournalTree::MarkItemModified( const wxTreeItemId& item, Bool force )
{
	ASSERT( item.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	ASSERT( itemData != NULL );

	if( itemData->m_state != CEdJournalTreeItemData::State_Modified || force )
	{
		itemData->m_state = CEdJournalTreeItemData::State_Modified;

		if( itemData->m_resource )
		{
			SetItemTextColour( item, TREE_COLOUR_EDITED );
		}
		else
		{
			// Item has no associated resource - travel up the hierarchy until a resource is found
			wxTreeItemId parentItem = GetItemParent( item );
			ASSERT( parentItem.IsOk() );

			SetItemTextColour( item, TREE_COLOUR_EDITED_CHILD );

			return MarkItemModified( parentItem, force );
		}

		return true;
	}

	return false;
}

Bool CEdJournalTree::MarkItemSaved( const wxTreeItemId& item, Bool force )
{
	ASSERT( item.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	ASSERT( itemData != NULL );

	if( itemData->m_state != CEdJournalTreeItemData::State_ReadyForSourceControl || force )
	{
		itemData->m_state = CEdJournalTreeItemData::State_ReadyForSourceControl;

		SetItemTextColour( item, TREE_COLOUR_SAVED );

		MarkChildItems( item, CEdJournalTreeItemData::State_Modified, CEdJournalTreeItemData::State_ReadyForSourceControl, TREE_COLOUR_SAVED_CHILD );

		return true;
	}

	return false;
}

Bool CEdJournalTree::MarkItemUnmodified( const wxTreeItemId& item, Bool force )
{
	ASSERT( item.IsOk() );

	CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( GetItemData( item ) );
	ASSERT( itemData != NULL );

	if( itemData->m_state != CEdJournalTreeItemData::State_Unmodified || force )
	{
		itemData->m_state = CEdJournalTreeItemData::State_Unmodified;

		SetItemTextColour( item, TREE_COLOUR_UNMODIFIED );

		MarkChildItems( item, CEdJournalTreeItemData::State_Any, CEdJournalTreeItemData::State_Unmodified, TREE_COLOUR_UNMODIFIED );

		return true;
	}

	return false;
}

void CEdJournalTree::MarkChildItems( const wxTreeItemId& item, CEdJournalTreeItemData::eState oldState, CEdJournalTreeItemData::eState newState, const wxColour& colour )
{
	wxTreeItemIdValue cookie;
	wxTreeItemId child = GetFirstChild( item, cookie );

	while( child.IsOk() )
	{
		CEdJournalTreeItemData* itemData = static_cast< CEdJournalTreeItemData* >( GetItemData( child ) );

		// Don't mark items that haven't been modified
		if( itemData->m_state == oldState || oldState == CEdJournalTreeItemData::State_Any )
		{
			itemData->m_state = newState;

			SetItemTextColour( child, colour );

			MarkChildItems( child, oldState, newState, colour );
		}

		child = GetNextChild( item, cookie );
	}
}

void CEdJournalTree::Sort( SJournalItemHandle* parentHandle )
{
	SJournalwxTreeItem* handle = static_cast< SJournalwxTreeItem* >( parentHandle );

	SortChildren( handle->m_itemId );
}

void CEdJournalTree::PopulateDuplicatedTreeContainers( const wxTreeItemId& entryItem, CJournalBase* entry, Bool entryAlreadyAdded )
{
	wxTreeItemId newItem;

	if ( entryAlreadyAdded )
	{
		newItem = entryItem;
	}
	else
	{
		entry->RecreateGUID();
		newItem = AddItemAppend( entryItem, entry->GetName().AsChar(), new CEdJournalTreeItemData( entry ) );
	}

	if ( entry->IsA< CJournalContainer >() )
	{
		CJournalContainer* container = static_cast< CJournalContainer* >( entry );
		 
		for ( Uint32 entryIdx = 0; entryIdx < container->GetNumChildren(); ++entryIdx )
		{
			CJournalContainerEntry* childEntry = container->GetChild( entryIdx );

			ASSERT( entryIdx == static_cast< Uint32 >( childEntry->GetIndex() ), TXT( "CJournalContainerEntry potentially corrupt - mismatching indices for item %u in %s" ), entryIdx, container->GetName().AsChar() );

			if ( childEntry )
			{
				PopulateDuplicatedTreeContainers( newItem, childEntry, false );
			}
		}
	}
}

TDynArray< CDirectory* > CEdJournalTree::GetDirectories( const Char* path )
{
	ASSERT( IsDirectoryDefined(), TXT( "Functions should check IsDirectoryDefined() before calling to request a directory" ) );

	TDynArray< CDirectory* > directories;

	// Base game directory
	CWitcherGameResource* gameResource = Cast< CWitcherGameResource >( GGame->GetGameResource() );
	const String& basePath = gameResource->GetJournalPath();
	CDirectory* baseDir = GDepot->CreatePath( basePath );
	directories.PushBack( baseDir->CreateNewDirectory( path ) );

	// DLC directories
	CDirectory* dlcRoot = GDepot->FindPath( TXT("dlc\\") );
	if ( dlcRoot != nullptr )
	{
		// Scan each DLC directory
		for ( CDirectory* dlcDir : dlcRoot->GetDirectories() )
		{
			CDirectory* dlcJournal = GDepot->FindPath( String::Printf( TXT("dlc\\%ls\\journal\\"), dlcDir->GetName().AsChar() ).AsChar() );

			// Create journal subdirectories only if we have a journal directory for the DLC
			if ( dlcJournal != nullptr )
			{
				directories.PushBack( dlcJournal->CreateNewDirectory( path ) );
			}
		}
	}

	// Done
	return directories;
}


CJournalTree::CJournalTree()
{

}

CJournalTree::~CJournalTree()
{
	m_handles.ClearPtr();
}

void CJournalTree::PopulateTreeSection( SJournalItemHandle* rootHandle, const TDynArray< CDirectory* >& sectionDirectories )
{
	m_handles.PushBack( rootHandle );

	TDynArray< CDiskFile* > allFiles;
	for ( CDirectory* dir : sectionDirectories )
	{
		for ( CDiskFile* file : dir->GetFiles() )
		{
			allFiles.PushBack( file );
		}
	}

	TJournalItemHandleMap groupHandles;
	TDynArray< CJournalResource* > childResources;

	MaximumNumberOfChildEntries( rootHandle, allFiles.Size() );

	for ( CDiskFile* file : allFiles )
	{
		CResource* resource = GDepot->LoadResource( file->GetDepotPath() );

		if( resource )
		{
			if( resource->IsA< CJournalResource >() )
			{
				resource->GetFile()->GetStatus();

				CJournalResource* journalResource = static_cast< CJournalResource* >( resource );

				if( journalResource->Get()->IsA< CJournalChildBase >() )
				{
					// Child, store for later addage
					childResources.PushBack( journalResource );
				}
				else
				{
					// If it's not a child then it must be a top level entry
					SJournalItemHandle* handle = AddItemAppend( rootHandle, journalResource->Get(), journalResource, file->GetDirectory() );
					ASSERT( handle != NULL, TXT( "SJournalItemHandle instance must be created by AddItemAppend()" ) );
					
					handle->m_resource = journalResource;
					m_handles.PushBack( handle );

					groupHandles.Set( journalResource->Get()->GetGUID(), handle );
				}
			}
			else
			{
				DATA_HALT( DES_Tiny, resource, TXT( "Journal" ), TXT( "Invalid Resource in Journal - Please move it out of the Journal root folder" ) );
			}
		}
		else
		{
			DATA_HALT( DES_Tiny, file->GetResource(), TXT( "Journal" ), TXT( "Invalid file in Journal - Please move it out of the Journal root folder" ) );
		}
	}

	PopulateTreeSubStep( childResources, groupHandles );

	// Sort items
	Sort( rootHandle );

	for( TJournalItemHandleMap::iterator iter = groupHandles.Begin(); iter != groupHandles.End(); ++iter )
	{
		Sort( iter->m_second );
	}
	
	m_handles.ClearPtr();
}

void CJournalTree::PopulateTreeSubStep( TDynArray< CJournalResource* >& childResources, TJournalItemHandleMap& groupHandles )
{
	for( Uint32 i = 0; i < childResources.Size(); ++i )
	{
		ASSERT( childResources[ i ]->Get()->IsA< CJournalChildBase >() );
		CJournalChildBase* child = static_cast< CJournalChildBase* >( childResources[ i ]->Get() );

		SJournalItemHandle** parentResource = groupHandles.FindPtr( child->GetParentGUID() );

		if( parentResource )
		{
			CJournalBase* parent = ( *parentResource )->m_resource->Get();

			SJournalItemHandle* parentHandle = NULL;
			VERIFY( groupHandles.Find( parent->GetGUID(), parentHandle ), TXT( "Parent must have a handle" ) );

			if( child->IsA< CJournalContainer >() )
			{
				PopulateTreeContainers( parentHandle, child, childResources[ i ] );
			}
			else
			{
				// TODO: Find a better place - This will be called multiple times for the same data
				MaximumNumberOfChildEntries( parentHandle, childResources.Size() );

				// If it's not a child then it must be a top level entry
				SJournalItemHandle* handle = AddItemAppend( parentHandle, child, childResources[ i ], childResources[ i ]->GetFile()->GetDirectory() );
				ASSERT( handle != NULL, TXT( "SJournalItemHandle instance must be created by AddItemAppend()" ) );

				handle->m_resource = childResources[ i ];
				m_handles.PushBack( handle );

				groupHandles.Set( child->GetGUID(), handle );
			}
		}
		else
		{
			CDiskFile* file = childResources[ i ]->GetFile();
			RED_LOG( RED_LOG_CHANNEL( JournalError ), TXT("Parentless resource [%s] [%s]"), file? file->GetDepotPath().AsChar(): TXT("unknown"), child->GetName().AsChar() );
		}
	}
}

void CJournalTree::PopulateTreeContainers( SJournalItemHandle* parentHandle, CJournalBase* entry, CJournalResource* resource /*= NULL */ )
{
	SJournalItemHandle* handle = AddItemAppend( parentHandle, entry, resource, NULL );
	handle->m_resource = resource;
	m_handles.PushBack( handle );

	if( entry->IsA< CJournalContainer >() )
	{
		CJournalContainer* container = static_cast< CJournalContainer* >( entry );

		MaximumNumberOfChildEntries( handle, container->GetNumChildren() );

		for( Uint32 entryIdx = 0; entryIdx < container->GetNumChildren(); ++entryIdx )
		{
			CJournalContainerEntry* childEntry = container->GetChild( entryIdx );

			ASSERT( entryIdx == static_cast< Uint32 >( childEntry->GetIndex() ), TXT( "CJournalContainerEntry potentially corrupt - mismatching indices for item %u in %s" ), entryIdx, container->GetName().AsChar() );

			//Emergency Surgery
			if( childEntry == NULL )
			{
				JOURNAL_LOG( TXT( "Journal container '%s' (%s) thinks it has %u child entries, but item %u is missing. This is probably due to the structure of the journal being changed" ), container->GetName().AsChar(), container->GetClass()->GetName().AsString().AsChar(), container->GetNumChildren(), entryIdx );
				container->RemoveChildEntry( entryIdx );

				// Since we've reduced the size of the array, we'll need to iterate over this index again
				--entryIdx;
			}
			else
			{
				PopulateTreeContainers( handle, childEntry, NULL );
			}
		}

		if( container->GetNumChildren() > 1 )
		{
			Sort( handle );
		}
	}
}
