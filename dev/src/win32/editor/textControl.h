#pragma once

wxDECLARE_EVENT( wxEVT_EDTC_STRING_UPDATED, wxCommandEvent );

// Reimplements functionality that seems broken or unreliable in wxWidgets
class CEdTextControl : public wxTextCtrlEx
{
	wxDECLARE_CLASS( CEdTextControl );

public:
	CEdTextControl();
	CEdTextControl
	(
		wxWindow* parent,
		wxWindowID id,
		const wxString& value			= wxEmptyString,
		const wxPoint& pos				= wxDefaultPosition,
		const wxSize& size				= wxDefaultSize,
		long style						= 0,
		const wxValidator& validator	= wxDefaultValidator,
		const wxString& name			= wxTextCtrlNameStr
	);
	
	virtual ~CEdTextControl();

	virtual wxString GetValue() const;
	virtual bool IsEmpty() const;

	virtual bool SetHint( const wxString& hint );

	virtual void SetValue( const wxString& value );
	virtual void ChangeValue( const wxString& value );

	virtual void OnHintFocus( wxFocusEvent& event );
	
	virtual Bool CustomArrowTraverseRule( wxKeyEvent &event ){ return false; }

	void EnableEnterKeyWorkaround( Bool enable = true ) { m_enterKeyWorkaroundEnabled = enable; }

private:

	void DisplayHint();
	void HideHint();

	void OnClipboard( wxClipboardTextEvent& event );
	void OnChar( wxKeyEvent& event );

	void SendStringUpdatedEvent();

	virtual bool MSWShouldPreProcessMessage( WXMSG* msg );

private:
	Bool m_hintSet;
	Bool m_hintDisplayed;
	wxString m_hintText;

	wxColour m_hintColour;
	wxColour m_otherColour;

	Bool m_enterKeyWorkaroundEnabled;
};

