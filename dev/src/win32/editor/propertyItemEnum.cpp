/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemEnum.h"

CPropertyItemEnum::CPropertyItemEnum( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
}

void CPropertyItemEnum::CreateMainControl()
{
	// Calculate placement, hacked !
	wxRect valueRect = CalcValueRect();
	wxPoint pos = m_page->CalcScrolledPosition( wxPoint( valueRect.x, valueRect.y ) );
	valueRect.y = pos.y;
	valueRect.x = pos.x;

	// Get enum			
	CEnum* enumObject = (CEnum*)m_propertyType;

	// Create choices
	wxArrayString choices;
	/*const THashMap< String, Int32 >& options = enumObject->GetValues();
	for ( THashMap< String, Int32 >::const_iterator i=options.Begin(); i!=options.End(); ++i )
	{
	choices.Add( i->m_first.AsChar() );
	}*/
	const TDynArray< CName > &options = enumObject->GetOptions();
	for ( TDynArray<CName>::const_iterator i=options.Begin(); i!=options.End(); ++i )
	{
		choices.Add( i->AsString().AsChar() );
	}

	// Create choice list
	m_ctrlChoice = new CEdChoice( m_page, valueRect.GetTopLeft(), valueRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->Append( choices );
	m_ctrlChoice->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CPropertyItem::OnEditKeyDown ), NULL, this );
	m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CPropertyItem::OnChoiceSelected ), NULL, this );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlChoice->SetFont( m_page->GetStyle().m_drawFont );
	m_ctrlChoice->Enable( !IsReadOnly() );
	m_ctrlChoice->SetFocus();
}