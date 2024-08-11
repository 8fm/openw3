/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CViewportWidgetMoveAxis::CViewportWidgetMoveAxis( const Vector &axis, const Color &axisColor )
	: CViewportWidgetLines( axisColor, TXT("Move"), true )
	, m_moveAxis( axis )
{
	// Generate arrow
	const Float len = 1.0f;
	Matrix space = axis.ToEulerAngles().ToMatrix();
	Vector head = space.GetAxisY() * len;
	Vector base = head * 0.8f;
	Vector u = space.GetAxisX() * len * 0.05f;
	Vector v = space.GetAxisZ() * len * 0.05f;

	// Arrow lines
	AddLine( Vector::ZEROS, head );
	AddLine( head, base + u + v );
	AddLine( head, base + u - v );
	AddLine( head, base - u + v );
	AddLine( head, base - u - v );
}

/*Bool CViewportWidgetMoveAxis::FindClosestPointsBetweenLines(const Vector &p0, const Vector &u, const Vector &q0, const Vector &v, Float &sc, Vector &p1, Float &tc, Vector &q1)
{
	Vector w0 = p0 - q0;
	Float a = Vector::Dot3(u, u);
	Float b = Vector::Dot3(u, v);
	Float c = Vector::Dot3(v, v);
	Float d = Vector::Dot3(u, w0);
	Float e = Vector::Dot3(v, w0);
	Float D = a * c - b * b;
	ASSERT(a >= 0);
	ASSERT(c >= 0);
	ASSERT(D >= 0);

	// Compute the line parameters of two closest points
	Bool isParallel = false;
	if (D < 0.001f)
	{
		// The lines are almost parallel
		isParallel = true;

		sc = 0.0f;
		tc = (b > c) ? (d / b) : (e / c);	// use the largest denominator
	}
	else
	{
		sc = (b * e - c * d) / D;
		tc = (a * e - b * d) / D;
	}

	p1 = p0 + u * sc;
	q1 = q0 + v * tc;

	return isParallel;
}*/

Bool CViewportWidgetMoveAxis::GetMostPerpendicularPlane( const Vector &forward, const Vector &axis, const Vector &point, Plane &plane )
{
	const Float dot = Vector::Dot3( forward, axis );
	//if ( fabsf( dot ) < 0.99f )
	{
		Vector cross = Vector::Cross( forward, axis );
		cross.Normalize3();
		Vector normal = Vector::Cross( cross, axis );
		normal.Normalize3();
		plane = Plane( normal, point );
		return true;
	}

	return false;
}

Bool CViewportWidgetMoveAxis::OnViewportMouseMove( const CMousePacket& packet )
{
	// Calculate movement
	CRenderFrameInfo frameInfo( packet.m_viewport );
	Vector moveAxis = CalcLocalAxis( m_moveAxis );

	Plane plane;
	if (GetMostPerpendicularPlane(frameInfo.m_camera.GetCameraForward(), moveAxis, m_initialPosition, plane))
	{
		Vector movedPosition;
		Vector intersectionPoint;
		Float intersectionDistance;
		if (plane.IntersectRay(packet.m_rayOrigin, packet.m_rayDirection, intersectionPoint, intersectionDistance))
		{
			if (m_firstMove)
			{
				m_firstMove = false;
				m_initialOffset = Vector::Dot3(intersectionPoint - m_initialPosition, moveAxis);
			}

			// Cast intersectionPoint onto the moveAxis
			Float delta = Vector::Dot3(intersectionPoint - m_initialPosition, moveAxis) - m_initialOffset;
			movedPosition = m_initialPosition + moveAxis * delta;
		}
		else
		{
			movedPosition = m_initialPosition;
		}

		// Snap
		Vector snapped;
		if ( wxTheFrame->GetGridSettings().m_usePositionGridLength )
		{
			Vector preSnapMoveDelta = movedPosition - m_initialPosition;
			snapped = m_initialPosition + wxTheFrame->GetGridSettings().SnapLength( preSnapMoveDelta );
		}
		else
		{
			snapped = wxTheFrame->GetGridSettings().Snap( movedPosition, moveAxis );
		}
		if ( snapped != m_pivotSnappedPosition )
		{
			// Calculate movement step
			Vector moveDelta = snapped - m_pivotPosition;
			m_pivotSnappedPosition = snapped;
			m_pivotPosition = snapped;

			CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

			if ( rendPanel->GetWorld() != nullptr )
			{
				// Move along movement axis
				if ( RIM_IS_KEY_DOWN( IK_LControl ) )
				{
					Vector pivotPosition = rendPanel->GetSelectionManager()->GetPivotPosition();
					pivotPosition += moveDelta;
					rendPanel->GetSelectionManager()->SetPivotPosition( pivotPosition );
				}
				else
				{
					rendPanel->GetTransformManager()->MoveSelection( moveDelta );
				}
			}
		}
	}
	// Filtered
	return true;
}

Bool CViewportWidgetMoveAxis::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	m_firstMove = true;
	
	if ( CWorld* activeWorld = GetManager()->GetRenderingPanel()->GetWorld() )
	{
		m_initialOffset = 0.f;
		m_initialPosition = activeWorld->GetSelectionManager()->GetPivotPosition();
		if ( wxTheFrame->GetGridSettings().m_usePositionGridLength )
		{
			m_pivotSnappedPosition = m_initialPosition;
		}
		else
		{
			m_pivotSnappedPosition = wxTheFrame->GetGridSettings().Snap( m_initialPosition, m_moveAxis );
		}
		m_pivotPosition = m_initialPosition;
	}
	return true;
}

void CViewportWidgetMoveAxis::Deactivate()
{
	__super::Deactivate();
}

