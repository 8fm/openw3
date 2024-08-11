/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/fonts.h"

CGatheredResource resGizmoTextFont( TXT("gameplay\\gui\\fonts\\arial.w2fnt"), RGF_Startup );

CViewportWidgetRotateAxis::CViewportWidgetRotateAxis( const EulerAngles& rotation, const Vector& axis, const Color &axisColor, Float radius /*= 1.0f*/ )
	: CViewportWidgetLines( axisColor, TXT("Rotate"), true )
	, m_rotationMask( rotation )
	, m_font( nullptr )
	, m_axis( axis )
	, m_radius( radius )
	, m_startAngle( 0.f )
	, m_deltaAngle( 0.f )
	, m_firstMove( false )
	, m_disableSnapping( false )
	, m_screenSpace( false )
{
	m_font = resGizmoTextFont.LoadAndGet< CFont >();

	CreateCircleLines( m_points, m_axis, m_radius );
}

CViewportWidgetRotateAxis::CViewportWidgetRotateAxis( const Color &axisColor, Float radius /*= 1.0f*/ )
	: CViewportWidgetLines( axisColor, TXT("Rotate"), true )
	, m_rotationMask( EulerAngles( 1.f, 0.f, 0.f ) )
	, m_font( nullptr )
	, m_axis( Vector::EY )
	, m_radius( radius )
	, m_startAngle( 0.f )
	, m_deltaAngle( 0.f )
	, m_firstMove( false )
	, m_disableSnapping( false )
	, m_screenSpace( true )
{
	m_font = resGizmoTextFont.LoadAndGet< CFont >();

	CreateCircleLines( m_points, m_axis, m_radius );

	for ( Int32 i = m_points.SizeInt() - 1; i >= 0; i-=2 ) // make dashed line
	{
		m_points.Erase( m_points.Begin() + i );
	}
}

Matrix CViewportWidgetRotateAxis::CalcLocalToWorld() const
{
	Matrix mat = __super::CalcLocalToWorld();

	if ( m_screenSpace )
	{
		Matrix matrix = GetManager()->GetViewport()->GetCameraRotation().ToMatrix();
		Vector origin = mat.GetTranslation();
		matrix.SetTranslation( origin );
		return matrix;
	}
	else
	{
		return mat;
	}
}

Vector CViewportWidgetRotateAxis::GetRotateAxis() const 
{ 
	if ( m_screenSpace )
	{
		return CalcLocalToWorld().GetRow( 1 );
	}
	else
	{
		return m_axis;
	}
}

Float CViewportWidgetRotateAxis::GetAngle(const Vector &origin, const Vector &axis, const Vector &rayOrigin, const Vector &rayDirection, Vector &tangent) const
{
	const Matrix space = axis.ToEulerAngles().ToMatrix();
	const Vector u = space.GetAxisX();
	const Vector v = space.GetAxisZ();

	Float a0 = 0.f;
	Float c0 = cos( a0 );
	Float s0 = sin( a0 );
	Vector startVector = u * c0 + v * s0;
	startVector.Normalize3();

	const Vector normal = (Vector::Dot3( axis, rayDirection ) < 0.f ? axis : - axis);
	Plane plane(normal, origin);

	Vector intersectionPoint;
	Float intersectionDistance;
	if (plane.IntersectRay(rayOrigin, rayDirection, intersectionPoint, intersectionDistance))
	{
		Vector endVector = intersectionPoint - origin;
		endVector.Normalize3();

		Float angle = acos( Vector::Dot3(startVector, endVector) );
		Vector right = Vector::Cross(startVector, axis);
		tangent = Vector::Cross(axis, endVector);

		if (Vector::Dot3(right, endVector) < 0.f)
		{
			angle = 2 * M_PI - angle;
		}

		return angle;
	}

	tangent = Vector::ZEROS;
	return 0.0f;
}

Bool CViewportWidgetRotateAxis::OnViewportMouseMove( const CMousePacket& packet )
{
	CRenderFrameInfo frameInfo( packet.m_viewport );

	if (m_firstMove)
	{
		m_firstMove = false;

		const Vector worldOrigin = CalcLocalToWorld().GetTranslation();
		const Vector axis = m_widgetRotation.TransformVector( GetRotateAxis() );
		Vector tangent;
		m_startAngle = GetAngle(worldOrigin, axis, packet.m_rayOrigin, packet.m_rayDirection, tangent);

		// Project 3D tangent vector onto a screen plane
		Vector screenOrigin;
		frameInfo.ProjectPoints(&worldOrigin, &screenOrigin, 1);
		const Vector worldPoint = worldOrigin + tangent;
		Vector screenPoint;
		frameInfo.ProjectPoints(&worldPoint, &screenPoint, 1);
		m_tangent = screenPoint - screenOrigin;
		m_tangent.Normalize2();
	}

	// Rotate
	Matrix matrix = CalcLocalToWorld();
	Float move = (m_tangent.X * packet.m_dx + m_tangent.Y * packet.m_dy) * /*m_lastRenderScale **/ 0.2f;
	if ( move )
	{
		Matrix localToWorld = CalcLocalToWorld();

		// Update angle
		m_deltaAngle -= DEG2RAD(move);

		// Accumulate move
		m_localRotation += m_rotationMask * move;
		EulerAngles localSnapped = m_disableSnapping ? m_localRotation : wxTheFrame->GetGridSettings().Snap( m_localRotation, m_rotationMask );
		m_localRotation -= localSnapped;

		Matrix wolrdDeltaMatrix = localToWorld.FullInverted() * localSnapped.ToMatrix() * localToWorld;
		m_pivotMovedRotation += wolrdDeltaMatrix.ToEulerAngles();

		// Snap
		EulerAngles snapped = m_pivotMovedRotation;
		if ( snapped != m_pivotSnappedRotation )
		{
			// Calculate movement step
			EulerAngles rotateDelta = snapped - m_pivotRotation;
			m_pivotSnappedRotation = snapped;
			m_pivotRotation = snapped;

			CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

			if ( rendPanel->GetWorld() != nullptr )
			{
				rendPanel->GetTransformManager()->RotateSelection( rotateDelta, RIM_IS_KEY_DOWN( IK_Alt ) );
			}
		}
	}

	// Filtered
	return true;
}

Bool CViewportWidgetRotateAxis::Activate()
{
	if ( !__super::Activate() )
	{
		return false;
	}

	m_firstMove = true;
	m_deltaAngle = 0.f;

	// Get widget space
	CEdRenderingPanel* rendPanel = GetManager()->GetRenderingPanel();

	if ( rendPanel->GetWorld() )
	{
		m_localRotation = EulerAngles::ZEROS;
		m_widgetRotation = CalcLocalToWorld().SetTranslation( 0, 0, 0 );
		m_pivotMovedRotation = rendPanel->GetSelectionManager()->GetPivotRotation();
		m_pivotSnappedRotation = wxTheFrame->GetGridSettings().Snap( m_pivotMovedRotation, m_rotationMask );
		m_pivotRotation = m_pivotMovedRotation;
		m_initialRotation = m_pivotMovedRotation;
	}

	return true;
}

void CViewportWidgetRotateAxis::Deactivate()
{
	m_initialRotation = m_pivotRotation;
	m_startAngle = 0.f;
	m_deltaAngle = 0.f;
	__super::Deactivate();
}

// Generate rendering fragments
Bool CViewportWidgetRotateAxis::GenerateFragments( CRenderFrame *frame, Bool isActive )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_Gizmo ) ) ) return false;

	CViewportWidgetLines::GenerateFragments( frame, isActive );

	// Draw pie
	if (isActive)
	{
		// PaweL: show rotating pies when user works in global space only
		// (in local space pies are not displayed correctly)
		if ( GetManager()->GetWidgetSpace() == RPWS_Global )
		{
			// Snap
			DrawPie(frame, m_startAngle, m_deltaAngle);
		}
	}
	
	// Draw text
	if (isActive)
	{
		Vector worldOrigin = CalcLocalToWorld().GetTranslation() + frame->GetFrameInfo().m_camera.GetCameraUp() * m_radius * m_lastRenderScale;
		Vector screenOrigin;
		frame->GetFrameInfo().ProjectPoints(&worldOrigin, &screenOrigin, 1);
		EulerAngles deltaRotation = m_pivotSnappedRotation - m_initialRotation;
		String text = String::Printf(TXT("[%.0f, %.0f, %.0f]"), deltaRotation.Pitch, deltaRotation.Roll, deltaRotation.Yaw);
		if ( m_font )
		{
			Int32 x, y;
			Uint32 width, height;
			m_font->GetTextRectangle(text, x, y, width, height);
			//frame->AddDebugScreenText((Int32)screenOrigin.X - width / 2, (Int32)screenOrigin.Y - height - 5, text, Color::YELLOW, m_font);
			m_font->Print( frame, (Int32)screenOrigin.X - width / 2, (Int32)screenOrigin.Y - height - 5, 0.5f, text.AsChar(), Color::YELLOW );
		}
	}

	return true;
}

void CViewportWidgetRotateAxis::DrawPie( CRenderFrame *frame, Float startAngle, Float deltaAngle )
{
	if (MAbs(deltaAngle) < 0.005f)
	{
		// Nothing to draw
		return;
	}

	Int32 startSegment = (Int32)(GetCircleSegmentsCount() * startAngle / (2 * M_PI));
	Int32 numSegments = (Int32)(GetCircleSegmentsCount() * deltaAngle / (2 * M_PI));

	// Make sure at least one segment is drawn
	if (numSegments == 0)
	{
		numSegments = (deltaAngle < 0.f ? -1 : 1);
	}

	// Make sure numSegments > 0
	if (numSegments < 0)
	{
		numSegments = -numSegments;
		startSegment = (startSegment - numSegments);

		if (startSegment < 0)
		{
			Int32 n = abs(startSegment) / GetCircleSegmentsCount();
			startSegment += (n + 1) * GetCircleSegmentsCount();
		}
	}
	startSegment %= GetCircleSegmentsCount();

	Color pieColor = m_lineColor;
	pieColor.A = 50;
	TDynArray< DebugVertex > points;
	TDynArray< Uint16 > indices;

	// Get world space matrix
	Vector worldOrigin = CalcLocalToWorld().GetTranslation();
	Matrix localToWorld = m_widgetRotation;
	localToWorld.SetTranslation( worldOrigin );

	// Get origin
	points.Insert(0, DebugVertex(worldOrigin, pieColor));

	// Transform lines
	for (Uint32 i = 0; i <= (Uint32)numSegments; i++) 
	{
		const Uint32 id = i + startSegment;
		const Uint32 index = (2 * id) % m_points.Size();
		Vector worldPoint = localToWorld.TransformPoint( m_points[index] * m_lastRenderScale );
		points.PushBack(DebugVertex(worldPoint, pieColor));

		if (i > 0)
		{
			indices.PushBack(0);
			indices.PushBack(i);
			indices.PushBack(i + 1);

			indices.PushBack(0);
			indices.PushBack(i + 1);
			indices.PushBack(i);
		}
	}

	if (indices.Size() > 0)
	{
		//frame->AddDebugTriangles(Matrix::IDENTITY, &points[0], points.Size(), &indices[0], indices.Size(), pieColor);
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, &points[0], points.Size(), &indices[0], indices.Size(), RSG_DebugOverlay );
	}
}

void CViewportWidgetRotateAxis::CreateCircleLines( TDynArray<Vector> &points, const Vector &axis, Float radius ) const
{
	// Generate arrow
	const Matrix space = axis.ToEulerAngles().ToMatrix();
	const Vector u = space.GetAxisX() * radius;
	const Vector v = space.GetAxisZ() * radius;

	// Circle lines
	const Uint32 circleSegments = GetCircleSegmentsCount();
	const Float invCircleSegments = 1.f / (Float)circleSegments;
	for ( Uint32 i = 0; i < circleSegments; i++ )
	{
		// Start point
		Float a0 = 2.0f * M_PI * (i * invCircleSegments);
		Float c0 = cos( a0 );
		Float s0 = sin( a0 );
		Vector start = u * c0 + v * s0;

		// End point
		Float a1 = 2.0f * M_PI * ((i + 1) * invCircleSegments);
		Float c1 = cos( a1 );
		Float s1 = sin( a1 );
		Vector end = u * c1 + v * s1;

		// Point
		points.PushBack(start);
		points.PushBack(end);
	}
}

void CViewportWidgetRotateAxis::PrepareLines( const CRenderFrameInfo &frameInfo, TDynArray< DebugVertex > &outLines, const Color &color )
{
	// Get world space matrix
	Matrix localToWorld = CalcLocalToWorld();
	Vector origin = localToWorld.GetTranslation();
	// Get render scale
	Float scale = frameInfo.CalcScreenSpaceScale( origin );
	m_lastRenderScale = scale;

	if ( m_screenSpace )
	{
		// Transform lines
		for ( Uint32 i = 0; i < m_points.Size(); i++ ) 
		{
			Vector point = localToWorld.TransformPoint( m_points[i] * scale );
			outLines.PushBack( DebugVertex( point, color ) );
		}
	}
	else
	{
		// Get clipping plane
		Vector normal = origin - frameInfo.m_camera.GetPosition();
		normal.Normalize3();
		Plane clipPlane(-normal, origin);

		// Transform lines
		for ( Uint32 i = 1; i < m_points.Size(); i++ )
		{
			Vector outPoint1 = localToWorld.TransformPoint( m_points[i - 1] * scale );
			Vector outPoint2 = localToWorld.TransformPoint( m_points[i] * scale );

			// Display only the lines which are in front of the clipPlane
			if (clipPlane.DistanceTo( outPoint1 ) >= 0.f && clipPlane.DistanceTo( outPoint2 ) >= 0.f)
			{
				outLines.PushBack( DebugVertex( outPoint1, color ) );
				outLines.PushBack( DebugVertex( outPoint2, color ) );
			}
		}
	}
}

