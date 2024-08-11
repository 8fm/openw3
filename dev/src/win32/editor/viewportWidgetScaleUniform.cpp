/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/renderFragment.h"

CViewportWidgetScaleUniform::CViewportWidgetScaleUniform( const Vector &scaleAxis, const Vector &scaleAxisTangent, const Vector &scaleAxisBitangent, const Color &axisColor )
	: CViewportWidgetLines( axisColor, TXT("Scale"), false )
	, m_scaleAxis( scaleAxis )
	, m_scaleAxisTangent( scaleAxisTangent )
	, m_scaleAxisBitangent( scaleAxisBitangent )
{
	// Generate wedge corner
	const Float len = 0.4f;
	Vector u = m_scaleAxisTangent * len;
	Vector v = m_scaleAxisBitangent * len;
	Vector w = m_scaleAxis * len;

	// Widget lines
	AddLine( u, v );
	AddLine( v, w );
	AddLine( w, u );

	m_polyPoints.PushBack(u);
	m_polyPoints.PushBack(v);
	m_polyPoints.PushBack(w);
}

Bool CViewportWidgetScaleUniform::OnViewportMouseMove( const CMousePacket& packet )
{
	// Get local movement axis
	Matrix matrix = CalcLocalToWorld();
	Vector scaleAxis = matrix.TransformVector( m_scaleAxis );
	Vector axisTangent = matrix.TransformVector( m_scaleAxisTangent );
	Vector axisBitangent = matrix.TransformVector( m_scaleAxisBitangent );

	// Calculate movement
	CRenderFrameInfo frameInfo( packet.m_viewport );
	Float move = CalcMovement( frameInfo, scaleAxis, packet.m_dx, packet.m_dy );
	Float moveTangent = CalcMovement( frameInfo, axisTangent, packet.m_dx, packet.m_dy );
	Float moveBiTangent = CalcMovement( frameInfo, axisBitangent, packet.m_dx, packet.m_dy );

	Float moveTotal = move + moveBiTangent + moveTangent;

	if ( move )
	{
		// Move along movement plane
		Vector delta = ( scaleAxis + axisTangent + axisBitangent ) * moveTotal * m_lastRenderScale;
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

Bool CViewportWidgetScaleUniform::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	return true;
}

void CViewportWidgetScaleUniform::Deactivate()
{
	__super::Deactivate();
}

Bool CViewportWidgetScaleUniform::GenerateFragments( CRenderFrame *frame, Bool isActive )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_Gizmo ) ) ) return false;

	CViewportWidgetLines::GenerateFragments( frame, isActive );

	if (isActive)
	{
		Matrix localToWorld = CalcLocalToWorld();
		TDynArray< DebugVertex > points;
		Uint16 indices[6] = { 0, 1, 2, 2, 1, 0 };
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
