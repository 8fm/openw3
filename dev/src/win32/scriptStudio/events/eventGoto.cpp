/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "eventGoto.h"

wxDEFINE_EVENT( ssEVT_OPEN_FILE_EVENT, COpenFileEvent );
wxDEFINE_EVENT( ssEVT_GOTO_EVENT, CGotoEvent );

IMPLEMENT_DYNAMIC_CLASS( COpenFileEvent, wxEvent );
IMPLEMENT_DYNAMIC_CLASS( CGotoEvent, COpenFileEvent );

//////////////////////////////////////////////////////////////////////////

COpenFileEvent::COpenFileEvent( wxEventType commandType, const wxString& file, Red::System::Bool generateHistory )
:	wxEvent( wxID_ANY, commandType )
,	m_file( file )
,	m_generateHistory( generateHistory )
{

}

COpenFileEvent::COpenFileEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
{

}

COpenFileEvent::COpenFileEvent( const wxString& file, Red::System::Bool generateHistory )
:	wxEvent( wxID_ANY, ssEVT_OPEN_FILE_EVENT )
,	m_file( file )
,	m_generateHistory( generateHistory )
{

}

COpenFileEvent::~COpenFileEvent()
{

}

//////////////////////////////////////////////////////////////////////////

CGotoEvent::CGotoEvent( wxEventType commandType, int winid )
:	COpenFileEvent( commandType, winid )
{

}

CGotoEvent::CGotoEvent( const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory )
:	COpenFileEvent( ssEVT_GOTO_EVENT, file, generateHistory )
,	m_line( line )
{

}

CGotoEvent::CGotoEvent( wxEventType commandType, const wxString& file, Red::System::Int32 line, Red::System::Bool generateHistory )
:	COpenFileEvent( commandType, file, generateHistory )
,	m_line( line )
{

}

CGotoEvent::~CGotoEvent()
{

}
