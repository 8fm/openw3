/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CViewportWidgetScaleAxis::CViewportWidgetScaleAxis( const Vector &axis, const Color &axisColor )
	: CViewportWidgetLines( axisColor, TXT("Scale"), false )
	, m_scaleAxis( axis )
{
	// Generate arrow
	const Float len = 1.0f;
	Matrix space = axis.ToEulerAngles().ToMatrix();
	Vector head = space.GetAxisY() * len;
	Vector base = head * 0.9f;
	Vector u = space.GetAxisX() * len * 0.05f;
	Vector v = space.GetAxisZ() * len * 0.05f;

	// Box lines
	AddLine( Vector::ZEROS, base );
	AddLine( base + u + v, head + u + v );
	AddLine( base + u - v, head + u - v );
	AddLine( base - u - v, head - u - v );
	AddLine( base - u + v, head - u + v );
	AddLine( base + u + v, base - u + v );
	AddLine( base - u + v, base - u - v );
	AddLine( base - u - v, base + u - v );
	AddLine( base + u - v, base + u + v );
	AddLine( head + u + v, head - u + v );
	AddLine( head - u + v, head - u - v );
	AddLine( head - u - v, head + u - v );
	AddLine( head + u - v, head + u + v );
}

Bool CViewportWidgetScaleAxis::OnViewportMouseMove( const CMousePacket& packet )
{
	Matrix localToWorld = CalcLocalToWorld();
	localToWorld.SetTranslation( 0.f, 0.f, 0.f );
	Matrix worldToLocal = localToWorld.Inverted();
	Vector axis = CalcLocalAxis( m_scaleAxis );

	// Calculate movement
	CRenderFrameInfo frameInfo( packet.m_viewport );
	Float move = CalcMovement( frameInfo, axis, packet.m_dx, packet.m_dy );
	if ( move )
	{		
		// Move along movement axis
		Vector delta = axis * move;

		CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

		if ( rendPanel->GetWorld() != nullptr )
		{
			Matrix matrix = CalcLocalToWorld();
			Bool individually = RIM_IS_KEY_DOWN( IK_Alt );
			rendPanel->GetTransformManager()->ScaleSelection( delta, matrix, GetManager()->GetWidgetSpace(), individually );
		}
	}

    // Filtered
	return true;
}

Bool CViewportWidgetScaleAxis::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	return true;
}

void CViewportWidgetScaleAxis::Deactivate()
{
	__super::Deactivate();
}
