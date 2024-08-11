/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "viewportWidgetLines.h"
#include "../../common/engine/renderVertices.h"


// Viewport widget that symbolizes rotating around given axis
class CViewportWidgetRotateAxis : public CViewportWidgetLines
{
protected:

	const EulerAngles m_rotationMask;
	CFont *m_font;
	Matrix m_widgetRotation;
	Vector m_axis;
	Vector m_tangent;
	EulerAngles m_pivotRotation;
	EulerAngles m_pivotMovedRotation;
	EulerAngles m_pivotSnappedRotation;
	EulerAngles m_initialRotation;
	EulerAngles m_localRotation;
	Float m_radius;
	Float m_startAngle;
	Float m_deltaAngle;
	Bool m_firstMove;
	Bool m_disableSnapping;
	Bool m_screenSpace;

public:
	CViewportWidgetRotateAxis( const EulerAngles& rotation, const Vector& axis, const Color &axisColor, Float radius = 1.0f );
	
	// creates screen-space version
	CViewportWidgetRotateAxis( const Color &axisColor, Float radius = 1.0f );

	virtual Matrix CalcLocalToWorld() const override;

	// Activate action
	virtual Bool Activate() override;

	// Deactivate
	virtual void Deactivate() override;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;

	// Generate rendering fragments
	virtual Bool GenerateFragments( CRenderFrame *frame, Bool isActive ) override;

private:

	Vector GetRotateAxis() const;

	Float GetAngle(const Vector &origin, const Vector &axis, const Vector &rayOrigin, const Vector &rayDirection, Vector &tangent) const;

	void DrawPie( CRenderFrame *frame, Float startAngle, Float deltaAngle );

	// Add circle to the widget
	void CreateCircleLines( TDynArray<Vector> &points, const Vector &axis, Float radius) const;

	// Prepare lines for rendering
	void PrepareLines( const CRenderFrameInfo &frameInfo, TDynArray< DebugVertex > &outLines, const Color &color );

	// Returns number of segments used to draw a circle
	RED_INLINE Uint32 GetCircleSegmentsCount() const { return 90; }

};
