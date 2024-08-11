/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "soundMaterialSelector.h"

CEdSoundMaterialSelector::CEdSoundMaterialSelector( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlChoice( NULL )
{
	// Get materials
}

void CEdSoundMaterialSelector::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// fill choice control with values
	FillChoices();

	// Find current value on list and select it
	Int8 value;
	m_propertyItem->Read( &value );

	StringAnsi* strPtr = m_materials.FindPtr( value );
	if ( strPtr )
	{
		unsigned int index = m_ctrlChoice->FindString( strPtr->AsChar() );
		if ( index >=0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
	}

	m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdSoundMaterialSelector::OnChoiceChanged ), NULL, this );
}

void CEdSoundMaterialSelector::CloseControls()
{
	// Close combo box
	if ( m_ctrlChoice )
	{
		delete m_ctrlChoice;
		m_ctrlChoice = NULL;
	}
}

Bool CEdSoundMaterialSelector::GrabValue( String& displayValue )
{
	// Grab value
	Int8 value;
	m_propertyItem->Read( &value );

	// Grab string representation
	StringAnsi* strPtr = m_materials.FindPtr( value );
	if ( strPtr != NULL )
	{
		displayValue = ANSI_TO_UNICODE( strPtr->AsChar() );
		return true;
	}
	
	return false;
}

Bool CEdSoundMaterialSelector::SaveValue()
{
	StringAnsi value = m_ctrlChoice->GetString( m_ctrlChoice->GetSelection() ).c_str();

	// Find value
	for( THashMap< Uint32, StringAnsi >::const_iterator matIter = m_materials.Begin();
		matIter != m_materials.End(); ++matIter )
	{
		if( value == matIter->m_second )
		{
			m_propertyItem->Write( const_cast< Uint32* >( &matIter->m_first ) );
			return true;
		}
	}

	return false;
}

void CEdSoundMaterialSelector::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
}

void CEdSoundMaterialSelector::FillChoices()
{
	for( THashMap< Uint32, StringAnsi >::const_iterator matIter = m_materials.Begin();
		matIter != m_materials.End(); ++matIter )
	{
		m_ctrlChoice->AppendString( matIter->m_second.AsChar() );
	}
}
