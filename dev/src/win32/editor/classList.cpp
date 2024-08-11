/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "classList.h"

CClassListSelection::CClassListSelection( CPropertyItem* item, CClass* baseClass )
	: CListSelection( item )
	, m_baseClass( baseClass )
{
}

void CClassListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill choice control with values
	TDynArray< CClass* > classes;
	SRTTI::GetInstance().EnumClasses( m_baseClass, classes );

    if ( !m_baseClass->IsAbstract() )
        classes.Insert( 0, m_baseClass );

	if ( !classes.Size() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no matching classes )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Add classes
		TDynArray< String > classNames;
		for ( Uint32 i=0; i<classes.Size(); i++ )
		{
			classNames.PushBack( classes[i]->GetName().AsString() );
		}

		// Sort list and add to combo box
		Sort( classNames.Begin(), classNames.End() );
		for ( Uint32 i=0; i<classNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( classNames[i].AsChar() );
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CClassListSelection::OnChoiceChanged ), NULL, this );
	}
}
