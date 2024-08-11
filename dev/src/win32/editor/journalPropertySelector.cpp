#include "build.h"
#include "journalPropertySelector.h"
#include "journalTree.h"

#include "../../common/game/journalPath.h"

CEdJournalPropertySelector::CEdJournalPropertySelector( CPropertyItem* propertyItem, Uint32 journalFlags, const CClass* typeSelectable )
:	ICustomPropertyEditor( propertyItem ),
	m_tree( NULL ),
	m_flags( journalFlags ),
	m_typeSelectable( typeSelectable )
{

}

CEdJournalPropertySelector::~CEdJournalPropertySelector()
{
}

void CEdJournalPropertySelector::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxSize size = propertyRect.GetSize();
	size.SetHeight( 200 );

	m_tree = new CEdJournalTree();

	m_tree->Create
	(
		m_propertyItem->GetPage(),
		wxID_ANY,
		propertyRect.GetTopLeft(),
		size,
		wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxTR_SINGLE | wxBORDER
	);

	{
		wxBusyCursor busy;
	
		m_tree->Initialize();

		for( Uint32 i = 0; i < CEdJournalTree::TreeCategory_Max; ++i )
		{
			if( ( m_flags & ( 1 << i ) ) )
			{
				// Are there other categories specified?
				if( !ISPOW2( m_flags ) )
				{
					m_tree->AddCategoryRoot( static_cast< CEdJournalTree::eTreeCategory >( i ) );
				}

				m_tree->PopulateTreeSection( static_cast< CEdJournalTree::eTreeCategory >( i ) );
			}
		}
	}

	m_tree->Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CEdJournalPropertySelector::OnItemSelected, this );


	THandle< CJournalPath > path;
	m_propertyItem->Read( &path );

	if( path )
	{
		m_tree->ExpandPath( path );
	}

	m_tree->SetFocus();
	m_tree->Show();

	outSpawnedControls.PushBack( m_tree );
}

void CEdJournalPropertySelector::CloseControls()
{
	if( m_tree != NULL )
	{
		m_tree->Destroy();
		m_tree = nullptr;
	}
}

Bool CEdJournalPropertySelector::SaveValue()
{
	if ( m_selectedPath )
	{
		m_propertyItem->Write( &m_selectedPath );
		return true;
	}

	return false;
}


Bool CEdJournalPropertySelector::GrabValue( String& displayValue )
{
	THandle< CJournalPath > path;
	m_propertyItem->Read( &path );

	if ( path )
	{
		displayValue = path->GetPathAsString();
		return true;
	}
	else
	{
		return false;
	}
}

void CEdJournalPropertySelector::OnItemSelected( const wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();

	wxTreeItemData* data = m_tree->GetItemData( item );

	if( data )
	{
		CEdJournalTreeItemData* journalData = static_cast< CEdJournalTreeItemData* >( data );

		if( !m_typeSelectable || journalData->m_entry->IsA( m_typeSelectable ) )
		{
			m_selectedPath = CJournalPath::ConstructPathFromTargetEntry( journalData->m_entry, journalData->m_resource );
 			m_propertyItem->SavePropertyValue();
 			m_propertyItem->GrabPropertyValue();
			m_propertyItem->GetPage()->SelectItem( nullptr );
		}
	}
}
