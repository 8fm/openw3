/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdToggleButtonPanel : public wxEvtHandler
{
protected:
	wxToggleButton*		m_button;	//!< Controlling button
	wxPanel*			m_panel;	//!< Collapsible panel
	wxWindow*			m_parent;	//!< Parent window

public:
	CEdToggleButtonPanel();
	void Init( wxWindow* window, const Char* baseName );
	void Expand( Bool expand );

private:
	void OnToggle( wxCommandEvent& event );
};
