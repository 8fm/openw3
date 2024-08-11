/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef REFRESHVOICES_H
#define REFRESHVOICES_H

class CEdRefreshVoicesDlg : private wxDialog
{
	DECLARE_EVENT_TABLE()

public:
	CEdRefreshVoicesDlg( wxWindow* parent );
	void Execute();
	
	void OnRefresh( wxCommandEvent &event );

private:
	wxChoice*		m_langChoice;
	wxTextCtrl*		m_sourceTextBox;

	String			m_sourcePath;
};

#endif // REFRESHVOICES_H