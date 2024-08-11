/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

DEFINE_LOCAL_EVENT_TYPE( wxEVT_SPIN_SLIDER_CHANGED )

CEdSpinSliderControl::CEdSpinSliderControl()
: m_static( NULL )
, m_spin( NULL )
, m_slider( NULL )
, m_parent( NULL )
, m_edit( NULL )
, m_precision( 0.01f )
, m_min( 0.0f )
, m_max( 1.0f )
, m_value( 0.0f )
, m_updateValue( NULL )
{
}

void CEdSpinSliderControl::SetEnabled( bool enabled )
{
	m_edit->Enable( enabled );
	m_spin->Enable( enabled );
	m_slider->Enable( enabled );
}

void CEdSpinSliderControl::Init( wxWindow* window, const Char* baseName, Float min, Float max, Float initial, Float* updateValue /*= NULL*/, Float precision /*= 0.1f*/ )
{
	ASSERT( window );
	m_parent = window;

	// Find slider
	String sliderName = baseName + String( TXT("Slider") );
	m_slider = (wxSlider*) m_parent->FindWindow( sliderName.AsChar() );

	// Find static
	String staticName = baseName + String( TXT("Static") );
	m_static = (wxStaticBox*) m_parent->FindWindow( staticName.AsChar() );

	// Find spin
	String spinName = baseName + String( TXT("Spin") );
	wxSpinButton* spin = (wxSpinButton*) m_parent->FindWindow( spinName.AsChar() );
	if ( spin )
	{
		// replace normal wxSpinButton with our CEdDraggedSpinButton
		m_spin = new CEdDraggedSpinButton( spin->GetParent(), spin->GetId(), spin->GetPosition(), spin->GetSize(), spin->GetWindowStyle() );		
		m_spin->SetPrecision( precision );
		wxSizer* sizer = spin->GetContainingSizer();
		if ( sizer )
		{
			wxSizerItem* sizerItem = sizer->GetItem( spin );
			if ( sizerItem )
			{
				sizerItem->AssignWindow( m_spin );
			}
		}
		spin->Hide();
	}

	// Find text edit
	String editName = baseName + String( TXT("Edit") );
	m_edit = (wxTextCtrl*) m_parent->FindWindow( editName.AsChar() );

	m_precision = precision;
	m_min = min;
	m_max = max;
	m_updateValue = updateValue;

	Int32 imin = (Int32) ( m_min / m_precision );
	Int32 imax = (Int32) ( m_max / m_precision );

	// Setup controls and connect events
	if ( m_spin )	
	{
		m_spin->SetRange( imin, imax );
		m_spin->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CEdSpinSliderControl::OnSpinChanged ), NULL, this );	
	}

	if ( m_slider )
	{
		m_slider->SetRange( imin, imax );
		// Connect events
		m_slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CEdSpinSliderControl::OnSliderChanged ), NULL, this );
		m_slider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( CEdSpinSliderControl::OnSliderChanged ), NULL, this );
	}	

	if ( m_edit )
	{
		m_edit->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdSpinSliderControl::OnTextChanged ), NULL, this );
		m_edit->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( CEdSpinSliderControl::OnTextLostFocus ), NULL, this );	
	}
	
	UpdateValue( initial );
}


void CEdSpinSliderControl::UpdateRange( Float min, Float max )
{
	m_min = min;
	m_max = max;

	Int32 imin = (Int32) ( m_min / m_precision );
	Int32 imax = (Int32) ( m_max / m_precision );

	// Setup controls and connect events
	if ( m_spin )	
	{
		m_spin->SetRange( imin, imax );
	}

	if ( m_slider )
	{
		m_slider->SetRange( imin, imax );
	}
}


void CEdSpinSliderControl::OnSliderChanged( wxScrollEvent& event )
{
	UpdateValue( m_slider->GetValue() * m_precision, m_slider );
}

void CEdSpinSliderControl::OnSpinChanged( wxScrollEvent& event )
{
	UpdateValue( m_spin->GetValue() * m_precision, m_spin );
}

void CEdSpinSliderControl::UpdateValue( Float value, wxControl* ctrl /*= NULL*/, bool muteEvents /*= false*/ )
{
	// --------------------------------
	struct SEventsAutoMute
	{
		SEventsAutoMute ( wxEvtHandler *handler )
			: m_handler ( NULL )
			, m_wasEnabled ( false )
		{
			if ( handler )
			{
				m_handler    = handler;
				m_wasEnabled = handler->GetEvtHandlerEnabled();
				handler->SetEvtHandlerEnabled( false );
			}
		}

		~SEventsAutoMute ()
		{
			if ( m_handler )
			{
				m_handler->SetEvtHandlerEnabled( m_wasEnabled );
			}
		}

		wxEvtHandler *m_handler;
		bool m_wasEnabled;
	};
	// --------------------------------

	Float _m_value = Clamp<Float>( value, m_min, m_max );

	if( _m_value != m_value )
	{
		m_value = _m_value;

		if ( !muteEvents )
		{
			wxCommandEvent eventChanged( wxEVT_SPIN_SLIDER_CHANGED );
			ProcessEvent( eventChanged );
		}
	}

	if ( m_updateValue )
	{
		*m_updateValue = m_value;
	}

	if ( m_slider )
	{
		SEventsAutoMute autoMute ( muteEvents ? m_slider : NULL );
		m_slider->SetValue( (Int32) ( m_value / m_precision ) );
	}
	
	if ( m_spin )
	{
		SEventsAutoMute autoMute ( muteEvents ? m_spin : NULL );
		m_spin->SetValue( (Int32) ( m_value / m_precision ) );
	}	

	String txtVal = String::Printf( TXT("%g"), m_value );
	
	if ( m_edit && ctrl != m_edit )
	{
		SEventsAutoMute autoMute ( muteEvents ? m_edit : NULL );
		m_edit->SetValue( txtVal.AsChar() );
	}

	if ( m_static )
	{
		SEventsAutoMute autoMute ( muteEvents ? m_static : NULL );
		m_static->SetLabel( txtVal.AsChar() );
	}
}

void CEdSpinSliderControl::OnTextChanged( wxCommandEvent& event )
{
	Float val;
	if ( swscanf( m_edit->GetValue().wc_str(), TXT("%f"), &val ) == 1 )
	{
		UpdateValue( val, m_edit );
	}
	else
	{
		if ( m_edit->GetValue().Len() == 0 )
		{
			UpdateValue( val, m_edit );
		}
	}
}

void CEdSpinSliderControl::OnTextLostFocus( wxFocusEvent& event )
{
	UpdateValue( GetValue() );

	// HACK HACK HACK set the focus on the same control to avoid lost focus problem
	event.Skip();
	event.Skip(true);
}
