/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Viewport widget that symbolizes moving in given axis
class CViewportWidgetMoveAxis : public CViewportWidgetLines
{
protected:
	Vector		m_moveAxis;
	Vector		m_pivotPosition;
	Vector		m_pivotSnappedPosition;
	Vector		m_initialPosition;
	Float		m_initialOffset;
	Bool		m_firstMove;

	Bool GetMostPerpendicularPlane(const Vector &forward, const Vector &axis, const Vector &point, Plane &plane);

public:
	CViewportWidgetMoveAxis( const Vector &axis, const Color &axisColor );

	// Activate action
	virtual Bool Activate() override;

	// Deactivate
	virtual void Deactivate() override;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;
};
