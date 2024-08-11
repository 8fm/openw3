/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "entityAppearanceSelection.h"

ISelectionEditor::ISelectionEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlChoice( NULL )
{

}

void ISelectionEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	Bool isTextEditable = IsTextEditable();
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize(), isTextEditable );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();
	m_ctrlChoice->Freeze();
	outSpawnedControls.PushBack( m_ctrlChoice );

	// fill choice control with values
	FillChoices();

	// Find current value on list and select it
	String str;
	GrabValue( str );
	Int32 index = m_ctrlChoice->FindString( str.AsChar() );
	if ( index >=0 )
	{
		m_ctrlChoice->SetSelection( index );
	}
	else
	{
		m_ctrlChoice->SetValue( str.AsChar() );
	}

	m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( ISelectionEditor::OnChoiceChanged ), NULL, this );

	m_ctrlChoice->Thaw();
}

void ISelectionEditor::CloseControls()
{
	// Close combo box
	if ( m_ctrlChoice )
	{
		wxPendingDelete.Append( m_ctrlChoice );
		m_ctrlChoice = nullptr;
	}
}

Bool ISelectionEditor::GrabValue( String& displayValue )
{
	String stored;

	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName name;
		m_propertyItem->Read( &name );
		stored = name.AsString();
	}
	else
	{
		m_propertyItem->Read( &stored );
	}

	displayValue = StoredToDisplayed( stored );

	return true;
}

Bool ISelectionEditor::SaveValue()
{
	String displayed = m_ctrlChoice->GetValue().wc_str();
	String stored = DisplayedToStored( displayed );

	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		m_propertyItem->Write( &CName( stored ) );
	}
	else
	{
		m_propertyItem->Write( &stored );
	}

	return true;
}

void ISelectionEditor::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
}

Bool ISelectionEditor::IsTextEditable() const
{
	return false;
}

String ISelectionEditor::DisplayedToStored( const String& val ) 
{ 
	return val; 
}

String ISelectionEditor::StoredToDisplayed( const String& val ) 
{ 
	return val; 
}


// --------------------------------

CEdMappedSelectionEditor::CEdMappedSelectionEditor( CPropertyItem* item )
	: ISelectionEditor( item )
{
}

void CEdMappedSelectionEditor::FillChoices()
{
	if ( m_map.Empty() )
	{
		FillMap( m_map ); // cannot be called from the constructor (as it's virtual), so "fill on the first use" approach is chosen
	}

	for ( auto elem : m_map )
	{
		m_ctrlChoice->AppendString( elem.m_second.AsChar() );
	}
}

String CEdMappedSelectionEditor::DisplayedToStored( const String& val )
{
	if ( m_map.Empty() )
	{
		FillMap( m_map );
	}

	auto it = FindIf( m_map.Begin(), m_map.End(), [&val]( const TPair< String, String >& elem ) { return elem.m_second == val; } );
	return it != m_map.End() ? it->m_first : String::EMPTY;
}

String CEdMappedSelectionEditor::StoredToDisplayed( const String& val )
{
	if ( m_map.Empty() )
	{
		FillMap( m_map );
	}

	auto it = FindIf( m_map.Begin(), m_map.End(), [&val]( const TPair< String, String >& elem ) { return elem.m_first == val; } );
	return it != m_map.End() ? it->m_second : String::EMPTY;
}

Bool CEdMappedSelectionEditor::IsTextEditable() const
{
	// having mapped combo doesn't make much sense when user can enter a random value into it
	return false; 
}
