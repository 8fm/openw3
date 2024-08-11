/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdDraggablePanel : public wxPanel
{
	wxDECLARE_CLASS( CEdDraggablePanel );
	wxDECLARE_EVENT_TABLE();

protected: 
	
	Bool					m_isHandMode;
	Bool					m_isDragging;
	wxPoint					m_prevMousePosition;
	wxPoint					m_scrollPosition;	
	Bool					m_isScrollBarInitialized;

public: 
	CEdDraggablePanel();

protected:

	virtual bool Layout();
	virtual WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam);

	void OnPaint( wxPaintEvent& event );
	void SetInternalWindowPos( const wxPoint& pos );
	
	wxRect GetScrollBarRect();
	wxRect GetScrollBarButtonRect();
	Float GetScrollRatio();

	friend class CEdVScrollStatusBar;
};
