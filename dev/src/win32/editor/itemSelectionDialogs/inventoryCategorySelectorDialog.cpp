#include "build.h"

#include "inventoryCategorySelectorDialog.h"

CEdInventoryCategorySelector::CEdInventoryCategorySelector( wxWindow* parent, const CName& defaultSelected )
	: CEdItemSelectorDialog( parent, TXT( "/Frames/StandardSelectorDialog" ), TXT( "Inventory Category" ) )
	, m_defaultSelected( defaultSelected )
{
}

CEdInventoryCategorySelector::~CEdInventoryCategorySelector()
{

}

void CEdInventoryCategorySelector::Populate()
{
	InventoryEditor::GetItemCategoriesList( m_categories );

	for( Uint32 i = 0; i < m_categories.Size(); ++i )
	{
		AddItem( m_categories[ i ].AsString(), &m_categories[ i ], true, -1, m_defaultSelected == m_categories[ i ] );
	}
}
