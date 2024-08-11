#pragma once

class CEdDetachablePanel
{
public:
	CEdDetachablePanel();
	virtual ~CEdDetachablePanel();

	// Call after window creation
	void Initialize( wxPanel* panel, const String& title );

private:
	void OnDetach( wxCommandEvent& event );

private:
	wxWindow* m_parent;
	wxPanel* m_panel;

	wxDialog* m_dialog;

	String m_title;
};
