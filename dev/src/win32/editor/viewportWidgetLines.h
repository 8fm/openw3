/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "viewportWidgetBase.h"
#include "../../common/engine/renderVertices.h"

/// Viewport widget based on lines
class CViewportWidgetLines : public CViewportWidgetBase
{
protected:
	TDynArray< Vector >	m_points;
	Color				m_lineColor;
	Float				m_lastRenderScale;

public:
	CViewportWidgetLines( const Color& lineColor, const String& groupName, Bool enableDuplicateOnStart );

	// Check collision with widget
	virtual Bool CheckCollision( const CRenderFrameInfo &frameInfo, const wxPoint &mousePos ) override;

	// Generate rendering fragments
	virtual Bool GenerateFragments( CRenderFrame *frame, Bool isActive ) override;

	// Set mouse cursor
	virtual Bool SetCursor() override;

protected:
	// Add line to widget
	void AddLine( const Vector &start, const Vector &end );

	// Prepare lines for rendering
	virtual void PrepareLines( const CRenderFrameInfo &frameInfo, TDynArray< DebugVertex > &outLines, const Color &color );
};