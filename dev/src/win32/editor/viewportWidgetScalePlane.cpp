/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/renderFragment.h"

CViewportWidgetScalePlane::CViewportWidgetScalePlane( const Vector &scaleAxisTangent, const Vector &scaleAxisBitangent, const Color &axisColor )
	: CViewportWidgetLines( axisColor, TXT("Scale"), false )
	, m_scaleAxisTangent( scaleAxisTangent )
	, m_scaleAxisBitangent( scaleAxisBitangent )
{
	// Generate wedge corner
	const Float len1 = 0.7f;
	const Float len2 = 0.4f;
	Vector u1 = m_scaleAxisTangent * len1;
	Vector v1 = m_scaleAxisBitangent * len1;
	Vector u2 = m_scaleAxisTangent * len2;
	Vector v2 = m_scaleAxisBitangent * len2;

	// Widget line
	AddLine( u1, v1 );

	m_polyPoints.PushBack( u1 );
	m_polyPoints.PushBack( v1 );
	m_polyPoints.PushBack( v2 );
	m_polyPoints.PushBack( u2 );
}

Bool CViewportWidgetScalePlane::OnViewportMouseMove( const CMousePacket& packet )
{
	// Get local movement axis
	Matrix matrix = CalcLocalToWorld();
	Vector axisTangent = matrix.TransformVector( m_scaleAxisTangent );
	Vector axisBitangent = matrix.TransformVector( m_scaleAxisBitangent );

	// Calculate movement
	CRenderFrameInfo frameInfo( packet.m_viewport );
	Float moveTangent = CalcMovement( frameInfo, axisTangent, packet.m_dx, packet.m_dy );
	Float moveBiTangent = CalcMovement( frameInfo, axisBitangent, packet.m_dx, packet.m_dy );

	Float move = moveBiTangent + moveTangent;
	if ( move )
	{
		axisTangent.Normalize3();
		axisBitangent.Normalize3();
		Vector localScaleAxis = axisTangent + axisBitangent;

		// Move along movement plane
		Vector delta = localScaleAxis * move * m_lastRenderScale;
		CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

		if ( rendPanel->GetWorld() )
		{
			Matrix matrix = CalcLocalToWorld();
			Bool individually = RIM_IS_KEY_DOWN( IK_Alt );
			rendPanel->GetTransformManager()->ScaleSelection( delta, matrix, GetManager()->GetWidgetSpace(), individually );
		}
	}

	// Filtered
	return true;
}

Bool CViewportWidgetScalePlane::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	return true;
}

void CViewportWidgetScalePlane::Deactivate()
{
	__super::Deactivate();
}

Bool CViewportWidgetScalePlane::GenerateFragments( CRenderFrame *frame, Bool isActive )
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

		// frame->AddDebugTriangles(Matrix::IDENTITY, &points[0], points.Size(), &indices[0], sizeof(indices) / sizeof(Uint16), polyColor);
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, &points[0], points.Size(), &indices[0], sizeof(indices) / sizeof(Uint16), RSG_DebugOverlay );
	}

	return true;
}
