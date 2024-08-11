/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Simple window capable of rendering 3D scene
class CEdRenderingWindow : public wxPanel
{
	DECLARE_EVENT_TABLE();

protected:
	ViewportHandle		m_viewport;

public:
	CEdRenderingWindow( wxWindow* parent );
	~CEdRenderingWindow();

	RED_INLINE ViewportHandle GetViewport() { return m_viewport; }

private:
	// Events
	void OnEraseBackground( wxEraseEvent &event );
	void OnPaint(wxPaintEvent& event);
	void OnSize( wxSizeEvent& event );
};

