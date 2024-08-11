/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/renderFragment.h"

CViewportWidgetLines::CViewportWidgetLines( const Color& lineColor, const String& groupName, Bool enableDuplicateOnStart )
	: CViewportWidgetBase( groupName, enableDuplicateOnStart )
	, m_lineColor( lineColor )
	, m_lastRenderScale( 1.0f )
{
}

// Calculate point distance to segment
static Float CalcDistanceToSegment( const Vector &p, const Vector &a, const Vector &b )
{
	Vector dir = b - a; 
	Float len = dir.Mag3(); 
	dir = dir.Normalized3();
	Float dot = Vector::Dot3(  p - a, dir );

	if ( dot <= 0.0f )
	{
		return p.DistanceTo( a );
	}
	else if ( dot >= len )
	{
		return p.DistanceTo( b );
	}
	else
	{
		Vector r = a + dir * dot;
		return r.DistanceTo( p );
	} 
}
 
Bool CViewportWidgetLines::CheckCollision( const CRenderFrameInfo &frameInfo, const wxPoint &mousePos )
{
	// Transform rendering lines to frame space
	TDynArray< DebugVertex > lines;
	PrepareLines( frameInfo, lines, Color::BLACK );

	// Grab point list in world space
	TDynArray< Vector > worldSpacePoints;
	Vector worldSpacePoint;
	for ( Uint32 i=0; i<lines.Size(); i++ )
	{
		worldSpacePoint.X = lines[ i ].x;
		worldSpacePoint.Y = lines[ i ].y;
		worldSpacePoint.Z = lines[ i ].z;
		worldSpacePoint.W = 1.0f;
		worldSpacePoints.PushBack( worldSpacePoint );
	}

	// Project points to screen space
	TDynArray< Vector > screenSpacePoints;
	screenSpacePoints.Resize( worldSpacePoints.Size() );
	frameInfo.ProjectPoints( worldSpacePoints.TypedData(), screenSpacePoints.TypedData(), worldSpacePoints.Size() );

	// Convert mouse click to screen space coords
	Vector mousePosVector( mousePos.x, mousePos.y, 0.0f );

	// Check distance to each line in screen space
	const Float minDist = 10.0f;
	for ( Uint32 i=0; i<screenSpacePoints.Size(); i+=2 )
	{
		// Check distance
		const Float dist = CalcDistanceToSegment( mousePosVector, screenSpacePoints[i+0], screenSpacePoints[i+1] );
		if ( dist < minDist )
		{
			return true;
		}
	}

	// No collision
	return false;
}

Bool CViewportWidgetLines::GenerateFragments( CRenderFrame *frame, Bool isActive )
{
	if ( !( frame->GetFrameInfo().IsShowFlagOn( SHOW_Gizmo ) ) ) return false;

	// Transform rendering lines to frame space
	TDynArray< DebugVertex > lines;
	PrepareLines( frame->GetFrameInfo(), lines, isActive ? Color::YELLOW : m_lineColor );

	// Draw lines
	//frame->AddDebugLines( &lines[0], lines.Size(), true );
	new ( frame ) CRenderFragmentDebugLineList( frame, Matrix::IDENTITY, &lines[0], lines.Size(), RSG_DebugOverlay );

	return true;
}

Bool CViewportWidgetLines::SetCursor()
{
	::SetCursor( ::LoadCursor( NULL, IDC_SIZEALL ) );
	return true;
}

void CViewportWidgetLines::AddLine( const Vector &start, const Vector &end )
{
	m_points.PushBack( start );
	m_points.PushBack( end );
}

void CViewportWidgetLines::PrepareLines( const CRenderFrameInfo &frameInfo, TDynArray< DebugVertex > &outLines, const Color &color )
{
	// Get world space matrix
	Matrix localToWorld = CalcLocalToWorld();

	// Get render scale
	Vector origin = localToWorld.GetTranslation();
	Float scale = frameInfo.CalcScreenSpaceScale( origin );
	m_lastRenderScale = scale;

	// Calculate render color
	Uint32 renderColor = (Uint32&)color;
	
	// Transform lines
	outLines.Resize( outLines.Size() + m_points.Size() );
	for ( Uint32 i=0; i<m_points.Size(); i++) 
	{
		Vector outPoint = localToWorld.TransformPoint( m_points[i] * scale );
		Red::System::MemoryCopy( &outLines[ i ], &outPoint, 3 * sizeof( Float ) );
		outLines[ i ].color = renderColor;
	}
}
