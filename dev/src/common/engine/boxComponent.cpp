/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "boxComponent.h"
#include "renderFragment.h"
#include "world.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CBoxComponent );

const Uint16 CBoxComponent::m_indices[]	= 
{ 
	1, 0, 4,    4, 5, 1,
	3, 1, 5,    5, 7, 3,
	0, 2, 6,    6, 4, 0,
	3, 7, 6,    6, 2, 3,
	2, 0, 1,    1, 3, 2,
	5, 4, 6,    6, 7, 5,
};

CBoxComponent::CBoxComponent()
	: m_width( 10 )
	, m_height( 10 )
	, m_depth( 10 )
	, m_drawingColor( Color( 0, 255, 0, 1 ) )
{
}

CBoxComponent::~CBoxComponent()
{

}

void CBoxComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	Color color = m_drawingColor;

	// Hit proxy mode
	if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
	{
#ifndef NO_COMPONENT_GRAPH
		color = GetHitProxyID().GetColor();
#endif
	}
	else
	{
		// Bright when selected
		if( ! IsSelected() )
		{
			color.R /= 2;
			color.G /= 2;
			color.B /= 2;
		}

		// Alpha
		color.A = 80;
	}

	// Create debug vertices
	DebugVertex vertices[ 8 ];
	for( Uint32 i = 0; i < 8; ++i )
	{
		vertices[ i ].Set( m_vertices[ i ], color );
	}

	// Add outline
	frame->AddDebugBox( m_boundingBox, GetLocalToWorld(), color );

	// Draw faces
	new ( frame ) CRenderFragmentDebugPolyList( frame, Matrix::IDENTITY, vertices, 8, m_indices, 6 * 6, RSG_DebugTransparent );
}

void CBoxComponent::OnAttached( CWorld* world )
{
	// Pass to the base class
	TBaseClass::OnAttached( world );

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Sound );
}

void CBoxComponent::OnDetached( CWorld* world )
{
	// Pass to the base class
	TBaseClass::OnDetached( world );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Sound );
}

void CBoxComponent::OnUpdateBounds()
{
	// Update bounding box
	m_boundingBox = Box( Vector::ZEROS, Vector( m_width, m_depth, m_height ) );
	
	// Get box corners
	m_boundingBox.CalcCorners( m_vertices );

	// Transform to world coordinates
	Matrix localToWorld;
	GetLocalToWorld( localToWorld );

	for( Uint32 i = 0; i < 8; ++i )
	{
		m_vertices[ i ] = localToWorld.TransformPoint( m_vertices[ i ] );
	}
}
