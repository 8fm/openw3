#pragma once

class CEdTimeValueEditor : public wxSmartLayoutDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdTimeValueEditor( wxWindow *parent, Float defaultValue );
	~CEdTimeValueEditor();

public:
	/// Get value
	Float GetValue() const { return m_value; }

	/// Get default value
	Float GetDefaultValue() const { return m_defaultValue; }

public:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnFocus(wxFocusEvent& event);
	void OnTimeChanged( wxDateEvent& event );

private:
	Float m_value;
	Float m_defaultValue;
	wxTimePickerCtrl* m_timePicker;
};
