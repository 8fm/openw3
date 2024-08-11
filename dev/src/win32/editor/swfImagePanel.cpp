/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "swfImagePanel.h"

BEGIN_EVENT_TABLE( CEdSwfImagePanel, wxPanel )
	EVT_PAINT( CEdSwfImagePanel::OnPaint )
	EVT_SIZE( CEdSwfImagePanel::OnSize )
END_EVENT_TABLE()

CEdSwfImagePanel::CEdSwfImagePanel( wxPanel* parent )
	: wxPanel( parent )
	, m_dirty( false )
{
	SetBackgroundColour( *wxCYAN );
}

CEdSwfImagePanel::~CEdSwfImagePanel()
{
}

void CEdSwfImagePanel::SetSourceImage( const wxImage& sourceImage )
{
	m_sourceImage = sourceImage;
	m_dirty = true;
	Refresh();
}

void CEdSwfImagePanel::OnPaint( wxPaintEvent& event )
{
	TBaseClass::OnPaint( event );

	if ( ! m_sourceImage.IsOk() )
	{
		return;
	}

	wxPaintDC dc(this);
	
	wxSize dcSize = dc.GetSize();

	if ( dcSize.GetWidth() < 1 || dcSize.GetHeight() < 1 )
	{
		return;
	}

	if ( m_sourceImage.GetWidth() < 1 || m_sourceImage.GetHeight() < 1 )
	{
		return;
	}

	if ( m_dirty || m_dcSize != dcSize )
	{
		m_dirty = false;
		m_dcSize = dcSize;

		Int32 scaledWidth = m_sourceImage.GetWidth();
		Int32 scaledHeight = m_sourceImage.GetHeight();

		if (  m_sourceImage.GetHeight() < m_sourceImage.GetWidth() )
		{
			Float heightScale = m_sourceImage.GetHeight() / Float(m_sourceImage.GetWidth());
			scaledWidth = m_dcSize.GetWidth();
			scaledHeight = heightScale * scaledWidth;
		}
		else
		{
			Float widthtScale = m_sourceImage.GetWidth() / Float(m_sourceImage.GetHeight());
			scaledHeight = m_dcSize.GetHeight();
			scaledWidth = widthtScale * scaledHeight;
		}

		m_drawBitmap = wxBitmap( m_sourceImage.Scale( scaledWidth, scaledHeight, wxIMAGE_QUALITY_HIGH ) );
	}
	
	const wxSize size = m_drawBitmap.GetSize();
	const Int32 x = (m_dcSize.GetWidth() - size.GetWidth())/2;
	const Int32 y = (m_dcSize.GetHeight() - size.GetHeight())/2;
	dc.DrawBitmap( m_drawBitmap, x, y, false );
}

void CEdSwfImagePanel::OnSize( wxSizeEvent& event )
{
	Refresh();
	event.Skip();
}
