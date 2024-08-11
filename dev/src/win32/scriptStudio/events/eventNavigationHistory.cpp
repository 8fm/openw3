/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "eventNavigationHistory.h"

wxDEFINE_EVENT( ssEVT_NAVIGATION_HISTORY_EVENT, CNavigationHistoryEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CNavigationHistoryEvent, wxEvent );

CNavigationHistoryEvent::CNavigationHistoryEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CNavigationHistoryEvent::CNavigationHistoryEvent( const wxString& file, const wxString& snippet, Red::System::Int32 line )
:	wxEvent( wxID_ANY, ssEVT_NAVIGATION_HISTORY_EVENT )
,	m_file( file )
,	m_snippet( snippet )
,	m_line( line )
{
}

CNavigationHistoryEvent::~CNavigationHistoryEvent()
{

}
