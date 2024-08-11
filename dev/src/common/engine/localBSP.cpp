/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localBSP.h"

void CLocalBSP::Face::Split( const Plane &plane, Face *&front, Face *&back )
{
	ASSERT( m_polygon );

	// Test side  
	Plane::ESide side = m_polygon->Clasify( plane );
	if ( side == Plane::PS_Front )
	{
		front = new Face( m_polygon, m_wasSplitter );
		back = NULL;
	}
	else if ( side == Plane::PS_Back )
	{
		front = NULL;
		back = new Face( m_polygon, m_wasSplitter );
	}
	else if ( side == Plane::PS_None )
	{
		Float dot = Vector::Dot3( m_polygon->m_plane.NormalDistance, plane.NormalDistance );
		if ( dot > 0.0f)
		{
			// HACK: if we are coplanar with splitter assume we were used as splitter to
			front = new Face( m_polygon, true );	
			back = NULL;
		}
		else
		{
			// HACK: if we are coplanar with splitter assume we were used as splitter to
			front = NULL;
			back = new Face( m_polygon, true );
		}
	}
	else
	{
		// Split polygon for real
		CPolygon *frontPoly = NULL;
		CPolygon *backPoly=NULL;
		m_polygon->Split( plane, frontPoly, backPoly );
		ASSERT( frontPoly );
		ASSERT( backPoly );

		// Create parts
		front = new Face( frontPoly, m_wasSplitter );
		back = new Face( backPoly, m_wasSplitter );

		// Delete original polygon
		delete m_polygon;
	}

	// Remove reference to original polygon since it was either passed to new face or deleted
	m_polygon = NULL;
}

void CLocalBSP::Print() const
{
	if ( !m_nodes.Size() )
	{
		LOG_ENGINE( TXT("No tree") );
	}
	else
	{
		PrintNode( 0, 1 );
	}
}

void CLocalBSP::Build( const TDynArray< CPolygon* > &faces )
{
	// Clear current tree
	m_nodes.Clear();

	// Create local faces
	TDynArray< Face* > localFaces;
	for ( Uint32 i=0; i<faces.Size(); i++ )
	{
		CPolygon *poly = faces[i];
		localFaces.PushBack( new Face( poly, false ));
	}

	// Build tree
	CompileNode( localFaces );
}

void CLocalBSP::AddClipPlane( const Plane &plane, Bool outsideZone )
{
	// Not needed
}

Int32 CLocalBSP::CompileNode( const TDynArray< Face* > &faces )
{
	// Allocate node
	Int32 nodeIndex = static_cast< Int32 >( m_nodes.Grow( 1 ) );

	// No faces ? Inside leaf
	if ( !faces.Size() )
	{
		m_nodes[ nodeIndex ].m_leafType = LEAF_Inside;
		return nodeIndex;
	}

	// Find best splitter
	Int32 bestScore = INT_MAX;
	Face *bestSplitter = NULL;
	for ( Uint32 i=0; i<faces.Size(); i++ )
	{
		Face *face = faces[i];

		// Do not test faces that were already used as splitter
		if ( face->m_wasSplitter )
		{
			continue; 
		}

		// Get plane
		ASSERT( face->m_polygon );
		const Plane &plane = face->m_polygon->m_plane;

		// Count splits
		Int32 count=0;
		for ( Uint32 j=0; j<faces.Size(); j++ )
		{
			Face *test = faces[j];

			// Skip tested face and other faces that were already used by splitters
			if ( test==face || test->m_wasSplitter )
			{
				continue;
			}

			// Clasify to see if we are making splits
			ASSERT( test->m_polygon );
			if ( test->m_polygon->Clasify( plane ) == Plane::PS_Both )
			{
				if ( ++count >= bestScore )
				{
					break;
				}
			}
		}

		// Keep face with best score
		if ( !bestSplitter || count<bestScore )
		{
			bestSplitter = face;
			bestScore = count;
		}
	}

	// No splitter, create outside leaf
	if ( !bestSplitter )
	{
		// Delete faces, not needed anymore
		for ( Uint32 i=0; i<faces.Size(); i++ )
		{ 
			delete faces[i];
		}

		// Create leaf
		m_nodes[ nodeIndex ].m_leafType = LEAF_Outside;
		return nodeIndex;
	}

	// Setup node plane
	ASSERT( bestSplitter->m_polygon );
	Plane splitPlane = bestSplitter->m_polygon->m_plane;
	m_nodes[ nodeIndex ].m_plane = splitPlane;
	m_nodes[ nodeIndex ].m_orgPlane = splitPlane;

	// Mark splitter face as used
	ASSERT( !bestSplitter->m_wasSplitter );
	bestSplitter->m_wasSplitter = true;

	// Split faces into front and back list
	TDynArray< Face * > frontFaces, backFaces;
	for ( Uint32 i=0; i<faces.Size(); i++ )
	{
		Face *face = faces[i];

		// Split
		Face *frontFrag = NULL;
		Face *backFrag = NULL;
		face->Split( splitPlane, frontFrag, backFrag );

		// Add generated fragments to lists
		if ( frontFrag )
		{
			frontFaces.PushBack( frontFrag );
		}
		if ( backFrag )
		{
			backFaces.PushBack( backFrag );
		}
	}

	// Recurse
	m_nodes[ nodeIndex ].m_children[0] = ( Int16 ) CompileNode( frontFaces );
	m_nodes[ nodeIndex ].m_children[1] = ( Int16 ) CompileNode( backFaces );

	// return index of created node
	return nodeIndex;
}

void CLocalBSP::PrintNode( Int32 nodeIndex, Int32 level ) const
{
	// Fill trail
	String trail;
	for ( Int32 i=0; i<level; i++ )
	{
		trail += TXT("  ");
	}

	// Print
	const Node& node = m_nodes[ nodeIndex ];
	if ( node.IsLeaf() )
	{
		if ( node.m_leafType == LEAF_Inside )
		{
			LOG_ENGINE( TXT("%sINSIDE"), trail.AsChar() );
		}
		else
		{
			LOG_ENGINE( TXT("%sOUTSIDE"), trail.AsChar() );
		}
	}
	else
	{
		LOG_ENGINE( TXT("%sNODE [ %1.2f,%1.2f,%1.2f ] : %1.4f"), trail.AsChar(), node.m_plane.NormalDistance.X, node.m_plane.NormalDistance.Y, node.m_plane.NormalDistance.Z, node.m_plane.NormalDistance.W );
		PrintNode( node.m_children[0], level + 1 );
		PrintNode( node.m_children[1], level + 1 );
	}
}

void CLocalBSP::Transform( const Matrix& matrix )
{
	Matrix temp = matrix.FullInverted().Transposed();
	for ( Uint32 i=0; i<m_nodes.Size(); i++ )
	{
		Node& node = m_nodes[i];
		if ( !node.IsLeaf() )
		{
			Vector planeNormal = temp.TransformVector( node.m_orgPlane.NormalDistance ).Normalized3();
			Vector planeOffset = matrix.TransformPoint( node.m_orgPlane.NormalDistance * (-node.m_orgPlane.NormalDistance.W) );
			node.m_plane = Plane( planeNormal, planeOffset );
		}
	}
}

