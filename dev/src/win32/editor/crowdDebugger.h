#pragma once

#include "maraudersMapCanvas.h"

class CCrowdDebuggerCanvas : public CMaraudersMapCanvasDrawing
{
	DECLARE_EVENT_TABLE();

protected:
	EngineTime	m_lastRegionUpdateTime;
	Vector		m_currentRegionPos;

public:
	static const Vector DEFAULT_HALF_EXTENTS;

	CCrowdDebuggerCanvas( wxWindow* parent );

	virtual void PaintCanvas( Int32 width, Int32 height );

	void UpdateRegion( const Vector& pos );
	void DrawBoxCanvas( const Box& worldBox, const wxColour& color, Float width = 1.0f );
};


class CEdCrowdDebugger : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE()

	CCrowdDebuggerCanvas*	m_canvas;
	CEdTimer*				m_timer;

public:
	CEdCrowdDebugger( wxWindow* parent );
	~CEdCrowdDebugger();

	// ISavableToConfig interface
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	// Menu
	void OnExit( wxCommandEvent &event );
	void OnClose( wxCloseEvent &event );

	void OnTimer( wxCommandEvent &event );
};