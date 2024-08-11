#pragma once

#include "animbrowser.h"

class CAnimBrowserPasteEventsRelativeToOtherEventDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CAnimBrowserPasteEventsRelativeToOtherEventDialog( wxWindow* parent, String const & title = TXT("") );
	~CAnimBrowserPasteEventsRelativeToOtherEventDialog();

	wxTextCtrl* m_eventNameBox;
	wxTextCtrl* m_relPosBox;

	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

	CName GetEventName() const { return m_eventName; }
	Float GetRelPos() const { return m_relPos; }

private:
	CName m_eventName;
	Float m_relPos;
};

