#include "build.h"
#include "maraudersMapAIDebugCanvas.h"
#include "maraudersMapItems.h"
#include "maraudersMap.h"


///////////////////////////////////////////////////////////////////////////////

SAIEvent::SAIEvent( Uint32 trackIdx, const String& name, const String& description, Float startTime )
	: m_trackIdx( trackIdx )
	, m_startTime( startTime )
	, m_name( name )
	, m_description( description )
	, m_endTime( 0.0f )
	, m_result( EAIR_InProgress )
{
}

void SAIEvent::Draw( CAIHistoryDebugCanvas& canvas ) const
{
	Float trackOffset = canvas.GetTrackOffset( m_trackIdx );
	const Float trackHeight = canvas.GetTrackHeight();

	// adjust the color with respect to the result
	wxColor color;
	switch( m_result )
	{
		default:
		case EAIR_InProgress:
		{
			color = wxColor( 200, 200, 200 );
			break;
		}

		case EAIR_Success:
		{
			color = wxColor( 100, 200, 100 );
			break;
		}

		case EAIR_Failure:
		{
			color = wxColor( 200, 100, 100 );
			break;
		}

		case EAIR_Exception:
		{
			color = wxColor( 50, 50, 50 );
			break;
		}

		case EAIR_Interrupted:
		{
			color = wxColor( 100, 100, 200 );
			break;
		}
	}

	// adjust time settings
	Float startTime = m_startTime;
	Float endTime = m_endTime;
	if ( m_result == EAIR_InProgress )
	{
		endTime = (Float)GGame->GetEngineTime();
	}

	Uint32 trackStart = canvas.WorldToCanvas( startTime );
	Uint32 trackEnd = canvas.WorldToCanvas( endTime );
	Uint32 trackDuration = ::Max< Uint32 >( 1, trackEnd - trackStart );

	if ( trackDuration <= 1 )
	{
		canvas.DrawLine( trackStart, trackOffset, trackStart, trackOffset + trackHeight, color );
	}
	else
	{
		canvas.FillRect( trackStart, trackOffset, trackDuration, trackHeight, color );
		canvas.DrawRect( trackStart, trackOffset, trackDuration, trackHeight, wxColor( 255, 198, 30 ) );

		// draw the description
		canvas.DrawText( wxPoint( trackStart, trackOffset ), trackDuration, trackHeight, canvas.GetFont(), m_name.AsChar(), wxColor( 255, 255, 255 ) );
	}
}

Bool SAIEvent::IsInRange( Float time, Int32 trackIdx ) const
{
	// adjust time settings
	if ( m_result == EAIR_InProgress )
	{
		return time >= m_startTime && m_trackIdx == trackIdx;
	}
	else
	{
		return time >= m_startTime && time <= m_endTime && m_trackIdx == trackIdx;
	}
}

Bool SAIEvent::IsInRange( CAIHistoryDebugCanvas& canvas, Uint32 trackPos, Int32 trackIdx ) const
{
	// adjust time settings
	Float startTime = m_startTime;
	Float endTime = m_endTime;
	if ( m_result == EAIR_InProgress )
	{
		endTime = (Float)GGame->GetEngineTime();
	}

	Int32 trackStart = canvas.WorldToCanvas( startTime );
	Int32 trackEnd = canvas.WorldToCanvas( endTime );
	Int32 trackDuration = ::Max< Int32 >( 1, trackEnd - trackStart );
	trackEnd = trackStart + trackDuration;

	return (Int32)trackPos >= trackStart && (Int32)trackPos <= trackEnd && m_trackIdx == trackIdx;
}

///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( CAIHistoryDebugCanvas, CEdCanvas )
	EVT_MOUSEWHEEL( CAIHistoryDebugCanvas::OnMouseWheel )
	EVT_CHAR( CAIHistoryDebugCanvas::OnCharPressed )
END_EVENT_TABLE()

CAIHistoryDebugCanvas::CAIHistoryDebugCanvas( wxWindow* parentWindow, CMaraudersMap* parent )
	: CEdCanvas( parentWindow )
	, m_parent( parent )
	, m_scaleWorld( 0.1f )
	, m_trackHeight( 50.0f )
	, m_headersWidth( 200.0f )
	, m_viewStartOffset( 0.0f )
	, m_scrolling( false )
	, m_syncWithTime( true )
	, m_currWidth( 0 )
{
	wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
	m_drawFont = new Gdiplus::Font( ::GetDC( (HWND) this->GetHWND() ), (HFONT) font.GetHFONT() );
}

CAIHistoryDebugCanvas::~CAIHistoryDebugCanvas()
{
	delete m_drawFont;
	m_drawFont = NULL;
}

void CAIHistoryDebugCanvas::PaintCanvas( Int32 width, Int32 height )
{
	// memorize current canvas width 
	m_currWidth = width - m_headersWidth;

	// Colors
	wxColour back( 255, 255, 255 );

	// Paint background
	Clear( back );

	// update the view offset position
	Float currTime = GGame->GetEngineTime();
	if ( m_syncWithTime )
	{
		m_viewStartOffset = currTime - ( m_currWidth * 0.5f ) * m_scaleWorld;
	}

	// draw the selected item's history
	const CMaraudersMapItemBase* selectedItem = m_parent->GetSelectedItem();
	if ( selectedItem )
	{
		// draw the history
		const TDynArray< SAIEvent >& aiHistory = selectedItem->GetAIHistory();
		for ( TDynArray< SAIEvent >::const_iterator it = aiHistory.Begin(); it != aiHistory.End(); ++it )
		{
			it->Draw( *this );
		}
	}

	// draw the time line
	Float timeLinePos = WorldToCanvas( currTime );
	DrawLine( timeLinePos, 0, timeLinePos, height, wxColour( 100, 150, 200 ), 2.0f );

	// draw the time line
	Float timelineStartTime = (Int32)CanvasToWorld( m_headersWidth );
	Float timelineEndTime = (Int32)CanvasToWorld( m_currWidth + m_headersWidth );
	Float timeStep = ::Max( 1, (Int32)( ( timelineEndTime - timelineStartTime ) * 0.1f ) );
	for ( Float i = timelineStartTime; i < timelineEndTime; i += timeStep )
	{
		Float timeStepPos = WorldToCanvas( (Int32)i );
		DrawLine( timeStepPos, 0, timeStepPos, 10, wxColor( 0, 0, 0 ), 2 );
		DrawText( wxPoint( timeStepPos + 2, 0 ), *m_drawFont, String::Printf( TXT( "%.3f" ), i ).AsChar(), wxColor( 0, 0, 0 ) );
	}

	// draw the track names
	FillRect( 0, 0, m_headersWidth, height, wxColour( 255, 255, 255 ) );
	if ( selectedItem )
	{
		const TDynArray< String >& trackNames = selectedItem->GetAITrackNames();
		Uint32 count = trackNames.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			Float trackOffset = GetTrackOffset( i );
			DrawText( wxPoint( 0, trackOffset ), 200, (Uint32)m_trackHeight, *m_drawFont, trackNames[i].AsChar(), wxColor( 0, 0, 0 ) );
			if ( i > 0 )
			{
				DrawLine( 0, trackOffset, width, trackOffset, wxColour( 0, 0, 0 ), 2.0f );
			}
		}
	}
	DrawLine( m_headersWidth, 0, m_headersWidth, height, wxColour( 0, 0, 0 ), 2.0f );
}

Float CAIHistoryDebugCanvas::WorldToCanvas( Float time ) const
{
	return m_headersWidth + ( time - m_viewStartOffset ) / m_scaleWorld;
}

Float CAIHistoryDebugCanvas::CanvasToWorld( Float point ) const
{
	return ( point - m_headersWidth ) * m_scaleWorld + m_viewStartOffset;
}

void CAIHistoryDebugCanvas::DisplayDescription( const wxPoint& pos )
{
	// get the selected track
	Uint32 trackIdx = pos.y / ( m_trackHeight + 2.0f );

	// send an item request to the selected item
	const CMaraudersMapItemBase* selectedItem = m_parent->GetSelectedItem();
	if ( selectedItem )
	{
		const TDynArray< SAIEvent >& aiHistory = selectedItem->GetAIHistory();
		for ( TDynArray< SAIEvent >::const_iterator it = aiHistory.Begin(); it != aiHistory.End(); ++it )
		{
			Bool isInTimeRange = it->IsInRange( *this, pos.x, trackIdx );
			if ( isInTimeRange )
			{
				// send the event description to the parent
				m_parent->DisplayAIEventDescription( *it );
				break;
			}
		}
	}
}

void CAIHistoryDebugCanvas::MouseClick( wxMouseEvent& event )
{
	CEdCanvas::MouseClick( event );

	if ( !m_scrolling )
	{
		if ( event.RightDown() )
		{
			CaptureMouse( true, false );
			m_scrolling = true;
			m_syncWithTime = false;
		}
		else if ( event.LeftDown() )
		{
			DisplayDescription( event.GetPosition() );
		}
	}

	if ( event.RightUp() )
	{
		CaptureMouse( false, false );
		m_scrolling = false;

		// check the position and decide if we should keep the track in sync with time flow
		Float currTime = (Float)GGame->GetEngineTime();
		Float inSyncStartOffset = currTime - ( m_currWidth * 0.5f ) * m_scaleWorld;
		m_syncWithTime = ( m_viewStartOffset > inSyncStartOffset );
	}
}

void CAIHistoryDebugCanvas::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	if ( m_scrolling )
	{
		m_viewStartOffset -= (Float)( delta.x ) * m_scaleWorld;
	}
}

void CAIHistoryDebugCanvas::OnMouseWheel( wxMouseEvent& event )
{
	// Calculate scale, but sometimes GetWheelRotation() returns zero
	Float delta = 0.0f;
	if ( event.GetWheelRotation() != 0 )
	{
		delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	}
	// increase speed if shift is pressed
	Float wheelScale = event.ShiftDown() ? 0.25f : 0.05f;
	Float newScaleWorld = ::Clamp( m_scaleWorld * (1.0f - delta * wheelScale), 0.001f, 10.0f );
	m_scaleWorld = newScaleWorld;

	Refresh();
}

void CAIHistoryDebugCanvas::OnCharPressed( wxKeyEvent& event )
{
	if ( event.GetKeyCode() == WXK_HOME )
	{
		m_syncWithTime = true;
		Refresh();
	}
}

///////////////////////////////////////////////////////////////////////////////
