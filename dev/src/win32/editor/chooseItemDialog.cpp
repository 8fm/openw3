/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "chooseItemDialog.h"

DEFINE_EVENT_TYPE( wxEVT_CHOOSE_ITEM_OK )
DEFINE_EVENT_TYPE( wxEVT_CHOOSE_ITEM_CANCEL )

// Event table
BEGIN_EVENT_TABLE( CEdChooseItemDialog, wxDialog )
	EVT_BUTTON( XRCID( "m_okButton" ), CEdChooseItemDialog::OnOK )
	EVT_BUTTON( XRCID( "m_cancelButton" ), CEdChooseItemDialog::OnCancel )
	EVT_BUTTON( XRCID( "m_anyItemButton" ), CEdChooseItemDialog::OnAny )
	EVT_BUTTON( XRCID( "m_noneItemButton" ), CEdChooseItemDialog::OnNoneItem )
	EVT_CLOSE( CEdChooseItemDialog::OnClose )
	EVT_LISTBOX_DCLICK( XRCID( "m_itemsList"), CEdChooseItemDialog::OnItemDoubleClicked )
	EVT_LISTBOX( XRCID( "m_categoriesList" ), CEdChooseItemDialog::OnCategorySelected )
	EVT_LISTBOX( XRCID( "m_itemsList" ), CEdChooseItemDialog::OnItemSelected )
END_EVENT_TABLE()

CEdChooseItemDialog::CEdChooseItemDialog( wxWindow* parent, CName currentItem )
{
	// Load designed dialog from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT("ChooseItemEditor") );

	// Get controls from resource
	m_categoriesList = XRCCTRL( *this, "m_categoriesList", wxListBox );
	m_itemsList = XRCCTRL( *this, "m_itemsList", wxListBox );
	m_itemDescription = XRCCTRL( *this, "m_itemDescription", wxTextCtrl );

	// Fill categories list
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	TDynArray< CName > categories;
	defMgr->GetItemCategories( categories );
	m_categoriesList->Freeze();
	for ( Uint32 i=0; i<categories.Size(); ++i )
	{
		m_categoriesList->Append( categories[i].AsString().AsChar() );
	}
	m_categoriesList->Thaw();

	// Select current category
	// 1. Check whether current value is a category
	if ( currentItem && categories.Exist( currentItem ) )
	{
		m_categoriesList->SetStringSelection( currentItem.AsString().AsChar() );
		FillItemsList( currentItem );
		FillDescriptionBox( currentItem );
	}
	// 2. Check whether current value is an item name
	else if ( currentItem && defMgr->GetItemDefinition( currentItem ) )
	{
		const SItemDefinition* itemDef = defMgr->GetItemDefinition( currentItem );
		// Select category of that item
		m_categoriesList->SetStringSelection( itemDef->m_category.AsString().AsChar() );

		// Fill items of this category
		FillItemsList( itemDef->m_category );
		m_itemsList->SetStringSelection( currentItem.AsString().AsChar() );
		FillDescriptionBox( currentItem );
	}

	m_item = currentItem;

	Layout();
	Refresh();
}

CEdChooseItemDialog::~CEdChooseItemDialog()
{
}

void CEdChooseItemDialog::OnOK( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_CHOOSE_ITEM_OK );
	okPressedEvent.SetString( m_item.AsString().AsChar() );
	okPressedEvent.SetEventObject( this );

	ProcessEvent( okPressedEvent );

	// Close window
	EndModal(wxOK);
	Destroy();
}

void CEdChooseItemDialog::OnCancel( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_CHOOSE_ITEM_CANCEL );
	okPressedEvent.SetEventObject( this );

	ProcessEvent( okPressedEvent );

	// Close window
	EndModal(wxCANCEL);
	Destroy();
}

void CEdChooseItemDialog::OnAny( wxCommandEvent& event )
{
	// Send to parent
	m_item = CNAME( Any );

	wxCommandEvent fakeEvent;
	OnOK( fakeEvent );
}

void CEdChooseItemDialog::OnNoneItem( wxCommandEvent& event )
{
	// Send to parent
	m_item = CName::NONE;

	wxCommandEvent fakeEvent;
	OnOK( fakeEvent );
}

void CEdChooseItemDialog::OnClose( wxCloseEvent& event )
{
	wxCommandEvent fakeEvent;
	OnCancel( fakeEvent );
}

void CEdChooseItemDialog::FillItemsList( CName category )
{
	const CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	ASSERT( defMgr );
	
	m_itemsList->Freeze();
	const TDynArray< CName >& items = defMgr->GetItemsOfCategory( category );
	m_itemsList->Clear();
	m_itemsList->Append( TXT("None") );
	for ( Uint32 i=0; i<items.Size(); ++i )
	{
		m_itemsList->Append( items[i].AsString().AsChar() );
	}
	m_itemsList->Thaw();
}

void CEdChooseItemDialog::OnCategorySelected( wxCommandEvent& event )
{
	CName category( m_categoriesList->GetStringSelection().c_str() );
	FillItemsList( category );
	m_item = category;
	FillDescriptionBox( category );
}

void CEdChooseItemDialog::OnItemSelected( wxCommandEvent& event )
{
	CName item( m_itemsList->GetStringSelection().c_str() );

	m_item = item;
	FillDescriptionBox( item );
}

void CEdChooseItemDialog::OnItemDoubleClicked( wxCommandEvent& event )
{
	wxCommandEvent fakeEvent;
	OnOK( fakeEvent );
}

void CEdChooseItemDialog::FillDescriptionBox( CName item )
{
	String desc;
	if ( GCommonGame->GetDefinitionsManager()->CategoryExists( item ) )
	{
		// It is a category
		Uint32 numItems = GCommonGame->GetDefinitionsManager()->GetItemsOfCategory( item ).Size();
		desc = String::Printf( TXT("Selected category: %s\nNumber of items: %d"), item.AsString().AsChar(), numItems );
	}
	else
	{
		// It is a specific item
		const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item );
		
		//ASSERT( itemDef );

		if( itemDef )
		{
			itemDef->GetItemDescription( desc, false );
		}
		
	}

	m_itemDescription->Clear();
	m_itemDescription->AppendText( desc.AsChar() );
}
