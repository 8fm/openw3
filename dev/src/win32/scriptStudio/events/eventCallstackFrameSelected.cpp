/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "eventCallstackFrameSelected.h"

wxDEFINE_EVENT( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, CCallstackFrameSelectedEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CCallstackFrameSelectedEvent, wxEvent );

CCallstackFrameSelectedEvent::CCallstackFrameSelectedEvent()
:	CGotoEvent( wxEVT_NULL, 0 )
{

}

CCallstackFrameSelectedEvent::CCallstackFrameSelectedEvent( wxEventType commandType, int winid )
:	CGotoEvent( commandType, winid )
{

}

CCallstackFrameSelectedEvent::CCallstackFrameSelectedEvent( Red::System::Uint32 stackFrame, const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory )
:	CGotoEvent( ssEVT_CALLSTACK_FRAME_SELECTED_EVENT, file, line, generateHistory )
,	m_stackFrame( stackFrame )
{

}

wxEvent* CCallstackFrameSelectedEvent::Clone() const
{
	return new CCallstackFrameSelectedEvent( m_stackFrame, GetFile(), GetLine(), GenerateHistory() );
}

CCallstackFrameSelectedEvent::~CCallstackFrameSelectedEvent()
{

}
