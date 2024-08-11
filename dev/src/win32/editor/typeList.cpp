/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "typeList.h"

CTypeListSelection::CTypeListSelection( CPropertyItem* item )
	: CListSelection( item )
	, m_ctrlComboBox( NULL )
{
}

void CTypeListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlComboBox = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlComboBox->SetWindowStyle( wxCB_DROPDOWN | wxTE_PROCESS_ENTER );
	m_ctrlComboBox->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlComboBox->SetFocus();
	
	m_ctrlComboBox->AppendString( RED_NAME( Bool ).AsString().AsChar() );
	m_ctrlComboBox->AppendString( RED_NAME( Float ).AsString().AsChar() );
	m_ctrlComboBox->AppendString( RED_NAME( Int32 ).AsString().AsChar() );
	m_ctrlComboBox->AppendString( RED_NAME( CName ).AsString().AsChar() );
	m_ctrlComboBox->AppendString( RED_NAME( GameTime ).AsString().AsChar() );
	m_ctrlComboBox->AppendString( TXT("*CEntity") );

	// Find current value on list and select it
	String str;
	GrabValue( str );
	int index = m_ctrlComboBox->FindString( str.AsChar() );
	if ( index >= 0 )
	{
		m_ctrlComboBox->SetSelection( index );
	}
	else
	{
		m_ctrlComboBox->SetValue( str.AsChar() );
	}

	// Notify of selection changes
	m_ctrlComboBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CTypeListSelection::OnChoiceChanged ), NULL, this );
	m_ctrlComboBox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CTypeListSelection::OnChoiceChanged ), NULL, this );
}

void CTypeListSelection::CloseControls()
{
	// Close combo box
	if ( m_ctrlComboBox )
	{
		delete m_ctrlComboBox;
		m_ctrlComboBox = NULL;
	}
}

Bool CTypeListSelection::GrabValue( String& displayValue )
{
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName name;
		m_propertyItem->Read( &name );
		displayValue = name.AsString();
	}
	else
		m_propertyItem->Read( &displayValue );
	return true;
}

Bool CTypeListSelection::SaveValue()
{
	String value = m_ctrlComboBox->GetValue().wc_str();
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName name( value );
		m_propertyItem->Write( &name );
	}
	else
	{
		m_propertyItem->Write( &value );
	}
	return true;
}