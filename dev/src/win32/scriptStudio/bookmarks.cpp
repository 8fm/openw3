/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "bookmarks.h"

#include "events/eventbookmarks.h"
#include "events/eventGoto.h"

wxIMPLEMENT_CLASS( CSSBookmarks, CSSMarkers );

CSSBookmarks::CSSBookmarks( wxWindow* parent, Solution* solution )
:	CSSMarkers( parent, solution )
,	m_currBookmarkIdx( 0 )
{
}

CSSBookmarks::~CSSBookmarks()
{
}

void CSSBookmarks::Goto( int direction )
{
	if ( m_entries.size() > 0 )
	{
		m_currBookmarkIdx += direction;

		if ( m_currBookmarkIdx >= m_entries.size() )
		{
			m_currBookmarkIdx = 0;
		}
		else if( m_currBookmarkIdx < 0 )
		{
			m_currBookmarkIdx = m_entries.size() - 1;
		}

		int startingIndex = m_currBookmarkIdx;

		do
		{
			Entry* entry = m_entries[ m_currBookmarkIdx ];

			if( entry->m_state == Marker_Enabled )
			{
				CGotoEvent* newEvent = new CGotoEvent( entry->m_file->m_solutionPath.c_str(), static_cast< Red::System::Int32 >( entry->m_lineNo ) );
				QueueEvent( newEvent );

				break;
			}

			m_currBookmarkIdx += direction;

			if ( m_currBookmarkIdx >= m_entries.size() )
			{
				m_currBookmarkIdx = 0;
			}
			else if( m_currBookmarkIdx < 0 )
			{
				m_currBookmarkIdx = m_entries.size() - 1;
			}
		}
		while( m_currBookmarkIdx != startingIndex );
	}
}

void CSSBookmarks::GotoNext()
{
	Goto( 1 );
}

void CSSBookmarks::GotoPrev()
{
	Goto( -1 );
}

CMarkerToggledEvent* CSSBookmarks::CreateToggleEvent( const SolutionFilePtr& file, Red::System::Int32 line, EMarkerState state ) const  
{
	return new CBookmarkToggledEvent( file, line, state );
}
