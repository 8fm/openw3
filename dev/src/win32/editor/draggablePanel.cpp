/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// vertical scroll width
#define VS_WIDTH 8

wxIMPLEMENT_CLASS( CEdDraggablePanel, wxPanel );

//////////////////////////////////////////////////////////////////////////
BEGIN_EVENT_TABLE( CEdDraggablePanel, wxPanel )
	EVT_PAINT( CEdDraggablePanel::OnPaint )
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////
CEdDraggablePanel::CEdDraggablePanel() 
: wxPanel()
, m_isHandMode ( false )
, m_isDragging ( false )
, m_isScrollBarInitialized( false )
{
	// for manual erasing background using buffered DC
	SetBackgroundStyle( wxBG_STYLE_CUSTOM );
}

//////////////////////////////////////////////////////////////////////////
wxRect CEdDraggablePanel::GetScrollBarRect()
{
	wxSize size = GetSize();
	return wxRect( size.GetWidth()-VS_WIDTH, 0, VS_WIDTH, size.GetHeight() );
}

//////////////////////////////////////////////////////////////////////////
wxRect CEdDraggablePanel::GetScrollBarButtonRect()
{
	Float ratio = GetScrollRatio();
	wxSize winSize = GetSize();

	wxRect rcRet( winSize.GetWidth() - VS_WIDTH, (Float) -m_scrollPosition.y * ratio, VS_WIDTH, (Float) winSize.GetHeight()* ratio + 1 );
	 
	 if ( rcRet.GetHeight() <= 8 )
	 {
		 rcRet.height = 8;
	 }

	return rcRet;
}

//////////////////////////////////////////////////////////////////////////
Float CEdDraggablePanel::GetScrollRatio()
{
	wxSize winSize = GetSize();
	wxSize realSize = GetSizer()->GetMinSize();
	return (Float) winSize.GetHeight() / (Float) realSize.GetHeight();
}

//////////////////////////////////////////////////////////////////////////
void CEdDraggablePanel::OnPaint( wxPaintEvent& event )
{
	wxAutoBufferedPaintDC dc(this);

	wxSize winSize = GetSize();

	// clear main background
	wxColour bgCol = GetBackgroundColour();
	dc.SetBrush( bgCol );
	dc.SetPen( bgCol );
	wxRegionIterator upd(GetUpdateRegion()); 
	while (upd)
	{
		wxRect rcUpd = upd.GetRect();

		// without scrollbar region
		if ( rcUpd.x > winSize.x  - VS_WIDTH )
		{
			rcUpd.x = winSize.x  - VS_WIDTH;
		}

		dc.DrawRectangle( rcUpd );
		upd ++ ;
	}
	
	// draw vertical scroll bar
	if ( m_isScrollBarInitialized )
	{
		wxSize realSize = GetSizer()->GetMinSize();
		Float ratio = (Float) winSize.GetHeight() / (Float) realSize.GetHeight();

		wxColour lightCol = wxSystemSettings::GetColour( wxSYS_COLOUR_SCROLLBAR );
		wxColour darkCol = wxSystemSettings::GetColour(  wxSYS_COLOUR_3DDKSHADOW );
		
		if ( ratio >= 1)
		{
			dc.SetBrush( lightCol );
			dc.SetPen( lightCol );
			dc.DrawRectangle( GetScrollBarRect());	
		}
		else
		{
			dc.SetBrush( darkCol );
			dc.SetPen( darkCol );
			dc.DrawRectangle( GetScrollBarRect() );
			
			dc.SetBrush( lightCol );
			dc.SetPen( lightCol );
			dc.DrawRectangle( GetScrollBarButtonRect() );
		}

		event.Skip();
	}
}

//////////////////////////////////////////////////////////////////////////
WXLRESULT CEdDraggablePanel::MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam )
{
	// choosing mouse cursor
	if ( nMsg == WM_SETCURSOR )
	{
		HWND hwnd = ( HWND ) wParam;					
		
		bool hand = false;
		wxWindow* wnd = NULL;

		wxSize winSize = GetSize();
		wxSize realSize = GetSizer()->GetMinSize();

		// need to scroll?
		if ( winSize.GetHeight() < realSize.GetHeight() )
		{
			// no hand on vertical scroll bar
			wxPoint clientMousePos = ScreenToClient( wxGetMousePosition() );
			if ( GetScrollBarRect().Contains( clientMousePos ) )
			{
				hand = false;
			}
			else
			{
				// is mouse above place to drag ?
				if ( hwnd == GetHwnd() )
				{
					// above main panel
					wnd = this;
					hand = true;
				}
				else
				{
					// above static child control ?
					wnd = FindItemByHWND( hwnd );
					if ( wnd )
					{
						wxString className = wnd->GetClassInfo()->GetClassName();
						//LOG_EDITOR( TXT("WM_SETCURSOR: %s" ), className.wc_str() );
						if ( className == TXT("wxPanel") || className == TXT("wxStaticText")  )
						{
							hand = true;
						}
					}
				}
			}
		}

		// need to change cursor ?
		if ( hand != m_isHandMode )
		{
			if ( hand )
			{
				SetCursor( wxCURSOR_HAND );			
			}
			else
			{				
				SetCursor(  wxCURSOR_DEFAULT );
			}

			m_isHandMode = hand;
			m_isDragging = false;
			
			//LOG_EDITOR( m_isHandMode ? TXT("hand") : TXT("normal") );
		}
	}

	// scroll using mouse wheel
	if ( nMsg == WM_MOUSEWHEEL )
	{
		wxPoint newScrollPosition = m_scrollPosition;
		newScrollPosition.y += GET_WHEEL_DELTA_WPARAM(wParam);
		SetInternalWindowPos( newScrollPosition );
	}

	// left mouse button click
	if ( nMsg == WM_LBUTTONDOWN || ( nMsg == WM_PARENTNOTIFY && ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) ) )
	{
		m_prevMousePosition = wxGetMousePosition();
		wxPoint clientMousePos = ScreenToClient( wxGetMousePosition() );

		// click for dragging scrollbar ?
		if ( GetScrollBarRect().Contains( clientMousePos ) )
		{
			wxRect rcScrollButon = GetScrollBarButtonRect();

			// mouse is outside scroll button - need to snap
			if ( !rcScrollButon.Contains( clientMousePos ) )
			{
				Float ratio = GetScrollRatio();
				if ( clientMousePos.y <= rcScrollButon.y )
				{
					// above
					SetInternalWindowPos( wxPoint( 0, -clientMousePos.y * 1.0f/ratio ) );
				}
				else
				{
					// bellow					
					SetInternalWindowPos( wxPoint( 0, -( clientMousePos.y - rcScrollButon.GetHeight() ) * 1.0f/ratio ) );
				}
			}

			m_isDragging = true;
			SetCapture( GetHwnd() );
		}
		else
		{
			// click for dragging panel ?
			if ( m_isHandMode )
			{
				m_isDragging = true;
				SetCapture( GetHwnd() );
			}
		}
	}

	// dragging finished ?
	if ( m_isDragging && !( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) )
	{
		m_isDragging = false;
		ReleaseCapture( );
	}

	// handle dragging panel
	if ( m_isDragging && nMsg == WM_MOUSEMOVE )
	{
		wxPoint newMousePosition = wxGetMousePosition();
		wxPoint delta =  newMousePosition - m_prevMousePosition;

		if ( delta.y != 0 )
		{
			if ( m_isHandMode )
			{
				// dragging panel by hand
				SetInternalWindowPos( m_scrollPosition + delta );
			}
			else
			{
				// dragging scroll bar 
				Float ratio = GetScrollRatio();
				delta.y /= ratio;
				SetInternalWindowPos( m_scrollPosition - delta );
			}			
		}

		m_prevMousePosition = newMousePosition;		
	}

	// resizing main panel
	if ( nMsg == WM_SIZE )
	{
		Layout();
		// skip processing this event by wx
		return 0;
	}

	return wxPanel::MSWWindowProc( nMsg, wParam, lParam );
}

//////////////////////////////////////////////////////////////////////////
bool CEdDraggablePanel::Layout()
{
	Freeze();

	// add space for vertical scroll bar on right side of panel
	if ( !m_isScrollBarInitialized && GetSizer() )
	{
		wxBoxSizer* bSizerMain = new wxBoxSizer( wxHORIZONTAL );

		wxBoxSizer* rightSide = new wxBoxSizer( wxVERTICAL );
		rightSide->Add ( new wxWindow( this, wxID_ANY, wxDefaultPosition, wxSize( VS_WIDTH, 0 ) ), 0, 0, 0 );

		bSizerMain->Add( GetSizer(), 1, wxEXPAND, 5 );
		bSizerMain->Add( rightSide, 0, 0, 0 );

		SetSizer( bSizerMain, false );

		m_isScrollBarInitialized = true;
	}

	if( GetSizer() )
	{
		SetVirtualSize( -1, GetSizer()->GetMinSize().y );
	}
	
	wxWindow::Layout();	
	
	// because was reseted by layout
	ScrollWindow( 0, m_scrollPosition.y );

	// fit to new size
	SetInternalWindowPos( m_scrollPosition );
	
	Thaw();

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CEdDraggablePanel::SetInternalWindowPos( const wxPoint& pos )
{
	if (!GetSizer() )
	{
		return;
	}

	// calculate new position
	wxPoint newPos = pos;

	wxSize realSize = GetSizer()->GetMinSize();
	wxSize winSize = GetSize();

	// fit best to panel area
	if ( winSize.GetHeight() >= realSize.GetHeight() )
	{
		newPos.y = 0;
	}
	else
	{
		int dy = realSize.GetHeight() - winSize.GetHeight();
		if ( newPos.y < -dy )
		{
			newPos.y = -dy;
		}
	}
	
	if ( newPos.y > 0 )
	{
		newPos.y = 0;
	}	

	// scroll to new position	
	Int32 dy = newPos.y - m_scrollPosition.y;
	ScrollWindow( 0, dy );

	m_scrollPosition = newPos;

	// repaint scrollbar
	Refresh( false, &GetScrollBarRect() );

	// redraw controls on uncovered area
	Update();
}
