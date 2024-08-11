/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

///////////////////////////////////////////////////////////////////////////////

class CAIHistoryDebugCanvas;
class CMaraudersMapItemBase;

///////////////////////////////////////////////////////////////////////////////

struct SAIEvent
{
	Uint32			m_trackIdx;
	String			m_name;
	String			m_description;
	Float			m_startTime;
	Float			m_endTime;
	EAIEventResult	m_result;

	SAIEvent( Uint32 trackIdx = -1, const String& name = TXT( "" ), const String& description = TXT( "" ), Float startTime = 0 );

	void Draw( CAIHistoryDebugCanvas& canvas ) const;

	Bool IsInRange( Float time, Int32 trackIdx ) const;

	Bool IsInRange( CAIHistoryDebugCanvas& canvas, Uint32 trackPos, Int32 trackIdx ) const;
};

///////////////////////////////////////////////////////////////////////////////

class CAIHistoryDebugCanvas : public CEdCanvas
{
	DECLARE_EVENT_TABLE();

private:
	CMaraudersMap*						m_parent;
	Gdiplus::Font*						m_drawFont;

	Float								m_scaleWorld;
	const Float							m_trackHeight;
	const Float							m_headersWidth;
	Float								m_viewStartOffset;
	Int32									m_currWidth;

	Bool								m_scrolling;
	Bool								m_syncWithTime;

public:
	CAIHistoryDebugCanvas( wxWindow* parentWindow, CMaraudersMap* parent );
	~CAIHistoryDebugCanvas();

	void PaintCanvas( Int32 width, Int32 height );

	// ------------------------------------------------------------------------
	// Drawing helper API
	// ------------------------------------------------------------------------
	RED_INLINE Gdiplus::Font& GetFont() { return *m_drawFont; }

	RED_INLINE Float GetTrackOffset( Uint32 idx ) const { return idx * ( m_trackHeight + 2.0f ); }

	RED_INLINE const Float GetTrackHeight() const { return m_trackHeight; }

	// Returns a point on the canvas
	Float WorldToCanvas( Float time ) const;

	// Returns the time that corresponds to the canvas point
	Float CanvasToWorld( Float point ) const;

	// ------------------------------------------------------------------------
	// Mouse events
	// ------------------------------------------------------------------------
	void MouseMove( wxMouseEvent& event, wxPoint delta );
	void MouseClick( wxMouseEvent& event );
	void OnMouseWheel( wxMouseEvent& event );
	void OnCharPressed( wxKeyEvent& event );

private:
	void DisplayDescription( const wxPoint& pos );
};

///////////////////////////////////////////////////////////////////////////////
