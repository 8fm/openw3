/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once 

class CRenderFrame;

class Convex
{
	Vector m_verts[32];
	Int32 m_clippedFlag;
	Int32 m_numVerts;

public:
	Convex()
	{
		m_numVerts = 0;
		m_clippedFlag = 0;
	}
	RED_INLINE void Add( const Vector& point, const Int32 clipped )
	{
		m_verts[m_numVerts++] = point;
		m_clippedFlag = (m_clippedFlag << 1) | clipped;
	}
	RED_INLINE Bool EdgeClipped( const Int32 index ) const
	{
		Bool flag = (m_clippedFlag & (1 << (m_numVerts-index-1))) != 0;
		return flag;
	}
	RED_INLINE Int32 GetVertexCount() const
	{
		return m_numVerts;
	}
	RED_INLINE const Vector& GetVertex( const Int32 index ) const
	{
		return m_verts[index];
	}

	Plane::ESide Split( const Plane& plane, Convex*& front, Convex*& back, const Float eps = 1e-4f ) const;
};

struct CBeamTreeNode
{
	Plane		m_plane;
	Vector		m_planeEx;
	Uint8		m_side;
	Uint32		m_children[2];	// Child 0 is front side of plane
};

#define BEAM_TREE_NODES	3072

class CBeamTree
{
	friend class CHierarchicalGridNode;

	// Where is the root of the beam tree (camera position)
	Vector			m_rootPoint;
	Uint32			m_nodeIndex;
	CBeamTreeNode*	m_nodes;

public:
	CBeamTree();
	~CBeamTree();

	// Build beamTree
	void InsertFrustum( const class CRenderCamera& camera );
	void InsertFrustum( const Vector& position, const TDynArray< Vector >& corners );
	void InsertConvex( const TDynArray< Vector >& verts, Bool reverse = false );

	// Check visibility
	Plane::ESide CheckVisible( Int32 root, const Box& box ) const;
	Bool IsVisible( const Convex& convex ) const;

	// Generate editor fragments for debugging
	void GenerateFragments( CRenderFrame* frame, Color color ) const;

	// Check bounding box against beamTree and return first node in hierarchy which split the box
	Plane::ESide GetSideRecursive( const Uint32 beamTreeNodeIndex, const Box& box, Uint32& bothSideNodeIndex ) const;

protected:
	Bool IsVisibleRecursive( const Uint32 beamTreeNodeIndex, const Convex* convex ) const;
	void InsertConvexRecursive( const Uint32 beamTreeNodeIndex, const Vector& rootPoint, const Convex* convex );
	void GenerateFragmentsRecursive( const Uint32 beamTreeNodeIndex, CRenderFrame* frame, Color color, const Convex* convex ) const;
	Uint32 GetNewNodeIndex( const Vector& p1, const Vector& p2, const Vector& p3 );
};