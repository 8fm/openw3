/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "selectionList.h"

CListSelection::CListSelection( CPropertyItem* item )
	: ICustomPropertyEditor( item )
	, m_ctrlChoice( NULL )
	, m_clearButton( NULL )
	, m_isCleared( false )
{
	ASSERT( item->GetPropertyType()->GetName() == CNAME( String ) ||
			item->GetPropertyType()->GetName() == CNAME( CName ) );
}

void CListSelection::CloseControls()
{
	// Close combo box
	if ( m_ctrlChoice )
	{
		delete m_ctrlChoice;
		m_ctrlChoice = NULL;
	}

	// Remove clear button
	if ( m_clearButton )
	{
		delete m_clearButton;
		m_clearButton = NULL;
	}
}

Bool CListSelection::GrabValue( String& displayValue )
{
	// Get value from target
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		// As CName
		CName name;
		m_propertyItem->Read( &name );
		displayValue = name.AsString();

		if ( m_isCleared && m_ctrlChoice != nullptr )
		{
			m_ctrlChoice->SetText( wxT("None") );
		}
	}
	else
	{
		// As string
		m_propertyItem->Read( &displayValue );
	}

	// Assume grabbed value
	return true;
}

Bool CListSelection::SaveValue()
{
	// If the last action was to press the clear button, clear the value
	if ( m_isCleared )
	{
		ClearValue();
		return true;
	}

	// Get string value from list
	String value = m_ctrlChoice->GetString( m_ctrlChoice->GetSelection() ).wc_str();

	// Write result
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		// Write as CName
		CName name( value );
		m_propertyItem->Write( &name );
	}
	else
	{
		// Write as string
		m_propertyItem->Write( &value );
	}

	// Assume written
	return true;
}

void CListSelection::ClearValue()
{
	// For CNames write None
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		// Write as CName
		CName name = CName::NONE;
		m_propertyItem->Write( &name );
	}
	else
	{
		String empty;

		// Write as empty string
		m_propertyItem->Write( &empty );
	}
}

void CListSelection::OnChoiceChanged( wxCommandEvent &event )
{
	m_isCleared = false;
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void CListSelection::OnClearClicked( wxCommandEvent &event )
{
	m_isCleared = true;
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}
