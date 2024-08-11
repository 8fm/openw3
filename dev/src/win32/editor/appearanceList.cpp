/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/appearanceComponent.h"

#include "appearanceList.h"


CAppearanceListSelection::CAppearanceListSelection( CPropertyItem* item )
: CListSelection( item )
{
	CAppearanceComponent* comp = item->GetParentObject(0).As< CAppearanceComponent >();
	CEntity* ent = comp->GetEntity();

	m_template = Cast<CEntityTemplate>( ent->GetTemplate() );
}

void CAppearanceListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	if ( ! m_template )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no appearances )") );
		m_ctrlChoice->SetSelection( 0 );
		return;
	}

	// Fill choice control with values
	const TDynArray< CName > & names = m_template->GetEnabledAppearancesNames();

	if ( names.Empty() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no appearances )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Add classes
		TDynArray< String > strNames;
		for ( Uint32 i=0; i<names.Size(); i++ )
			strNames.PushBack( names[i].AsString() );
		
		// Sort list and add to combo box
		Sort( strNames.Begin(), strNames.End() );
		for ( Uint32 i=0; i<strNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( strNames[i].AsChar() );
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CAppearanceListSelection::OnChoiceChanged ), NULL, this );
	}
}
