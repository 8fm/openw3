/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/display.h>

BEGIN_EVENT_TABLE( CEdColorPicker, wxFrame )

	EVT_PAINT( CEdColorPicker::OnPaint )
	EVT_ERASE_BACKGROUND( CEdColorPicker::OnEraseBackground )
	EVT_MOTION( CEdColorPicker::OnMouseMove )
	EVT_LEFT_DOWN( CEdColorPicker::OnMouseClick )
	EVT_LEFT_UP( CEdColorPicker::OnMouseClick )
	EVT_ACTIVATE( CEdColorPicker::OnActivate )

END_EVENT_TABLE()

namespace
{
	const Int32 PICKER_WIDTH				= 220;
	const Int32 PICKER_HEIGHT				= 180;

	const Int32 FRAME_WIDTH					= PICKER_WIDTH;
	const Int32 FRAME_HEIGHT				= 260;

	const Int32 PICKER_COLOR_RADIUS			= 50;
	const Int32 PICKER_RING_RADIUS			= 20;
	const Int32 PICKER_DRAG_RADIUS			= 5;
	const Int32 WHEEL_TOP_OFFSET			= 90;
	const Int32 WHEEL_CX					= ( PICKER_WIDTH/2 ) - 30; 
	const Int32 WHEEL_CY					= ( PICKER_HEIGHT/2 );

	const Float PICKER_LIGHT_RING_ANG_MIN	= 0.0f;
	const Float PICKER_LIGHT_RING_ANG_MAX	= 0.75f;
}

// --------------------------------------------------------------------------
// Utility functions

wxPoint CalcPickerPosition()
{
	wxPoint mousePos = wxGetMousePosition();
	wxPoint ret = mousePos - wxPoint( FRAME_WIDTH/2, FRAME_HEIGHT/2 );

	int displayIdx = wxDisplay::GetFromPoint( mousePos );
	wxRect d = wxDisplay( displayIdx ).GetClientArea();

	ret.x = Max< Int32 >( d.x, Min< Int32 >( ret.x, d.x + d.width  - FRAME_WIDTH ) );
	ret.y = Max< Int32 >( d.y, Min< Int32 >( ret.y, d.y + d.height - FRAME_HEIGHT ) );
	return ret;
}

void HSLToRGB( Float hue, Float saturation, Float lightness, Float& r, Float& g, Float& b )
{
	Int32 i;
	Float f, p, q, t;

	if( saturation == 0 ) 
	{
		// achromatic (grey)
		r = g = b = lightness;
		return;
	}

	if( hue == 1.0f )
	{
		i = 5;
		f = 1.0f;
	}
	else
	{
		hue *= 6.0f;			// sector 0 to 5
		i = floor( hue );
		f = hue - i;			// factorial part of h
	}
	p = lightness * ( 1 - saturation );
	q = lightness * ( 1 - saturation * f );
	t = lightness * ( 1 - saturation * ( 1 - f ) );

	switch( i ) {
		case 0:
			r = lightness;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = lightness;
			b = p;
			break;
		case 2:
			r = p;
			g = lightness;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = lightness;
			break;
		case 4:
			r = t;
			g = p;
			b = lightness;
			break;
		default:		// case 5:
			r = lightness;
			g = p;
			b = q;
			break;
	}

	ASSERT( r >= 0 && r <= 255 );
	ASSERT( g >= 0 && g <= 255 );
	ASSERT( b >= 0 && b <= 255 );
}

void RGBToHSL( Float r, Float g, Float b, Float &hue, Float &saturation, Float &lightness )
{
	Float x = Min(Min(r, g), b);
	lightness = Max(Max(r, g), b);
	if( x == lightness ) 
	{
		hue = 0.5f; 
		saturation = 0.0f;
	}
	else 
	{
		Float i, f;
		if( r == x )
		{
			f = g-b;
			i = 3.0f;
		}
		else
			if( g == x )
			{
				f = b-r;
				i = 5.0f;
			}
			else
			{
				f = r-g;
				i = 1.0;
			}
			hue = (i-f/(lightness-x)) / 6.0f;
			if( hue < 0.0f )
				hue += 1.0f;
			saturation = ((lightness-x)/lightness);

			ASSERT( hue >= 0.0f && hue <= 1.0f );
			ASSERT( saturation >= 0.0f && saturation <= 1.0f );
	}
}

CEdColorPicker::CEdColorPicker( wxWindow* parent )
	: wxFrame( parent, -1, wxEmptyString, wxDefaultPosition, wxSize( FRAME_WIDTH, FRAME_HEIGHT ), wxSTAY_ON_TOP | wxFRAME_NO_TASKBAR )
	, m_backImage( PICKER_WIDTH, PICKER_HEIGHT, true )
	, m_dragMode( DM_None )
	, m_color( Color::BLACK )
	, m_initialColor( Color::BLACK )
{
	m_panel = new wxPanel();
	wxXmlResource::Get()->LoadPanel( m_panel, this, wxT("ColorPicker") );

	m_red  .Init( m_panel, TXT("Red"),   0, 255, m_initialColor.R, 0, 1.0f );
	m_green.Init( m_panel, TXT("Green"), 0, 255, m_initialColor.G, 0, 1.0f );
	m_blue .Init( m_panel, TXT("Blue"),  0, 255, m_initialColor.B, 0, 1.0f );
	m_alpha.Init( m_panel, TXT("Alpha"), 0, 255, m_initialColor.A, 0, 1.0f );

	m_red  .Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdColorPicker::OnColorRedChanged ), NULL, this );
	m_green.Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdColorPicker::OnColorGreenChanged ), NULL, this );
	m_blue .Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdColorPicker::OnColorBlueChanged ), NULL, this );
	m_alpha.Connect( wxEVT_SPIN_SLIDER_CHANGED, wxCommandEventHandler( CEdColorPicker::OnColorAlphaChanged ), NULL, this );

 	wxBoxSizer *sizer = new wxBoxSizer( wxVERTICAL );
 	//sizer->Add( m_panel, 1, wxEXPAND, 0 );
 	sizer->Add( m_panel );
 	SetSizer( sizer );

	// Turn window into transparent overlay
	/*
	const HWND wnd = (HWND)GetHWND();
	SetWindowLong( wnd, GWL_EXSTYLE, GetWindowLong( wnd, GWL_EXSTYLE ) | WS_EX_LAYERED );
	SetWindowLong( wnd, GWL_STYLE, GetWindowLong( wnd, GWL_STYLE ) & ~WS_BORDER );
	SetWindowPos( wnd, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE );
	SetLayeredWindowAttributes( wnd, RGB(0,0,0), 255, LWA_COLORKEY );
*/

	// Draw color ring
	m_bgColor = m_panel->GetBackgroundColour();
	SetBackgroundColour( m_bgColor );
	m_backImage.SetRGB( wxRect( 0, 0, PICKER_WIDTH-1, PICKER_HEIGHT-1 ), m_bgColor.Red(), m_bgColor.Green(), m_bgColor.Blue() );
	DrawColorCircle( m_backImage, WHEEL_CX, WHEEL_CY, PICKER_COLOR_RADIUS );

	// Show the shit
	Layout();
	Refresh();
}

CEdColorPicker::~CEdColorPicker()
{
}

void CEdColorPicker::Show( Color initialColor )
{
	SetPosition( CalcPickerPosition() );

	m_color = m_initialColor = initialColor;

	// Decompose color
	m_rgb[0] = (Float)initialColor.R / 255.0f;
	m_rgb[1] = (Float)initialColor.G / 255.0f;
	m_rgb[2] = (Float)initialColor.B / 255.0f;
	m_a      = (Float)initialColor.A / 255.0f;
	RGBToHSL( m_rgb[0], m_rgb[1], m_rgb[2], m_hsl[0], m_hsl[1], m_hsl[2] );

	m_red.  UpdateValue( m_color.R, NULL, true );
	m_green.UpdateValue( m_color.G, NULL, true );
	m_blue. UpdateValue( m_color.B, NULL, true );
	m_alpha.UpdateValue( m_color.A, NULL, true );

	wxFrame::Show();
}

void CEdColorPicker::OnActivate( wxActivateEvent& event )
{
	// Hide window when deactivated
	if ( !event.GetActive() )
	{
		Hide();
	}
}

void CEdColorPicker::OnEraseBackground( wxEraseEvent& event )
{

}

void CEdColorPicker::OnPaint( wxPaintEvent& event )
{
	wxBufferedPaintDC dc( this );

	// Clear background
	dc.SetBackground( wxBrush( m_bgColor ) );
	dc.Clear();

	// Draw light ring
	DrawLightRing( m_backImage, WHEEL_CX, WHEEL_CY, PICKER_COLOR_RADIUS, PICKER_COLOR_RADIUS+PICKER_RING_RADIUS );

	// Create a bitmap from the color data and blit it
	wxBitmap blitBitmap( m_backImage, 24 ); 
	dc.DrawBitmap( blitBitmap, 0, WHEEL_TOP_OFFSET );

	//if( m_rgb[0]+m_rgb[1]+m_rgb[2] < 1.5f )
	//	dc.SetPen( wxPen( wxColour(255,255,255) ) );
	//else
	//	dc.SetPen( wxPen( wxColour( 1, 1, 1 ) ) );

	// Draw color selector
	{
		dc.SetPen( wxPen( wxColour( 1, 1, 1 ) ) );
		float r, g, b;
		HSLToRGB( m_hsl[0], m_hsl[1], 1.0f, r, g, b );
		dc.SetBrush( wxBrush( wxColour( 255.0f * r, 255.0f * g, 255.0f * b ) ) );
		dc.DrawEllipse( CalcColorDragRect() );
	}

	wxColour clr( m_color.R, m_color.G, m_color.B );

	// Draw shade selector
	{
		Uint8 gray = 224 - 192.0f * ( 0.3f*m_rgb[0]+0.55f*m_rgb[1]+0.15f*m_rgb[2] );
		dc.SetPen( wxPen( wxColour( gray, gray, gray ) ) );
		dc.SetBrush( wxBrush( clr ) );
		dc.DrawEllipse( CalcLightnessDragRect() );
	}

	// Draw selected/initial color
	{
		dc.SetPen( wxColour( 0, 0, 0 ) );
		dc.SetBrush( wxNullBrush );
		dc.DrawRectangle( 160, WHEEL_TOP_OFFSET + 25, 50, 136 );

		dc.SetPen( wxPen( clr ) );
		dc.SetBrush( wxBrush( clr ) );
		dc.DrawRectangle( 161, WHEEL_TOP_OFFSET + 26, 48, 67 );

		wxColour initClr( m_initialColor.R, m_initialColor.G, m_initialColor.B );

		dc.SetPen( wxPen( initClr ) );
		dc.SetBrush( wxBrush( initClr ) );
		dc.DrawRectangle( 161, WHEEL_TOP_OFFSET + 93, 48, 67 );
	}

	wxFrame::OnPaint( event );
}

void CEdColorPicker::DrawColorCircle( wxImage &image, Int32 x, Int32 y, Int32 radius )
{
	const Int32 extraRadius = radius+1;
	for ( Int32 dy=-extraRadius; dy<=extraRadius; dy++ )
	{
		for ( Int32 dx=-extraRadius; dx<=extraRadius; dx++ )
		{
			Float dist = sqrtf( dx*dx + dy*dy );
			Float angle = atan2f( dx, -dy ) / ( 2*M_PI );
			if( angle < 0.0f )
				angle += 1.0f;
			
			if ( dist <= radius )
			{
				Float r, g, b;
				Float frac = dist / (Float)radius;
				HSLToRGB( angle, frac, 1.0f, r, g, b );
				image.SetRGB( x+dx, y+dy, 255.0f*r, 255.0f*g, 255.0f*b );
			}
		}
	}
}

void CEdColorPicker::DrawLightRing( wxImage &image, Int32 x, Int32 y, Int32 radiusInner, Int32 radiusOuter )
{
	const Int32 extraRadius = radiusOuter+1;
	for ( Int32 dy=-extraRadius; dy<=extraRadius; dy++ )
	{
		for ( Int32 dx=-extraRadius; dx<=extraRadius; dx++ )
		{
			Float dist = sqrtf( dx*dx + dy*dy );
			if ( radiusInner <= dist && dist <= radiusOuter )
			{
				Float angle = ( M_PI - atan2f( dy, -dx ) ) / ( 2*M_PI );
				if ( PICKER_LIGHT_RING_ANG_MIN <= angle && angle <= PICKER_LIGHT_RING_ANG_MAX )
				{
					Float r, g, b;
					Float angleFrac = ( angle - PICKER_LIGHT_RING_ANG_MIN ) / ( PICKER_LIGHT_RING_ANG_MAX - PICKER_LIGHT_RING_ANG_MIN );
					HSLToRGB( m_hsl[0], m_hsl[1], angleFrac, r, g, b );
					image.SetRGB( x+dx, y+dy, 255.0f*r, 255.0f*g, 255.0f*b );
				}
			}
		}
	}
}

wxRect CEdColorPicker::CalcColorDragRect()
{
	const Float rad = PICKER_COLOR_RADIUS * m_hsl[1];
	const Float ang = 2.0f * M_PI * m_hsl[0];
	const Int32 rx = WHEEL_CX + rad * sinf( ang );
	const Int32 ry = WHEEL_CY + WHEEL_TOP_OFFSET - rad * cosf( ang );
	return wxRect( rx-PICKER_DRAG_RADIUS, ry-PICKER_DRAG_RADIUS, 2*PICKER_DRAG_RADIUS, 2*PICKER_DRAG_RADIUS );
}

wxRect CEdColorPicker::CalcLightnessDragRect()
{
	const Float rad = PICKER_COLOR_RADIUS + PICKER_RING_RADIUS * 0.5f;
	const Float ang = PICKER_LIGHT_RING_ANG_MIN + m_hsl[2] * ( PICKER_LIGHT_RING_ANG_MAX - PICKER_LIGHT_RING_ANG_MIN );
	const Int32 rx = WHEEL_CX + rad * cosf( ang * 2.0f * M_PI );
	const Int32 ry = WHEEL_CY + WHEEL_TOP_OFFSET + rad * sinf( ang * 2.0f * M_PI );
	return wxRect( rx-PICKER_DRAG_RADIUS, ry-PICKER_DRAG_RADIUS, 2*PICKER_DRAG_RADIUS, 2*PICKER_DRAG_RADIUS );
}

void CEdColorPicker::OnMouseClick( wxMouseEvent& event )
{
	if( event.LeftDown() && m_dragMode == DM_None )
	{
		if ( CalcColorDragRect().Contains( event.GetPosition() ) )
		{
			m_dragMode = DM_Color;
			SetCapture( (HWND)GetHWND() );
			return;
		}

		if ( CalcLightnessDragRect().Contains( event.GetPosition() ) )
		{
			m_dragMode = DM_Lightness;
			SetCapture( (HWND)GetHWND() );
			return;
		}

		if( ClickedColor( event ) )
		{
			UpdateColor();
			m_dragMode = DM_Color;
			SetCapture( (HWND)GetHWND() );
			return;
		}

		if( ClickedLightness( event ) )
		{
			UpdateColor();
			m_dragMode = DM_Lightness;
			SetCapture( (HWND)GetHWND() );
			return;
		}
	}

	if ( event.LeftUp() && m_dragMode != DM_None )
	{
		m_dragMode = DM_None;
		ReleaseCapture();
	}
}

bool CEdColorPicker::ClickedColor( wxMouseEvent& event )
{
	const Int32 dx = event.GetPosition().x - WHEEL_CX;
	const Int32 dy = event.GetPosition().y - WHEEL_CY - WHEEL_TOP_OFFSET;
	//LOG ( TXT("dx: %d dy: %d"), dx, dy );
	const Float dist = sqrtf( dx*dx + dy*dy );
	if( dist > PICKER_COLOR_RADIUS )
		return false;
	Float angle = atan2f( dx, -dy ) / ( 2*M_PI );
	if( angle < 0.0f )
		angle += 1.0f;
	m_hsl[0] = angle;
	m_hsl[1] = dist / (Float)PICKER_COLOR_RADIUS;
	return true;
}

bool CEdColorPicker::ClickedLightness( wxMouseEvent& event, bool exact )
{
	const Int32 dx = event.GetPosition().x - WHEEL_CX;
	const Int32 dy = event.GetPosition().y - WHEEL_CY - WHEEL_TOP_OFFSET;
	const Float dist = sqrtf( dx*dx + dy*dy );
	if( !exact || ( dist > PICKER_COLOR_RADIUS && dist <= PICKER_COLOR_RADIUS + PICKER_RING_RADIUS ) )
	{
		const Float angle = ( M_PI - atan2f( dy, -dx ) ) / ( 2*M_PI );
		if( angle >= PICKER_LIGHT_RING_ANG_MIN && angle <= PICKER_LIGHT_RING_ANG_MAX )
		{
			m_hsl[2] = Clamp( ( angle - PICKER_LIGHT_RING_ANG_MIN ) / ( PICKER_LIGHT_RING_ANG_MAX - PICKER_LIGHT_RING_ANG_MIN ), 0.0f, 1.0f );
			return true;
		}
	}
	return false;
}

void CEdColorPicker::OnMouseMove( wxMouseEvent& event )
{
	if ( m_dragMode != DM_None )
	{
		// Color bar drag
		if ( m_dragMode == DM_Color )
		{
			if ( !ClickedColor( event ) )
				return;
		}

		// Lightness drag
		if ( m_dragMode == DM_Lightness )
		{
			if ( !ClickedLightness( event, false ) )
				return;
		}

		UpdateColor();
	}
}

void CEdColorPicker::UpdateColor()
{
	// Update picked color
	HSLToRGB( m_hsl[0], m_hsl[1], m_hsl[2], m_rgb[0], m_rgb[1], m_rgb[2] );

	m_color = Color( 255.0f * m_rgb[0], 255.0f * m_rgb[1], 255.0f * m_rgb[2], 255.0f * m_a );

	m_red.  UpdateValue( m_color.R, NULL, true );
	m_green.UpdateValue( m_color.G, NULL, true );
	m_blue. UpdateValue( m_color.B, NULL, true );
	m_alpha.UpdateValue( m_color.A, NULL, true );

	Refresh( false );
	wxCommandEvent fake( wxEVT_COMMAND_SCROLLBAR_UPDATED );
	ProcessEvent( fake );
}

void CEdColorPicker::AfterRGBSpinSliderChange()
{
	m_color = Color( m_red.GetValue(), m_green.GetValue(), m_blue.GetValue(), m_alpha.GetValue() );
	m_rgb[0] = (Float)m_color.R/255.0f;
	m_rgb[1] = (Float)m_color.G/255.0f;
	m_rgb[2] = (Float)m_color.B/255.0f;
	m_a      = (Float)m_color.A/255.0f;
	RGBToHSL( m_rgb[0], m_rgb[1], m_rgb[2], m_hsl[0], m_hsl[1], m_hsl[2] );
	
	Refresh( false );
	wxCommandEvent fake( wxEVT_COMMAND_SCROLLBAR_UPDATED );
	ProcessEvent( fake );
}

void CEdColorPicker::OnColorRedChanged( wxCommandEvent &event )
{
	if( m_red.GetValue() != 255.0f*m_rgb[0] )
	{
		AfterRGBSpinSliderChange();
	}
}

void CEdColorPicker::OnColorGreenChanged( wxCommandEvent &event )
{
	if( m_green.GetValue() != 255.0f*m_rgb[1] )
	{
		AfterRGBSpinSliderChange();
	}
}

void CEdColorPicker::OnColorBlueChanged( wxCommandEvent &event )
{
	if( m_blue.GetValue() != 255.0f*m_rgb[2] )
	{
		AfterRGBSpinSliderChange();
	}
}

void CEdColorPicker::OnColorAlphaChanged( wxCommandEvent &event )
{
	if( m_alpha.GetValue() != 255.0f*m_a )
	{
		AfterRGBSpinSliderChange();
	}
}
