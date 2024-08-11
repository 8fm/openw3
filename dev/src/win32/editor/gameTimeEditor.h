#pragma once

/// Editor of tags
class CEdGameTimeEditor : public wxSmartLayoutDialog
{
	DECLARE_EVENT_TABLE()
private:
	GameTime		*m_time;
	Bool			m_dayPeriodOnly;
public:
	CEdGameTimeEditor( wxWindow *parent, GameTime *time, Bool dayPeriodOnly );
	~CEdGameTimeEditor();

	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
};