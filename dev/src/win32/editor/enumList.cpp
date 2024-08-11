/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "enumList.h"

CEnumListSelection::CEnumListSelection( CPropertyItem* item )
	: CListSelection( item )
{
}

void CEnumListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill choice control with values
	TDynArray< CEnum* > enums;
	SRTTI::GetInstance().EnumEnums( enums );

	if ( !enums.Size() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Add enums
		TDynArray< String > enumNames;
		for ( Uint32 i=0; i<enums.Size(); i++ )
		{
			enumNames.PushBack( enums[i]->GetName().AsString() );
		}

		// Sort list and add to combo box
		Sort( enumNames.Begin(), enumNames.End() );
		for ( Uint32 i=0; i<enumNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( enumNames[i].AsChar() );
		}

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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEnumListSelection::OnChoiceChanged ), NULL, this );
	}
}
