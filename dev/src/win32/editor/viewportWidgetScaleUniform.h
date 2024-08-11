/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Viewport widget that symbolizes scaling in two axes
class CViewportWidgetScaleUniform : public CViewportWidgetLines
{
protected:
	Vector		m_scaleAxis;
	Vector		m_scaleAxisTangent;
	Vector		m_scaleAxisBitangent;
	TDynArray< Vector > m_polyPoints;

public:
	CViewportWidgetScaleUniform( const Vector &scaleAxis, const Vector &scaleAxisTangent, const Vector &scaleAxisBitangent, const Color &axisColor );

	// Activate action
	virtual Bool Activate() override;

	// Deactivate
	virtual void Deactivate() override;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;

	// Generate rendering fragments
	virtual Bool GenerateFragments( CRenderFrame *frame, Bool isActive );
};
