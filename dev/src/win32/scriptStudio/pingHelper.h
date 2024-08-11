/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __PING_HELPER_H__
#define __PING_HELPER_H__

#include "../../common/redNetwork/ping.h"

//////////////////////////////////////////////////////////////////////////
// Ping wx event
//////////////////////////////////////////////////////////////////////////

class CPingEvent : public wxEvent
{
public:
	CPingEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 );
	CPingEvent( double ping );
	virtual ~CPingEvent();

	inline double GetPing() const { return m_ping; }

private:
	virtual wxEvent* Clone() const override final { return new CPingEvent( m_ping ); }

private:
	double m_ping;

	wxDECLARE_DYNAMIC_CLASS_NO_ASSIGN( CPingEvent );
};

wxDECLARE_EVENT( ssEVT_PING_EVENT, CPingEvent );

//////////////////////////////////////////////////////////////////////////
// Ping helper
//////////////////////////////////////////////////////////////////////////

class CPingHelper : public wxEvtHandler, public Red::Network::Ping
{
	wxDECLARE_CLASS( CPingHelper );

public:
	CPingHelper();
	~CPingHelper();

private:
	virtual void OnPongReceived( double ms ) override final;
};

#endif // __PING_HELPER_H__
