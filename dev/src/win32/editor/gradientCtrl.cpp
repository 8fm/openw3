/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

BEGIN_EVENT_TABLE( CEdGradientPicker, wxControl )
	EVT_SIZE( CEdGradientPicker::OnSize )
	EVT_PAINT( CEdGradientPicker::OnPaint )
	EVT_MOTION( CEdGradientPicker::OnMouseMove )
	EVT_LEFT_DOWN( CEdGradientPicker::OnMouseClick )
	EVT_LEFT_UP( CEdGradientPicker::OnMouseClick )
	EVT_RIGHT_DOWN( CEdGradientPicker::OnMouseClick )
	EVT_RIGHT_UP( CEdGradientPicker::OnMouseClick )
	EVT_KEY_DOWN( CEdGradientPicker::OnKeyDown )
	EVT_LEFT_DCLICK( CEdGradientPicker::OnMouseLeftDoubleClick )
	EVT_ERASE_BACKGROUND( CEdGradientPicker::OnEraseBackground )
END_EVENT_TABLE()

CEdGradientPicker::CEdGradientPicker()
	: m_backImage( 1, 1, true )
	, m_dragMode( DM_None )
	, m_updateMode( UM_Both )
	, m_displayMode( DM_Both )
	, m_ctrlColorPicker( NULL )
{
	m_curve[0] = m_curve[1] = m_curve[2] = m_curve[3] = NULL;

	m_ctrlColorPicker = new CEdColorPicker( this );
	m_ctrlColorPicker->Bind( wxEVT_COMMAND_SCROLLBAR_UPDATED, &CEdGradientPicker::OnColorPicked, this );

}

CEdGradientPicker::~CEdGradientPicker()
{
	if ( m_curve[0] ) delete m_curve[0];
	if ( m_curve[1] ) delete m_curve[1];
	if ( m_curve[2] ) delete m_curve[2];
	if ( m_curve[3] ) delete m_curve[3];

	if ( m_ctrlColorPicker )
	{
		delete m_ctrlColorPicker;
	}
}
void CEdGradientPicker::SetCurves( SCurveData* curveR, SCurveData* curveG, SCurveData* curveB, SCurveData* curveA )
{
	if ( m_curve[0] ) delete m_curve[0];
	if ( m_curve[1] ) delete m_curve[1];
	if ( m_curve[2] ) delete m_curve[2];
	if ( m_curve[3] ) delete m_curve[3];
	m_curve[0] = new CCurveEditionWrapper( *curveR );
	m_curve[1] = new CCurveEditionWrapper( *curveG );
	m_curve[2] = new CCurveEditionWrapper( *curveB );
	m_curve[3] = new CCurveEditionWrapper( *curveA );

	// Force refresh when come curves changed
	Refresh();
}

void CEdGradientPicker::OnSize( wxSizeEvent& event )
{
	// Resize image for new size
	wxSize size = GetSize();
	m_backImage.Resize( size, wxPoint(0,0) );

	// Force refresh when size of control changed
	Refresh();
}

void CEdGradientPicker::OnPaint( wxPaintEvent& event )
{
	ASSERT( m_curve[0] && m_curve[1] && m_curve[2] && m_curve[3] );

	// Draw gradient
	DrawGradient( m_backImage );

	// Draw tickers
	DrawTickers( m_backImage );

	// Draw selected tickers
	for ( Uint32 i = 0; i < m_controlPointsSelected.Size(); ++i )
	{
		Float fraction = m_controlPointsSelected[i].GetTime();
		DrawCursor( m_backImage, fraction, Color::BLUE );
	}

	// Draw mouse over tickers

	// Create a bitmap from the color data and blit it
	wxBitmap blitBitmap( m_backImage, 24 ); 
	wxBufferedPaintDC dc( this );
	dc.DrawBitmap( blitBitmap, 0, 0 );
}


void CEdGradientPicker::DrawGradient( wxImage &image )
{
	// Get image size
	Int32 width = image.GetWidth();
	Int32 height = image.GetHeight();
	ASSERT( height > GRADIENT_TICKER_HEIGHT );

	// Draw gradient into image
	for ( Int32 x = 0; x < width; ++x )
	{
		Uint8 r, g, b, a = 255;
		const Float fraction = (Float) x / (width-1);
		if ( m_displayMode == DM_Color )
		{
			r = Clamp( (Int32) (m_curve[0]->GetValue( fraction ) * 255.0f), 0, 255 );
			g = Clamp( (Int32) (m_curve[1]->GetValue( fraction ) * 255.0f), 0, 255 );
			b = Clamp( (Int32) (m_curve[2]->GetValue( fraction ) * 255.0f), 0, 255 );
		}
		else if ( m_displayMode == DM_Alpha )
		{
			r = g = b = Clamp( (Int32) (m_curve[3]->GetValue( fraction ) * 255.0f), 0, 255 );
		}
		else if ( m_displayMode == DM_Both )
		{
			r = Clamp( (Int32) (m_curve[0]->GetValue( fraction ) * 255.0f), 0, 255 );
			g = Clamp( (Int32) (m_curve[1]->GetValue( fraction ) * 255.0f), 0, 255 );
			b = Clamp( (Int32) (m_curve[2]->GetValue( fraction ) * 255.0f), 0, 255 );
			a = Clamp( (Int32) (m_curve[3]->GetValue( fraction ) * 255.0f), 0, 255 );
		}

		// Left GRADIENT_TICKER_HEIGHT lines for tickers
		for ( Int32 y = GRADIENT_TICKER_HEIGHT; y < height; ++y )
		{
			Uint32 back = ( ( (x/GRADIENT_GRID_SIZE)%2 + (y/GRADIENT_GRID_SIZE)%2 )%2 == 0 ) ? 255 : 128;
			Uint8 rr = r*(a/255.0f) + back*(1.0f-(a/255.0f));
			Uint8 gg = g*(a/255.0f) + back*(1.0f-(a/255.0f));
			Uint8 bb = b*(a/255.0f) + back*(1.0f-(a/255.0f));
			image.SetRGB( x, y, rr, gg, bb );
		}
	}
}

void CEdGradientPicker::DrawTickers( wxImage &image )
{
	// Get image size
	Int32 width = image.GetWidth();
	Int32 height = image.GetHeight();
	ASSERT( height > GRADIENT_TICKER_HEIGHT );

	// Clear background for tickers - first GRADIENT_TICKER_HEIGHT lines
	Red::System::MemorySet( image.GetData(), 0xFF, 3*image.GetWidth()*GRADIENT_TICKER_HEIGHT );

	// Draw all tickers
	TDynArray< ControlPointInfo > controlPoints;
	ControlPointsFromCurves( controlPoints );
	for ( Uint32 i = 0; i < controlPoints.Size(); ++i )
	{
		Float fraction = controlPoints[i].GetTime();
		DrawCursor( image, fraction, Color::RED );
	}
}

Bool CEdGradientPicker::CheckAlphaToColor()
{
	const SCurveData& data = m_curve[0]->GetCurveData();

	// Check length of alpha curve
	Bool controlPointsMatch = (m_curve[3]->GetCurveData().Size() == data.Size());

	// Check RGB and alpha curves
	for( Uint32 i = 0; i < data.Size(); ++i )
	{
		// Make sure that RGB control points match
		for( Uint32 index = 1; index < 3; ++index )
		{
			ASSERT( m_curve[index]->GetCurveData().Size() == data.Size() );
			ASSERT( m_curve[index]->GetCurveData().GetTimeAtIndex(i) == data.GetTimeAtIndex(i) );
		}

		// Check if alpha control points match
		if ( controlPointsMatch == true )
		{
			if ( m_curve[3]->GetCurveData().GetTimeAtIndex(i) != data.GetTimeAtIndex(i) )
			{
				controlPointsMatch = false;
			}
		}
	}

	return controlPointsMatch;
}

void CEdGradientPicker::ForceAlphaToColor()
{
	if ( m_curve[0] == NULL )
	{
		return;
	}

	// Check if alpha match color
	Bool controlPointsMatch = CheckAlphaToColor();

	const TDynArray< CCurveEditionWrapper::ControlPoint* >& controlPoints = m_curve[0]->GetControlPoints();

	// If don't match, clear curve and add new control points that match
	if ( controlPointsMatch == false )
	{
		// Make a copy of the basic curve data. We don't need what CCurve adds on top, so this is fine.
		SCurveData oldAlphaCurve = m_curve[3]->GetCurveData();
		m_curve[3]->Clear();
		for( Uint32 i = 0; i < controlPoints.Size(); ++i )
		{
			const Float time = controlPoints[i]->GetTime();
			m_curve[3]->AddControlPoint( time, oldAlphaCurve.GetFloatValue( time ) );
		}
	}
}

void CEdGradientPicker::ControlPointsFromCurves( TDynArray< ControlPointInfo >& outControlPoints ) const
{
	Uint32 indexStart = 0;
	Uint32 indexCount = 0;
	if ( m_updateMode == UM_Both )
	{
		indexStart = 0;
		indexCount = 4;
	}
	else if ( m_updateMode == UM_Color )
	{
		indexStart = 0;
		indexCount = 3;
	}
	else if ( m_updateMode == UM_Alpha )
	{
		indexStart = 3;
		indexCount = 1;
	}

	Uint32 prototypeControlPointsCount = m_curve[indexStart]->GetControlPoints().Size();

	for( Uint32 i = 0; i < prototypeControlPointsCount; ++i )
	{
		// Create new ControlPointInfo
		ControlPointInfo controlPointInfo;

		// Create prototype control point
		CCurveEditionWrapper::ControlPoint* prototypeControlPoint = m_curve[indexStart]->GetControlPoints()[i];

		for( Uint32 index = indexStart; index < (indexStart+indexCount); ++index )
		{
			ASSERT( m_curve[index]->GetControlPoints().Size() == prototypeControlPointsCount );
			ASSERT( m_curve[index]->GetControlPoints()[i]->GetTime() == prototypeControlPoint->GetTime() );
			controlPointInfo.PushBack( m_curve[index]->GetControlPoints()[i] );
		}
		outControlPoints.PushBack( controlPointInfo );
	}	
}

void CEdGradientPicker::DrawCursor( wxImage &image, const Float fraction, const Color& color )
{
	Int32 width, height;
	GetSize( &width, &height );
	Int32 dx = fraction * (width-1);
	for ( Int32 y = 0; y < GRADIENT_TICKER_HEIGHT; ++y )
	{
		for ( Int32 x = Max( dx - (GRADIENT_TICKER_HEIGHT-y) , 0 ); x < Min( dx + (GRADIENT_TICKER_HEIGHT-y) + 1, width-1 ); ++x )
		{
			image.SetRGB( x, y, color.R, color.G, color.B );
		}
	}
}

void CEdGradientPicker::OnMouseClick( wxMouseEvent& event )
{
	Int32 width, height;
	GetSize( &width, &height );

	if ( event.LeftDown() && m_dragMode == DM_None )
	{
		TDynArray< ControlPointInfo > controlPoints;
		ControlPointsFromCurves( controlPoints );
		for ( Uint32 i = 0; i < controlPoints.Size(); ++i )
		{
			if ( ! event.ShiftDown() )
			{
				m_controlPointsSelected.Clear();
			}
			if ( controlPoints[i].GetClientRect( width ).Contains( event.GetPosition() ) )
			{
				m_controlPointsSelected.PushBackUnique( controlPoints[i] );
				m_dragMode = DM_MoveSelected;
				MouseCapture();
				return;
			}

		}
	}
	else if ( event.LeftUp() && m_dragMode != DM_None )
	{
		m_dragMode = DM_None;
		MouseRelease();
	}
	else if ( event.RightDown() && m_dragMode == DM_None )
	{
		if ( m_controlPointsSelected.Size() > 0 )
		{
			Color color;
			const Float fraction = m_controlPointsSelected[0].GetTime();
			if ( m_displayMode == DM_Color || m_displayMode == DM_Both )
			{
				color.R = Clamp( (Int32) (m_curve[0]->GetValue( fraction ) * 255.0f), 0, 255 );
				color.G = Clamp( (Int32) (m_curve[1]->GetValue( fraction ) * 255.0f), 0, 255 );
				color.B = Clamp( (Int32) (m_curve[2]->GetValue( fraction ) * 255.0f), 0, 255 );
			}
			else if ( m_displayMode == DM_Alpha )
			{
				color.R = color.G = color.B = Clamp( (Int32) (m_curve[3]->GetValue( fraction ) * 255.0f), 0, 255 );
			}
			// LOG_EDITOR( TXT("Fraction: %.2f RGB: %d %d %d"), fraction, color.R, color.G, color.B );
			ColorOpenBrowser( color );
		}
	}

	Refresh();
}

void CEdGradientPicker::OnMouseMove( wxMouseEvent& event )
{
	Int32 width, height;
	GetSize( &width, &height );

	wxPoint delta = event.GetPosition() - m_lastMousePosition;
	m_lastMousePosition = event.GetPosition();

	if ( m_dragMode == DM_MoveSelected )
	{
		for( Uint32 i = 0; i < m_controlPointsSelected.Size(); ++i )
		{
			Float newTime = m_controlPointsSelected[i].GetTime() + (delta.x/((Float)width-1.0f));
			newTime = Clamp( newTime, 0.0f , 1.0f );
			m_controlPointsSelected[i].SetTime( newTime );
		}

		m_curve[0]->SortControlPoints();
		m_curve[1]->SortControlPoints();
		m_curve[2]->SortControlPoints();
		m_curve[3]->SortControlPoints();
	
		ControlPointsChanged();
	}
	SetFocus();
}

void CEdGradientPicker::ColorOpenBrowser( const Color& color )
{
	m_ctrlColorPicker->Show( color );
}

void CEdGradientPicker::OnColorPicked( wxCommandEvent& event )
{
	ASSERT( m_ctrlColorPicker );
	Color color = m_ctrlColorPicker->GetColor();

	for( Uint32 i = 0; i < m_controlPointsSelected.Size(); ++i )
	{
		if ( m_displayMode == UM_Color || m_displayMode == UM_Both )
		{
			m_controlPointsSelected[i].SetValue( 0, color.R / 255.0f );
			m_controlPointsSelected[i].SetValue( 1, color.G / 255.0f );
			m_controlPointsSelected[i].SetValue( 2, color.B / 255.0f );
		}
		else if ( m_displayMode == UM_Alpha )
		{
			if ( m_updateMode == UM_Alpha )
			{
				m_controlPointsSelected[i].SetValue( 0, color.R / 255.0f );
			}
			else
			{
				ASSERT( m_updateMode == UM_Both );
				m_controlPointsSelected[i].SetValue( 3, color.R / 255.0f );
			}
		}
	}

	ControlPointsChanged();
	Refresh();
}

void CEdGradientPicker::OnMouseLeftDoubleClick( wxMouseEvent &event )
{
	Int32 width, height;
	GetSize( &width, &height );

	wxPoint position = event.GetPosition();
	const Float fraction = (Float) position.x / (width-1);

	if ( m_updateMode == UM_Color || m_updateMode == UM_Both )
	{
		m_curve[0]->AddControlPoint( fraction, m_curve[0]->GetValue( fraction ) );
		m_curve[0]->SortControlPoints();
		m_curve[1]->AddControlPoint( fraction, m_curve[1]->GetValue( fraction ) );
		m_curve[1]->SortControlPoints();
		m_curve[2]->AddControlPoint( fraction, m_curve[2]->GetValue( fraction ) );
		m_curve[2]->SortControlPoints();
	}
	if ( m_updateMode == UM_Alpha || m_updateMode == UM_Both )
	{
		m_curve[3]->AddControlPoint( fraction, m_curve[3]->GetValue( fraction ) );
		m_curve[3]->SortControlPoints();
	}

	// Signal that control points changed
	ControlPointsChanged();
}

void CEdGradientPicker::OnKeyDown( wxKeyEvent &event )
{
	if ( event.GetKeyCode() == WXK_DELETE )
	{
		for( Uint32 i = 0; i < m_controlPointsSelected.Size(); ++i )
		{
			TDynArray< CCurveEditionWrapper::ControlPoint* >& controlPoints = m_controlPointsSelected[i].GetControlPoints();
			for ( Uint32 j = 0; j < controlPoints.Size(); ++j )
			{
				m_curve[j]->RemoveControlPoint( controlPoints[j] );
			}
		}

		// Clear selection in both controls
		{
			CEdGradientEditor* editor = static_cast< CEdGradientEditor* >( m_parent->GetParent() );
			ASSERT( editor  );
			editor->OnClearSelection();
		}
		ClearSelection();
		ControlPointsChanged();
	}
	Refresh();
}

void CEdGradientPicker::ControlPointsChanged()
{
	CEdGradientEditor* editor = static_cast< CEdGradientEditor* >( m_parent->GetParent() );
	ASSERT( editor  );
	editor->OnControlPointsChanged();
}

void CEdGradientPicker::MouseCapture()
{
	RECT rect;
	::GetClientRect( (HWND) GetHandle(), &rect );
	POINT topLeft, bottomRight;
	topLeft.x = rect.left;
	topLeft.y = rect.top;
	bottomRight.x = rect.right;
	bottomRight.y = rect.bottom;
	::ClientToScreen( (HWND) GetHandle(), &topLeft );
	::ClientToScreen( (HWND) GetHandle(), &bottomRight );
	rect.left = topLeft.x;
	rect.top = topLeft.y;
	rect.right = bottomRight.x;
	rect.bottom = bottomRight.y;
	::ClipCursor( &rect );
	SetCapture( (HWND)GetHWND() );
}

void CEdGradientPicker::MouseRelease()
{
	ReleaseCapture();
	::ClipCursor( NULL );
}
