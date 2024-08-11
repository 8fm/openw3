#pragma once

class CSSCodeToolTip : public wxPopupWindow
{
	wxDECLARE_CLASS( CSSCodeToolTip );

public:
	CSSCodeToolTip( wxWindow* parent, const wxString& contents, wxPoint position = wxDefaultPosition );
	virtual ~CSSCodeToolTip();

protected:
	// Doesn't consume the event
	void CloseMeSkipEvent( wxEvent& event );

	// This function exists 
	void CloseMe( wxEvent& event );
};
