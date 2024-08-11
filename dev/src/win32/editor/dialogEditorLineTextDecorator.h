#pragma once

class CEdStorySceneLineTextDecorator : public wxEvtHandler
{
private:
	wxTextCtrl* m_textField;
	wxString m_leftBracket;
	wxString m_rightBracket;

public:
	CEdStorySceneLineTextDecorator( wxTextCtrl* textField );
	virtual ~CEdStorySceneLineTextDecorator();

	void ChangeValue( wxString text );
	wxString GetValue();

	inline wxTextCtrl* GetTextField() { return m_textField; }
	void SetBrackets( const wxString leftBracket, const wxString rightBracket );

protected:
	void OnValueChanged( wxCommandEvent& event );
	void OnFocus( wxFocusEvent& event );
};
