/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibNavmesh.h"

///////////////////////////////////////////////////////////////////////////////
// TMPSHIT LEGACY!!!!!!!
///////////////////////////////////////////////////////////////////////////////
class CNavmesh : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CNavmesh, CResource, "navmesh", "Navmesh" );

	friend class PathLib::CNavmeshBinTree;										// direct access to data buffer
	friend void PathLib_TEMP_Convert( ::CNavmesh* legacyNavmesh, PathLib::CNavmesh* navmesh );
public:
	typedef Uint16 VertexIndex;
	typedef Uint16 TriangleIndex;
	static const TriangleIndex INVALID_INDEX = 0xffff;
	static const TriangleIndex MASK_EDGE = 0x8000;
	static const Uint16 CURRENT_BIN_VERSION;
protected:
	void InitializeBuffers( Uint32 extraSpace = 0 );
	void RestoreDataPostSerialization( Bool scrapBinTree = false );
	void PostDataInitializationProcess();

	////////////////////////////////////////////////////////////////////////
	struct CTriangleVertexes
	{
		VertexIndex					m_vertex[3];
		void Set( VertexIndex v0, VertexIndex v1, VertexIndex v2 )
		{
			m_vertex[ 0 ] = v0;
			m_vertex[ 1 ] = v1;
			m_vertex[ 2 ] = v2;
		}
	};
	struct CTriangleBorder
	{
		TriangleIndex				m_triangle[3];
	};
	////////////////////////////////////////////////////////////////////////
	// Navigation mesh
	VertexIndex								m_vertexCount;
	TriangleIndex							m_triangleCount;
	TriangleIndex							m_phantomEdgesCount;
	Uint16									m_binariesVersion;
	Vector3*								m_vertexes;
	CTriangleVertexes*						m_triangleVertexes;
	CTriangleBorder*						m_triangleAdjecent;
	Vector3*								m_triangleNormals;
	Vector									m_centralPoint;
	Float									m_radius;
	Box										m_bbox;
	DataBuffer								m_dataBuffer;
public:

	CNavmesh();
	~CNavmesh();

	////////////////////////////////////////////////////////////////////////
	// Navigation mesh handling
	RED_INLINE TriangleIndex GetTrianglesCount() const					{ return m_triangleCount; }
	void GetTriangleVerts( TriangleIndex nTri, Vector3* vOut ) const
	{
		vOut[0] = m_vertexes[m_triangleVertexes[nTri].m_vertex[0]];
		vOut[1] = m_vertexes[m_triangleVertexes[nTri].m_vertex[1]];
		vOut[2] = m_vertexes[m_triangleVertexes[nTri].m_vertex[2]];
	}
	void GetTriangleVertsIndex( TriangleIndex nTri, VertexIndex* indOut ) const
	{
		indOut[0] = m_triangleVertexes[nTri].m_vertex[0];
		indOut[1] = m_triangleVertexes[nTri].m_vertex[1];
		indOut[2] = m_triangleVertexes[nTri].m_vertex[2];
	}
	void GetTriangleEdge( TriangleIndex nTri, Uint32 n, Vector3* vOut ) const
	{
		vOut[0] = m_vertexes[m_triangleVertexes[nTri].m_vertex[n]];
		vOut[1] = m_vertexes[m_triangleVertexes[nTri].m_vertex[n == 2 ? 0 : n + 1]];
	}
	void GetTriangleEdgeIndex( TriangleIndex nTri, Uint32 n, VertexIndex* indOut ) const
	{
		indOut[0] = m_triangleVertexes[nTri].m_vertex[n];
		indOut[1] = m_triangleVertexes[nTri].m_vertex[n == 2 ? 0 : n + 1];
	}
	void GetTriangleNeighbours( TriangleIndex nTri, TriangleIndex* aOut ) const
	{
		aOut[0] = m_triangleAdjecent[nTri].m_triangle[0];
		aOut[1] = m_triangleAdjecent[nTri].m_triangle[1];
		aOut[2] = m_triangleAdjecent[nTri].m_triangle[2];
	}

	RED_INLINE const Vector3& GetVertex(VertexIndex vert)					{ return m_vertexes[vert]; }
	RED_INLINE VertexIndex GetVertexesCount() const						{ return m_vertexCount; }
	RED_INLINE const Vector3& GetTriangleNormal(TriangleIndex nTri) const	{ return m_triangleNormals[nTri]; }
	RED_INLINE const Box& GetBoundingBox() const							{ return m_bbox; }
	RED_INLINE TriangleIndex GetPhantomEdgesCount() const					{ return m_phantomEdgesCount; }

	////////////////////////////////////////////////////////////////////////
	// CObject interface
	void OnSerialize( IFile& file ) override;
	Bool IsValid();

	////////////////////////////////////////////////////////////////////////
	// Initialization
	void InitializeMesh( const TDynArray< Vector3 >& vertexes, const TDynArray< Uint16 >& triangleVertexes );
	void Clear();
	void CopyFrom( CNavmesh* navmesh );
	TriangleIndex GetClosestTriangle( const Box& bbox );

	void ComputeConvexBoundings( Box& outBBox, TDynArray< Vector2 >& outBoundings ) const;

	void CollectPhantomEdges( TDynArray< Vector3 >& outEdges );				// generation sub-procedures to collect phantom edges coordinates for future re-addition
	void RemarkPhantomEdges( const TDynArray< Vector3 >& outEdges );		// generation sub-procedure that re-adds phantom edges from previous navmesh version
	void MarkPhantomEdge( TriangleIndex tri, Uint32 n );						// set given triangle border as phantom edge
	void ClearPhantomEdge( TriangleIndex tri, Uint32 n );						// clear given triangle border from phantom edge

	TriangleIndex Debug_GetTriangleIntersectingRay( const Vector3& rayOrigin, const Vector3& rayDir, Vector3& collisionPoint );				// This function might be used in offline editing tools. Its veeery costly!
	static RED_INLINE Bool IsEdge( TriangleIndex tri )						{ return (tri & MASK_EDGE) != 0; }
	static RED_INLINE Bool IsPhantomEdge( TriangleIndex tri )					{ return (tri & MASK_EDGE) && tri != INVALID_INDEX; }
	static RED_INLINE TriangleIndex PhantomEdgeNeighbourIndex( TriangleIndex tri ) { ASSERT( IsPhantomEdge( tri ) ); return tri & (~MASK_EDGE); }

	Bool IsEmptyMesh() const;

#ifdef DEBUG_NAVMESH_COLORS
	TDynArray< Uint32 >			m_triangleColours;
#endif

};

BEGIN_CLASS_RTTI( CNavmesh );
PARENT_CLASS( CResource );
PROPERTY( m_vertexCount );
PROPERTY( m_triangleCount );
PROPERTY( m_phantomEdgesCount );
PROPERTY( m_binariesVersion )
	PROPERTY( m_centralPoint );
PROPERTY( m_radius );
PROPERTY( m_bbox );
END_CLASS_RTTI();

void PathLib_TEMP_Convert( ::CNavmesh* legacyNavmesh, PathLib::CNavmesh* newNavmesh );
