#pragma once

class CEdHelpBubble : public wxPopupWindow
{
	DECLARE_CLASS( CEdHelpBubble );

public:
	CEdHelpBubble( wxWindow* parent, const String& helpText = String::EMPTY, wxPoint position = wxDefaultPosition );
	virtual ~CEdHelpBubble();

	void SetLabel( const String& helpText );
	void SetPosition( wxPoint position = wxDefaultPosition );

protected:
	void OnParentMoved( wxMoveEvent& event );
	void OnParentShown( wxShowEvent& event );

private:
	wxStaticText* m_helpTextCtrl;
};
