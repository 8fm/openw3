/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/renderFragment.h"

CViewportWidgetMovePlane::CViewportWidgetMovePlane( const Vector &axisTangent, const Vector &axisBitangent, const Color &axisColor )
	: CViewportWidgetLines( axisColor, TXT("Move"), true )
	, m_moveAxisTangent( axisTangent )
	, m_moveAxisBitangent( axisBitangent )
{
	// Generate wedge corner
	const Float len = 0.4f;
	Vector u = m_moveAxisTangent * len;
	Vector v = m_moveAxisBitangent * len;
	m_snapMask = m_moveAxisTangent + m_moveAxisBitangent;

	// widget lines
	AddLine( u, u + v );
	AddLine( v, u + v );

	m_polyPoints.PushBack(Vector::ZERO_3D_POINT);
	m_polyPoints.PushBack(u);
	m_polyPoints.PushBack(u + v);
	m_polyPoints.PushBack(v);
}

Bool CViewportWidgetMovePlane::OnViewportMouseMove( const CMousePacket& packet )
{
	// Calculate movement
	CRenderFrameInfo frameInfo( packet.m_viewport );

	Vector moveAxisTangent = CalcLocalAxis( m_moveAxisTangent );
	Vector moveAxisBitangent = CalcLocalAxis( m_moveAxisBitangent );
	
	// Find a normal vector for desired plane
	Vector normal = Vector::Cross(moveAxisTangent, moveAxisBitangent);
	if (Vector::Dot3(normal, packet.m_rayDirection) > 0.f)
	{
		normal = -normal;
	}

	// Find a plane
	Plane plane(normal, m_pivotPosition);

	// Find an intersection point for the plane and the ray direction
	Vector intersectionPoint;
	Float intersectionDistance;
	if (plane.IntersectRay(packet.m_rayOrigin, packet.m_rayDirection, intersectionPoint, intersectionDistance))
	{
		if (m_firstMove)
		{
			m_firstMove = false;
			m_initialOffset = intersectionPoint - m_initialPosition;
		}

		Vector movedPosition = intersectionPoint - m_initialOffset;

		// Snap
		Vector snapped;
		Vector snapMask = CalcLocalToWorld().TransformVector( m_snapMask );
		if ( wxTheFrame->GetGridSettings().m_usePositionGridLength )
		{
			Vector preSnapMoveDelta = movedPosition - m_initialPosition;
			snapped = m_initialPosition + wxTheFrame->GetGridSettings().SnapLength( preSnapMoveDelta );
			if ( snapMask.X == 0.0f ) snapped.X = m_initialPosition.X;
			if ( snapMask.Y == 0.0f ) snapped.Y = m_initialPosition.Y;
			if ( snapMask.Z == 0.0f ) snapped.Z = m_initialPosition.Z;
		}
		else
		{
			snapped = wxTheFrame->GetGridSettings().Snap( movedPosition, snapMask );
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

Bool CViewportWidgetMovePlane::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	m_firstMove = true;

	// Get widget space
	CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

	if ( rendPanel->GetWorld() )
	{
		m_initialOffset = Vector::ZEROS;
		m_initialPosition = rendPanel->GetSelectionManager()->GetPivotPosition();
		if ( wxTheFrame->GetGridSettings().m_usePositionGridLength )
		{
			m_pivotSnappedPosition = m_initialPosition;
		}
		else
		{
			m_pivotSnappedPosition = wxTheFrame->GetGridSettings().Snap( m_initialPosition, m_snapMask );
		}
		m_pivotPosition = m_initialPosition;
	}

	return true;
}

void CViewportWidgetMovePlane::Deactivate()
{
	__super::Deactivate();
}

Bool CViewportWidgetMovePlane::GenerateFragments( CRenderFrame *frame, Bool isActive )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_Gizmo ) ) ) return false;

	CViewportWidgetLines::GenerateFragments( frame, isActive );

	if (isActive)
	{
		Matrix localToWorld = CalcLocalToWorld();
		TDynArray< DebugVertex > points;
		Uint16 indices[12] = { 0, 1, 2, 2, 3, 0,   0, 3, 2, 2, 1, 0 };
		Color polyColor = Color::YELLOW;
		polyColor.A = 50;

		for (Uint32 i = 0; i < m_polyPoints.Size(); ++i)
		{
			Vector outPoint = localToWorld.TransformPoint( m_polyPoints[i] * m_lastRenderScale );
			points.PushBack(DebugVertex(outPoint, polyColor));
		}

		//frame->AddDebugTriangles(Matrix::IDENTITY, &points[0], points.Size(), &indices[0], sizeof(indices) / sizeof(Uint16), polyColor);
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, &points[0], points.Size(), &indices[0], sizeof(indices) / sizeof(Uint16), RSG_DebugOverlay );
	}

	return true;
}
