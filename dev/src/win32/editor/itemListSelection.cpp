/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "itemListSelection.h"
#include "../../common/game/inventoryEditor.h"
#include "../../common/game/inventoryDefinition.h"
#include "../../common/game/extAnimItemEvents.h"

#include "itemSelectionDialogs/inventoryCategorySelectorDialog.h"
#include "itemSelectionDialogs/inventoryItemSelectorDialog.h"

RED_DEFINE_STATIC_NAME(effects);
RED_DEFINE_STATIC_NAME(name_name);

CItemListSelection::CItemListSelection( CPropertyItem* item )
: CEdBasicSelectorDialogPropertyEditor( item )
{
}

Bool CItemListSelection::GrabValue( String& displayValue )
{
	// As CName
	CName name;
	m_propertyItem->Read( &name );
	displayValue = name.AsString();

	return true;
}

void CItemListSelection::OnSelectDialog( wxCommandEvent& event )
{
	CName currentValue;
	m_propertyItem->Read( &currentValue );

	CEdInventoryItemSelector selector( m_focusPanel, currentValue );
	if ( CName* selected = selector.Execute() )
	{
		m_propertyItem->Write( selected );
		m_textDisplay->ChangeValue( selected->AsString().AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

CAbilityListSelection::CAbilityListSelection( CPropertyItem* item )
: CListSelection( item )
{
}

void CAbilityListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();
	m_ctrlChoice->Freeze();

	// Empty item
	//m_ctrlChoice->AppendString( wxEmptyString );

	// Get items
	m_abilities.Clear();
	InventoryEditor::GetAbilityList( m_abilities );

	wxArrayString abilities;
	for (Uint32 i = 0; i < m_abilities.Size(); ++i)
	{
		abilities.Add( m_abilities[i].AsString().AsChar() );
	}

	// Append sorted items
	abilities.Sort();
	m_ctrlChoice->Append( abilities );

	// Make sure empty list is displayed properly
	if ( !m_ctrlChoice->GetCount() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no items )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = m_ctrlChoice->FindString( str.AsChar() );
		if ( index >= 0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
		else
		{
			m_ctrlChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CAbilityListSelection::OnChoiceChanged ), NULL, this );
	}

	// Refresh
	m_ctrlChoice->Thaw();
	m_ctrlChoice->Refresh();
}

//////////////////////////////////////////////////////////////////////////

CItemCategoriesListSelection::CItemCategoriesListSelection( CPropertyItem* item )
: CEdBasicSelectorDialogPropertyEditor( item )
{
}

Bool CItemCategoriesListSelection::GrabValue( String& displayValue )
{
	// As CName
	CName name;
	m_propertyItem->Read( &name );
	displayValue = name.AsString();

	return true;
}

void CItemCategoriesListSelection::OnSelectDialog( wxCommandEvent& event )
{
	CName currentValue;
	m_propertyItem->Read( &currentValue );

	CEdInventoryCategorySelector selector( m_focusPanel, currentValue );
	if ( CName* selected = selector.Execute() )
	{
		m_propertyItem->Write( selected );
		m_textDisplay->ChangeValue( selected->AsString().AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

CSuggestedListSelection::CSuggestedListSelection( CPropertyItem* item )
: CEdBasicSelectorDialogPropertyEditor( item )
{
}

Bool CSuggestedListSelection::GrabValue( String& displayValue )
{
	// As CName
	CName name;
	m_propertyItem->Read( &name );
	displayValue = name.AsString();

	return true;
}

void CSuggestedListSelection::OnSelectDialog( wxCommandEvent& event )
{
	// Get options list from parent using INamesListOwner interface
	INamesListOwner*	propertyOwner = NULL;
	CBasePropItem*		pitem = m_propertyItem;

	while ( propertyOwner == NULL && pitem != NULL )
	{
		propertyOwner = dynamic_cast< INamesListOwner* >( pitem->GetParentObject( 0 ).AsObject() );
		pitem = pitem->GetParent();
	}

	if( propertyOwner )
	{
		CName currentValue;
		m_propertyItem->Read( &currentValue );

		CEdInventoryItemSelector selector( m_focusPanel, currentValue, propertyOwner );
		if ( CName* selected = selector.Execute() )
		{
			m_propertyItem->Write( selected );
			m_textDisplay->ChangeValue( selected->AsString().AsChar() );
		}
	}
	else
	{
		HALT( "Couldn't Find INamesListOwner for property %s", m_propertyItem->GetName().AsChar() );
	}
}

//////////////////////////////////////////////////////////////////////////

CAnimEvent_SuggestedListSelection::CAnimEvent_SuggestedListSelection( CPropertyItem* item )
: CListSelection( item )
{
}

void CAnimEvent_SuggestedListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();
	m_ctrlChoice->Freeze();

	// Empty item
	m_ctrlChoice->AppendString( TXT("None") );

	// Get options list from parent using INamesListOwner interface
	INamesListOwner *propertyOwner = dynamic_cast< INamesListOwner * >( m_propertyItem->GetParentObject( 0 ).As< CExtAnimItemEvent >( ) );

	ASSERT( propertyOwner );
	if ( ! propertyOwner )
		return;

	propertyOwner->GetNamesList( m_options );

	wxArrayString items;
	for (Uint32 i = 0; i < m_options.Size(); ++i)
	{
		items.Add( m_options[i].AsString().AsChar() );
	}

	// Append sorted items
	items.Sort();
	m_ctrlChoice->Append( items );

	// Make sure empty list is displayed properly
	if ( !m_ctrlChoice->GetCount() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no items )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = m_ctrlChoice->FindString( str.AsChar() );
		if ( index >= 0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
		else
		{
			m_ctrlChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CAnimEvent_SuggestedListSelection::OnChoiceChanged ), NULL, this );
	}

	// Refresh
	m_ctrlChoice->Thaw();
	m_ctrlChoice->Refresh();
}

void CEffectsListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();
	m_ctrlChoice->Freeze();

	// Empty item
	//m_ctrlChoice->AppendString( wxEmptyString );

	// Get items
	m_options.Clear();

	const TDynArray< SCustomNode >& nodes = GCommonGame->GetDefinitionsManager()->GetCustomNodes();
	for( auto custNode = nodes.Begin(); custNode != nodes.End() ; ++custNode )
	{
		if( custNode->m_nodeName == CNAME( effects ) )
		{
			for( auto effectNode = custNode->m_subNodes.Begin(); effectNode != custNode->m_subNodes.End() ; ++effectNode )
			{
				for( auto attribute = effectNode->m_attributes.Begin(); attribute != effectNode->m_attributes.End() ; ++attribute )
				{
					if( attribute->GetAttributeName() == CNAME( name_name ) )
					{
						m_options.PushBack( attribute->GetValueAsCName() );
					}
				}
			}
		}
	}

	wxArrayString abilities;
	for (Uint32 i = 0; i < m_options.Size(); ++i)
	{
		abilities.Add( m_options[i].AsString().AsChar() );
	}

	// Append sorted items
	abilities.Sort();
	m_ctrlChoice->Append( abilities );

	// Make sure empty list is displayed properly
	if ( !m_ctrlChoice->GetCount() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no items )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = m_ctrlChoice->FindString( str.AsChar() );
		if ( index >= 0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
		else
		{
			m_ctrlChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEffectsListSelection::OnChoiceChanged ), NULL, this );
	}

	// Refresh
	m_ctrlChoice->Thaw();
	m_ctrlChoice->Refresh();
}
