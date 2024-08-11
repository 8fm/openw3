/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct SCursorPoint
{
	SCursorPoint( Float x, Float y )
		: m_x(x)
		, m_y(y)
	{}

	SCursorPoint()
		: m_x(-1.f)
		, m_y(-1.f)
	{}

	Bool IsValid() const { return m_x >= 0.f && m_y >= 0.f; }

	Float m_x;
	Float m_y;
};

class CHardwareCursorManager
{
private:
	Bool m_stateRequestedByGame;
	Bool m_stateRequestedByViewport;
	Bool m_updateCursorVisibilityPending;
	Float m_lastSentMouseX;
	Float m_lastSentMouseY;
	SCursorPoint m_requestedMousePos;

public:
	CHardwareCursorManager();

	// Delay the update so mouse move requests that comes right after showing the cursor can be accumulated
	RED_INLINE void Game_RequestHardwareCursor( Bool r ) { m_stateRequestedByGame = r; m_updateCursorVisibilityPending = true; }
	void Viewport_RequestHardwareCursor( Bool r );

	void OnMouseMove( Float x, Float y );
	void Update();

public:
	void RequestMouseMove( const SCursorPoint& point ) { m_requestedMousePos = point; }
	void ClearRequestMouseMove() { m_requestedMousePos = SCursorPoint(); }
	const SCursorPoint& GetRequestMouseMove() const { return m_requestedMousePos; }
};

extern CHardwareCursorManager GHardwareCursorManager;
