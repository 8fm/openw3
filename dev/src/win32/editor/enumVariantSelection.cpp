/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "enumVariantSelection.h"
#include "../../common/engine/extAnimScriptEvent.h"

CEnumVariantSelection::CEnumVariantSelection( CPropertyItem* item )
	: ICustomPropertyEditor( item )
	, m_typeChoice( NULL )
	, m_valueChoice( NULL )
{
	ASSERT( item->GetPropertyType()->GetName() == CNAME( SEnumVariant ) );
}

void CEnumVariantSelection::CloseControls()
{
	// Close combo box
	if ( m_typeChoice )
	{
		delete m_typeChoice;
		m_typeChoice = NULL;
	}

	if ( m_valueChoice )
	{
		delete m_valueChoice;
		m_valueChoice = NULL;
	}
}

Bool CEnumVariantSelection::GrabValue( String& displayValue )
{
	SEnumVariant variant;
	m_propertyItem->Read( &variant );
	CEnum* type = SRTTI::GetInstance().FindEnum( variant.m_enumType );
	if( !type )
	{
		displayValue = TXT("None");
		return true;
	}
	
	CName name;
	type->FindName( variant.m_enumValue, name );
	displayValue = name.AsString();

	return true;
}

Bool CEnumVariantSelection::SaveValue()
{
	const CEnum* selection = SRTTI::GetInstance().FindEnum( CName( m_typeChoice->GetString( m_typeChoice->GetSelection() ).wc_str() ) );
	if( !selection )
	{
		return false;
	}

	SEnumVariant variant;
	variant.m_enumType = selection->GetName();
	selection->FindValue( CName( m_valueChoice->GetString( m_valueChoice->GetSelection() ).wc_str() ), variant.m_enumValue );

	m_propertyItem->Write( &variant );

	// Assume written
	return true;
}

void CEnumVariantSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	wxSize size = propRect.GetSize();
	size.x /= 2.f;
	wxPoint pos = propRect.GetTopLeft();
	m_typeChoice = new wxChoice( m_propertyItem->GetPage(), wxID_ANY, pos, size );
	m_typeChoice->SetWindowStyle( wxNO_BORDER );
	m_typeChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_typeChoice->SetFocus();

	pos.x += size.x;
	m_valueChoice = new wxChoice( m_propertyItem->GetPage(), wxID_ANY, pos, size );
	m_typeChoice->SetWindowStyle( wxNO_BORDER );
	m_typeChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	// Fill choice control with values
	TDynArray< CEnum* > enums;
	SRTTI::GetInstance().EnumEnums( enums );

	if ( !enums.Size() )
	{
		// Add empty stub
		m_typeChoice->Enable( false );
		m_typeChoice->SetSelection( 0 );
	}
	else
	{
		// Add enums
		TDynArray< String > enumNames;
		for ( Uint32 i=0; i<enums.Size(); i++ )
		{
			enumNames.PushBack( enums[i]->GetName().AsString().AsChar() );
		}

		// Sort list and add to combo box
		Sort( enumNames.Begin(), enumNames.End() );
		for ( Uint32 i=0; i<enumNames.Size(); i++ )
		{
			m_typeChoice->AppendString( enumNames[i].AsChar() );
		}

		// Find current value on list and select it
		SEnumVariant variant;
		m_propertyItem->Read( &variant );
		int index = m_typeChoice->FindString( variant.m_enumType.AsString().AsChar() );
		if ( index >= 0 )
		{
			m_typeChoice->SetSelection( index );
		}
		else
		{
			m_typeChoice->SetSelection( 0 );
		}

		OnChoiceChanged( wxCommandEvent() );

		// Notify of selection changes
		m_typeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEnumVariantSelection::OnChoiceChanged ), NULL, this );
	}
}

void CEnumVariantSelection::OnChoiceChanged( wxCommandEvent &event )
{
	m_valueChoice->Clear();
	const CEnum* selection = SRTTI::GetInstance().FindEnum( CName( m_typeChoice->GetString( m_typeChoice->GetSelection() ).wc_str() ) );
	if( !selection )
	{
		return;
	}

	const TDynArray<CName>& options = selection->GetOptions();
	const Uint32 size = options.Size();

	if ( !size )
	{
		// Add empty stub
		m_valueChoice->Enable( false );
		m_valueChoice->SetSelection( 0 );
	}
	else
	{
		for ( Uint32 i = 0; i < size; i++ )
		{
			m_valueChoice->AppendString( options[i].AsString().AsChar() );
		}

		// Find current value on list and select it
		SEnumVariant value;
		m_propertyItem->Read( &value );
		CName name;
		selection->FindName( value.m_enumValue, name );
		int index = m_valueChoice->FindString( name.AsString().AsChar() );
		if ( index >= 0 )
		{
			m_valueChoice->SetSelection( index );
		}
		else
		{
			m_valueChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_valueChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEnumVariantSelection::OnValueChanged ), NULL, this );
	}

	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void CEnumVariantSelection::OnValueChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}
