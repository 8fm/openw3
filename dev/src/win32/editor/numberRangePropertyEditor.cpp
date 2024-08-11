/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "numberRangePropertyEditor.h"

INumberRangePropertyEditor::INumberRangePropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlSpin( NULL )
{
	
}

INumberRangePropertyEditor::~INumberRangePropertyEditor()
{

}

void INumberRangePropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxSize size = propertyRect.GetSize();

	// Create editor
	m_ctrlSpin = new wxSpinCtrl( m_propertyItem->GetPage(), wxID_ANY, wxEmptyString, propertyRect.GetTopLeft(), size );
	ApplyRange();

	// Set previous value
	Int32 currentValue;
	m_propertyItem->Read( &currentValue );
	m_ctrlSpin->SetValue( currentValue );

	// Notify of selection changes
	m_ctrlSpin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, 
		wxCommandEventHandler( INumberRangePropertyEditor::OnSpinChanged ), NULL, this );

	Int32 rangeMin = m_ctrlSpin->GetMin();
	int rangeMax = m_ctrlSpin->GetMax();

	// snap current value to range by simulating value change event
	if ( currentValue < rangeMin || currentValue > rangeMax )
	{
		wxCommandEvent dummyEvent( wxEVT_COMMAND_SPINCTRL_UPDATED ) ;
		dummyEvent.SetInt( currentValue < rangeMin ? rangeMin : min( rangeMax, currentValue ) );
		m_ctrlSpin->GetEventHandler()->ProcessEvent( dummyEvent );
	}

	m_ctrlSpin->SetWindowStyle( wxBORDER );
	m_ctrlSpin->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlSpin->SetFocus();
}

void INumberRangePropertyEditor::CloseControls()
{
	delete m_ctrlSpin;
	m_ctrlSpin = NULL;
}

Bool INumberRangePropertyEditor::DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour )
{
	return ICustomPropertyEditor::DrawValue( dc, valueRect, textColour );
}

Bool INumberRangePropertyEditor::SaveValue()
{
	return ICustomPropertyEditor::SaveValue();
}

Bool INumberRangePropertyEditor::GrabValue( String& displayValue )
{
	return ICustomPropertyEditor::GrabValue( displayValue );
}

void INumberRangePropertyEditor::OnSpinChanged( wxCommandEvent& event )
{
	Int32 numberValue( event.GetInt() );

	m_propertyItem->Write( &numberValue );
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void INumberRangePropertyEditor::ApplyRange()
{
	m_ctrlSpin->SetRange( 0, 0 );
}