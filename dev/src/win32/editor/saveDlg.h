#pragma once

#define wxID_CHECKOUT 999901
#define wxID_OVERWRITE 999902

class CEdSaveDialog : public wxDialog 
{
	DECLARE_EVENT_TABLE();
private:
	String m_path;

public:
	CEdSaveDialog( wxWindow *parent, const String &name );
	String GetPath(){ return m_path; }

public:	
	void OnSaveAs( wxCommandEvent &event );
	void OnCancel( wxCommandEvent &event );
	void OnCheckOut( wxCommandEvent &event );
	void OnOverwrite( wxCommandEvent &event );
};