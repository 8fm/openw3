/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiLabel.h"
#include "redGuiSlider.h"
#include "redGuiPanel.h"
#include "redGuiAdvancedSlider.h"

namespace RedGui
{
	CRedGuiAdvancedSlider::CRedGuiAdvancedSlider(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
		: CRedGuiControl( x, y, width, 40 )
	{
		SetBorderVisible( false );
		SetBackgroundColor( Color::CLEAR );
		SetNeedKeyFocus( true );

		CRedGuiPanel* textPanel = new CRedGuiPanel( 0, 0, 100, 20 );
		textPanel->SetDock( DOCK_Bottom );
		textPanel->SetBorderVisible( false );
		AddChild( textPanel );

		m_minValue = new CRedGuiLabel( 0, 0, 50, 20 );
		m_minValue->SetAlign( IA_MiddleLeft );
		m_minValue->SetAutoSize( true );
		m_minValue->SetMargin( Box2( 10, 0, 0, 0 ) );
		textPanel->AddChild( m_minValue );

		m_maxValue = new CRedGuiLabel( 0, 0, 50, 20 );
		m_maxValue->SetAlign( IA_MiddleRight );
		m_maxValue->SetAutoSize( true );
		m_maxValue->SetMargin( Box2( 0, 0, 10, 0 ) );
		textPanel->AddChild( m_maxValue );

		m_value = new CRedGuiLabel( 0, 0, 50, 20 );
		m_value->SetAlign( IA_MiddleCenter );
		m_value->SetAutoSize( true );
		textPanel->AddChild( m_value );

		m_slider = new CRedGuiSlider( 0, 0, 100, 20 );
		m_slider->SetDock( DOCK_Fill );
		m_slider->SetNeedKeyFocus( false );
		m_slider->EventScroll.Bind( this, &CRedGuiAdvancedSlider::NotifyScrollChanged );
		AddChild( m_slider );
	}

	CRedGuiAdvancedSlider::~CRedGuiAdvancedSlider()
	{		
	}

	void CRedGuiAdvancedSlider::OnPendingDestruction()
	{
		m_slider->EventScroll.Unbind( this, &CRedGuiAdvancedSlider::NotifyScrollChanged );
	}

	void CRedGuiAdvancedSlider::Draw()
	{
		GetTheme()->DrawPanel( this );
	}

	void CRedGuiAdvancedSlider::SetValue( Float value )
	{
		m_slider->SetValue( value );
	}

	Float CRedGuiAdvancedSlider::GetValue() const
	{
		return m_slider->GetValue();
	}

	void CRedGuiAdvancedSlider::SetMinValue( Float value )
	{
		m_slider->SetMinValue( value );
		m_minValue->SetText( String::Printf( TXT("%1.2f"), m_slider->GetMinValue() ) );
	}

	Float CRedGuiAdvancedSlider::GetMinValue() const
	{
		return m_slider->GetMinValue();
	}

	void CRedGuiAdvancedSlider::SetMaxValue( Float value )
	{
		m_slider->SetMaxValue( value );
		m_maxValue->SetText( String::Printf( TXT("%1.2f"), m_slider->GetMaxValue() ) );
	}

	Float CRedGuiAdvancedSlider::GetMaxValue() const
	{
		return m_slider->GetMaxValue();
	}

	void CRedGuiAdvancedSlider::NotifyScrollChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		RED_UNUSED( eventPackage );

		m_value->SetText( String::Printf( TXT("%1.2f"), value ) );
		EventScroll( this, value );
	}

	void CRedGuiAdvancedSlider::SetStepValue( Float value )
	{
		m_slider->SetStepValue( value );
	}

	Float CRedGuiAdvancedSlider::GetStepValue() const
	{
		return m_slider->GetStepValue();
	}

	Bool CRedGuiAdvancedSlider::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Left || event == RGIE_Down )
		{
			SetValue( GetValue() - GetStepValue() );
			return true;
		}
		else if( event == RGIE_Right || event == RGIE_Up )
		{
			SetValue( GetValue() + GetStepValue() );
			return true;
		}
		
		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
