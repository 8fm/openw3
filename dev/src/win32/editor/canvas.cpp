/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "canvas.h"

BEGIN_EVENT_TABLE( CEdCanvas,wxWindow )
	EVT_PAINT( CEdCanvas::OnPaint )
	EVT_SIZE( CEdCanvas::OnSize )
	EVT_ERASE_BACKGROUND( CEdCanvas::OnEraseBackground )
	EVT_LEFT_DOWN( CEdCanvas::OnMouseEvent )
	EVT_LEFT_UP( CEdCanvas::OnMouseEvent )
	EVT_RIGHT_DOWN( CEdCanvas::OnMouseEvent )
	EVT_RIGHT_UP( CEdCanvas::OnMouseEvent )
	EVT_MOTION( CEdCanvas::OnMouseMove )
END_EVENT_TABLE()

CEdCanvas::CEdCanvas( wxWindow *parent, long style, CanvasType canvasType )
	: wxWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style )
	, CDropTarget( this )
	, m_bufferDevice( NULL )
	, m_bitmapBuffer( NULL )
	, m_paintCached( false )
	, m_lastWidth( -1 )
	, m_lastHeight( -1 )
	, m_isMouseCaptured( false )
	, m_isHardCaptured( false )
	, m_scale( 1.0f )
	, m_invScale( 1.0f )
	, m_offset( 0,0 )
	, m_bitmap( nullptr )
	, m_memDC( nullptr )
	, m_canvasType( canvasType )
{
	SetBackgroundStyle( wxBG_STYLE_PAINT );

	RecreateDevice( GetClientSize().x, GetClientSize().y );

	// draw font
	m_wxDrawFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
	m_drawFont = new Gdiplus::Font( ::GetDC( (HWND) this->GetHWND() ), (HFONT) m_wxDrawFont.GetHFONT() );
	
	// bold font
	m_wxBoldFont = m_wxDrawFont;
	m_wxBoldFont.SetWeight( wxFONTWEIGHT_BOLD );
	m_boldFont = new Gdiplus::Font( ::GetDC( (HWND) this->GetHWND() ), (HFONT) m_wxBoldFont.GetHFONT() );

	// tooltip font
	m_wxTooltipFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial");
	m_tooltipFont = new Gdiplus::Font( ::GetDC( (HWND) this->GetHWND() ), (HFONT) m_wxTooltipFont.GetHFONT() );
}

CEdCanvas::~CEdCanvas()
{
	// Delete fonts
	if ( m_drawFont )
	{
		delete m_drawFont;	
		m_drawFont = NULL;
	}
	
	if ( m_boldFont )
	{
		delete m_boldFont;
		m_boldFont = NULL;
	}

	delete m_tooltipFont;

	// Delete bitmap buffer
	if ( m_bitmapBuffer )
	{
		delete m_bitmapBuffer;
		m_bitmapBuffer = NULL;
	}

	// Delete buffer device
	if ( m_bufferDevice )
	{
		delete m_bufferDevice;
		m_bufferDevice = NULL;
	}

	delete m_bitmap;
}

void CEdCanvas::Repaint( Bool immediate )
{
	// Invalidate canvas area, it will be repainted soon
	Refresh( false );

	// Update now
	if ( immediate )
	{
		Update();
	}
}

void CEdCanvas::CaptureMouse( Bool state, Bool hard )
{
	/* Changing this capture code because it creates many conflicts with the
	 * game and other instances of the canvas that cause the mouse cursor to
	 * jump around.  This is an initial version of the new code which i'm not
	 * sure if it breaks something or not.  I leave the previous code below
	 * ifdef'd out in case something else breaks.  If this works for a while
	 * withou causing problems i will remove this comment and the commented
	 * out code, assuming the fix worked. */

	// Enable capture
	if ( state )
	{
		// Get current cursor position
		::GetCursorPos( (POINT*)&m_preCapturePosition );

		// Enable mouse event capture
		::SetCapture( (HWND)GetHandle() );

		// Hard mode, hide the cursor
		if ( hard )
		{
			while ( ::ShowCursor( false ) >= 0 ) {}
		}
	}
	else // Disable capture
	{
		// Hard mode, show the cursor
		if ( m_isHardCaptured )
		{
			while ( ::ShowCursor( TRUE ) < 0 ) {}
		}

		// Release capture
		::ReleaseCapture();

		// Hard mode, restore cursor position
		if ( m_isHardCaptured )
		{
			::SetCursorPos( m_preCapturePosition.x, m_preCapturePosition.y );
		}
	}

	// Save the requested state
	m_isMouseCaptured = state;
	m_isHardCaptured = state && hard;

#if 0
	// Hard capture
	if ( state )
	{
		// Grab current mouse position
		::GetCursorPos( ( POINT*) &m_preCapturePosition );

		// Capture mouse
		::SetCapture( ( HWND) GetHandle() );

		// Grab mouse cursor
		if ( hard )
		{
			// Hide cursor
			while ( ::ShowCursor( false ) >= 0 ) {}
		}
		else
		{
			// Create clip region
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
		}
	}
	else
	{
		// Restore position
		::SetCursorPos( m_preCapturePosition.x, m_preCapturePosition.y );

		// Release capture
		::ReleaseCapture();

		// Release cursor
		if ( hard )
		{
			// Show cursor
			while ( ::ShowCursor( TRUE ) < 0 ) {}
		}
		else
		{
			// Destroy cursor clipping
			::ClipCursor( NULL );
		}
	}
#endif 
}

void CEdCanvas::SetScale( Float scale, Bool repaint /*= true*/ )
{
	// New scale ?
	if ( scale != m_scale )
	{
		// Set new scale
		m_invScale = 1.0f / scale;
		m_scale = scale;

		// Redraw canvas
		UpdateTransformMatrix();
		if( repaint )
		{
			Repaint();
		}
	}
}

void CEdCanvas::SetOffset( wxPoint offset, Bool repaint /*= true*/ )
{
	// New offset ?
	if ( offset != m_offset )
	{
		m_offset = offset;

		// Redraw
		UpdateTransformMatrix();
		if( repaint )
		{
			Repaint();
		}
	}
}

void CEdCanvas::UpdateTransformMatrix()
{
	if ( m_bufferDevice )
	{
		Gdiplus::Matrix trans( m_scale, 0.0f, 0.0f, m_scale, m_offset.x * m_scale, m_offset.y * m_scale );
		m_bufferDevice->SetTransform( &trans );
	}
}

wxPoint CEdCanvas::CanvasToClient( wxPoint point ) const
{
	Int32 sx = (point.x + m_offset.x) * m_scale;
	Int32 sy = (point.y + m_offset.y) * m_scale;
	return wxPoint( sx, sy );
}

wxPoint CEdCanvas::ClientToCanvas( wxPoint point ) const
{
	Int32 sx = (point.x * m_invScale) - m_offset.x;
	Int32 sy = (point.y * m_invScale) - m_offset.y;
	return wxPoint( sx, sy );
}

wxRect CEdCanvas::CanvasToClient( wxRect rect ) const
{
	// Transform corners
	wxPoint a = CanvasToClient( wxPoint( rect.GetLeft(), rect.GetTop() ) );
	wxPoint b = CanvasToClient( wxPoint( rect.GetRight(), rect.GetBottom() ) );

	// Return transformed rect
	return wxRect( a.x, a.y, b.x - a.x, b.y - a.y );
}

wxRect CEdCanvas::ClientToCanvas( wxRect rect ) const
{
	// Transform corners
	wxPoint a = ClientToCanvas( wxPoint( rect.GetLeft(), rect.GetTop() ) );
	wxPoint b = ClientToCanvas( wxPoint( rect.GetRight(), rect.GetBottom() ) );

	// Return transformed rect
	return wxRect( a.x, a.y, b.x - a.x, b.y - a.y );
}

wxPoint CEdCanvas::TextExtents( Gdiplus::Font& font, const String &text )
{
	if ( !text.Empty() )
	{
		Gdiplus::RectF boundingRect( 0,0,0,0 );
		m_bufferDevice->MeasureString( text.AsChar(), -1, &font, Gdiplus::PointF(0,0), &boundingRect );
		return wxPoint( boundingRect.Width, boundingRect.Height );
	}
	else
	{
		Gdiplus::RectF boundingRect( 0,0,0,0 );
		m_bufferDevice->MeasureString( TXT("A"), -1, &font, Gdiplus::PointF(0,0), &boundingRect );
		return wxPoint( 0, boundingRect.Height );
	}
}

wxRect CEdCanvas::TextRect( Gdiplus::Font& font, const String &text )
{
	wxPoint size = TextExtents( font, text );
	return wxRect( 0, 0, size.x, size.y );
}

void CEdCanvas::SetClip( const wxRect &rect )
{
	m_clip = rect;
	Gdiplus::Rect clipRect( rect.x, rect.y, rect.width, rect.height );
	m_bufferDevice->SetClip( clipRect );
}

void CEdCanvas::ResetClip()
{
	m_bufferDevice->ResetClip();
	m_clip = wxRect( 0, 0, GetSize().x, GetSize().y );
}
const wxRect& CEdCanvas::GetClip() const
{
	return m_clip;
}

void CEdCanvas::Clear( wxColour& color )
{
	m_bufferDevice->Clear( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
}

void CEdCanvas::DrawText( const wxPoint& offset, Gdiplus::Font& font, const String &text, const wxColour &color )
{
	Gdiplus::SolidBrush textBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->DrawString( text.AsChar(), -1, &font, Gdiplus::PointF( offset.x, offset.y ), &textBrush );
}

void CEdCanvas::DrawText( const wxPoint& topLeft, Uint32 width, Uint32 height, Gdiplus::Font& font, const String &text, const wxColour &color )
{
	Gdiplus::SolidBrush textBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->DrawString( text.AsChar(), -1, &font,
		Gdiplus::RectF( topLeft.x, topLeft.y, width, height ) , NULL, &textBrush );
}

void CEdCanvas::DrawText( const wxPoint& offset, Gdiplus::Font& font, const String &text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign )
{
	wxPoint alignedOffset = offset;
	wxPoint stringSize = TextExtents( font, text );

	// Calc vertical position
	if ( vAlign == CVA_Center )
	{
		alignedOffset.y -= stringSize.y / 2;
	}
	else if ( vAlign == CVA_Bottom )
	{
		alignedOffset.y -= stringSize.y;
	}

	// Calc horizontal position
	if ( hAlign == CHA_Center )
	{
		alignedOffset.x -= stringSize.x / 2;
	}
	else if ( hAlign == CHA_Right )
	{
		alignedOffset.x -= stringSize.x;
	}

	// Draw
	Gdiplus::SolidBrush textBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->DrawString( text.AsChar(), -1, &font, Gdiplus::PointF( alignedOffset.x, alignedOffset.y ), &textBrush );
}

void CEdCanvas::DrawTooltip(const wxPoint& point, String text, const wxColour& colorBackground, const wxColour& colorForeground)
{
	// If there is no tooltip, just exit
	if ( text == String() ) return;

	Gdiplus::Matrix backupMatrix;
	m_bufferDevice->GetTransform( &backupMatrix );

	Gdiplus::Matrix trans( 1.0f, 0.0f, 
		0.0f, 1.0f,
		m_offset.x, m_offset.y);

	m_bufferDevice->SetTransform( &trans );

	// Convert (mouse) position to apropriate value
	wxPoint convPoint = CanvasToClient(point);
	convPoint.x = (convPoint.x * 1.0f) - m_offset.x;
	convPoint.y = (convPoint.y * 1.0f) - m_offset.y;

	// Define our custom font for drawing tooltip
	Gdiplus::REAL fontSize = 8;
	if ( m_scale > 1.0f ) fontSize *= m_scale;
	Gdiplus::Font font( L"Arial", fontSize ); // m_drawFont

	// If tooltip is too long, divide it into multiple lines
	const Uint32 TooltipDivideWidth = 30;
	if (text.GetLength() > TooltipDivideWidth)
	{
		String tmpText(text);
		int k = 0;
		text.Clear();

		const unsigned int TextLength = tmpText.GetLength();

		for ( unsigned int i=0; i < TextLength; i++ )
		{
			if (++k > 30 && tmpText[i] == L' ')
			{
				text += TXT("\n");
				k = 0;
			}
			else
			{
				text += String::Chr(tmpText[i]);
			}
		}
	}

	// Draw rectangle (usually yellow transparent beneath tooltip text)

	Gdiplus::RectF rectString; // dimensions of the string
	const int rectBorder = 3;

	// Extend rect to the right, so the tooltip text fits better
	int compensate = 0;
	if ( m_scale <= 1.3f ) compensate = 6;
	else if ( m_scale >= 1.4f ) compensate = -3;

	m_bufferDevice->MeasureString( text.AsChar(), -1, &font, Gdiplus::PointF(0,0), &rectString );
	convPoint.y -= rectString.Height; // go up
	wxRect rect( convPoint.x - rectBorder, convPoint.y - rectBorder,
		rectString.Width + rectBorder + compensate, rectString.Height + rectBorder);
	FillRoundedRect( rect, colorBackground, 3 );
	DrawRoundedRect( rect, colorForeground, 3 );

	// Draw tooltip text
	DrawText( convPoint, font, text, colorForeground );

	// Restore original transformation matrix
	m_bufferDevice->SetTransform( &backupMatrix );
}

void CEdCanvas::DrawRect( const wxRect& rect, const wxColour& color, FLOAT width /* = 1.0f */ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawRectangle( &drawPen, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::DrawRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, FLOAT width /* = 1.0f */ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawRectangle( &drawPen, x, y, w, h );
}

void CEdCanvas::FillRect( const wxRect& rect, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillRectangle( &drawBrush, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::FillRect( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillRectangle( &drawBrush, x, y, w, h );
}

void CEdCanvas::FillGradientHorizontalRect( const wxRect& rect, const wxColour& leftColor, const wxColour& rightColor )
{
	Gdiplus::LinearGradientBrush linearGradientBrush(
		Gdiplus::Point( rect.GetX(), 0 ),
		Gdiplus::Point( rect.GetX() + rect.GetWidth(), 0 ),
		Gdiplus::Color( leftColor.Alpha(), leftColor.Red(), leftColor.Green(), leftColor.Blue() ),
		Gdiplus::Color( rightColor.Alpha(), rightColor.Red(), rightColor.Green(), rightColor.Blue() ));

	m_bufferDevice->FillRectangle( &linearGradientBrush, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::FillGradientVerticalRect( const wxRect& rect, const wxColour& upColor, const wxColour& downColor )
{
	Gdiplus::LinearGradientBrush linearGradientBrush(
		Gdiplus::Point( 0, rect.GetY() ),
		Gdiplus::Point( 0, rect.GetY() + rect.GetHeight() ),
		Gdiplus::Color( upColor.Alpha(), upColor.Red(), upColor.Green(), upColor.Blue() ),
		Gdiplus::Color( downColor.Alpha(), downColor.Red(), downColor.Green(), downColor.Blue() ));

	m_bufferDevice->FillRectangle( &linearGradientBrush, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::FillGradientVerticalRect( const wxRect& rect, const wxColour& upColor, const wxColour& downColor, const wxColour& middleColor )
{
	// Put the points of a rect in an array.
	Gdiplus::Point points[] = {
		// up line
		Gdiplus::Point(rect.GetX(), rect.GetY()), Gdiplus::Point(rect.GetX()+rect.GetWidth(), rect.GetY()),
		// right line
		Gdiplus::Point(rect.GetX()+rect.GetWidth(), rect.GetY()), Gdiplus::Point(rect.GetX()+rect.GetWidth(), rect.GetY() + rect.GetHeight()),
		// down line
		Gdiplus::Point(rect.GetX()+rect.GetWidth(), rect.GetY() + rect.GetHeight()), Gdiplus::Point(rect.GetX(), rect.GetY() + rect.GetHeight() ),
		// left line
		Gdiplus::Point(rect.GetX(), rect.GetY() + rect.GetHeight() ), Gdiplus::Point(rect.GetX(), rect.GetY())
	};

	// Use the array of points to construct a path.
	Gdiplus::GraphicsPath path;
	path.AddLines( points, 8 );

	// Use the path to construct a brush.
	Gdiplus::PathGradientBrush pthGrBrush( &path );

	// Set the color at the center of the path.
	pthGrBrush.SetCenterColor( Gdiplus::Color(middleColor.Alpha(), middleColor.Red(), middleColor.Green(), middleColor.Blue()) );

	// Set the color along the entire boundary of the path to aqua.
	Gdiplus::Color upColorGdiPlus(upColor.Alpha(), upColor.Red(), upColor.Green(), upColor.Blue());
	Gdiplus::Color downColorGdiPlus(downColor.Alpha(), downColor.Red(), downColor.Green(), downColor.Blue());
	Gdiplus::Color colors[] = {
		upColorGdiPlus,   upColorGdiPlus,
		upColorGdiPlus,   downColorGdiPlus,
		downColorGdiPlus, downColorGdiPlus,
		downColorGdiPlus, upColorGdiPlus
	};
	int count = 8;
	pthGrBrush.SetSurroundColors( colors, &count );

	// Extend center point.
	pthGrBrush.SetFocusScales( 1.0f, 0.0f );

	m_bufferDevice->FillRectangle( &pthGrBrush, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::DrawLine( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& color, FLOAT width/*=1.0f*/ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawLine( &drawPen, x1, y1, x2, y2 );
}

void CEdCanvas::DrawLine( const wxPoint& start, const wxPoint &end, const wxColour& color, FLOAT width/*=1.0f*/ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawLine( &drawPen, start.x, start.y, end.x, end.y );
}

void CEdCanvas::DrawLines( const wxPoint* points, const Int32 numPoints, const wxColour& color, FLOAT width/*=1.0f*/ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawLines( &drawPen, (const Gdiplus::Point*) points, numPoints );
}

void CEdCanvas::DrawArc( const wxRect &rect, Float startAngle, Float sweepAngle, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawArc( &drawPen, rect.x, rect.y, rect.width, rect.height, startAngle, sweepAngle );
}

void CEdCanvas::DrawPie( const wxRect &rect, Float startAngle, Float sweepAngle, const wxColour& color, FLOAT width )
{
		Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
		m_bufferDevice->DrawPie( &drawPen, rect.x, rect.y, rect.width, rect.height, startAngle, sweepAngle );
}

void CEdCanvas::DrawCircle( Int32 x, Int32 y, Int32 radius, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawEllipse( &drawPen, x, y, radius, radius );
}

void CEdCanvas::DrawCircle( const wxPoint &center, Int32 radius, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width ); 
	m_bufferDevice->DrawEllipse( &drawPen, center.x, center.y, radius, radius );
}

void CEdCanvas::DrawCircleCentered( const wxPoint &center, Int32 radius, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width ); 
	m_bufferDevice->DrawEllipse( &drawPen, center.x - radius, center.y - radius, radius * 2, radius * 2 );
}

void CEdCanvas::FillCircle( Int32 x, Int32 y, Int32 radius, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillEllipse( &drawBrush, x, y, radius, radius );
}

void CEdCanvas::FillCircle( const wxPoint &topLeft, Int32 radius, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillEllipse( &drawBrush, topLeft.x, topLeft.y, radius, radius );
}

void CEdCanvas::FillCircleCentered( const wxPoint &center, Int32 radius, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillEllipse( &drawBrush, center.x - radius, center.y - radius, radius * 2, radius * 2 );
}

void CEdCanvas::DrawCircle( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawEllipse( &drawPen, x, y, w, h );
}

void CEdCanvas::DrawCircle( const wxRect& rect, const wxColour& color, FLOAT width )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawEllipse( &drawPen, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::FillCircle( Int32 x, Int32 y, Int32 w, Int32 h, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillEllipse( &drawBrush, x, y, w, h );
}

void CEdCanvas::FillCircle( const wxRect& rect, const wxColour& color )
{
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillEllipse( &drawBrush, rect.x, rect.y, rect.width, rect.height );
}

void CEdCanvas::DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y )
{
	m_bufferDevice->DrawImage( bitmap, Gdiplus::Point( x, y ) );
}

void CEdCanvas::DrawImage( Gdiplus::Bitmap* bitmap, const wxPoint& point )
{
	m_bufferDevice->DrawImage( bitmap, Gdiplus::Point( point.x, point.y ) );
}

void CEdCanvas::DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, Int32 w, Int32 h )
{
	Gdiplus::Rect rect( x, y, w, h );
	m_bufferDevice->DrawImage( bitmap, rect );
}

void CEdCanvas::DrawImage( Gdiplus::Bitmap* bitmap, const wxRect& rect )
{
	Gdiplus::Rect drawRect( rect.x, rect.y, rect.width, rect.height );
	m_bufferDevice->DrawImage( bitmap, drawRect );
}

void CEdCanvas::DrawImage( Gdiplus::Bitmap* bitmap, Int32 x, Int32 y, const wxColour& transparentColor )
{
	if( bitmap == NULL )
	{
		return;
	}

	Gdiplus::Rect drawRect( x, y, bitmap->GetWidth(), bitmap->GetHeight() );
	Gdiplus::ImageAttributes attributes;
	attributes.SetColorKey( Gdiplus::Color( transparentColor.Alpha(), transparentColor.Red(),
		transparentColor.Green(), transparentColor.Blue() ),
		Gdiplus::Color( transparentColor.Alpha(), transparentColor.Red(),
		transparentColor.Green(), transparentColor.Blue() ) );

	m_bufferDevice->DrawImage( bitmap, drawRect, 0, 0, bitmap->GetWidth(), bitmap->GetHeight(),
		Gdiplus::UnitPixel, &attributes );
}

void CEdCanvas::DrawCurve( const wxPoint *points, const wxColour& color, FLOAT width/*=1.0f*/ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	m_bufferDevice->DrawBezier( &drawPen, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y );
}

void CEdCanvas::DrawCardinalCurve( const wxPoint *points, Int32 numPoints, const wxColour& color, FLOAT width /*= 1.0f*/ )
{
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width );
	TDynArray< Gdiplus::Point > gdiPlusPoints;
	for( Int32 i=0; i<numPoints; ++i )
		gdiPlusPoints.PushBack( Gdiplus::Point( points[i].x, points[i].y ) );

	m_bufferDevice->DrawCurve( &drawPen, gdiPlusPoints.TypedData(), numPoints, 0.5f );
}

Bool CEdCanvas::HitTestCurve( const wxPoint *points, const wxPoint& point, const Float range/*=1.0f*/ )
{
	const Vector testPoint( point.x, point.y, 0 );

	// Initialize bezier blend weights on first use
	const Uint32 numPoints = 20;
	static Vector bezierWeights[ numPoints+1 ];
	if ( !bezierWeights[0].X )
	{
		for ( Uint32 i=0; i<=numPoints; i++ )
		{
			Float t = i / (Float)numPoints;
			bezierWeights[i].W = t*t*t;
			bezierWeights[i].Z = 3.0f*t*t*(1.0f-t);
			bezierWeights[i].Y = 3.0f*t*(1.0f-t)*(1.0f-t);
			bezierWeights[i].X = (1.0f-t)*(1.0f-t)*(1.0f-t);
		}
	}

	// Test bounding box
	Int32 minX = Min( Min( points[0].x, points[1].x ), Min( points[2].x, points[3].x ) ) - range;
	Int32 minY = Min( Min( points[0].y, points[1].y ), Min( points[2].y, points[3].y ) ) - range;
	Int32 maxX = Max( Max( points[0].x, points[1].x ), Max( points[2].x, points[3].x ) ) + range;
	Int32 maxY = Max( Max( points[0].y, points[1].y ), Max( points[2].y, points[3].y ) ) + range;
	if ( point.x >= minX && point.y >= minY && point.x <= maxX && point.y <= maxY )
	{
		// Hit test curve
		Vector lastPoint;
		for ( Uint32 i=0; i<=numPoints; i++ )
		{
			const Vector weights = bezierWeights[i];
			const Float px = (points[0].x * weights.X) + (points[1].x * weights.Y) + (points[2].x * weights.Z) + (points[3].x * weights.W);
			const Float py = (points[0].y * weights.X) + (points[1].y * weights.Y) + (points[2].y * weights.Z) + (points[3].y * weights.W);
			const Vector point( px, py, 0, 1 );

			// Calculate distance to curve
			if ( i )
			{
				const Vector delta = point - lastPoint;
				const Float s = Vector::Dot3( lastPoint, delta );
				const Float e = Vector::Dot3( point, delta );
				const Float t = Vector::Dot3( testPoint, delta );
				Float dist;
				if ( t < s )
				{
					dist = lastPoint.DistanceTo( testPoint );
				}
				else if ( t > e )
				{
					dist = point.DistanceTo( testPoint );
				}
				else
				{
					const Vector projected = lastPoint + delta * (t - s);
					dist = projected.DistanceTo( testPoint );
				}

				// Close enough
				if ( dist < range )
				{
					return true;
				}
			}

			// Remember for next pass
			lastPoint = point;
		}
	}

	// Not hit
	return false;
}

Gdiplus::Bitmap* CEdCanvas::ConvertToGDI(const wxBitmap &bmp)
{
	Gdiplus::Bitmap *result = new Gdiplus::Bitmap(bmp.GetWidth(), bmp.GetHeight(), PixelFormat32bppARGB );
	wxImage image = bmp.ConvertToImage();
	if( image.HasAlpha() )
	{
		for (Int32 i = 0; i < bmp.GetWidth(); i++)
		{
			for (Int32 j = 0; j < bmp.GetHeight(); j++)
			{
				Gdiplus::Color color(image.GetAlpha(i, j), image.GetRed(i, j), image.GetGreen(i, j), image.GetBlue(i, j));
				result->SetPixel(i, j, color);
			};
		}
	}
	else
	{
		for (Int32 i = 0; i < bmp.GetWidth(); i++)
		{
			for (Int32 j = 0; j < bmp.GetHeight(); j++)
			{
				Gdiplus::Color color(255, image.GetRed(i, j), image.GetGreen(i, j), image.GetBlue(i, j));
				result->SetPixel(i, j, color);
			};
		}
	}
	return result;
}

Gdiplus::Bitmap *CEdCanvas::ChangeImageColor( Gdiplus::Bitmap *bitmap, const wxColor &color )
{
	Int32 width = bitmap->GetWidth();
	Int32 height = bitmap->GetHeight();

	Gdiplus::Bitmap *destBitmap = new Gdiplus::Bitmap(width, height, bitmap->GetPixelFormat());
	if (Gdiplus::Graphics *graphics = Gdiplus::Graphics::FromImage(destBitmap))
	{
		// Prepare color matrix
		const Float inv255 = 1 / 255.0f;
		const Float r = color.Red() * inv255;
		const Float g = color.Green() * inv255;
		const Float b = color.Blue() * inv255;
		const Float a = color.Alpha() * inv255;
		Float matrixItems[5][5] = 
		{
			{ r, 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, g, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, b, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, a, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }
		};
		
		Gdiplus::ColorMatrix colorMatrix;
		Red::System::MemoryCopy(colorMatrix.m, matrixItems, sizeof(Float) * 25);

		// Create image attributes and set color matrix to it
		Gdiplus::ImageAttributes imageAttributes;
		imageAttributes.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

		// Draw source bitmap on dest bitmap using color matrix
		Gdiplus::Rect drawRect( 0, 0, width, height );
		graphics->DrawImage(bitmap, drawRect, 0.0f, 0.0f, (Float)width, (Float)height, Gdiplus::UnitPixel, &imageAttributes);
		delete graphics;
	}

	return destBitmap;
}

wxColor CEdCanvas::ConvertToGrayscale(const wxColor &color)
{
	int value = Int32(0.3f * color.Red() + 0.59 * color.Red() + 0.11f * color.Blue());
	return wxColor(value, value, value, color.Alpha());
}

void CEdCanvas::RGBToHSV(const wxColor &color, Float &hue, Float &sat, Float &val)
{
	static Float inv255 = 1 / 255.0f;
	Float r = color.Red() * inv255;
	Float g = color.Green() * inv255;
	Float b = color.Blue() * inv255;

	Float x = min(min(r, g), b);
	val = max(max(r, g), b);
	Float f;
	Int32 i;
	if (x == val)
	{
		hue = 0;
		sat = 0;
	}
	else
	{
		if (r == x)
			f = g - b;
		else if (g == x)
			f = b - r;
		else
			f = r - g;

		if (r == x)
			i = 3;
		else if (g == x)
			i = 5;
		else
			i = 1;

		hue = Int32((i - f / (val - x)) * 60) % 360;
		sat = (val - x) / val;
	}
}

wxColor CEdCanvas::HSVToRGB(Float hue, Float sat, Float val)
{
	Float r, g, b;
	if (val == 0)
	{
		r = 0;
		g = 0;
		b = 0;
	}
	else
	{
		hue /= 60;
		Float i = floorf(hue);
		Float f = hue - i;
		Float p = val * (1 - sat);
		Float q = val * (1 - (sat * f));
		Float t = val * (1 - (sat * (1 - f)));
		if (i == 0) { r = val; g = t; b = p;}
		else if (i == 1) {r = q; g = val; b = p; }
		else if (i == 2) {r = p; g = val; b = t; }
		else if (i == 3) {r = p; g = q; b = val; }
		else if (i == 4) {r = t; g = p; b = val; }
		else if (i == 5) {r = val; g = p; b = q; }
	}

	return wxColor(Int32(255 * r), Int32(255 * g), Int32(255 * b));
}

void CEdCanvas::DrawRoundedRect( const wxRect& rect, const wxColour& color, Int32 radius, Float borderWidth )
{
	DrawRoundedRect( rect.x, rect.y, rect.width, rect.height, color, radius, borderWidth );
}

void CEdCanvas::DrawRoundedRect( Int32 x, Int32 y, Int32 width, Int32 height, const wxColour& color, Int32 radius, Float borderWidth )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x + radius, y, x + width - (radius*2), y );
	gp.AddArc( x + width - (radius*2), y, radius*2, radius*2, 270, 90 );
	gp.AddLine( x + width, y + radius, x + width, y + height - (radius*2) );
	gp.AddArc( x + width - (radius*2), y + height - (radius*2), radius*2, radius*2, 0, 90 );
	gp.AddLine( x + width - (radius*2), y + height, x + radius, y + height );
	gp.AddArc( x, y + height - (radius*2), radius*2, radius*2, 90, 90 );
	gp.AddLine( x, y + height - (radius*2), x, y + radius );
	gp.AddArc( x, y, radius*2, radius*2, 180, 90 );
	gp.CloseFigure();

	// Draw path
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), borderWidth ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::FillRoundedRect( const wxRect& rect, const wxColour& color, Int32 radius )
{
	FillRoundedRect( rect.x, rect.y, rect.width, rect.height, color, radius );
}

void CEdCanvas::FillRoundedRect( Int32 x, Int32 y, Int32 width, Int32 height, const wxColour& color, Int32 radius )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x + radius, y, x + width - (radius*2), y );
	gp.AddArc( x + width - (radius*2), y, radius*2, radius*2, 270, 90 );
	gp.AddLine( x + width, y + radius, x + width, y + height - (radius*2) );
	gp.AddArc( x + width - (radius*2), y + height - (radius*2), radius*2, radius*2, 0, 90 );
	gp.AddLine( x + width - (radius*2), y + height, x + radius, y + height );
	gp.AddArc( x, y + height - (radius*2), radius*2, radius*2, 90, 90 );
	gp.AddLine( x, y + height - (radius*2), x, y + radius );
	gp.AddArc( x, y, radius*2, radius*2, 180, 90 );
	gp.CloseFigure();

	// Draw path
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillPath( &drawBrush, &gp );
}

void CEdCanvas::DrawCuttedRect( const wxRect& rect, const wxColour& color, Int32 radius, FLOAT borderWidth )
{
	DrawCuttedRect( rect.x, rect.y, rect.width, rect.height, color, radius, borderWidth );
}

void CEdCanvas::DrawCuttedRect( Int32 x, Int32 y, Int32 width, Int32 height, const wxColour& color, Int32 radius, FLOAT borderWidth )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x + radius, y, x + width - radius, y );
	gp.AddLine( x + width - radius, y, x + width, y + radius );
	gp.AddLine( x + width, y + radius, x + width, y + height - radius );
	gp.AddLine( x + width, y + height - radius, x + width - radius, y + height );
	gp.AddLine( x + width - radius, y + height, x + radius, y + height );
	gp.AddLine( x + radius, y + height, x, y + height - radius );
	gp.AddLine( x, y + height - radius, x, y + radius );
	gp.AddLine( x, y + radius, x + radius, y );
	gp.CloseFigure();

	// Draw path
	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), borderWidth ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::FillCuttedRect( const wxRect& rect, const wxColour& color, Int32 radius )
{
	FillCuttedRect( rect.x, rect.y, rect.width, rect.height, color, radius );
}

void CEdCanvas::FillCuttedRect( Int32 x, Int32 y, Int32 width, Int32 height, const wxColour& color, Int32 radius )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x + radius, y, x + width - radius, y );
	gp.AddLine( x + width - radius, y, x + width, y + radius );
	gp.AddLine( x + width, y + radius, x + width, y + height - radius );
	gp.AddLine( x + width, y + height - radius, x + width - radius, y + height );
	gp.AddLine( x + width - radius, y + height, x + radius, y + height );
	gp.AddLine( x + radius, y + height, x, y + height - radius );
	gp.AddLine( x, y + height - radius, x, y + radius );
	gp.AddLine( x, y + radius, x + radius, y );
	gp.CloseFigure();

	// Draw path
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) );
	m_bufferDevice->FillPath( &drawBrush, &gp );
}

void CEdCanvas::DrawDownArrow( const wxRect& rect, const wxColour& baseColor, const wxColour& borderColor )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( rect.x, rect.y, rect.x + rect.width, rect.y );
	gp.AddLine( rect.x + rect.width, rect.y, rect.x + rect.width/2, rect.y+rect.height );
	gp.AddLine( rect.x + rect.width/2, rect.y+rect.height, rect.x, rect.y );
	gp.CloseFigure();

	// Draw path
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( baseColor.Alpha(), baseColor.Red(), baseColor.Green(), baseColor.Blue() ) ); 
	m_bufferDevice->FillPath( &drawBrush, &gp );
	Gdiplus::Pen drawPen( Gdiplus::Color( borderColor.Alpha(), borderColor.Red(), borderColor.Green(), borderColor.Blue() ), 1 ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::DrawUpArrow( const wxRect& rect, const wxColour& baseColor, const wxColour& borderColor )
{
	// Assemble bounds
	Gdiplus::GraphicsPath gp;
	gp.AddLine( rect.x, rect.y, rect.x + rect.width, rect.y+rect.height );
	gp.AddLine( rect.x + rect.width, rect.y, rect.x + rect.width/2, rect.y );
	gp.AddLine( rect.x + rect.width/2, rect.y+rect.height, rect.x, rect.y+rect.height );
	gp.CloseFigure();

	// Draw path
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( baseColor.Alpha(), baseColor.Red(), baseColor.Green(), baseColor.Blue() ) ); 
	m_bufferDevice->FillPath( &drawBrush, &gp );
	Gdiplus::Pen drawPen( Gdiplus::Color( borderColor.Alpha(), borderColor.Red(), borderColor.Green(), borderColor.Blue() ), 1 ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::DrawTriangle( Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color, FLOAT borderWidth )
{
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x1, y1, x2, y2 );
	gp.AddLine( x2, y2, x3, y3 );
	gp.AddLine( x3, y3, x1, y1 );
	gp.CloseFigure();

	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), borderWidth ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::FillTriangle( Int32 x1, Int32 y1, Int32 x2, Int32 y2, Int32 x3, Int32 y3, const wxColour& color )
{
	Gdiplus::GraphicsPath gp;
	gp.AddLine( x1, y1, x2, y2 );
	gp.AddLine( x2, y2, x3, y3 );
	gp.AddLine( x3, y3, x1, y1 );
	gp.CloseFigure();

	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) ); 
	m_bufferDevice->FillPath( &drawBrush, &gp );
}

void CEdCanvas::DrawTipRect( Int32 x1, Int32 y1, Int32 x2, Int32 y2, const wxColour& borderColor, const wxColour& interiorColor, Uint32 size )
{
	{
		wxPoint tip( x1, y1 );
		FillTriangle( tip.x + size, tip.y + size, tip.x - size, tip.y + size, tip.x + size, tip.y - size, interiorColor );
		DrawTriangle( tip.x + size, tip.y + size, tip.x - size, tip.y + size, tip.x + size, tip.y - size, borderColor );
	}

	{
		wxPoint tip( x2, y1 );
		FillTriangle( tip.x - size, tip.y + size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, interiorColor );
		DrawTriangle( tip.x - size, tip.y + size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, borderColor );
	}

	{
		wxPoint tip( x1, y2 );
		FillTriangle( tip.x + size, tip.y - size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, interiorColor );
		DrawTriangle( tip.x + size, tip.y - size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, borderColor );
	}

	{
		wxPoint tip( x2, y2 );
		FillTriangle( tip.x - size, tip.y - size, tip.x + size, tip.y - size, tip.x - size, tip.y + size, interiorColor );
		DrawTriangle( tip.x - size, tip.y - size, tip.x + size, tip.y - size, tip.x - size, tip.y + size, borderColor );
	}
}

void CEdCanvas::DrawCross( const wxRect& rect, const wxColour& color, FLOAT width /* = 1.0f */ )
{
	Gdiplus::GraphicsPath gp;
	gp.AddLine( rect.x, rect.y, rect.x+rect.width, rect.y+rect.height );

	Gdiplus::Pen drawPen( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ), width ); 
	m_bufferDevice->DrawPath( &drawPen, &gp );

	gp.Reset();
	gp.AddLine( rect.x+rect.width, rect.y, rect.x, rect.y+rect.height );
	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::DrawPoly( const wxPoint *points, Uint32 numPoints, const wxColour& borderColor, Float borderWidth /*= 1.0f*/ )
{
	Gdiplus::GraphicsPath gp;
	Gdiplus::Pen drawPen( Gdiplus::Color(borderColor.Alpha(), borderColor.Red(), borderColor.Green(), borderColor.Blue()), borderWidth ); 
	TDynArray< Gdiplus::Point > gdiPoints;
	for (Uint32 i = 0; i < numPoints; ++i)
	{
		gdiPoints.PushBack( Gdiplus::Point(points[i].x, points[i].y) );
	}
	gp.AddPolygon( &gdiPoints[0], numPoints );
	gp.CloseFigure();

	m_bufferDevice->DrawPath( &drawPen, &gp );
}

void CEdCanvas::FillPoly( const wxPoint *points, Uint32 numPoints, const wxColour& color )
{
	Gdiplus::GraphicsPath gp;
	Gdiplus::SolidBrush drawBrush( Gdiplus::Color( color.Alpha(), color.Red(), color.Green(), color.Blue() ) ); 
	TDynArray< Gdiplus::Point > gdiPoints;
	for (Uint32 i = 0; i < numPoints; ++i)
	{
		gdiPoints.PushBack( Gdiplus::Point(points[i].x, points[i].y) );
	}
	gp.AddPolygon( &gdiPoints[0], numPoints );
	gp.CloseFigure();

	m_bufferDevice->FillPath( &drawBrush, &gp );
}

void CEdCanvas::PaintCanvas( Int32 width, Int32 height )
{
	// Implemented in derived classes
}

void CEdCanvas::MouseClick( wxMouseEvent& event )
{
	// Implemented in derived classes
	m_lastClickPoint = event.GetPosition();
}

void CEdCanvas::MouseMove( wxMouseEvent& event, wxPoint delta )
{
	// Implemented in derived classes
}

void CEdCanvas::RecreateDevice( Int32 newWidth, Int32 newHeight )
{
	// Recreate only if size differs
	if ( (newWidth != m_lastWidth) || (newHeight != m_lastHeight) )
	{
		if( GetCanvasType() == CanvasType::gdiplus )
		{
			// Destroy old buffered device
			if ( m_bufferDevice )
			{
				delete m_bufferDevice;
				m_bufferDevice = NULL;
			}

			// Destroy old buffer bitmap
			if ( m_bitmapBuffer )
			{
				delete m_bitmapBuffer;
				m_bitmapBuffer = NULL;

				delete m_bitmap;
				m_bitmap = nullptr;
			}
		}
		else
		{
			// Destroy old buffer bitmap
			delete m_bitmap;
			m_bitmap = nullptr;
		}

		// Remember new size
		m_lastWidth = max(1, newWidth);
		m_lastHeight = max(1, newHeight);

		// Create new buffering device with bitmap
		if ( m_lastHeight && m_lastWidth )
		{
			if( GetCanvasType() == CanvasType::gdiplus)
			{
				// Create new GDI objects
				m_bitmapBuffer = new Gdiplus::Bitmap( newWidth, newHeight );
				m_bufferDevice = new Gdiplus::Graphics( m_bitmapBuffer );

				// Setup rendering options, highest quality
				m_bufferDevice->SetInterpolationMode( Gdiplus::InterpolationModeDefault );
				m_bufferDevice->SetTextRenderingHint( Gdiplus::TextRenderingHintAntiAliasGridFit );
				m_bufferDevice->SetCompositingMode( Gdiplus::CompositingModeSourceOver );
				m_bufferDevice->SetSmoothingMode( Gdiplus::SmoothingModeAntiAlias );
			}
			else
			{
				m_bitmap = new wxBitmap( newWidth, newHeight );
				ASSERT(m_bitmap);
			}

			// TODO: handle this in gdi path
			// Restore transformation matrix
			UpdateTransformMatrix();
		}
	}
}

void CEdCanvas::OnPaint( wxPaintEvent &event )
{
	wxPaintDC paintDC( this );

	// Get current size
	Int32 w, h;
	GetClientSize( &w, &h );

	// Recreate device
	RecreateDevice( w, h );

	if( GetCanvasType() == CanvasType::gdiplus )
	{
		// Draw buffered
		if ( m_bufferDevice )
		{
			// Draw window content to buffered device
			PaintCanvas( w, h );

			Gdiplus::Graphics graphics( (HDC)paintDC.GetHDC() );
			graphics.DrawImage( m_bitmapBuffer, 0, 0, w, h );
		}
	}
	else
	{
		// TODO: don't recreate mem dc each time
		wxMemoryDC memDC( &paintDC );
		m_memDC = &memDC;
		m_memDC->SelectObject( *m_bitmap );

		PaintCanvas( w, h );

		paintDC.Blit(0, 0, m_bitmap->GetWidth(), m_bitmap->GetHeight(), m_memDC, 0, 0);
		m_memDC = nullptr;
	}
}

void CEdCanvas::OnSize( wxSizeEvent& event )
{
	// Repaint whole window after sizing
	Refresh( false );
}

void CEdCanvas::OnEraseBackground( wxEraseEvent& event )
{
	// Do not erase background
}

void CEdCanvas::OnMouseEvent( wxMouseEvent& event )
{
	if ( !event.RightUp() && !event.LeftUp() )
	{
		// Always focus the canvas window when clicked, needed for mouse wheel to work
		// Focus shouldn't be set, when button is released - this returns focus to the
		// window, from which we may have just left
		SetFocus();
	}

	// Process event
	MouseClick( event );
}

void CEdCanvas::OnMouseMove( wxMouseEvent& event )
{
	wxPoint delta( 0,0 );

	// Not captured, no delta
	if ( !m_isMouseCaptured )
	{
		MouseMove( event, delta );
		return;
	}

	// Calculate delta
	POINT curPos;
	::GetCursorPos( &curPos );
	delta.x = curPos.x - m_preCapturePosition.x;
	delta.y = curPos.y - m_preCapturePosition.y;

	// Hard capture
	if ( m_isHardCaptured )
	{
		// Move cursor back so we can do large moves
		::SetCursorPos( m_preCapturePosition.x, m_preCapturePosition.y );
	}
	else
	{
		// Remember so we have relative move
		m_preCapturePosition.x = curPos.x;
		m_preCapturePosition.y = curPos.y;
	}

	// Call event
	MouseMove( event, delta );
}
