/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdAttitudeSearchlDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();


public:
	CEdAttitudeSearchlDialog( wxCommandEvent& event );
	~CEdAttitudeSearchlDialog();

	void OnSearchClicked( wxCommandEvent& event );

private:
	wxChoice* m_attitudeList;
	wxCommandEvent m_event;

};
