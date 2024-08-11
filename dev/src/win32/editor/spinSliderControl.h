/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( wxEVT_SPIN_SLIDER_CHANGED, wxNewEventType() )
END_DECLARE_EVENT_TYPES()

class CEdDraggedSpinButton;

class CEdSpinSliderControl : public wxEvtHandler
{
protected:
	wxStaticBox*			m_static;		//!< Static box
	wxTextCtrl*				m_edit;			//!< Text edit control
	CEdDraggedSpinButton*	m_spin;			//!< Spin control
	wxSlider*				m_slider;		//!< Slider control
	wxWindow*				m_parent;		//!< Parent window
	Float					m_precision;	//!< Precision
	Float					m_min;			//!< Min value
	Float					m_max;			//!< Max value
	Float					m_value;		//!< Current value
	Float*					m_updateValue;  //!< Pointer to update value

public:
	CEdSpinSliderControl();

	void Init( wxWindow* window, const Char* baseName, Float min, Float max, Float initial, Float* updateValue = NULL, Float precision = 0.01f );

	void UpdateValue( Float value, wxControl* ctrl = NULL, bool muteEvents = false );

	Float GetValue() const { return m_value; };

	void SetEnabled( bool enabled );

public:
	Float GetMin() { return m_min; }
	Float GetMax() { return m_max; }

	void UpdateRange( Float min, Float max );

private:
	void OnSliderChanged( wxScrollEvent& event );

	void OnSpinChanged( wxScrollEvent& event );
	
	void OnTextChanged( wxCommandEvent& event );

	void OnTextLostFocus( wxFocusEvent& event );
	
};
