/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "pingHelper.h"

//////////////////////////////////////////////////////////////////////////

wxDEFINE_EVENT( ssEVT_PING_EVENT, CPingEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CPingEvent, wxEvent );

CPingEvent::CPingEvent( wxEventType commandType, int winid )
	:	wxEvent( commandType, winid )
	,	m_ping( 0.0 )
{

}

CPingEvent::CPingEvent( double ping )
	:	wxEvent( wxID_ANY, ssEVT_PING_EVENT )
	,	m_ping( ping )
{

}

CPingEvent::~CPingEvent()
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS( CPingHelper, wxEvtHandler );

CPingHelper::CPingHelper()
{

}

CPingHelper::~CPingHelper()
{

}

void CPingHelper::OnPongReceived( double ms )  
{
	CPingEvent* event = new CPingEvent( ms );

	QueueEvent( event );
}
