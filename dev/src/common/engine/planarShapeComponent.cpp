/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderFragment.h"
#include "planarShapeComponent.h"
#include "hitProxyId.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CPlanarShapeComponent );

CPlanarShapeComponent::CPlanarShapeComponent()
{
}

Color CPlanarShapeComponent::CalcLineColor() const
{
	return Color::GREEN;
}

Color CPlanarShapeComponent::CalcFaceColor() const
{
	// Make sheet more transparent
	Color color = Color::GREEN;
	color.A = 80;
	return color;
}

Float CPlanarShapeComponent::CalcLineWidth() const
{
	return 1.0f;
}

Bool CPlanarShapeComponent::HasPlanarConstraint() const
{
	return true;
}

Bool CPlanarShapeComponent::HasConvexConstraint() const
{
	return true;
}

void CPlanarShapeComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	// Pass to base class
	TBaseClass::OnSpawned( spawnInfo );

	// Add 4 quad corners
    if ( m_localPoints.Empty() )
    {
        m_localPoints.PushBack( Vector( -0.5f, -0.5f, 0.0f ) );
        m_localPoints.PushBack( Vector( +0.5f, -0.5f, 0.0f ) );
        m_localPoints.PushBack( Vector( +0.5f, +0.5f, 0.0f ) );
        m_localPoints.PushBack( Vector( -0.5f, +0.5f, 0.0f ) );
    }
}

void CPlanarShapeComponent::RenderPolygon( CRenderFrame* frame )
{
	// Do not render lines when rendering hit proxies in vertex edit mode
	if ( m_vertices.Size() && frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
		return;
	}

	// Hit proxy mode, render fat lines
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
#ifndef NO_COMPONENT_GRAPH
		Color color = GetHitProxyID().GetColor();

		// Render fat contours
		for ( Uint32 i=0; i<m_worldPoints.Size(); i++ )
		{
			// Base
			const Vector a = m_worldPoints[ i ];
			const Vector b = m_worldPoints[ (i+1)%m_worldPoints.Size() ];
			frame->AddDebugFatLine( a, b, color, 0.1f );
		}
#endif
		// Done
		return;
	}

	// Get color
	Color color = CalcLineColor();
	if ( !IsSelected() )
	{
		color.R /= 2;
		color.G /= 2;
		color.B /= 2;
	}

	// Alpha
	color.A = 255;

	// Draw lines
	{
		TDynArray< DebugVertex > vertices;
		vertices.Resize( m_worldPoints.Size() * 2 );

		// Draw contour
		DebugVertex* write = vertices.TypedData();		
		for ( Uint32 i=0; i<m_worldPoints.Size(); i++ )
		{
			// Line segment
			(write++)->Set( m_worldPoints[ i ], color );
			(write++)->Set( m_worldPoints[ (i+1)%m_worldPoints.Size() ], color );
		}

		// Draw lines
		const Uint32 numVertices = PtrDiffToUint32( ( void* )( write - &vertices[0] ) );
		ASSERT( numVertices <= vertices.Size() );
		frame->AddDebugLines( vertices.TypedData(), numVertices, false );
	}

	// Face
	color = CalcFaceColor();
	{
		TDynArray< DebugVertex > vertices;
		TDynArray< Uint16 > indices;
		vertices.Resize( m_worldPoints.Size() * 2 );
		indices.Resize( (m_worldPoints.Size()-2) * 6 );

		// Fill vertices
		DebugVertex* vertex = vertices.TypedData();		
		for ( Uint32 i=0; i<m_worldPoints.Size(); i++, vertex++ )
		{
			vertex->Set( m_worldPoints[ i ], color );
		}

		// Fill indices
		Uint16* index = indices.TypedData();
		for ( Uint32 i=2; i<m_worldPoints.Size(); i++, index+=6 )
		{
			index[0] = 0;
			index[1] = (Uint16)(i-1);
			index[2] = (Uint16)(i);
			index[3] = (Uint16)(i);
			index[4] = (Uint16)(i-1);
			index[5] = 0;
		}

		// Render face
		new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), RSG_DebugTransparent );
	}
}

void CPlanarShapeComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CPlanarShapeComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );

	// Transform points to world space
	m_worldPoints.Resize( m_localPoints.Size() );
	for ( Uint32 i=0; i<m_localPoints.Size(); i++ )
	{
		m_worldPoints[i] = GetLocalToWorld().TransformPoint( m_localPoints[i] );
	}
}

void CPlanarShapeComponent::OnUpdateBounds()
{
	m_boundingBox.Clear();
	for( Uint32 i = 0; i < m_worldPoints.Size(); ++i )
	{
		m_boundingBox.AddPoint( m_worldPoints[i] );
	}
}

#ifndef NO_EDITOR

Bool CPlanarShapeComponent::OnEditorBeginVertexEdit( TDynArray< Vector >& vertices, Bool& isClosed, Float& height )
{
	vertices = m_worldPoints;
	isClosed = true;
	height   = 0.f;

	// Allow vertex edit
	return true;
}

void CPlanarShapeComponent::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	if ( ! MarkModified() )
	{
		allowedPosition = oldPosition;
		return;
	}

	ASSERT( vertexIndex >= 0 );
	ASSERT( (Uint32) vertexIndex < m_localPoints.Size() );

	Matrix worldToLocal;
	GetWorldToLocal( worldToLocal );

	// Keep the planar constraint
	if ( HasPlanarConstraint() )
	{
		// Project on plane
		Vector projected = worldToLocal.TransformPoint( wishedPosition );
		allowedPosition = GetLocalToWorld().TransformPoint( Vector( projected.X, projected.Y, 0.0f ) );
	}
	else
	{
		// Allow any position
		allowedPosition = wishedPosition;
	}

	// Update vertex
	m_worldPoints[ vertexIndex ] = allowedPosition;
	m_localPoints[ vertexIndex ] = worldToLocal.TransformPoint( allowedPosition );

	// Update transform
	ScheduleUpdateTransformNode();
}

Bool CPlanarShapeComponent::OnEditorVertexDestroy( Int32 vertexIndex )
{
	// To few vertices already
	if ( m_localPoints.Size() <= 3 )
	{
		return false;
	}
	if ( vertexIndex < 0 || (Uint32)vertexIndex >= m_localPoints.Size() )
	{
		return false;
	}

	// Remove from vertices list
	m_localPoints.Erase( m_localPoints.Begin() + vertexIndex );
	m_worldPoints.Erase( m_worldPoints.Begin() + vertexIndex );

	// Update transform
	ScheduleUpdateTransformNode();

	// Deleted
	return true;
}

Bool CPlanarShapeComponent::OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos )
{
	Uint32 numEdges = m_worldPoints.Size();

	while ( edge < 0 ) { edge += numEdges; }

	// Insert between first and second edge vertex 
	Uint32 insertPos = ( edge + 1 ) % numEdges;

	Matrix worldToLocal;
	GetWorldToLocal( worldToLocal );

	// Keep the planar constraint
	if ( HasPlanarConstraint() )
	{
		// Project on plane
		Vector projected = worldToLocal.TransformPoint( wishedPosition );
		allowedPosition = GetLocalToWorld().TransformPoint( Vector( projected.X, projected.Y, 0.0f ) );
	}
	else
	{
		// Allow any position
		allowedPosition = wishedPosition;
	}

	// Add point
	m_worldPoints.Insert( insertPos, allowedPosition );
	m_localPoints.Insert( insertPos, worldToLocal.TransformPoint( allowedPosition ) );
	outInsertPos = insertPos;

	// Update transform
	ScheduleUpdateTransformNode();

	return true;
}
#endif

Int32 CPlanarShapeComponent::FindClosestEdge( const Vector& point )
{
	Int32 closestEdge = -1;
	Float closestDistance = FLT_MAX;

	// Test distance to edges
	for ( Uint32 i=0; i<m_worldPoints.Size(); i++ )
	{
		// Get edge points
		Vector a = m_worldPoints[i];
		Vector b = m_worldPoints[(i+1)%m_worldPoints.Size()];

		// Calculate distance from edge to point
		Float distanceToEdge = point.DistanceToEdge( a, b );
		if ( distanceToEdge < closestDistance )
		{
			closestDistance = distanceToEdge;
			closestEdge = i;
		}
	}

	// Return first vertex of closest edge 
	return closestEdge;
}


Bool CPlanarShapeComponent::IsConvex( const TDynArray< Vector >& points, const Vector& normal )
{
	return true;
}
