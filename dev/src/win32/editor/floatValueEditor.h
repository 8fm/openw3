#pragma once

class CEdFloatValueEditor : public wxSmartLayoutDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdFloatValueEditor( wxWindow *parent, Float defaultValue );
	~CEdFloatValueEditor();

public:
	/// Get value
	Float GetValue() const { return m_value; }

	/// Get default value
	Float GetDefaultValue() const { return m_defaultValue; }

public:
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnFocus(wxFocusEvent& event);

private:
	Float m_value;
	Float m_defaultValue;
	wxString m_defaultValueString;
};
