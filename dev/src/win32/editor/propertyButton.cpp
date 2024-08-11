/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CPropertyButton::CPropertyButton( CBasePropItem* owner, const wxBitmap& bitmap, Uint32 width )
	: m_bitmap( bitmap )
	, m_width( width )
	, m_item( owner )
{
}

bool CPropertyButton::IsSelected() const
{
	return m_item->GetPage()->GetActiveButton() == this;
}

void CPropertyButton::UpdateLayout( Int32 &xOffset, Int32 y, Int32 height )
{
	m_rect.x = xOffset - m_width;
	m_rect.y = y + 1;
	m_rect.height = height - 2;
	m_rect.width = m_width;
	xOffset -= (m_width+1);
}

void CPropertyButton::DrawLayout( wxDC& dc )
{
	const wxColour back( 255, 255, 255 );
	const wxColour backh( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	const wxColour frame( 128, 128, 128 );

	// Draw frame
	dc.SetPen( wxPen( frame ) );
	dc.SetBrush( wxBrush( IsSelected() ? backh : back ) );
	dc.DrawRoundedRectangle( m_rect, 1 );

	// Draw icon
	wxRect iconRect( 0, 0, m_bitmap.GetWidth(), m_bitmap.GetHeight() );
	iconRect = iconRect.CenterIn( m_rect );
	dc.DrawBitmap( m_bitmap, iconRect.x, iconRect.y, true );
}

void CPropertyButton::Clicked()
{
	wxCommandEvent fake( wxEVT_COMMAND_BUTTON_CLICKED );
	wxEvtHandler::ProcessEvent( fake );
}
