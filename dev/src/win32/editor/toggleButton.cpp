#include "build.h"
#pragma hdrstop

#include "toggleButton.h"

BEGIN_EVENT_TABLE( CEdToggleButton, wxPanel )
	EVT_PAINT( CEdToggleButton::OnPaint )
	EVT_SIZE( CEdToggleButton::OnSize )
	EVT_ERASE_BACKGROUND( CEdToggleButton::OnEraseBackground )
	EVT_LEFT_DOWN( CEdToggleButton::OnMouseClick )
	EVT_LEFT_UP( CEdToggleButton::OnMouseClick )
	EVT_MOTION( CEdToggleButton::OnMouseMove )
END_EVENT_TABLE()

CEdToggleButton::CEdToggleButton( wxWindow* parent, const wxPoint &position, const wxSize& size, const wxString& caption )
	: wxPanel( parent, wxID_ANY, position, size )
	, m_drawFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) )
	, m_caption( caption )
	, m_color( 255, 200, 64 )
	, m_isToggled( false )
	, m_isPressed( false )
	, m_isMouseOver( false )
	, m_isAutoToggle( false )
{
	// Setup bold font
	m_boldFont = m_drawFont;
	m_boldFont.SetWeight( wxFONTWEIGHT_BOLD );
}

void CEdToggleButton::SetToggle( Bool toggle )
{
	m_isToggled = toggle;
	Refresh( false );
}

void CEdToggleButton::SetAutoToggle( Bool autoToggle )
{
	m_isAutoToggle = autoToggle;
}

void CEdToggleButton::SetCaption( const wxString& string )
{
	m_caption = string;
	Refresh( false );
}

void CEdToggleButton::SetDrawFont( const wxFont& font )
{
	m_drawFont = font;
	m_boldFont = m_drawFont;
	m_boldFont.SetWeight( wxFONTWEIGHT_BOLD );
	Refresh( false );
}

void CEdToggleButton::SetColor( const wxColor& color )
{
	m_color = color;
	Refresh( false );
}

void CEdToggleButton::OnSize( wxSizeEvent& event )
{
	Refresh();
}

void CEdToggleButton::OnPaint( wxPaintEvent& event )
{
	wxBufferedPaintDC dc( this );
	PrepareDC( dc );

	// Get system colors
	wxColour faceColor = wxSystemSettings::GetColour( wxSYS_COLOUR_3DFACE );
	wxColour shadowColor = wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW );
	wxColour lightColor = wxSystemSettings::GetColour( wxSYS_COLOUR_3DHIGHLIGHT );

	// Clear background
	dc.SetBackground( wxBrush( m_isToggled ? m_color : faceColor ) );
	dc.Clear();

	// Get client area size
	INT width, height;
	GetClientSize( &width, &height );

	// Set background color
	INT textOffset;
	if ( m_isToggled )
	{
		textOffset = 0;
		dc.SetFont( m_boldFont );
		dc.SetBrush( wxBrush( m_color ) );
	}
	else
	{
		textOffset = 0;
		dc.SetFont( m_drawFont );
		dc.SetBrush( wxBrush( faceColor ) );
	}

	// Draw border
	if ( m_isToggled || ( m_isPressed && m_isMouseOver ) )
	{
		dc.SetPen( wxPen( shadowColor ) );
		dc.DrawLine( 0, 0, width, 0 );
		dc.DrawLine( 0, 0, 0, height );
		dc.SetPen( wxPen( lightColor ) );
		dc.DrawLine( width - 1, 0, width - 1, height );
		dc.DrawLine( 0, height - 1, width, height - 1);
	}
	else
	{
		dc.SetPen( wxPen( lightColor ) );
		dc.DrawLine( 0, 0, width, 0 );
		dc.DrawLine( 0, 0, 0, height );
		dc.SetPen( wxPen( shadowColor ) );
		dc.DrawLine( width - 1, 0, width - 1, height );
		dc.DrawLine( 0, height - 1, width, height - 1);
	}

	// Draw caption
	wxRect textSize( dc.GetTextExtent( m_caption ) );
	wxRect textDrawRect = textSize.CenterIn( wxRect( 0, 0, width, height ) );
	dc.DrawText( m_caption, wxPoint( textOffset, textOffset ) + textDrawRect.GetTopLeft() );
}

void CEdToggleButton::OnEraseBackground( wxEraseEvent& event )
{
	// Do not repaint background
}

void CEdToggleButton::OnMouseClick( wxMouseEvent& event )
{
	// Left clicked
	if ( event.LeftDown() )
	{
		// Begin input capture
		m_isMouseOver = true;
		m_isPressed = true;
		SetCapture( (HWND) GetHandle() );
	}

	// Left button released
	if ( event.LeftUp() )
	{
		// Reset state
		m_isPressed = false;
		ReleaseCapture();
		Refresh( false );

		// Send event 
		if ( m_isMouseOver )
		{
			// Reset flag
			m_isMouseOver = false;

			// Send event
			if ( m_isAutoToggle )
			{
				m_isToggled = !m_isToggled;
			}

			// Post toggle event
			wxCommandEvent event( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED );
			event.SetEventObject( this );
			event.SetInt( m_isToggled );
			wxPostEvent( this, event );
		}
	}

	// Redraw
	Refresh( false );
}

void CEdToggleButton::OnMouseMove( wxMouseEvent& event )
{
	Bool isOver = GetClientRect().Contains( event.GetPosition() );
	if ( isOver != m_isMouseOver )
	{
		m_isMouseOver = isOver;
		Refresh( false );
	}
}
