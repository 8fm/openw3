/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "slider.h"

CSlider::CSlider( CPropertyItem* item )
	: ICustomPropertyEditor( item )
	, m_slider( NULL )
    , m_transaction( NULL )
{
}

void CSlider::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
    ASSERT( m_transaction == NULL );
    m_transaction = new CPropertyTransaction( * m_propertyItem->GetPage() );

	Int32 value(0), minValue(0), maxValue(100);
	String displayValue;
	m_zero = 0.f;
	m_scale = 1.0f / maxValue;
	
	if ( m_propertyItem->GetProperty()->GetType()->GetName() == CNAME( Float ) )
	{	
		Float floatValue;
		Float minFloat = Max( m_propertyItem->GetProperty()->GetRangeMin(), -FLT_MAX );
		Float maxFloat = Min( m_propertyItem->GetProperty()->GetRangeMax(), FLT_MAX );

		m_zero = minFloat;
		m_scale = (maxFloat - minFloat) ;
	
		m_propertyItem->Read( &floatValue );
		floatValue = (floatValue - m_zero) / m_scale;
		value = (Int32) (floatValue * maxValue);
	} 
	
	//
	m_slider = new wxSlider( m_propertyItem->GetPage(), wxID_ANY, value, minValue, maxValue, propRect.GetTopLeft(), propRect.GetSize() );
	m_slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CSlider::OnValueChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( CSlider::OnValueChanged ), NULL, this );

	m_slider->SetFocus();
	
	
}

void CSlider::CloseControls()
{
	if ( m_transaction )
	{
		delete m_transaction;
		m_transaction = NULL;
	}

	// Close combo box
	if ( m_slider )
	{
		delete m_slider;
		m_slider = NULL;
	}
}

Bool CSlider::GrabValue( String& displayValue )
{
	Float floatValue;
	m_propertyItem->Read( &floatValue );
	displayValue = String::Printf(TXT("%f"), floatValue );
	return true;
}

Bool CSlider::SaveValue()
{
	if (!m_slider)
		return false;

	Int32 intValue = m_slider->GetValue();
	Float floatValue = ((Float)intValue) / m_slider->GetMax() * m_scale + m_zero;

	m_propertyItem->Write( &floatValue );
	return true;
}

void CSlider::OnValueChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}