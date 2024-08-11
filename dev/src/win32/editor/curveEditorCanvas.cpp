/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */


#include "build.h"
#include "curveEditorCanvas.h"
#include "../../common/core/curveData.h"
#include "shapesPreviewItem.h"
#include "../../common/engine/curve.h"
//RED_DEFINE_STATIC_NAME( EditorTick )

class CurveInfoWrapper : public wxObject
{
public:
	CEdCurveEditorCanvas::CurveInfo			m_curveInfo;

public:
	CurveInfoWrapper( CEdCurveEditorCanvas::CurveInfo curveInfo )
		: m_curveInfo( curveInfo )
	{};
};

// Event table
BEGIN_EVENT_TABLE( CEdCurveEditorCanvas, CEdCanvas )
	EVT_KEY_DOWN( CEdCurveEditorCanvas::OnKeyDown )
	EVT_MOUSEWHEEL( CEdCurveEditorCanvas::OnMouseWheel )
	EVT_LEFT_DCLICK( CEdCurveEditorCanvas::OnMouseLeftDoubleClick )
END_EVENT_TABLE()

CEdCurveEditorCanvas::CEdCurveEditorCanvas( wxWindow *parent )
	: CEdCanvas( parent )
	, m_showSnappingLines( true )
	, m_showControlPoints( true )
	, m_showTangents( true )
	, m_action( MA_None )
	, m_scaleCurves( 1.0f, 1.0f, 0.0f )
	, m_activeRegionStart( 0.0f )
	, m_activeRegionEnd( 1.0f )
	, m_activeRegionShow( false )
	, m_hook( NULL )
	, m_editor( NULL )
{
	// Register as event processor
	SEvents::GetInstance().RegisterListener( CNAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( CNAME( Destroyed ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointRemoved ), this );
	SEvents::GetInstance().RegisterListener( CNAME( CurvePointAdded ), this );
}

CEdCurveEditorCanvas::~CEdCurveEditorCanvas()
{
	// Clear mouseOver and Selected arrays
	m_controlPointInfosSelected.Clear();
	m_controlPointInfosMouseOver.Clear();

	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		while ( m_curveGroups[i].m_curveInfos.Size() )
		{
			CCurveEditionWrapper* toDelete = m_curveGroups[i].m_curveInfos[ m_curveGroups[i].m_curveInfos.Size() - 1 ].m_curve;

			SCurveEditionEnded eventData( this, &toDelete->GetCurveData() );
			SEvents::GetInstance().DispatchEvent( CNAME( CurveEditionEnded ), CreateEventData( eventData ) );

			m_curveGroups[i].m_curveInfos.RemoveAt( m_curveGroups[i].m_curveInfos.Size() - 1 );
			delete toDelete;
		}
		LOG_EDITOR( TXT("Curve Group %s removed from curve editor"), m_curveGroups[i].m_curveGroupName.AsChar() );
	}
	for( Uint32 j = 0; j < m_curveContainers.Size(); ++j )
	{
		m_curveContainers[j]->RemoveFromRootSet();
	}

	// Unregister event processor
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdCurveEditorCanvas::SetHook( IEdCurveEditorCanvasHook* hook )
{
	m_hook = hook;
}

void CEdCurveEditorCanvas::DispatchEditorEvent( const CName& systemEvent, IEdEventData* data )
{
	if ( systemEvent == CNAME( EditorTick ) )
	{
		// Automatic scroll background
		if ( m_autoScroll.x != 0 || m_autoScroll.y != 0 )
		{
			ScrollBackgroundOffset( m_autoScroll );
		}
	}
	else if ( systemEvent == CNAME( Destroyed ) )
	{
		if ( CCurve* curve = GetEventData< CCurve* >( data ) )
		{
			RemoveCurve( &curve->GetCurveData() );
			Refresh();
		}
	}
	else if ( systemEvent == CNAME( CurvePointChanged ) )
	{
		const SCurveValueChanged& eventData = GetEventData< SCurveValueChanged >( data );
		if ( eventData.m_sender == this ) return;
		Repaint();
	}
	else if ( systemEvent == CNAME( CurvePointRemoved ) )
	{
		const SCurvePointRemoved& eventData = GetEventData< SCurvePointRemoved >( data );
		if ( eventData.m_sender == this ) return;
		for ( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
		{
			for ( Uint32 j = 0; j < m_curveGroups[i].GetCurveInfos().Size(); ++j )
			{
				if ( eventData.m_curve == &m_curveGroups[i].GetCurveInfos()[j].GetCurve()->GetCurveData() )
				{
					const TDynArray< CCurveEditionWrapper::ControlPoint* >& cp = m_curveGroups[i].GetCurveInfos()[j].GetCurve()->GetControlPoints();
					for ( Uint32 k = 0; k < cp.Size(); ++k )
					{
						if ( cp[k]->GetTime() == eventData.m_time )
						{
							m_curveGroups[i].GetCurveInfos()[j].GetCurve()->RemoveControlPoint( cp[k], false );
							Repaint();
						}
					}
					Repaint();
					return;
				}
				
			}
		}
	}
	else if ( systemEvent == CNAME( CurvePointAdded ) )
	{
		const SCurvePointAdded& eventData = GetEventData< SCurvePointAdded >( data );
		if ( eventData.m_sender == this ) return;
		for ( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
		{
			for ( Uint32 j = 0; j < m_curveGroups[i].GetCurveInfos().Size(); ++j )
			{
				if ( eventData.m_curve == &m_curveGroups[i].GetCurveInfos()[j].GetCurve()->GetCurveData() )
				{
					m_curveGroups[i].GetCurveInfos()[j].GetCurve()->AddControlPoint( eventData.m_time, 0.0f, CST_Interpolate, false );
					Repaint();
					return;
				}

			}
		}
	}
}

void CEdCurveEditorCanvas::ScrollBackgroundOffset( wxPoint delta )
{
	wxPoint point = GetOffset();

	// Scroll background offset
	point.x += delta.x;			
	point.y += delta.y;

	// Scroll canvas
	SetOffset( point );
}

Vector CEdCurveEditorCanvas::CanvasToCurve( wxPoint point ) const
{
	return Vector( point.x * m_scaleCurves.X, -point.y * m_scaleCurves.Y, 0.0f );
}

wxPoint CEdCurveEditorCanvas::CurveToCanvas( const Vector& point ) const
{
	return wxPoint( point.X / m_scaleCurves.X, -point.Y / m_scaleCurves.Y );
}

Vector CEdCurveEditorCanvas::ClientToCurve( wxPoint point ) const
{
	return CanvasToCurve( ClientToCanvas( point ) );
}

wxPoint CEdCurveEditorCanvas::CurveToClient( const Vector& point ) const
{
	return CanvasToClient( CurveToCanvas( point ) );
}

void CEdCurveEditorCanvas::DrawLine( Float x1, Float y1, Float x2, Float y2, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = CurveToCanvas( Vector( x1, y1, 0.0f ) );
	wxPoint v2 = CurveToCanvas( Vector( x2, y2, 0.0f ) );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

void CEdCurveEditorCanvas::DrawLine( const Vector& start, const Vector& end, const wxColour& color, Float width/*=1.0f*/ )
{
	wxPoint v1 = CurveToCanvas( start );
	wxPoint v2 = CurveToCanvas( end );
	CEdCanvas::DrawLine( v1, v2, color, width );
}

void CEdCurveEditorCanvas::DrawVLine( Float x, const wxColour& color, Float width/*=1.0f*/ )
{
	Vector zoomedRegion = GetZoomedRegion();
	wxPoint start = CurveToCanvas( Vector( x, zoomedRegion.Y, 0.0f )  );
	wxPoint end   = CurveToCanvas( Vector( x, zoomedRegion.W, 0.0f )  );
	CEdCanvas::DrawLine( start, end, color, width );
}

void CEdCurveEditorCanvas::DrawHLine( Float y, const wxColour& color, Float width/*=1.0f*/ )
{
	Vector zoomedRegion = GetZoomedRegion();
	wxPoint start = CurveToCanvas( Vector( zoomedRegion.X, y, 0.0f )  );
	wxPoint end   = CurveToCanvas( Vector( zoomedRegion.Z, y, 0.0f )  );
	CEdCanvas::DrawLine( start, end, color, width );
}

void CEdCurveEditorCanvas::DrawText( const Vector& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = CurveToCanvas( offset );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CEdCurveEditorCanvas::DrawText( const Float x, const Float y, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint v1 = CurveToCanvas( Vector( x, y, 0.0f ) );
	CEdCanvas::DrawText( v1, font, text, color, vAlign, hAlign );
}

void CEdCurveEditorCanvas::DrawControlPoint( wxPoint point, const wxColour& borderColor, const wxColour &baseColor )
{
	const Int32 size = 8;
	wxColour shadowColor( baseColor.Red() * 3 / 4, baseColor.Green() * 3 / 4, baseColor.Blue() * 3 / 4 );
	DrawCircle( point.x-size/2+0, point.y-size/2+0, size  , borderColor );
	FillCircle( point.x-size/2+1, point.y-size/2+1, size-2, baseColor );
	DrawCircle( point.x-size/2+1, point.y-size/2+1, size-2, shadowColor );
}

void CEdCurveEditorCanvas::DrawTangentPoint( wxPoint point, const wxColour& borderColor, const wxColour &baseColor )
{
	const Int32 size = 6;
	wxColour shadowColor( baseColor.Red() * 3 / 4, baseColor.Green() * 3 / 4, baseColor.Blue() * 3 / 4 );
	DrawCircle( point.x-size/2+0, point.y-size/2+0, size  , borderColor );
	FillCircle( point.x-size/2+1, point.y-size/2+1, size-2, baseColor );
	DrawCircle( point.x-size/2+1, point.y-size/2+1, size-2, shadowColor );
}

Int32 CEdCurveEditorCanvas::GetSidePanelWidth() const
{
	// Width of side panel is the same as curveInfo width
	return CURVE_EDITOR_CANVAS_SIDE_PANEL_WIDTH;
}

Int32 CEdCurveEditorCanvas::GetSidePanelHeight() const
{
	// Height of side panel is full canvas height
	return GetClientSize().GetHeight();
}

Bool CEdCurveEditorCanvas::IsMouseOverCanvasPanel( const wxPoint& mousePosition ) const
{
	// Calc size of curveInfos side panel
	Int32 panelWidth  = GetSidePanelWidth();
	Int32 panelHeight = GetSidePanelHeight();
	wxRect sidePanelRect( 0, 0, panelWidth, panelHeight );
	return sidePanelRect.Contains( mousePosition ) ? false : true;
}

void CEdCurveEditorCanvas::MouseClick( wxMouseEvent& event )
{
	// Pass to base class
	CEdCanvas::MouseClick( event );

	// Check if mouse is over side panel or curve canvas panel
	if ( IsMouseOverCanvasPanel( event.GetPosition() ) )
	{
		MouseClickCanvasPanel( event );
	}
	else
	{
		MouseClickSidePanel( event );
	}

	// Always repaint canvas when clicked
	Repaint();
}

void CEdCurveEditorCanvas::MouseClickSidePanel( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	ASSERT( IsMouseOverCanvasPanel( point ) == false );

	// Check if we click over curve infos (curve infos are on the left side of canvas)
	wxPoint position( 0, 0 );
	if ( m_action == MA_None && event.LeftDown() )
	{
		for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
		{
			if( m_curveGroups[i].MouseClick( point, position ) )
			{
				return;
			}
			position.y += m_curveGroups[i].GetClientRect( position ).GetHeight();
		}
	}
}

void CEdCurveEditorCanvas::MouseClickCanvasPanel( wxMouseEvent& event )
{
	wxPoint point = event.GetPosition();

	ASSERT( IsMouseOverCanvasPanel( point ) == true );

	const Int32 maxMoveTotal = 5;
	const Int32 snappingBorderSize = 15;

	// Selecting control points
	if( m_action == MA_None && event.LeftDown() && m_controlPointInfosMouseOver.Size() > 0 )
	{
		// Clear already selected points (do not clear when SHIFT is pressed or mouse is over one of selected points)
		if ( ! event.ShiftDown() && !( m_controlPointInfosSelected.Exist( m_controlPointInfosMouseOver[0] ) ) )
		{
			m_controlPointInfosSelected.Clear();
		}

		// Selecting points and going to Moving Mode
		m_controlPointInfosSelected.PushBackUnique( m_controlPointInfosMouseOver[0] );
		SelectionChanged();

		m_action = MA_MovingControlPoints;
	}
	else if ( m_action == MA_MovingControlPoints && event.LeftUp() )
	{
		m_action = MA_None;
		ControlPointsChangedComplete();
		return;
	}

	// Snapping lines
	if ( m_action == MA_None && event.LeftDown() && m_showSnappingLines == true )
	{
		Int32 width, height;
		GetClientSize( &width, &height );
		if ( ( point.y < snappingBorderSize ) || ( point.y > ( height - snappingBorderSize ) ) )
		{
			CaptureMouse( true, false );
			m_snappingLines.PushBack( ClientToCurve( point ).Y );
			m_snappingLineSelectedIndex = m_snappingLines.Size() - 1;
			m_action = MA_MovingSnappingLine;
		}
		else
		{
			for( Uint32 i = 0; i < m_snappingLines.Size(); ++i )
			{
				if ( abs( ClientToCanvas( point ).y - CurveToCanvas( Vector( 0.0f, m_snappingLines[i], 0.0f ) ).y ) < 10 )
				{
					CaptureMouse( true, false );
					m_snappingLineSelectedIndex = i;
					m_action = MA_MovingSnappingLine;
				}
			}
		}
	}
	else if ( m_action == MA_MovingSnappingLine && event.LeftUp() )
	{
		Int32 width, height;
		GetClientSize( &width, &height );
		if ( ( point.y < snappingBorderSize ) || ( point.y > ( height - snappingBorderSize ) ) )
		{
			m_snappingLines.Remove( m_snappingLines[m_snappingLineSelectedIndex] );
		}
		CaptureMouse( false, false );
		m_snappingLineSelectedIndex = -1;
		m_action = MA_None;
	}

	// Selecting window
	if ( m_action == MA_None && event.LeftDown() && m_controlPointInfosMouseOver.Size() == 0 )
	{
		// Deselecting all selected points
		m_controlPointInfosSelected.Clear();
		SelectionChanged();

		// Going to Selecting Window Mode
		m_action = MA_SelectingWindows;
		CaptureMouse( true, false );

		m_selectionRegion.SetStartAndEndPoint( ClientToCanvas( point ) );
	}
	else if ( m_action == MA_SelectingWindows && event.LeftUp() )
	{
		m_action = MA_None;
		CaptureMouse( false, false );

		// Assemble selection rect
		m_selectionRegion.SetEndPoint( ClientToCanvas( point ) );

		// If SHIFT is pressed, so zoom in, if not add control points to selection array
		if ( event.ShiftDown() )
		{
			Vector start = CanvasToCurve( m_selectionRegion.GetStartPoint() );
			Vector end   = CanvasToCurve( m_selectionRegion.GetEndPoint() );
			SetZoomedRegion( start, end );
		}
		else
		{
			m_controlPointInfosSelected.PushBackUnique( m_controlPointInfosMouseOver );
			SelectionChanged();
		}
		return;
	}

	// Zooming via mouse move (x and y scale are not locked)
	if ( m_action == MA_None && event.RightDown() && event.ControlDown() )
	{
		m_action = MA_Zooming;
		CaptureMouse( true, false );
		return;
	}
	else if ( m_action == MA_Zooming && event.RightUp() )
	{
		m_action = MA_None;
		CaptureMouse( false , false );
		return;
	}

	// Background drag
	if ( m_action == MA_None && event.RightDown() )
	{
		m_action = MA_BackgroundScroll;
		CaptureMouse( true, true );
		m_moveTotal	 = 0;
	}
	else if ( m_action == MA_BackgroundScroll && event.RightUp() )
	{
		m_action = MA_None;
		CaptureMouse( false, true );

		// Minimal movement, show menu
		if ( m_moveTotal < maxMoveTotal && m_controlPointInfosMouseOver.Size() == 1 )
		{
			OpenControlPointContextMenu();
		}
	}
}

void CEdCurveEditorCanvas::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	wxPoint point = event.GetPosition();

	if ( IsMouseOverCanvasPanel( point ) )
	{
		const Int32 autoScrollBorderSize = 10;

		// We do not repaint if it's not necessary, because preview panel stops when we repaint
		if ( m_action == MA_Zooming )
		{
			Float wheelScale = 0.0001f;
			Vector curveDelta = ClientToCurve( delta ) * wheelScale;
			m_scaleCurves.X = Clamp< Float >( m_scaleCurves.X + delta.x * wheelScale, 0.0001f, 100.0f );
			m_scaleCurves.Y = Clamp< Float >( m_scaleCurves.Y + delta.y * wheelScale, 0.0001f, 100.0f );
			Refresh();
			Update();

			// We don't want autoscroll, and lastMousePosition updated
			return;
		}
		else if ( m_action == MA_MovingControlPoints )
		{
			ASSERT( m_controlPointInfosSelected.Size() > 0 );
			Vector deltaInCurveSpace = ClientToCurve( point ) - m_lastMousePosition;
			if ( !m_xAxisMoveEnabled )
			{
				deltaInCurveSpace.X = 0;
			}
			if ( !m_yAxisMoveEnabled )
			{
				deltaInCurveSpace.Y = 0;
			}
			ControlPointsMove( m_controlPointInfosSelected, deltaInCurveSpace );
			Refresh();
			Update();
		}
		else if ( m_action == MA_MovingSnappingLine )
		{
			ASSERT( m_snappingLineSelectedIndex >= 0 );
			m_snappingLines[m_snappingLineSelectedIndex] = ClientToCurve( point ).Y;
			Refresh();
			Update();
		}
		else if ( m_action == MA_None )
		{
			TDynArray< ControlPointInfo > oldMouseOver = m_controlPointInfosMouseOver;
			m_controlPointInfosMouseOver.Clear();

			for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
			{
				TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
				for ( Uint32 i = 0; i < curveInfos.Size(); ++i )
				{
					Bool skipTangents = false;
					ControlPointInfo mouseControlPoint;

					if ( curveInfos[i].m_showControlPoints )
					{
						TDynArray< ControlPointInfo > controlPoints;
						ControlPointsFromCurve( &m_curveGroups[j], i, true, false, controlPoints );
						if ( ControlPointsNearPoint( controlPoints, point, 5*5, mouseControlPoint ) )
						{
							m_controlPointInfosMouseOver.PushBackUnique( mouseControlPoint );
							skipTangents = true;
							break;
						}
						
					}
					if ( curveInfos[i].m_showTangents && skipTangents == false )
					{
						TDynArray< ControlPointInfo > controlPoints;
						ControlPointsFromCurve( &m_curveGroups[j], i, false, true, controlPoints );
						if ( ControlPointsNearPoint( controlPoints, point, 5*5, mouseControlPoint ) )
						{
							m_controlPointInfosMouseOver.PushBackUnique( mouseControlPoint );
							break;
						}
					}
				}
			}

			if ( !(oldMouseOver == m_controlPointInfosMouseOver) )
			{
				Repaint();
			}
		}
		else if ( m_action == MA_BackgroundScroll )
		{
			ScrollBackgroundOffset( delta );
			Repaint( true );
		}
		else if ( m_action == MA_SelectingWindows )
		{
			// Calc selection rect
			m_selectionRegion.SetEndPoint( ClientToCanvas( point ) );

			// Add control points to mouse over array
			TDynArray< ControlPointInfo > controlPoints;
			for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
			{
				TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
				for ( Uint32 i = 0; i < curveInfos.Size(); ++i )
				{
					if ( curveInfos[i].m_showControlPoints )
					{
						ControlPointsFromCurve( &m_curveGroups[j], i, true, false, controlPoints );
					}
				}
			}

			m_controlPointInfosMouseOver.Clear();
			ControlPointsInArea( controlPoints, m_selectionRegion.GetRect(), m_controlPointInfosMouseOver );
			Repaint();
		}

		// Common part, auto scroll, calc total mouse move, reset auto scroll and calc last mouse position
		{
			// Accumulate move
			m_moveTotal += Abs( delta.x ) + Abs( delta.y );

			// Remember mouse position
			m_lastMousePosition = ClientToCurve( point );

			// Auto scroll setup
			m_autoScroll = wxPoint(0,0);
			if ( m_action == MA_MovingControlPoints || m_action == MA_SelectingWindows || m_action == MA_MovingSnappingLine )
			{
				Int32 width, height;
				Int32 sidePanelSize = GetSidePanelWidth();
				GetClientSize( &width, &height );
				m_autoScroll.x += (point.x > sidePanelSize && point.x < sidePanelSize + autoScrollBorderSize) ? 5 : 0;
				m_autoScroll.x -= (point.x > ( width - autoScrollBorderSize )) ? 5 : 0;
				m_autoScroll.y += (point.y < autoScrollBorderSize) ? 5 : 0;
				m_autoScroll.y -= (point.y > ( height - autoScrollBorderSize )) ? 5 : 0;

				Repaint();
			}
		}
	}
	else
	{
		// Mouse is over side panel
		wxPoint position( 0, 0 );
		TDynArray< ControlPointInfo > oldMouseOver;
		oldMouseOver = m_controlPointInfosMouseOver;
		m_controlPointInfosMouseOver.Clear();
		for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
		{
			position.y += 40;
			for( Uint32 j = 0; j < m_curveGroups[i].GetCurveInfos().Size(); ++j )
			{
				wxRect curveInfoRect = m_curveGroups[i].GetCurveInfos()[j].GetClientRect( position );
				if ( curveInfoRect.Contains( point ) )
				{
					ControlPointsFromCurve(  &m_curveGroups[i], j, true, false, m_controlPointInfosMouseOver );
					Repaint();
					return;
				}
				position.y += curveInfoRect.GetHeight();
			}
		}
		Repaint();
	}
}

void CEdCurveEditorCanvas::OnMouseWheel( wxMouseEvent& event )
{
	// Find zooming pivot
	Vector zoomingPivot = ClientToCurve( event.GetPosition() );

	// Calc zoomed region
	Vector zoomedRegion = GetZoomedRegion();
	Vector zoomedMin = Vector( zoomedRegion.X, zoomedRegion.Y, 0.0f );
	Vector zoomedMax = Vector( zoomedRegion.Z, zoomedRegion.W, 0.0f );

	// Calc scale, but sometimes GetWheelRotation() returns zero
	Float delta = 0.0f;
	if ( event.GetWheelRotation() != 0 )
	{
		delta = ( event.GetWheelDelta() / (FLOAT)event.GetWheelRotation() );
	}
	Float wheelScale = event.ShiftDown() ? 0.25f : 0.05f;

	// Calc new zoomed region
	Vector newZoomedMin = zoomingPivot + (zoomedMin - zoomingPivot) * (1.0f - delta * wheelScale);
	Vector newZoomedMax = zoomingPivot + (zoomedMax - zoomingPivot) * (1.0f - delta * wheelScale);
	SetZoomedRegion( newZoomedMin, newZoomedMax );
}

void CEdCurveEditorCanvas::OnKeyDown( wxKeyEvent &event )
{
	wxPoint mousePosition = event.GetPosition();
	if ( IsMouseOverCanvasPanel( mousePosition ) )
	{
		if ( event.GetKeyCode() == 'A' && event.ControlDown() )
		{
			for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
			{
				TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
				for ( Uint32 i = 0; i < curveInfos.Size(); ++i )
				{
					Bool skipTangents = false;
					ControlPointInfo mouseControlPoint;

					TDynArray< ControlPointInfo > controlPoints;
					ControlPointsFromCurve( &m_curveGroups[j], i, true, false, controlPoints );
					m_controlPointInfosSelected.PushBack( controlPoints );
				}
			}
		}
		if ( event.GetKeyCode() == WXK_DELETE )
		{
			TDynArray< CCurveEditionWrapper::ControlPoint* > removedControlPoints;

			// Iterate over all selected control points
			for( Uint32 i = 0; i < m_controlPointInfosSelected.Size(); ++i )
			{
				const ControlPointInfo& cp = m_controlPointInfosSelected[i];

				// Remove all control points except first one
				if ( cp.GetCurve()->GetCurveData().Size() > 1 )
				{
					// Check if we already removed that control point
					// It may be deleted already, because we do it in batches for curve groups
					if ( !removedControlPoints.Exist( cp.GetControlPoint() ) )
					{
						SCurvePointRemoved eventData( this, cp.GetControlPoint()->GetTime(), &cp.GetControlPoint()->GetCurve()->GetCurveData() );
						cp.GetCurve()->RemoveControlPoint( cp.GetControlPoint() );
						removedControlPoints.PushBack( cp.GetControlPoint() );
						SEvents::GetInstance().DispatchEvent( CNAME( CurvePointRemoved ), CreateEventData( eventData ) );
					}
				}
			}

			// Clear selection and mouse over
			m_controlPointInfosMouseOver.Clear();
			m_controlPointInfosSelected.Clear();
		}
	}
	else
	{
		if ( event.GetKeyCode() == WXK_DELETE )
		{
			wxPoint position( 0, 0 );
			for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
			{
				wxRect curveInfoRect = m_curveGroups[i].GetClientRect( position );
				if( curveInfoRect.Contains( mousePosition ) )
				{
					RemoveCurveGroup( CName( m_curveGroups[i].m_curveGroupName ) );
					break;
				}
				position.y += m_curveGroups[i].GetClientRect( position ).GetHeight();
			}

			// Clear selection and mouse over
			m_controlPointInfosMouseOver.Clear();
			m_controlPointInfosSelected.Clear();
		}
	}

	// Common part - works on side panel and curve canvas panel
	if ( event.GetKeyCode() == WXK_HOME )
	{
		SetZoomedRegionToFit();
	}
	else if ( event.GetKeyCode() == WXK_PAGEUP || event.GetKeyCode() == WXK_PAGEDOWN ) 
	{
		Float zoomFactor = ( event.GetKeyCode() == WXK_PAGEUP ) ? +0.5f : -0.25f;
		Vector zoomedRegion = GetZoomedRegion();
		Float regionSizeX = zoomedRegion.Z - zoomedRegion.X;
		Float regionSizeY = zoomedRegion.W - zoomedRegion.Y;
		Vector corner1( zoomedRegion.X - zoomFactor * regionSizeX, zoomedRegion.Y - zoomFactor * regionSizeY, 0.0f );
		Vector corner2( zoomedRegion.Z + zoomFactor * regionSizeX, zoomedRegion.W + zoomFactor * regionSizeY, 0.0f );
		SetZoomedRegion( corner1, corner2 );
	}

	// Always repaint when key is pressed
	Repaint();
}

void CEdCurveEditorCanvas::SetActiveRegion( Float start, Float end )
{
	m_activeRegionStart = start;
	m_activeRegionEnd = end;
}

void CEdCurveEditorCanvas::GetActiveRegion( Float& start, Float& end ) const
{
	start = m_activeRegionStart;
	end = m_activeRegionEnd;
}

void CEdCurveEditorCanvas::SetZoomedRegion( const Vector& corner1, const Vector& corner2 )
{
	if ( corner1 == corner2 )
	{
		// Zoomed region has no area
		return;
	}

	// Calc upper left corner and lower right corner
	Vector start = Vector::Min4( corner1, corner2 );
	Vector end   = Vector::Max4( corner1, corner2 );

	// Calc proper clientRect (which take into account left side curvesInfo
	wxRect clientRect = GetClientRect();
	clientRect.width -= GetSidePanelWidth();

	// Calc new scale
	m_scaleCurves.X = (end.X - start.X) / Max(clientRect.width , 1);
	m_scaleCurves.Y = (end.Y - start.Y) / Max(clientRect.height, 1);

	// Calc new offset
	wxPoint offset;
	offset.x = -(start.X / m_scaleCurves.X) + GetSidePanelWidth();
	offset.y = +(start.Y / m_scaleCurves.Y) + clientRect.height;
	SetOffset( offset );
	Repaint();
}

void CEdCurveEditorCanvas::SetZoomedRegionToFit()
{
	TDynArray< ControlPointInfo > controlPoints;

	for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
	{
		TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
		for( Uint32 i = 0; i < curveInfos.Size(); ++i )
		{
			ControlPointsFromCurve( &m_curveGroups[j], i, curveInfos[i].m_showControlPoints, curveInfos[i].m_showTangents, controlPoints );
		}
	}

	if ( controlPoints.Size() == 0 )
	{	
		return;
	}

	Box box( Vector( m_activeRegionStart, 0.0f, 0.0f ), Vector( m_activeRegionEnd, 0.0f, 0.0f ) );
	for( Uint32 i = 0; i < controlPoints.Size(); ++i ) 
	{
		box.AddPoint( controlPoints[i].GetPosition() );
	}

	Vector extrude = box.CalcExtents().Mul4( 0.1f ) + Vector( 0.01f, 0.01f, 0.00f );
	SetZoomedRegion( box.Min - extrude, box.Max + extrude );
}

Vector CEdCurveEditorCanvas::GetZoomedRegion() const
{
	wxSize clientSize = GetClientSize();
	Vector corner1 = ClientToCurve( wxPoint( CURVE_EDITOR_CANVAS_SIDE_PANEL_WIDTH, 0 ) );
	Vector corner2 = ClientToCurve( wxPoint( clientSize.GetWidth(), clientSize.GetHeight() ) );
	return Vector( corner1.X, corner2.Y, corner2.X, corner1.Y );
}

Float CEdCurveEditorCanvas::SnapFloat( const Float& floatToSnap ) const
{
	ASSERT( floatToSnap > 0.0f );

	// Without this, I had endless loop on editor start
	if ( floatToSnap <= 0.0f )
	{
		return 0.0f;
	}

	Float snappedValue = 1024.0f;
	while( floatToSnap < snappedValue )
	{
		snappedValue /= 2.0f;
	}
	return snappedValue;
}

void CEdCurveEditorCanvas::SelectionChanged()
{
	if ( m_hook )
	{
		m_hook->OnCanvasHookSelectionChanged();
	}
}

void CEdCurveEditorCanvas::ControlPointsChanged()
{
	if ( m_hook )
	{
		m_hook->OnCanvasHookControlPointsChanged();
	}
}

void CEdCurveEditorCanvas::ControlPointsChangedComplete()
{
	if ( m_hook )
	{
		m_hook->OnCanvasHookControlPointsChangedComplete();
	}
}

void CEdCurveEditorCanvas::DrawGrid( Int32 width, Int32 height, Bool drawLines, Bool drawText )
{
	static wxColour text( 255, 255, 255 );
	static wxColour lines1( 100, 100, 100 );
	static wxColour lines2( 140, 140, 140 );
	static wxColour lines3( 180, 180, 180 );

	Vector gridSpacing;
	Vector zoomedRegion = GetZoomedRegion();
	wxSize clientSize = GetClientSize();

	// Safety check
	if( clientSize.GetHeight() < 10 || clientSize.GetWidth() < 10 )
	{
		return;
	}

	gridSpacing.X = SnapFloat( (zoomedRegion.Z - zoomedRegion.X) / ( (clientSize.GetWidth()+1) / 80 ) );
	gridSpacing.Y = SnapFloat( (zoomedRegion.W - zoomedRegion.Y) / ( (clientSize.GetHeight()+1) / 40 ) );

	// Vertical
	for( Int32 i = (Int32)( zoomedRegion.X / gridSpacing.X ); i <= (Int32)( zoomedRegion.Z / gridSpacing.X ); ++i )
	{
		if ( drawLines )
		{
			wxColour color = ( i == 0 ) ? lines3 : ( ( i % 8 ) ? lines1 : lines2 );
			DrawLine( i * gridSpacing.X, zoomedRegion.Y, i * gridSpacing.X, zoomedRegion.W, color );
		}	
		if ( drawText )
		{
			String string = String::Printf( TXT("%.2f"), i * gridSpacing.X );
			DrawText( i * gridSpacing.X, zoomedRegion.Y, GetGdiDrawFont(), string, text, CEdCanvas::CVA_Bottom, CEdCanvas::CHA_Center );
		}
	}

	// Horizontal
	for( Int32 i = (Int32)( zoomedRegion.Y / gridSpacing.Y ); i <= (Int32)( zoomedRegion.W / gridSpacing.Y ); ++i )
	{
		if ( drawLines )
		{
			wxColour color = ( i == 0 ) ? lines3 : ( ( i % 8 ) ? lines1 : lines2 );
			DrawLine( zoomedRegion.X, i * gridSpacing.Y, zoomedRegion.Z, i * gridSpacing.Y, color );
		}	
		if ( drawText )
		{
			String string = String::Printf( TXT("%.2f"), i * gridSpacing.Y );
			DrawText( zoomedRegion.X, i * gridSpacing.Y, GetGdiDrawFont(), string, text, CEdCanvas::CVA_Center, CEdCanvas::CHA_Left );
		}
	}

	// Draw region of interest
#if 0
	if ( m_region )
	{
		if ( m_regionStart > upperLeftCornerVector.X && m_regionStart < lowerRightCornerVector.X )
		{
			wxPoint start = CurveToCanvas( Vector( m_regionStart, upperLeftCornerVector.Y, 0.0f ) );
			wxPoint end   = CurveToCanvas( Vector( m_regionStart, lowerRightCornerVector.Y, 0.0f ) );
			if ( drawLines )
			{
				DrawLine( start, end, lines3 );
			}
			if ( drawText )
			{
				String string = String::Printf( TXT("%.2f"), m_regionStart );
				wxPoint stringSize = TextExtents( GetGdiDrawFont(), string );
				DrawText( wxPoint( start.x - (stringSize.x) - 5, start.y ), GetGdiDrawFont(), string, text );
			}
		}
		if ( m_regionEnd > upperLeftCornerVector.X && m_regionEnd < lowerRightCornerVector.X )
		{
			wxPoint start = CurveToCanvas( Vector( m_regionEnd, upperLeftCornerVector.Y, 0.0f ) );
			wxPoint end   = CurveToCanvas( Vector( m_regionEnd, lowerRightCornerVector.Y, 0.0f ) );
			if ( drawLines )
			{
				DrawLine( start, end, lines3 );
			}
			if ( drawText )
			{
				String string = String::Printf( TXT("%.2f"), m_regionEnd );
				wxPoint stringSize = TextExtents( GetGdiDrawFont(), string );
				DrawText( wxPoint( start.x + 5, start.y ), GetGdiDrawFont(), string, text );
			}
		}
	}
#endif
}

void CEdCurveEditorCanvas::DrawSnapLines( Bool drawLines, Bool drawText )
{
	if ( m_showSnappingLines )
	{
		Vector zoomedRegion = GetZoomedRegion();
		Float xStart = zoomedRegion.X;
		Float xEnd   = zoomedRegion.Z;
		Float yStart = zoomedRegion.Y;
		Float yEnd   = zoomedRegion.W;

		// Draw lines
		for( Uint32 i = 0; i < m_snappingLines.Size(); ++i )
		{
			if( m_snappingLines[i] >= yStart && m_snappingLines[i] <= yEnd )
			{
				wxPoint start = CurveToCanvas( Vector( xStart, m_snappingLines[i], 0.0f )  );
				wxPoint end   = CurveToCanvas( Vector( xEnd, m_snappingLines[i], 0.0f )  );
				if ( drawLines )
				{
					DrawHLine( m_snappingLines[i], wxColour(255,0,0) );
				}
				if ( drawText )
				{
					String string = String::Printf( TXT("%.2f"), m_snappingLines[i] );
					wxPoint stringSize = TextExtents( GetGdiDrawFont(), string );
					CEdCanvas::DrawText( wxPoint( end.x - stringSize.x, end.y - (stringSize.x/2) ), GetGdiDrawFont(), string, wxColour(255,0,0) );
				}
			}
		}
	}
}

Bool CEdCurveEditorCanvas::IsCurveGroupAdded( const CName& curveGroupName ) const
{
	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		if ( m_curveGroups[i].m_curveGroupName == curveGroupName.AsString().AsChar() )
		{
			return true;
		}
	}
	return false;
}

CCurveEditionWrapper* CEdCurveEditorCanvas::AddCurve( CCurve* curve, const String& curveName, const Vector &curveScale, const Color& color ) 
{ 
	return AddCurve( &curve->GetCurveData(), curveName, curveScale, curve, color ); 
}

CCurveEditionWrapper* CEdCurveEditorCanvas::AddCurve( SCurveData* curve, const String& curveName, const Vector &curveScale /*= Vector(0,1.0,0)*/, CObject* curveContainer /*= NULL*/, const Color& color /*= Color( 255, 255, 255 )*/)
{
	if ( IsCurveGroupAdded ( CName( curveName ) ) )
	{
		LOG_EDITOR( TXT("Curve %s already in Curve Editor"), curveName.AsChar() );
		return NULL;
	}

	if ( curveContainer )
	{
		curveContainer->AddToRootSet();
		m_curveContainers.PushBack( curveContainer );
	}

	CurveInfo info;
	info.m_curve = new CCurveEditionWrapper( *curve, color );
	info.m_showTangents = true;
	info.m_showControlPoints = true;

	CurveGroupInfo groupinfo( curveName );
	groupinfo.m_curveScale = curveScale;
	groupinfo.m_curveInfos.PushBack( info );
	m_curveGroups.PushBack( groupinfo );

	if ( curve->Size() == 0 )
	{
		info.m_curve->AddControlPoint( 0.0f, 1.0f );
	}
	
	return info.m_curve;
}

void CEdCurveEditorCanvas::RemoveCurve( SCurveData* curve )
{
	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		for( Uint32 j = 0; j < m_curveGroups[i].m_curveInfos.Size(); ++j )
		{
			if ( &m_curveGroups[i].m_curveInfos[j].m_curve->GetCurveData() == curve )
			{
				CCurveEditionWrapper* toDelete = m_curveGroups[i].m_curveInfos[j].m_curve;
				m_curveGroups[i].m_curveInfos.Erase( m_curveGroups[i].m_curveInfos.Begin() + j );
				if ( m_curveGroups[i].m_curveInfos.Empty() )
				{
					m_curveGroups.Erase( m_curveGroups.Begin() + i );
				}
				m_controlPointInfosSelected.Clear();
				delete toDelete;
				return;
			}
		}
	}
}

TDynArray< CCurveEditionWrapper* > CEdCurveEditorCanvas::AddCurveGroup( CurveParameter* curve, const Color& bckgColor, const Vector &curveScale /*= Vector(0,1.0,0)*/, const Color& color /*= Color( 255, 255, 255 )*/ )
{
	TDynArray< CCurveEditionWrapper* > added;
	if ( IsCurveGroupAdded ( curve->GetName() ) )
	{
		LOG_EDITOR( TXT("Curve Group %s already in Curve Editor"), curve->GetName().AsString().AsChar() );
		return added;
	}

	CurveGroupInfo groupinfo( curve->GetName().AsString() );
	groupinfo.m_curveScale = curveScale;
	groupinfo.SetColor( bckgColor );	

	for( Uint32 i = 0; i < curve->GetCurveCount(); ++i )
	{
		CurveInfo info;
		curve->GetCurve( i )->AddToRootSet();
		m_curveContainers.PushBack( curve->GetCurve( i ) );
		info.m_curve = new CCurveEditionWrapper( curve->GetCurve( i )->GetCurveData(), color );
		added.PushBack( info.m_curve );
		info.m_showTangents = true;
		info.m_showControlPoints = true;
		groupinfo.m_curveInfos.PushBack( info );

		if ( info.m_curve->GetControlPoints().Size() == 0 )
		{
			info.m_curve->AddControlPoint( 0.0f, 1.0f );
		}
	}

	m_curveGroups.PushBack( groupinfo );
	return added;
}

TDynArray< CCurveEditionWrapper* > CEdCurveEditorCanvas::AddCurveGroup( CurveParameter* curve, const String& moduleName, const Color& bckgColor, Bool pinned /* = false */, const Vector &curveScale /*= Vector(0,1.0,0)*/, const Color& color /*= Color( 255, 255, 255 )*/ )
{
	TDynArray< CCurveEditionWrapper* > added;
	CName nameOfModule = CName( moduleName );

	if ( IsCurveGroupAdded ( nameOfModule ) )
	{
		LOG_EDITOR( TXT("Curve Group %s already in Curve Editor"), moduleName.AsChar() );
		return added;
	}

	CurveGroupInfo groupinfo( nameOfModule.AsString() );
	groupinfo.m_curveScale = curveScale;
	groupinfo.SetColor( bckgColor );	
	groupinfo.m_pinned = pinned;

	for( Uint32 i = 0; i < curve->GetCurveCount(); ++i )
	{
		CurveInfo info;
		curve->GetCurve( i )->AddToRootSet();
		m_curveContainers.PushBack( curve->GetCurve( i ) );
		info.m_curve = new CCurveEditionWrapper( curve->GetCurve( i )->GetCurveData(), color );
		added.PushBack( info.m_curve );
		info.m_showTangents = true;
		info.m_showControlPoints = true;
		groupinfo.m_curveInfos.PushBack( info );

		if ( info.m_curve->GetControlPoints().Size() == 0 )
		{
			info.m_curve->AddControlPoint( 0.0f, 1.0f );
		}
	}
	m_curveGroups.PushBack( groupinfo );
	return added;
}

void CEdCurveEditorCanvas::RemoveCurveGroup( const CName& curveGroupName )
{
	String curveGroup = curveGroupName.AsString();

	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		if ( m_curveGroups[i].m_curveGroupName == curveGroup )
		{
			m_curveGroups.Remove( m_curveGroups[i] );
			m_controlPointInfosSelected.Clear();
			return;
		}
	}

	LOG_EDITOR( TXT("Curve Group %s is not in Curve Editor"), curveGroup.AsChar() );
}

void CEdCurveEditorCanvas::RemoveAllCurveGroups( )
{
	// if the name is not in the pinned curve groups list we remove it.

	Bool foundStr = true;

	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		if( ! m_curveGroups[i].m_pinned )
		{
			m_curveGroups.Remove( m_curveGroups[i] );
			m_controlPointInfosSelected.Clear();
		}
	}
}

void CEdCurveEditorCanvas::RemoveAllCurves()
{
	m_controlPointInfosSelected.Clear();

	Int32 size = m_curveGroups.SizeInt();

	for( Int32 i = size-1; i >= 0; --i )
	{
		m_curveGroups.Erase( m_curveGroups.Begin() + i );		
	}
}

void CEdCurveEditorCanvas::DrawControlPoints( const TDynArray< ControlPointInfo >& controlPoints, const wxColour& borderColor, const wxColour &baseColor, Bool drawControlPoints, Bool drawTangents )
{
	for( Uint32 i = 0; i < controlPoints.Size(); ++i )
	{
		CCurveEditionWrapper::ControlPoint* controlPoint = controlPoints[i].m_controlPoint;
		Int32 tangetnIndex = controlPoints[i].m_controlTangentIndex;

		Vector controlPointPosition( controlPoints[i].m_curveGroupInfo->GetTimeAdjusted(controlPoint->GetTime()) , controlPoint->GetValue(), 0.0f );
		if ( drawTangents )
		{
			Vector tangentPosition[2];
			for ( Int32 i = 0; i < 2; ++i )
			{
				if ( controlPoint->GetTangentType( i ) == CST_Bezier ||
					 controlPoint->GetTangentType( i ) == CST_BezierSmooth ||
					 controlPoint->GetTangentType( i ) == CST_BezierSmoothSymertric ||
					 controlPoint->GetTangentType( i ) == CST_BezierSmoothLyzwiaz ||
					 controlPoint->GetTangentType( i ) == CST_Bezier2D )
				{
					tangentPosition[i] = controlPointPosition + controlPoint->GetTangentValue(i);
					CEdCanvas::DrawLine( CurveToCanvas( tangentPosition[i] ), CurveToCanvas( controlPointPosition ), borderColor );
					DrawTangentPoint( CurveToCanvas( tangentPosition[i] ), borderColor, baseColor );
				}
			}
		}
		if ( drawControlPoints )
		{
			DrawControlPoint( CurveToCanvas( controlPointPosition ), borderColor, baseColor );
		}
	}
}

void CEdCurveEditorCanvas::DrawSampledCurve( const CurveGroupInfo *curveGroupInfo, CCurveEditionWrapper* curve, const Int32 numSamples, wxColour color )
{
	Float sampleStep = ( curve->GetCurveData().GetMaxTime() - curve->GetCurveData().GetMinTime() ) / (numSamples - 1);
	wxPoint* points = (wxPoint*) RED_ALLOCA( numSamples * sizeof(wxPoint) );
	for( Int32 i = 0; i < numSamples; ++i )
	{
		Float time = m_activeRegionStart + i*sampleStep;
		wxPoint p = CurveToCanvas( Vector( curveGroupInfo->GetTimeAdjusted(time), curve->GetValue( time ), 0.0f ) ); 
		points[i] = p;
	}
	CEdCanvas::DrawLines( points, numSamples, color, 3.0f );
}

void CEdCurveEditorCanvas::ControlPointsFromCurve( CurveGroupInfo* curveGroupInfo, Int32 curveGroupIndex, Bool createPoints, Bool createTangents, TDynArray< ControlPointInfo >& outControlPoints ) const
{
	const TDynArray< CCurveEditionWrapper::ControlPoint* >& curveControlPoints = curveGroupInfo->m_curveInfos[curveGroupIndex].m_curve->GetControlPoints();
	for ( Uint32 i = 0; i < curveControlPoints.Size(); ++i )
	{
		if ( createPoints )
		{
			outControlPoints.PushBack( ControlPointInfo( curveGroupInfo, curveGroupIndex, curveControlPoints[i], -1 ) );
		}
		if ( createTangents )
		{
			outControlPoints.PushBack( ControlPointInfo( curveGroupInfo, curveGroupIndex, curveControlPoints[i], 0 ) );
			outControlPoints.PushBack( ControlPointInfo( curveGroupInfo, curveGroupIndex, curveControlPoints[i], 1 ) );
		}
	}
}

Bool CEdCurveEditorCanvas::ControlPointsNearPoint( TDynArray< ControlPointInfo >& controlPoints, const wxPoint& clientPoint, const Int32 radiusSqr, ControlPointInfo& outControlPoint ) const
{
	for ( Uint32 i = 0; i < controlPoints.Size(); ++i )
	{
		// Checking distance in canvas space
		wxPoint mousePositionCanvas  = ClientToCanvas( clientPoint );
		wxPoint controlPointPosition = CurveToCanvas( controlPoints[i].GetPosition() );
		wxPoint mouseDistance = controlPointPosition - mousePositionCanvas;
		if ( mouseDistance.x*mouseDistance.x + mouseDistance.y*mouseDistance.y < radiusSqr )
		{
			outControlPoint = controlPoints[i];
			return true;
		}
	}
	return false;
}

Int32 CEdCurveEditorCanvas::ControlPointsInArea( TDynArray< ControlPointInfo >& controlPoints, const wxRect& canvasRect, TDynArray< ControlPointInfo >& outControlPoints ) const
{
	Int32 controlPointsAdded = 0;
	for ( Uint32 i = 0; i < controlPoints.Size(); ++i )
	{
		// Checking if controlPoint in canvas rect
		if ( canvasRect.Contains( CurveToCanvas( controlPoints[i].GetPosition() ) ) )
		{
			if ( outControlPoints.PushBackUnique( controlPoints[i] ) == true )
			{
				controlPointsAdded++;
			}
		}
	}
	return controlPointsAdded;
}

void CEdCurveEditorCanvas::ControlPointsMove( TDynArray< ControlPointInfo >& controlPoints, Vector& delta )
{
	ASSERT( controlPoints.Size() > 0 );

	// Move control points
	TDynArray< CurveGroupInfo* > curvesGroupsToUpdate;

	// Iterate over all selected control points
	for ( Uint32 i = 0; i < controlPoints.Size(); ++i )
	{
		ControlPointInfo& cp = controlPoints[i];
		Int32 tangentIndex = cp.m_controlTangentIndex;
		delta.X = cp.m_curveGroupInfo->GetTimeAdjustedRevertScaleOnly( delta.X );
		if ( tangentIndex == -1 ) 
		{
			CCurveEditionWrapper::ControlPoint* point = cp.GetControlPoint();
			SCurveValueChanged eventData( this, point->GetTime(), point->GetTime(), point->GetValue(), point->GetValue(), &point->GetCurve()->GetCurveData() );

			// Set position for the nontangent control point
			Float newTime = point->GetTime() + delta.X;//Clamp( point->GetTime() + delta.X, m_activeRegionStart, m_activeRegionEnd );
			cp.GetControlPoint()->SetTime( newTime );
			cp.m_controlPoint->SetValue( point->GetValue() + delta.Y );

			eventData.m_currentVal = point->GetValue();
			eventData.m_currentTime = point->GetTime();
			SEvents::GetInstance().DispatchEvent( CNAME( CurvePointChanged ), CreateEventData( eventData ) );

			// Add curve to resort list
			Int32 curveGroupInfoIndex = cp.GetCurveGroupInfoIndex();
			curvesGroupsToUpdate.PushBackUnique( controlPoints[i].GetCurveGroupInfo() );
		}
		else
		{
			// Set tangent control point position
			Vector newTangentValue = cp.GetControlPoint()->GetTangentValue( tangentIndex ) + delta;
			cp.GetControlPoint()->SetTangentValue( tangentIndex, newTangentValue );
		}
	}

	// Resort control points in curves
	for ( Uint32 i = 0; i < curvesGroupsToUpdate.Size(); ++i )
	{
		for ( Uint32 j = 0; j < curvesGroupsToUpdate[i]->GetCurveInfos().Size(); ++j )
		{
			curvesGroupsToUpdate[i]->GetCurveInfos()[j].GetCurve()->SortControlPoints();
		}
	}

	ControlPointsChanged();
	SelectionChanged();
}

void CEdCurveEditorCanvas::MoveSelectedControlPoints( const Float time, const Float value, Bool updateTime, Bool updateValue, Bool absoluteMove )
{
	if ( absoluteMove )
	{
		TDynArray< CurveGroupInfo* > curvesGroupsToUpdate;
		for ( Uint32 i = 0; i < m_controlPointInfosSelected.Size(); ++i )
		{
			CCurveEditionWrapper::ControlPoint* point = m_controlPointInfosSelected[i].GetControlPoint();
			SCurveValueChanged eventData( this, point->GetTime(), point->GetTime(), point->GetValue(), point->GetValue(), &point->GetCurve()->GetCurveData() );
			if ( updateTime )
			{
				// Add curve to resort list
				curvesGroupsToUpdate.PushBackUnique( m_controlPointInfosSelected[i].GetCurveGroupInfo() );
				m_controlPointInfosSelected[i].GetControlPoint()->SetTime( time );
				curvesGroupsToUpdate.PushBackUnique( m_controlPointInfosSelected[i].GetCurveGroupInfo() );
			}
			if ( updateValue )
			{
				m_controlPointInfosSelected[i].GetControlPoint()->SetValue( value );
			}
			eventData.m_currentVal = point->GetValue();
			eventData.m_currentTime = point->GetTime();
			SEvents::GetInstance().DispatchEvent( CNAME( CurvePointChanged ), CreateEventData( eventData ) );
		}

		// Resort control points in curves
		for ( Uint32 i = 0; i < curvesGroupsToUpdate.Size(); ++i )
		{
			for ( Uint32 j = 0; j < curvesGroupsToUpdate[i]->GetCurveInfos().Size(); ++j )
			{
				curvesGroupsToUpdate[i]->GetCurveInfos()[j].GetCurve()->SortControlPoints();
			}
		}
		ControlPointsChanged();
		SelectionChanged();
	}
	else
	{
		Vector delta( updateTime ? time : 0.0f, updateValue ? value : 0.0f, 0.0f );
		ControlPointsMove( m_controlPointInfosSelected, delta );
	}

	ControlPointsChangedComplete();
}

void CEdCurveEditorCanvas::PaintCanvas( Int32 width, Int32 height )
{
	// Colors
	wxColour back = GetCanvasColor();
	static wxColour text( 255, 255, 255 );
	static wxColour texth( 255, 255, 0 );
	static wxColour highlight( 0, 0, 80 );

	static wxColour colorSelecting( 0, 0, 255 );
	static wxColour colorZooming( 0, 255, 0 );

	// Paint background
	Clear( back );

	// Paint grid and snapping lines (lines)
	DrawGrid( width, height, true, false );
	DrawSnapLines( true, false );

	// Draw (0,0)
	{
		String string( TXT("(0,0)") );
		wxPoint stringSize = TextExtents( GetGdiDrawFont(), string );
		CEdCanvas::DrawText( wxPoint( -stringSize.x, 0 ), GetGdiDrawFont(), string, text );
	}

	// When adding the curve into the curveinfogroup we need to engrave the color
	// Draw
	for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
	{
		TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
		Uint32 count = 0;

		for ( Uint32 i = 0; i < curveInfos.Size(); ++i )
		{
			// We set this to default White
			wxColour color = wxColour(255,255,255);
			Color colorToSet = Color(255,255,255);
			// If size is only one we, set it to white else we add R G B
			if( curveInfos.Size() == 1 )
			{
				color = wxColour(255,255,255);
				colorToSet = Color(255,255,255);
				curveInfos[i].GetCurve()->SetColor( colorToSet );
			}
			else if(count == 0) // Red
			{
				color = wxColour(210,20,20);
				colorToSet = Color(210,20,20);
				curveInfos[i].GetCurve()->SetColor( colorToSet );
			}
			else if(count == 1) // Green
			{
				color = wxColour(20,210,20);
				colorToSet = Color(20,210,20);
				curveInfos[i].GetCurve()->SetColor( colorToSet );
			}
			else if(count == 2) // Blue
			{
				color = wxColour(20,20,210);
				colorToSet = Color(20,20,210);
				curveInfos[i].GetCurve()->SetColor( colorToSet );
			}
			count++;

			Int32 numSamples = 255;
			if ( curveInfos[i].m_showApproximated ) 
			{
				numSamples = CurveApproximation::NUM_SAMPLES;
			}
			DrawSampledCurve( &m_curveGroups[j], curveInfos[i].m_curve, numSamples, color );

			TDynArray< ControlPointInfo > controlPoints;
			ControlPointsFromCurve( &m_curveGroups[j], i, true, true, controlPoints );
			if ( curveInfos[i].m_showControlPoints && m_showControlPoints )
			{
				DrawControlPoints( controlPoints, wxColour(0,0,0), color, true, false );
			}
			if ( curveInfos[i].m_showTangents && m_showTangents )
			{
				DrawControlPoints( controlPoints, wxColour(0,0,0), color, false, true );
			}
		}
	}
	{
		// Draw mouse over control points
		DrawControlPoints( m_controlPointInfosMouseOver, wxColour(0,0,0), wxColour(255,255,0), true, true );
	
		// Draw selected points
		DrawControlPoints( m_controlPointInfosSelected, wxColour(0,0,0), wxColour(255,255,0), true, true );
	}

	// Draw selection rect
	if ( m_action == MA_SelectingWindows )
	{
		// Assemble selection rect
		wxRect selectionRect = m_selectionRegion.GetRect();
		DrawRect( selectionRect, RIM_IS_KEY_DOWN( IK_LShift ) ? colorZooming : colorSelecting );
	}

	// Paint grid and snapping lines (text)
	DrawGrid( width, height, false, true );
	DrawSnapLines( false, true );

	// Draw side panel background
	wxRect sidePanelRect( 0, 0, GetSidePanelWidth(), GetSidePanelHeight() );
	FillRect( ClientToCanvas( sidePanelRect ), back );

	// Draw curve infos
	wxPoint position( 0, 0 );

	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		Bool selected = false;
		Bool mouseOver = false;
		for ( Uint32 j = 0; j < m_controlPointInfosSelected.Size(); ++j )
		{
			if ( m_controlPointInfosSelected[j].m_curveGroupInfo == &m_curveGroups[i] )
			{
				selected = true;
			}
		}
		for ( Uint32 j = 0; j < m_controlPointInfosMouseOver.Size(); ++j )
		{
			if ( m_controlPointInfosMouseOver[j].m_curveGroupInfo == &m_curveGroups[i] )
			{
				mouseOver = true;
			}
		}
		wxColour borderColor = selected ? wxColour( 0, 0, 255 ) : ( mouseOver ? wxColour(255,255,0) : wxColour( 0, 0, 0 ) );
		m_curveGroups[i].Draw( this, position, borderColor );
		position.y += m_curveGroups[i].GetClientRect( position ).GetHeight();
	}
}

void CEdCurveEditorCanvas::OnMouseLeftDoubleClick( wxMouseEvent &event )
{
	wxPoint mousePosition = event.GetPosition();
	if ( m_action == MA_None && IsMouseOverCanvasPanel( mousePosition ) )
	{
		for ( Uint32 j = 0; j < m_curveGroups.Size(); ++j )
		{
			TDynArray< CurveInfo >& curveInfos = m_curveGroups[j].GetCurveInfos();
			for( Uint32 i = 0; i < curveInfos.Size(); ++i )
			{
				wxPoint point = event.GetPosition();
				Float curveValue = curveInfos[i].GetCurve()->GetValue( ClientToCurve( point ).X );
				if ( abs( CurveToCanvas( Vector( 0.0f, curveValue, 0.0f) ).y - ClientToCanvas( point ).y ) < 10 )
				{
					Float time = ClientToCurve( point ).X;

					curveInfos[i].GetCurve()->AddControlPoint( m_curveGroups[j].GetTimeAdjustedRevert(time), curveInfos[i].GetCurve()->GetValue( time ) );

					SCurvePointAdded eventData( this, m_curveGroups[j].GetTimeAdjustedRevert(time), &curveInfos[i].GetCurve()->GetCurveData() );
					SEvents::GetInstance().DispatchEvent( CNAME( CurvePointAdded ), CreateEventData( eventData ) );

					Repaint();
					return;
				}
			}
		}
	}
}

#define ID_CREATE_BLOCK_CLASS_FIRST		1000

class CurveControlPointInfoWrapper : public wxObject
{
public:
	CEdCurveEditorCanvas::ControlPointInfo	m_controlPointInfo;
	ECurveSegmentType		m_newCurveType;

public:
	CurveControlPointInfoWrapper( CEdCurveEditorCanvas::ControlPointInfo controlPointInfo, ECurveSegmentType newCurveType )
		: m_controlPointInfo( controlPointInfo )
		, m_newCurveType( newCurveType )
	{};
};

void CEdCurveEditorCanvas::OpenControlPointContextMenu()
{
	ASSERT( m_controlPointInfosMouseOver.Size() > 0 );
	ControlPointInfo cp = m_controlPointInfosMouseOver[0];

	TDynArray< TPair< String, ECurveSegmentType > > mapping;
	mapping.PushBack( MakePair( String( TXT("Constant") ), CST_Constant ) );
	mapping.PushBack( MakePair( String( TXT("Interpolate") ), CST_Interpolate ) );
	mapping.PushBack( MakePair( String( TXT("Bezier") ), CST_Bezier ) );
	mapping.PushBack( MakePair( String( TXT("Bezier Smooth") ), CST_BezierSmooth ) );
	mapping.PushBack( MakePair( String( TXT("Bezier Smooth Symertric") ), CST_BezierSmoothSymertric ) );
	mapping.PushBack( MakePair( String( TXT("Bezier Smooth Lyzwiaz") ), CST_BezierSmoothLyzwiaz ) );
	
	// Don't do it. Bezier 2d is too heavy for "normal" users
	//mapping.PushBack( MakePair( String( TXT("Bezier2D") ), CST_Bezier2D ) );

	wxMenu menu;
	menu.Append( wxID_ANY, ( cp.m_controlTangentIndex == -1 ) ? TXT("In/Out") : ( ( cp.m_controlTangentIndex == 0 ) ? TXT("In") : TXT("Out") ), wxEmptyString );
	menu.AppendSeparator();

	for( Uint32 i = 0; i < mapping.Size(); ++i )
	{
		menu.Append( ID_CREATE_BLOCK_CLASS_FIRST + i, mapping[i].m_first.AsChar() );
		menu.Connect( ID_CREATE_BLOCK_CLASS_FIRST + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdCurveEditorCanvas::OnSetTangentType ), new CurveControlPointInfoWrapper( cp, mapping[i].m_second ), this );
	}

	// Show menu
	PopupMenu( &menu );

	// clear mouse over array because, we don't move that points
	m_controlPointInfosMouseOver.Clear();
}


void CEdCurveEditorCanvas::OnSetTangentType( wxCommandEvent& event )
{
	CurveControlPointInfoWrapper* wrapper = ( CurveControlPointInfoWrapper* ) event.m_callbackUserData;

	ControlPointInfo& controlPointInfo = (ControlPointInfo) wrapper->m_controlPointInfo;
	ECurveSegmentType& newCurveType = (ECurveSegmentType) wrapper->m_newCurveType;
	
	Int32 tangentIndex = controlPointInfo.m_controlTangentIndex;
	if ( tangentIndex != -1 )
	{
		controlPointInfo.m_controlPoint->SetTangentType( tangentIndex, newCurveType );
	}
	else
	{
		controlPointInfo.m_controlPoint->SetTangentType( 0, newCurveType );
		controlPointInfo.m_controlPoint->SetTangentType( 1, newCurveType );
	}
}

Bool CEdCurveEditorCanvas::UpdateCurveParam( SCurveData* curve, const Vector &curveScale )
{
	for ( TDynArray< CurveGroupInfo >::iterator curveGroupInfo = m_curveGroups.Begin();
		  curveGroupInfo != m_curveGroups.End();
		  ++curveGroupInfo )
	{
		for ( TDynArray< CurveInfo >::iterator curveInfo = curveGroupInfo->m_curveInfos.Begin();
			  curveInfo != curveGroupInfo->m_curveInfos.End();
			  ++curveInfo )
		{
			if ( &curveInfo->m_curve->GetCurveData() == curve )
			{
				curveGroupInfo->m_curveScale = curveScale;
				return true;
			}
		}
	}
	return false;
}

Bool CEdCurveEditorCanvas::IsCurveAdded( const SCurveData* curve ) const
{
	for( Uint32 i = 0; i < m_curveGroups.Size(); ++i )
	{
		for( Uint32 j = 0; j < m_curveGroups[i].m_curveInfos.Size(); ++j )
		{
			if ( &m_curveGroups[i].m_curveInfos[j].m_curve->GetCurveData() == curve )
			{
				return true;
			}
		}
	}
	return false;
}

void CEdCurveEditorCanvas::GetSelectedCurves( TDynArray< CCurveEditionWrapper* >& curves )
{
	curves.Clear();
	for( Uint32 i = 0; i < m_controlPointInfosSelected.Size(); ++i )
	{
		if ( m_controlPointInfosSelected[i].GetCurve() )
		{
			curves.PushBackUnique( m_controlPointInfosSelected[i].GetCurve() );
		}
	}
}
