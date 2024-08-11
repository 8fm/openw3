/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "cameraShakeEnumList.h"

CEdCameraShakeEnumPropertyEditor::CEdCameraShakeEnumPropertyEditor( CPropertyItem* item )
	: CListSelection( item )
{

}

void CEdCameraShakeEnumPropertyEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill choice control with values
	CEnum* en = SRTTI::GetInstance().FindEnum( CNAME( ECameraShake ) );

	if ( !en )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		const TDynArray< CName > &enumVals = en->GetOptions();

		for ( Uint32 i=0; i<enumVals.Size(); i++ )
		{
			m_ctrlChoice->AppendString( enumVals[i].AsString().AsChar() );
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCameraShakeEnumPropertyEditor::OnChoiceChanged ), NULL, this );
	}

}