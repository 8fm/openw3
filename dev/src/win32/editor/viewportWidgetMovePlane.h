/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Viewport widget that symbolizes moving on given plane
class CViewportWidgetMovePlane : public CViewportWidgetLines
{
protected:
	Vector		m_moveAxisTangent;
	Vector		m_moveAxisBitangent;
	Vector		m_snapMask;
	Vector		m_pivotPosition;
	Vector		m_pivotSnappedPosition;
	Vector		m_initialPosition;
	Vector		m_initialOffset;
	Bool		m_firstMove;

	TDynArray< Vector > m_polyPoints;

public:
	CViewportWidgetMovePlane( const Vector &axisTangent, const Vector &axisBitangent, const Color &axisColor );

	// Activate action
	virtual Bool Activate() override;

	// Deactivate
	virtual void Deactivate() override;

	// Input from viewport, return true if you handle the message
	virtual Bool OnViewportMouseMove( const CMousePacket& packet ) override;

	// Generate rendering fragments
	virtual Bool GenerateFragments( CRenderFrame *frame, Bool isActive ) override;
};