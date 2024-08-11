/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdVirtualMemWalkerDlg : public wxDialog 
{
	DECLARE_EVENT_TABLE();

protected:
	Red::Threads::CSemaphore	m_taskEvent;
	Bool						m_taskDone;
	CEdTimer					m_taskTimer;

public:
	CEdVirtualMemWalkerDlg( wxWindow* parent );
	~CEdVirtualMemWalkerDlg();

protected:
	void OnShow( wxShowEvent& event );
	void OnClose( wxCommandEvent& event );
	void OnTaskTimer( wxTimerEvent& event );
	void OnMouseWheel( wxMouseEvent& event );

	static DWORD ReadFunc( LPVOID lpvThreadParam );
	void SetBusy(Bool state);
};