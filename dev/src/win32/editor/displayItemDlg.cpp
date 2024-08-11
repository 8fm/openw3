/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "displayItemDlg.h"
#include "../../common/game/inventoryEditor.h"

BEGIN_EVENT_TABLE( CEdDisplayItemDialog, wxDialog )
EVT_BUTTON( XRCID("m_OkButton"), CEdDisplayItemDialog::OnOK )
EVT_BUTTON( XRCID("m_cancelButton"), CEdDisplayItemDialog::OnCancel )
END_EVENT_TABLE()

CEdDisplayItemDialog::CEdDisplayItemDialog( wxWindow* parent, CItemDisplayer* displayer )
: m_itemDisplayer( displayer )
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("DisplayItemDialog") );
	m_itemCategoryCombo = XRCCTRL( *this, "m_itemCategoryCombo", wxComboBox );
	m_itemCategoryCombo->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CEdDisplayItemDialog::OnCategorySelected ), NULL, this );

	// Fill categories 
	TDynArray< CName > availableCategories;
	InventoryEditor::GetItemCategoriesList( availableCategories );
	m_itemCategoryCombo->Clear();
	for ( Uint32 i=0; i<availableCategories.Size(); ++i )
	{
		m_itemCategoryCombo->Append( availableCategories[i].AsString().AsChar() );
	}

	m_itemNameCombo = XRCCTRL( *this, "m_itemNameCombo", wxComboBox );
	m_slotCombo = XRCCTRL( *this, "m_targetCombo", wxComboBox );
}

void CEdDisplayItemDialog::OnCategorySelected( wxCommandEvent& event )
{
	String category = m_itemCategoryCombo->GetValue();
	CName categoryName( category );

	m_itemNameCombo->Clear();
	TDynArray< CName > availableItems;
	InventoryEditor::GetItemsOfCategory( availableItems, categoryName );
	for ( Uint32 i=0; i<availableItems.Size(); ++i )
	{
		m_itemNameCombo->Append( availableItems[i].AsString().AsChar() );
	}
}

void CEdDisplayItemDialog::OnOK( wxCommandEvent& event )
{
	if ( !m_itemDisplayer )
	{
		return;
	}

	// Get name of the item
	String item = m_itemNameCombo->GetValue();
	CName itemName( item );

	// Get target slot
	String slot = m_slotCombo->GetValue();
	EItemDisplaySlot displaySlot;
	if ( slot == TXT("RightHand") )
	{
		displaySlot = IDS_RightHand;
	}
	else if ( slot == TXT("LeftHand") )
	{
		displaySlot = IDS_LeftHand;
	}
	else if ( slot == TXT("Mount") )
	{
		displaySlot = IDS_Mount;
	}

	// Display
	m_itemDisplayer->DisplayItem( itemName, displaySlot );

	// Done
	EndDialog( wxID_OK );
}

void CEdDisplayItemDialog::OnCancel( wxCommandEvent& event )
{
	// Don't do anything
	EndDialog( wxID_CANCEL );
};