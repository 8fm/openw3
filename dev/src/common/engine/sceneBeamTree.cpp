/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sceneBeamTree.h"
#include "renderCamera.h"
#include "renderFrame.h"

Plane::ESide Convex::Split( const Plane& plane, Convex*& front, Convex*& back, const Float eps ) const
{
	ASSERT( GetVertexCount() < (32 - 1) );

	Int32     sides[32];
	Float   dists[32];
	Int32		numPointsOnSide[4] = { 0, 0, 0, 0 }; // 0, Front, Back, Both

	Int32 i;
	for ( i = 0; i < GetVertexCount(); ++i ) 
	{
		dists[i] = (const Float) plane.DistanceTo( GetVertex(i) );
		if ( dists[i] > +eps )
		{
			sides[i] = Plane::PS_Front;
			numPointsOnSide[ Plane::PS_Front ]++;
		}
		else if ( dists[i] < -eps )
		{
			sides[i] = Plane::PS_Back;
			numPointsOnSide[ Plane::PS_Back ]++;
		}
		else
		{
			sides[i] = Plane::PS_Both;
		}
	}

	// Link last with first for easy wrap
	sides[i] = sides[0];
	dists[i] = dists[0];

	if ( numPointsOnSide[ Plane::PS_Front ] == 0 )
	{
		front = NULL;
		back = (Convex*)this;
		return Plane::PS_Back;
	}
	else if ( numPointsOnSide[ Plane::PS_Back ] == 0 )
	{
		front = (Convex*)this;
		back = NULL;
		return Plane::PS_Front;
	}
	else 
	{
		for ( Int32 i = 0; i < GetVertexCount(); ++i ) 
		{
			const Vector& a = GetVertex(i);
			const Int32 clipped = EdgeClipped(i);

			// If it's on the plane, just add it to both lists (front and back) and get out
			if ( sides[i] == Plane::PS_Both ) 
			{
				front->Add( a, clipped );
				back->Add( a, clipped );
				continue;
			}

			if ( sides[i] == Plane::PS_Front )
			{
				front->Add( a, clipped );
			}
			else /*if ( sides[i] == Plane::PS_Back )*/
			{
				back->Add( a, clipped );
			}

			// At this point we know the vertex is either in front or in back of the plane
			// if the next vertex is on the plane or on the same side we don't need to split...
			if ( sides[i+1] == Plane::PS_Both || sides[i] == sides[i+1] )
			{
				continue;
			}

			// Create a split point
			Vector mid, b = GetVertex( (i+1) % GetVertexCount() ); // next vert with wrap
			const Float d = dists[i] / ( dists[i] - dists[i+1] );
#if 0
			for ( Int32 j = 0; j < 3; ++j )
			{
				if ( plane.NormalDistance.A[j] == +1.0f )
				{
					mid.A[j] = +plane.NormalDistance.A[3];
				}
				else if ( plane.NormalDistance.A[j] == -1.0f )
				{
					mid.A[j] = -plane.NormalDistance.A[3];
				}
				else
				{
					mid.A[j] = a.A[j] + d * ( b.A[j] - a.A[j] );
				}
			}
			mid.A[3] = 1.0f;
#else
			mid.X = a.X + d * ( b.X - a.X );
			mid.Y = a.Y + d * ( b.Y - a.Y );
			mid.Z = a.Z + d * ( b.Z - a.Z );
			mid.W = 1.0f;
#endif

			// Add mid point with proper clipping flag
			ASSERT( sides[i+1] == Plane::PS_Front || sides[i+1] == Plane::PS_Back );
			if ( sides[i+1] == Plane::PS_Front )
			{
				front->Add( mid, clipped );
				back->Add( mid, 1 );
			}
			else
			{
				front->Add( mid, 1 );
				back->Add( mid, clipped );
			}
		}
	}

	return Plane::PS_Both;
}

CBeamTree::CBeamTree()
{
	m_nodeIndex = 0;
	m_nodes = (CBeamTreeNode*) RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_SceneBeamTree, BEAM_TREE_NODES * sizeof(CBeamTreeNode) );
	Red::System::MemorySet( m_nodes, 0, BEAM_TREE_NODES * sizeof(CBeamTreeNode) );
}


CBeamTree::~CBeamTree()
{
	RED_MEMORY_FREE( MemoryPool_Default, MC_SceneBeamTree, m_nodes );
}


void CBeamTree::GenerateFragments( CRenderFrame* frame, Color color ) const
{
	const CRenderCamera& camera = frame->GetFrameInfo().m_camera;
	
	Vector corners[4];
	camera.GetFrustumCorners( 0.5f, corners );

	Convex convex;
	for ( Int32 i = 0; i < 4; ++i )
	{
		convex.Add( corners[i], false );
	}

	GenerateFragmentsRecursive( 0, frame, color, &convex );
}

Uint32 CBeamTree::GetNewNodeIndex( const Vector& p1, const Vector& p2, const Vector& p3 )
{
	ASSERT( m_nodeIndex < BEAM_TREE_NODES );

	// Set plane
	CBeamTreeNode& node = m_nodes[ m_nodeIndex ];
	node.m_plane.SetPlane( p1, p2, p3 );
	node.m_planeEx = node.m_plane.NormalDistance;
	node.m_side = 0;
	node.m_side += ( node.m_plane.NormalDistance.X < 0.0f ? 1 : 0 );
	node.m_side += ( node.m_plane.NormalDistance.Y < 0.0f ? 2 : 0 );
	node.m_side += ( node.m_plane.NormalDistance.Z < 0.0f ? 4 : 0 );

	// Done
	return m_nodeIndex++;
}

void CBeamTree::InsertFrustum( const CRenderCamera& camera )
{	
	// Get corners
	Vector nearCorners[4], farCorners[4];
	camera.GetFrustumCorners( 0.1f, nearCorners );
	camera.GetFrustumCorners( 0.9f, farCorners );

	// Perspective camera, simple case
	Uint32 right, down, left, up;
	if ( camera.IsPerspective() )
	{
		// Planes
		right = GetNewNodeIndex( camera.GetPosition(), farCorners[1], farCorners[0] );
		down  = GetNewNodeIndex( camera.GetPosition(), farCorners[2], farCorners[1] );
		left  = GetNewNodeIndex( camera.GetPosition(), farCorners[3], farCorners[2] );
		up    = GetNewNodeIndex( camera.GetPosition(), farCorners[0], farCorners[3] );

		// Center
		m_rootPoint = camera.GetPosition();
	}
	else
	{
		// Planes
		right = GetNewNodeIndex( nearCorners[1], farCorners[1], farCorners[0] );
		down  = GetNewNodeIndex( nearCorners[2], farCorners[2], farCorners[1] );
		left  = GetNewNodeIndex( nearCorners[3], farCorners[3], farCorners[2] );
		up    = GetNewNodeIndex( nearCorners[0], farCorners[0], farCorners[3] );

		// Fake center point
		m_rootPoint = camera.GetScreenToWorld().TransformVectorWithW( Vector(0,0,0,1) );
		m_rootPoint /= m_rootPoint.W;
	}

	// Make hierarchy of nodes
	ASSERT( right == 0 );
	m_nodes[ right ].m_children[0] = left;
	m_nodes[ left  ].m_children[0] = down;
	m_nodes[ down  ].m_children[0] = up;
}


void CBeamTree::InsertFrustum( const Vector& position, const TDynArray< Vector >& corners )
{
	ASSERT( corners.Size() == 4 );

	Uint32 right = GetNewNodeIndex( position, corners[1], corners[0] );
	Uint32 down  = GetNewNodeIndex( position, corners[2], corners[1] );
	Uint32 left  = GetNewNodeIndex( position, corners[3], corners[2] );
	Uint32 up    = GetNewNodeIndex( position, corners[0], corners[3] );

	// Make hierarchy of nodes
	ASSERT( right == 0 );
	m_nodes[ right ].m_children[0] = left;
	m_nodes[ left  ].m_children[0] = down;
	m_nodes[ down  ].m_children[0] = up;

	// Set beam tree root point to camera position
	m_rootPoint = position;
}


void CBeamTree::InsertConvex( const TDynArray< Vector >& verts, Bool reverse )
{
	Convex convex;
	if ( reverse == false )
	{
		// Don't change the winding, just create convex from verts
		for ( Uint32 i = 0; i < verts.Size(); ++i )
		{
			convex.Add( verts[i], false );
		}
	}
	else
	{
		// Reverse winding (when doing two sided occluders)
		for ( Int32 i = (Int32)verts.Size()-1; i >= 0; --i )
		{
			convex.Add( verts[i], false );
		}
	}
	InsertConvexRecursive( 0, m_rootPoint, &convex );
}


void CBeamTree::GenerateFragmentsRecursive( const Uint32 beamTreeNodeIndex, CRenderFrame* frame, Color color, const Convex* convex ) const
{
	Convex placeFront, placeBack;
	Convex *front = &placeFront, *back = &placeBack;
	Plane::ESide side = convex->Split( m_nodes[beamTreeNodeIndex].m_plane, front, back );

	// Go down into the tree
	if ( side & Plane::PS_Front )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[0];
		if ( child )
		{
			GenerateFragmentsRecursive( child, frame, color, front );
		}
		else
		{
			// Draw front convex
			for ( Int32 i = 0; i < front->GetVertexCount(); ++i )
			{
				frame->AddDebugLine( front->GetVertex(i), front->GetVertex((i+1)%front->GetVertexCount()), color, true );
			}
		}
	}
	if ( side & Plane::PS_Back )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[1];
		if ( child )
		{
			GenerateFragmentsRecursive( child, frame, color, back );
		}
	}
}


void CBeamTree::InsertConvexRecursive( const Uint32 beamTreeNodeIndex, const Vector& rootPoint, const Convex* convex )
{
	Convex placeFront, placeBack;
	Convex *front = &placeFront, *back = &placeBack;

	// Split the convex
	const Plane::ESide side = convex->Split( m_nodes[beamTreeNodeIndex].m_plane, front, back );
	if ( side & Plane::PS_Front )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[0];
		if ( child )
		{
			InsertConvexRecursive( child, rootPoint, front );
		}
		else
		{
			Int32 childIndex = 0;
			Uint32 node = beamTreeNodeIndex;
			for( Int32 i = 0; i < front->GetVertexCount(); ++i )
			{
				if ( front->EdgeClipped(i) == false )
				{
					m_nodes[node].m_children[childIndex] = GetNewNodeIndex( rootPoint, front->GetVertex((i+1)%front->GetVertexCount()), front->GetVertex(i) );
					node = m_nodes[node].m_children[childIndex];
					childIndex = 1;
				}
			}
			m_nodes[node].m_children[childIndex] = GetNewNodeIndex( front->GetVertex(0), front->GetVertex(2), front->GetVertex(1) );
		}
	}

	if ( side & Plane::PS_Back )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[1];
		if ( child )
		{
			// If m_chidren[1] is leaf, just reject
			const Bool isLeaf = ( m_nodes[ child ].m_children[0] | m_nodes[ child ].m_children[1] ) == 0;
			if ( isLeaf == false )
			{
				InsertConvexRecursive( child, rootPoint, back );
			}
			else
			{
				// Reject
			}
		}
		else
		{
			// Reject
		}
	}
}

Plane::ESide CBeamTree::GetSideRecursive( const Uint32 beamTreeNodeIndex, const Box& box, Uint32& bothSideNodeIndex ) const
{
	const Plane::ESide side = m_nodes[beamTreeNodeIndex].m_plane.GetSide( box );
	if ( side == Plane::PS_Both )
	{
		bothSideNodeIndex = beamTreeNodeIndex;
		return Plane::PS_Both;
	}

	if ( side & Plane::PS_Front )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[0];
		return ( child == 0 ) ? Plane::PS_Front : GetSideRecursive( child, box, bothSideNodeIndex );
	}
	else /* if ( side & Plane::PS_Back ) */
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[1];
		return ( child == 0 ) ? Plane::PS_Back : GetSideRecursive( child, box, bothSideNodeIndex );
	}
}


Bool CBeamTree::IsVisibleRecursive( const Uint32 beamTreeNodeIndex, const Convex* convex ) const
{
	Convex placeFront, placeBack;
	Convex *front = &placeFront, *back = &placeBack;
	const Plane::ESide side = convex->Split( m_nodes[beamTreeNodeIndex].m_plane, front, back );

	if ( side & Plane::PS_Front )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[0];
		Bool visible = ( child == 0 ) ? true : IsVisibleRecursive( child, front );
		if( visible == true )
		{
			return true;
		}
	}
	if ( side & Plane::PS_Back )
	{
		Uint32 child = m_nodes[beamTreeNodeIndex].m_children[1];
		Bool visible = ( child == 0 ) ? false : IsVisibleRecursive( child, back );
		return visible;
	}

	return false;
}

Plane::ESide CBeamTree::CheckVisible( Int32 root, const Box& box ) const
{
	const Vector boxCenter  = box.CalcCenter();
	const Vector boxExtents = box.CalcExtents();

	Int32				stackTop = 0;
	const Int32		stackSize = BEAM_TREE_NODES;
	Uint32			stackNodeIndex[stackSize];

	// Start with root node
	stackNodeIndex[ stackTop ] = root;
	++stackTop;

	// Count back side visits
	Int32 numBack = 0;

	// Walk the tree
	Bool isVisible = false;
	while ( stackTop > 0 )
	{
		// Pop node from stack
		const Uint32 nodeIndex = stackNodeIndex[ stackTop - 1 ];
		--stackTop;

		// Test side
#if 0
		Plane::ESide nodeSide = m_nodes[ nodeIndex ].m_plane.GetSide( boxCenter, boxExtents );
#else
		EFrustumResult frustumResult;

		__m128 bMin = *(__m128*)&box.Min.A;
		__m128 bMax = *(__m128*)&box.Max.A;
		__m128 plane = *(__m128*)&m_nodes[ nodeIndex ].m_planeEx;
		__m128 mask = _mm_cmplt_ps( plane, _mm_setzero_ps() );

		Plane::ESide nodeSide;
		if ( BoxBehindPlane( bMin, bMax, plane, mask, frustumResult ) )
		{
			nodeSide = Plane::PS_Back;
		}
		else
		{
			nodeSide = ( frustumResult == FR_Inside ) ? Plane::PS_Front : Plane::PS_Both;
		}
#endif

		// Test the back side
		if ( nodeSide & Plane::PS_Back )
		{
			const Uint32 child = m_nodes[ nodeIndex ].m_children[1];
			if ( child != 0 ) 
			{
				// Count the number of times we have visited the back side
				numBack++;

				// Push on stack
				ASSERT( stackTop + 1 < stackSize );
				stackNodeIndex[ stackTop ] = child;
				++stackTop;
			}
		}

		// Test on which side of the plane the box is
		if ( nodeSide & Plane::PS_Front )
		{
			const Uint32 child = m_nodes[ nodeIndex ].m_children[0];
			if ( child == 0 )
			{
				// We are visible
				isVisible = true;
				continue;
			}

			// Walk the tree
			ASSERT( stackTop + 1 < stackSize );
			stackNodeIndex[ stackTop ] = child;
			++stackTop;
		}
	}

	// Return visibility status
	if ( isVisible )
	{
		if ( numBack > 0 )
		{
			return Plane::PS_Both;
		}
		else
		{
			return Plane::PS_Front;
		}
	}
	else
	{
		return Plane::PS_Back;
	}
}


Bool CBeamTree::IsVisible( const Convex& convex ) const
{
	ASSERT( m_nodeIndex > 0 );
	return IsVisibleRecursive( 0, &convex );
}
