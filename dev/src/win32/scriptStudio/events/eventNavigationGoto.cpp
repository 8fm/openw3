/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "eventNavigationGoto.h"

wxDEFINE_EVENT( ssEVT_NAVIGATION_GOTO_EVENT, CNavigationGotoEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CNavigationGotoEvent, wxEvent );

CNavigationGotoEvent::CNavigationGotoEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

CNavigationGotoEvent::CNavigationGotoEvent( Red::System::Bool forwards )
:	wxEvent( wxID_ANY, ssEVT_NAVIGATION_GOTO_EVENT )
,	m_forwards( forwards )
{
}

CNavigationGotoEvent::~CNavigationGotoEvent()
{

}
