
#pragma once

#include "aaEditorIncludes.h"

class CEdAANetPanel : public wxPanel
{
	DECLARE_EVENT_TABLE()

private:
	aaServer	m_server;
	Bool		m_running;
	Bool		m_connected;

private:
	wxTextCtrl*		m_log;
	wxStaticText*	m_status;

public:
	CEdAANetPanel( wxWindow* parent );
	~CEdAANetPanel();

private:
	Bool ConnectNetwork();
	void DisconnectNetwork();

	void RefreshStatus();

	void SetConnection( Bool flag );

protected: // Functions for user
	void Log( const wxString& str );
	aaServer& GetServer();

	virtual void ProcessPacket( aaInt32 msgType, aaServer& server );

	virtual void OnConnectionAccepted() {}
	virtual void OnConnectionLost() {}

protected:
	void OnIdle( wxIdleEvent& event );
	void OnNetConnected( wxCommandEvent& event );
};
