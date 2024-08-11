/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

BEGIN_EVENT_TABLE( CEdDraggedSpinButton, wxSpinButton )
	EVT_MOTION( CEdDraggedSpinButton::OnMouseEvent )
END_EVENT_TABLE()

CEdDraggedSpinButton::CEdDraggedSpinButton(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
: wxSpinButton( parent, id, pos, size, style, name )
, m_mode( MODE_NORMAL )
, m_precision( 0.1f )
{
}

bool CEdDraggedSpinButton::MSWOnScroll( int orientation, WXWORD wParam, WXWORD pos, WXHWND control )
{
	//LOG_EDITOR( TXT("MSWOnScroll %d %d"), wParam, (int) control );

	// button pressed
	if ( wParam == SB_THUMBPOSITION )
	{
		if ( m_mode == MODE_NORMAL )
		{
			m_mode = MODE_CHECKMOUSE;
			m_mousePressedPosition = wxGetMousePosition();
		}
	}

	// button released
	if ( wParam == SB_ENDSCROLL )
	{
		SetCursor( wxCURSOR_DEFAULT );
		m_mode = MODE_NORMAL;
	}

	return wxSpinButton::MSWOnScroll( orientation, wParam, pos, control );
}

bool CEdDraggedSpinButton::MSWOnNotify( int idCtrl, WXLPARAM lParam, WXLPARAM *result )
{
	NM_UPDOWN *lpnmud = (NM_UPDOWN *)lParam;
	//LOG_EDITOR( TXT("MSWOnNotify %d"), lParam );

	if ( m_mode == MODE_DRAGGED )
	{
		lpnmud->iDelta = 0;		
	}

	return wxSpinButton::MSWOnNotify( idCtrl, lParam, result );
}

void CEdDraggedSpinButton::OnMouseEvent( wxMouseEvent& event )
{
	//LOG_EDITOR( TXT("OnMouseEvent %d"), event.GetPosition().y );

	wxPoint newMousePosition = wxGetMousePosition();
	wxPoint mouseDelta =  newMousePosition - m_mousePressedPosition;

	if ( m_mode == MODE_CHECKMOUSE )
	{
		if ( wxPoint2DInt( mouseDelta ).GetVectorLength( ) > 10 )
		{
			m_mode = MODE_DRAGGED;
			m_floatValue = GetValue();
			m_mousePressedPosition = newMousePosition;
			SetCursor( wxCURSOR_SIZENS );
		}
	}

	if ( m_mode == MODE_DRAGGED )
	{
		wxRect screen = wxGetClientDisplayRect();

		// teleport mouse cursor on screen edges
		if ( newMousePosition.y < 1 )
		{
			 newMousePosition.y = screen.height-1;
			 SetCursorPos( newMousePosition.x, newMousePosition.y );
		}

		if ( newMousePosition.y > screen.height-1 )
		{
			newMousePosition.y = 1;
			SetCursorPos( newMousePosition.x, newMousePosition.y );
		}

		m_mousePressedPosition = newMousePosition;

		// 1 percent of current value per pixel
		Float stepPerPix = Abs(m_floatValue) / 100.0f;
		stepPerPix =  Max <Float> ( stepPerPix, m_precision );

		if ( IsVertical() )
		{
			m_floatValue -= (Float) mouseDelta.y * stepPerPix;
		}
		else
		{
			m_floatValue -= (Float) mouseDelta.x * stepPerPix;
		}

		// clamp to range
		m_floatValue = Clamp<Float>( m_floatValue, GetMin(), GetMax() );		
		SetValue( m_floatValue );

		// send inform event
		wxScrollEvent evt;
		evt.SetEventType( wxEVT_SCROLL_THUMBTRACK );		
		AddPendingEvent( evt );
	}
}

