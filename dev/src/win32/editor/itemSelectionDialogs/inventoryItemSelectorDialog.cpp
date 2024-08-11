#include "build.h"

#include "inventoryItemSelectorDialog.h"

CEdInventoryItemSelector::CEdInventoryItemSelector( wxWindow* parent, const CName& defaultSelected, INamesListOwner* parentCategory )
:	CEdItemSelectorDialog( parent, TXT( "/Frames/StandardSelectorDialog" ), TXT( "Inventory Items" ) ),
	m_defaultSelected( defaultSelected ),
	m_parentCategory( parentCategory )
{
}

CEdInventoryItemSelector::~CEdInventoryItemSelector()
{

}

void CEdInventoryItemSelector::Populate()
{
	if ( m_parentCategory )
	{
		m_parentCategory->GetNamesList( m_options );
	}
	else
	{
		InventoryEditor::GetItemList( m_options );
	}

	m_options.Insert( 0, CName::NONE );

	for( Uint32 i = 0; i < m_options.Size(); ++i )
	{
		AddItem( m_options[ i ].AsString(), &m_options[ i ], true, -1, m_defaultSelected == m_options[ i ] );
	}
}
