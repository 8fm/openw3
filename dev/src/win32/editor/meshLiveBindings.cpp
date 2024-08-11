/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "meshLiveBindings.h"


CBoundControl::CBoundControl()
	: m_callbackContext( NULL )
	, m_callbackFunc( NULL )
{
}

void CBoundControl::NotifyChanged()
{
	if ( m_callbackFunc )
	{
		(*m_callbackFunc)( m_callbackContext, this );
	}
}

void CBoundControl::SetCallback( BoundControlCallback function, void* context )
{
	m_callbackContext = context;
	m_callbackFunc = function;
}


void CBoundRangedSlider::UpdateFromSource()
{
	Float pct = (Float)( *m_source - m_minValue ) / (Float)( m_maxValue - m_minValue );
	Int32 sliderValue = (Int32)( m_slider->GetMin() + pct * ( m_slider->GetMax() - m_slider->GetMin() ) );

	m_slider->SetValue( sliderValue );
	m_label->SetLabel( wxString::FromDouble(*m_source) );
}

CBoundRangedSlider::CBoundRangedSlider( wxSlider* slider, wxTextCtrl* label, Float minValue, Float maxValue, Float* source, Bool live )
	: m_slider(slider)
	, m_label(label)
	, m_minValue(minValue)
	, m_maxValue(maxValue)
	, m_source(source)
	, m_liveUpdate(live)
{
	UpdateFromSource();
	// For actual updates, we wait until a scrub is complete (THUMBRELEASE), or if the change is done by not scrubbing (keyboard or something).
	m_slider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( CBoundRangedSlider::OnSliderChange ), NULL, this );
	// Connect to THUMBTRACK so that the label will be udpated as the thumb is scrubbed, instead of only waiting until the change is complete.
	m_slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( CBoundRangedSlider::OnSliderTrack ), NULL, this );
	
	m_label->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBoundRangedSlider::OnTextBoxEdited ), NULL, this );
}

void CBoundRangedSlider::UpdateActualValue()
{
	// Rescale/offset the slider's value so that it is in the proper range.
	Float pct = (Float)( m_slider->GetValue() - m_slider->GetMin() ) / (Float)( m_slider->GetMax() - m_slider->GetMin() );
	m_actualValue = m_minValue + pct * ( m_maxValue - m_minValue );

	m_label->SetLabel( wxString::FromDouble( m_actualValue ) );
}

void CBoundRangedSlider::OnSliderChange( wxScrollEvent& event )
{
	UpdateActualValue();

	*m_source = m_actualValue;
	NotifyChanged();
}

void CBoundRangedSlider::OnSliderTrack( wxScrollEvent& event )
{
	if ( m_liveUpdate )
	{
		OnSliderChange( event );
	}
	else
	{
		m_label->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBoundRangedSlider::OnTextBoxEdited ), NULL, this );
		UpdateActualValue();
		m_label->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBoundRangedSlider::OnTextBoxEdited ), NULL, this );
	}
}

void CBoundRangedSlider::OnTextBoxEdited( wxCommandEvent& event )
{
	double dbl;
	if ( m_label->GetValue().ToDouble( &dbl ) )
	{
		// To make sure we're in range, we don't count on minValue actually being smaller than maxValue. This allows for 
		// the range to be reversed (so the max value is on the left, min on the right).
		Bool inRange = (dbl >= Min( m_minValue, m_maxValue ) && dbl <= Max( m_minValue, m_maxValue ) );
		if ( inRange )
		{
			*m_source = m_actualValue = (Float)dbl;

			Float pct = (Float)( *m_source - m_minValue ) / (Float)( m_maxValue - m_minValue );
			Int32 sliderValue = (Int32)( m_slider->GetMin() + pct * ( m_slider->GetMax() - m_slider->GetMin() ) );

			m_slider->SetValue( sliderValue );

			NotifyChanged();
		}
	}
}


void CBoundToggleControl::UpdateFromSource()
{
	Bool val = *m_source;
	if ( m_invert ) val = !val;
	m_checkbox->SetValue( val );
}

CBoundToggleControl::CBoundToggleControl( wxCheckBox* checkbox, Bool* source, Bool invert )
	: m_checkbox( checkbox )
	, m_source( source )
	, m_invert( invert )
{
	UpdateFromSource();
	m_checkbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CBoundToggleControl::OnCheckboxToggled ), NULL, this );
}

void CBoundToggleControl::OnCheckboxToggled( wxCommandEvent& event )
{
	Bool val = m_checkbox->GetValue();
	if ( m_invert ) val = !val;
	*m_source = val;
	NotifyChanged();
}




CEdBindingsBuilder::CEdBindingsBuilder()
	: m_callbackFunc( NULL )
{
}

CEdBindingsBuilder::~CEdBindingsBuilder()
{
	Clear();
}

void CEdBindingsBuilder::Clear()
{
	for ( Uint32 i = 0; i < m_bindings.Size(); ++i )
	{
		delete m_bindings[i];
	}
	m_bindings.Clear();

	// Also destroy any controls we may have created.
	for ( Uint32 i = 0; i < m_mainSizers.Size(); ++i )
	{
		// If this sizer is still attached to a window, remove the sizer and destroy any children we had added.
		wxWindow* wnd = m_mainSizers[i]->GetContainingWindow();
		if ( wnd )
		{
			wnd->SetSizer( NULL, true );
			wnd->DestroyChildren();
		}
	}
	m_mainSizers.Clear();
}

void CEdBindingsBuilder::RefreshControls()
{
	for ( Uint32 i = 0; i < m_bindings.Size(); ++i )
	{
		m_bindings[i]->UpdateFromSource();
	}
}

void CEdBindingsBuilder::SetCallback( BoundControlCallback function, void* context )
{
	m_callbackContext = context;
	m_callbackFunc = function;
}


CBoundControl* CEdBindingsBuilder::AddBoundControl( CBoundControl* boundCtrl )
{
	boundCtrl->SetCallback( m_callbackFunc, m_callbackContext );
	m_bindings.PushBack( boundCtrl );
	return boundCtrl;
}


void CEdBindingsBuilder::Begin( wxPanel* panel )
{
	m_panel = panel;
	m_mainSizer = new wxBoxSizer( wxHORIZONTAL );
	m_mainSizers.PushBack( m_mainSizer );

	m_column = NULL;
	
	NewColumn();
}

void CEdBindingsBuilder::End()
{
	m_panel->SetSizer( m_mainSizer );
	m_panel->Layout();
}

void CEdBindingsBuilder::NewColumn()
{
	if ( m_column != NULL )
	{
		wxStaticLine* line = new wxStaticLine( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
		m_mainSizer->Add( line, 0, wxEXPAND );
	}
	m_column = new wxBoxSizer( wxVERTICAL );
	m_mainSizer->Add( m_column, 1, wxEXPAND );
}

void CEdBindingsBuilder::AddSlider( const wxString& text, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip )
{
	AddSlider( text, "", minValue, maxValue, step, source, tooltip );
}
void CEdBindingsBuilder::AddSlider( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip )
{
	if ( !text.IsEmpty() )
	{
		wxStaticText* nameLbl = new wxStaticText( m_panel, wxID_ANY, text );
		m_column->Add( nameLbl, 0, wxALL, 5 );
	}

	wxBoxSizer* combo = new wxBoxSizer( wxHORIZONTAL );

	if ( !subtext.IsEmpty() )
	{
		wxStaticText* subNameLbl = new wxStaticText( m_panel, wxID_ANY, subtext );
		combo->Add( subNameLbl, 0, wxALL, 5 );
	}

	wxSlider* valSlider = new wxSlider( m_panel, wxID_ANY, 0, 0, (Int32)( (maxValue - minValue) / step ) );
	valSlider->SetToolTip( tooltip );
	combo->Add( valSlider, 0, wxEXPAND | wxALL, 0 );

	wxTextCtrl* valLbl = new wxTextCtrl( m_panel, wxID_ANY );
	valLbl->SetToolTip( tooltip );
	combo->Add( valLbl, 0, wxALL, 5 );

	m_column->Add( combo, 0, wxEXPAND | wxLEFT, 15 );

	AddBoundControl( new CBoundRangedSlider( valSlider, valLbl, minValue, maxValue, source, false ) );
}

void CEdBindingsBuilder::AddLiveSlider( const wxString& text, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip )
{
	AddLiveSlider( text, "", minValue, maxValue, step, source, tooltip );
}
void CEdBindingsBuilder::AddLiveSlider( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip )
{
	if ( !text.IsEmpty() )
	{
		wxStaticText* nameLbl = new wxStaticText( m_panel, wxID_ANY, text );
		m_column->Add( nameLbl, 0, wxALL, 5 );
	}

	wxBoxSizer* combo = new wxBoxSizer( wxHORIZONTAL );

	if ( !subtext.IsEmpty() )
	{
		wxStaticText* subNameLbl = new wxStaticText( m_panel, wxID_ANY, subtext );
		combo->Add( subNameLbl, 0, wxALL, 5 );
	}

	wxSlider* valSlider = new wxSlider( m_panel, wxID_ANY, 0, 0, (Int32)( (maxValue - minValue) / step ) );
	valSlider->SetToolTip( tooltip );
	combo->Add( valSlider, 0, wxEXPAND | wxALL, 0 );

	wxTextCtrl* valLbl = new wxTextCtrl( m_panel, wxID_ANY );
	valLbl->SetToolTip( tooltip );
	combo->Add( valLbl, 0, wxALL, 5 );

	m_column->Add( combo, 0, wxEXPAND | wxLEFT, 15 );

	AddBoundControl( new CBoundRangedSlider( valSlider, valLbl, minValue, maxValue, source, true ) );
}



wxTextCtrl* CEdBindingsBuilder::AddNumericText( const wxString& text, const wxString& subtext, const wxString& tooltip )
{
	if ( !text.IsEmpty() )
	{
		wxStaticText* nameLbl = new wxStaticText( m_panel, wxID_ANY, text );
		m_column->Add( nameLbl, 0, wxALL, 5 );
	}

	wxBoxSizer* combo = new wxBoxSizer( wxHORIZONTAL );

	if ( !subtext.IsEmpty() )
	{
		wxStaticText* subNameLbl = new wxStaticText( m_panel, wxID_ANY, subtext );
		combo->Add( subNameLbl, 0, wxALL, 5 );
	}

	wxTextCtrl* textbox = new wxTextCtrl( m_panel, wxID_ANY );
	textbox->SetToolTip( tooltip );
	combo->Add( textbox, 0, wxALL, subtext.IsEmpty() ? 0 : 5 );

	m_column->Add( combo, 0, wxEXPAND | wxLEFT, 15 );

	return textbox;
}

void CEdBindingsBuilder::AddNumericTextFloat( const wxString& text, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, "", -FLT_MAX, FLT_MAX, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextFloat( const wxString& text, const wxString& subtext, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, subtext, -FLT_MAX, FLT_MAX, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinFloat( const wxString& text, Float minValue, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, "", minValue, FLT_MAX, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinFloat( const wxString& text, const wxString& subtext, Float minValue, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, subtext, minValue, FLT_MAX, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMaxFloat( const wxString& text, Float maxValue, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, "", -FLT_MAX, maxValue, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMaxFloat( const wxString& text, const wxString& subtext, Float maxValue, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, subtext, -FLT_MAX, maxValue, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinMaxFloat( const wxString& text, Float minValue, Float maxValue, Float* source, const wxString& tooltip )
{
	AddNumericTextMinMaxFloat( text, "", minValue, maxValue, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinMaxFloat( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float* source, const wxString& tooltip )
{
	wxTextCtrl* textbox = AddNumericText( text, subtext, tooltip );
	AddBoundControl( new CBoundNumericTextBoxControl<Float>( textbox, minValue, maxValue, source ) );
}

void CEdBindingsBuilder::AddNumericTextU32( const wxString& text, Uint32* source, const wxString& tooltip )
{
	AddNumericTextMinMaxU32( text, "", 0, 0xFFFFFFFF, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextU32( const wxString& text, const wxString& subtext, Uint32* source, const wxString& tooltip )
{
	AddNumericTextMinMaxU32( text, subtext, 0, 0xFFFFFFFF, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinU32( const wxString& text, Uint32 minValue, Uint32* source, const wxString& tooltip )
{
	AddNumericTextMinMaxU32( text, "", minValue, 0xFFFFFFFF, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinU32( const wxString& text, const wxString& subtext, Uint32 minValue, Uint32* source, const wxString& tooltip )
{
	AddNumericTextMinMaxU32( text, subtext, minValue, 0xFFFFFFFF, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinMaxU32( const wxString& text, Uint32 minValue, Uint32 maxValue, Uint32* source, const wxString& tooltip )
{
	AddNumericTextMinMaxU32( text, "", minValue, maxValue, source, tooltip );
}
void CEdBindingsBuilder::AddNumericTextMinMaxU32( const wxString& text, const wxString& subtext, Uint32 minValue, Uint32 maxValue, Uint32* source, const wxString& tooltip )
{
	wxTextCtrl* textbox = AddNumericText( text, subtext, tooltip );
	AddBoundControl( new CBoundNumericTextBoxControl<Uint32>( textbox, minValue, maxValue, source ) );
}


void CEdBindingsBuilder::AddCheckbox( const wxString& text, Bool* source, const wxString& tooltip )
{
	wxCheckBox* checkbox = new wxCheckBox( m_panel, wxID_ANY, text );
	checkbox->SetToolTip( tooltip );
	m_column->Add( checkbox, 0, wxALL, 5 );

	AddBoundControl( new CBoundToggleControl( checkbox, source, false ) );
}
void CEdBindingsBuilder::AddCheckboxInverted( const wxString& text, Bool* source, const wxString& tooltip )
{
	wxCheckBox* checkbox = new wxCheckBox( m_panel, wxID_ANY, text );
	checkbox->SetToolTip( tooltip );
	m_column->Add( checkbox, 0, wxALL, 5 );

	AddBoundControl( new CBoundToggleControl( checkbox, source, true ) );
}


void CEdBindingsBuilder::AddSeparator()
{
	wxStaticLine* line = new wxStaticLine( m_panel );
	line->SetWindowStyle( wxLI_HORIZONTAL );
	m_column->Add( line, 0, wxEXPAND | wxALL, 5 );
}

void CEdBindingsBuilder::AddLabel( const wxString& label, const wxString& tooltip  )
{
	wxStaticText* lbl = new wxStaticText( m_panel, wxID_ANY, label );
	lbl->SetWindowStyle( wxLI_VERTICAL );
	lbl->SetToolTip( tooltip );
	m_column->Add( lbl, 0, wxALL, 5 );
}


