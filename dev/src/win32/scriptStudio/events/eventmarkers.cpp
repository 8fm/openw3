/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "eventmarkers.h"

wxDEFINE_EVENT( ssEVT_MARKER_TOGGLED_EVENT, CMarkerToggledEvent );
wxIMPLEMENT_ABSTRACT_CLASS( CMarkerToggledEvent, wxEvent );

CMarkerToggledEvent::CMarkerToggledEvent( wxEventType commandType, int winid )
:	wxEvent( commandType, winid )
,	m_line( 0 )
,	m_state( Marker_Disabled )
{

}

CMarkerToggledEvent::CMarkerToggledEvent( wxEventType type, const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state )
:	wxEvent( wxID_ANY, type )
,	m_file( file )
,	m_line( line )
,	m_state( state )
{

}

CMarkerToggledEvent::~CMarkerToggledEvent()
{

}

