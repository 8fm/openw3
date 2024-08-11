/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Viewport widget that symbolizes scaling 
class CViewportWidgetScaleAxis : public CViewportWidgetLines
{
protected:
	Vector		m_scaleAxis;

public:
	CViewportWidgetScaleAxis( const Vector &axis, const Color &axisColor );

	// Activate action
	virtual Bool Activate() override;

	// Deactivate
	virtual void Deactivate() override;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;
};