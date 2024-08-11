/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __EDITOR_CONNECTION_HELPER_H__
#define __EDITOR_CONNECTION_HELPER_H__

#include "../../common/redNetwork/manager.h"

#define RED_NET_CHANNEL_SCRIPT "scripts"

//////////////////////////////////////////////////////////////////////////
// Types
//////////////////////////////////////////////////////////////////////////
enum EConnectionStatus
{
	ConnStatus_Disconnected = 0,
	ConnStatus_Connect,
	ConnStatus_Connecting,
	ConnStatus_JustConnected,
	ConnStatus_Connected,
	ConnStatus_JustDisconnected,
	ConnStatus_JustDropped,

	ConnStatus_Max
};

//////////////////////////////////////////////////////////////////////////
// Events
//////////////////////////////////////////////////////////////////////////
class CConnectionEvent : public wxEvent
{
public:
	CConnectionEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CConnectionEvent( EConnectionStatus status );
	virtual ~CConnectionEvent();

	inline EConnectionStatus GetStatus() const { return m_status; }

private:
	virtual wxEvent* Clone() const override final { return new CConnectionEvent( m_status ); }

private:
	EConnectionStatus m_status;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CConnectionEvent );
};

wxDECLARE_EVENT( ssEVT_CONNECTION_EVENT, CConnectionEvent );

//////////////////////////////////////////////////////////////////////////
// Connection Helper
//////////////////////////////////////////////////////////////////////////
class CEditorConnectionHelper : public wxEvtHandler, public Red::Network::ConnectionEventListener
{
	wxDECLARE_CLASS( CEditorConnectionHelper );

public:
	CEditorConnectionHelper();
	virtual ~CEditorConnectionHelper();

	void ToggleConnectionToEditor();

	void SetTargetAddress( const wxChar* ip );
	void SetTargetPort( Red::System::Uint16 port );

private:
	virtual void OnConnectionSucceeded( const Red::Network::Address& peer ) override final;
	virtual void OnConnectionAvailable( const Red::Network::Address& peer ) override final;
	virtual void OnConnectionDropped( const Red::Network::Address& peer ) override final;
	virtual void OnConnectionClosed( const Red::Network::Address& peer ) override final;

	void OnUpdate( wxTimerEvent& event );

	void SendConnectionEvent();

	wxTimer m_updateTimer;
	Red::Network::Address m_targetAddress;
	Red::Threads::CAtomic< EConnectionStatus > m_connectionStatus;

	static const Red::System::Int32 CONNECTION_POLL_INTERVAL;
	static const Red::System::Int32 DISCONNECT_POLL_INTERVAL;
};

#endif __EDITOR_CONNECTION_HELPER_H__
