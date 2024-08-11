/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "eventLineMove.h"

wxDEFINE_EVENT( ssEVT_LINE_MOVE_EVENT, CLineMoveEvent );
wxIMPLEMENT_DYNAMIC_CLASS( CLineMoveEvent, wxEvent );

CLineMoveEvent::CLineMoveEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
,	m_line( 0 )
,	m_added( 0 )
{

}

CLineMoveEvent::CLineMoveEvent( const SolutionFilePtr& file, Red::System::Int32 line, Red::System::Int32 added )
:	wxEvent( wxID_ANY, ssEVT_LINE_MOVE_EVENT )
,	m_file( file )
,	m_line( line )
,	m_added( added )
{

}

CLineMoveEvent::~CLineMoveEvent()
{

}
