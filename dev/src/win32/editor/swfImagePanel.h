/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdSwfImagePanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

private:
	typedef wxPanel TBaseClass;

private:
	wxImage		m_sourceImage;
	wxBitmap	m_drawBitmap;
	wxSize		m_dcSize;
	Bool		m_dirty;

public:
	CEdSwfImagePanel( wxPanel* parent );
	~CEdSwfImagePanel();

public:
	void SetSourceImage( const wxImage& sourceImage );

protected:
//	void OnEraseBackground( wxEraseEvent &event );
	void OnPaint(wxPaintEvent& event);
	void OnSize( wxSizeEvent& event );
};
