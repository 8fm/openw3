#pragma once

#include "animbrowser.h"

class CAnimBrowserFindEventDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CAnimBrowserFindEventDialog( wxWindow* parent, String const & title = TXT("") );
	~CAnimBrowserFindEventDialog();

	wxTextCtrl* m_eventNameBox;

	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

	CName GetEventName() const { return m_eventName; }

private:
	CName m_eventName;
};

