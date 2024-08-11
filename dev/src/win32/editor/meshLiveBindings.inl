/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/



template<typename T>
void CBoundNumericTextBoxControl<T>::OnTextBoxEdited( wxCommandEvent& event )
{
	// Convert text to a double. From there, we can cast to the actual type. If we wanted, we could add template specialization for certain types.

	// TODO: The String->Double->T conversion could result in issues if negative values are entered for unsigned types. May be a good idea to specialize
	// for unsigned types, to clamp to 0 or something?
	wxDouble dbl;
	if ( m_textbox->GetValue().ToDouble(&dbl) )
	{
		Bool didAdjust = false;
		if ( m_useRange )
		{
			didAdjust = (dbl < m_rangeMin || dbl > m_rangeMax );
			if ( dbl < m_rangeMin ) dbl = m_rangeMin;
			else if (dbl > m_rangeMax ) dbl = m_rangeMax;
		}
		*m_source = (T)dbl;

		// If we had to clamp the value, update the text box to show the clamped value.
		// TODO: maybe if we had to round the number we should update the text box as well?
		if ( didAdjust )
		{
			UpdateFromSource();
		}
	}
	NotifyChanged();
}

template<typename T>
CBoundNumericTextBoxControl<T>::CBoundNumericTextBoxControl( wxTextCtrl* textbox, T* source )
	: m_textbox( textbox )
	, m_source( source )
	, m_useRange( false )
{
	UpdateFromSource();
	m_textbox->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBoundNumericTextBoxControl<T>::OnTextBoxEdited ), NULL, this );
}

template<typename T>
CBoundNumericTextBoxControl<T>::CBoundNumericTextBoxControl( wxTextCtrl* textbox, T minValue, T maxValue, T* source )
	: m_textbox( textbox )
	, m_source( source )
	, m_rangeMin( minValue )
	, m_rangeMax( maxValue )
	, m_useRange( true )
{
	UpdateFromSource();
	m_textbox->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CBoundNumericTextBoxControl<T>::OnTextBoxEdited ), NULL, this );
}

template<typename T>
void CBoundNumericTextBoxControl<T>::UpdateFromSource()
{
	wxDouble dbl = (wxDouble)*m_source;
	m_textbox->SetValue(wxString::FromDouble(dbl));
}
