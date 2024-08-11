/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "breakpoints.h"

#include "events/eventbreakpoints.h"

wxIMPLEMENT_CLASS( CSSBreakpoints, CSSMarkers );

CSSBreakpoints::CSSBreakpoints( wxWindow* parent, Solution* solution )
:	CSSMarkers( parent, solution )
{
}

CSSBreakpoints::~CSSBreakpoints()
{
}

void CSSBreakpoints::ConfirmAll()
{
	for( size_t i = 0; i < m_entries.size(); ++i )
	{
		if( m_entries[ i ]->m_state == Marker_Enabled )
		{
			CBreakpointToggledEvent* event = new CBreakpointToggledEvent( m_entries[ i ]->m_file, m_entries[ i ]->m_lineNo, m_entries[ i ]->m_state );
			QueueEvent( event );
		}
	}
}

CMarkerToggledEvent* CSSBreakpoints::CreateToggleEvent( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state ) const  
{
	return new CBreakpointToggledEvent( file, line, state );
}
