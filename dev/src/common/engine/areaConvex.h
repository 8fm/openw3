/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/refCountPointer.h"

#include "renderVertices.h"

//---------------------------------------------------------------------------

class CRenderFrame;
class IRenderResource;

/// Covnex hull build interfae
class IAreaConvexDataSource
{
public:
	virtual ~IAreaConvexDataSource() {};

	//! Get number of faces
	virtual const Uint32 GetNumFaces() const = 0;

	//! Get number of vertices in n-th face
	virtual const Uint32 GetNumFaceVertices( const Uint32 faceIndex ) const = 0;

	//! Get coordinates for n-th face vertex
	virtual const Vector GetFaceVertex( const Uint32 faceIndex, const Uint32 vertexIndex ) const = 0;
};

//---------------------------------------------------------------------------

/// Convex hull description to be used with trigger system
class CAreaConvex
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_AreaShapes );

protected:
	struct Edge
	{
		Uint16 m_vertex;
		Uint16 m_face;
		Int16 m_other;
		Uint16 m_next;
		Vector m_dir;
		Vector m_plane;

		Edge* GetOtherEdge()
		{
			return this + m_other;
		}

		const Edge* GetOtherEdge() const
		{
			return this + m_other;
		}
	};

	//! Number of planes in the convex
	Uint32 m_numPlanes;
	
	//! List of planes in local shape space (including beveling planes)
	Vector* m_planes;

	//! Number of veritces in the convex
	Uint32 m_numVertices;

	//! List of convex vertices
	Vector* m_vertices;

	//! Number of edges in the convex
	Uint32 m_numEdges;

	//! Edge data
	Edge* m_edges;

	//! Number of faces in the convex
	Uint32 m_numFaces;

	//! Face first edge
	Uint16* m_faces;

	//! Relative offset from shape center
	Vector m_offset;

	//! Bounding box in local space
	Box	m_box;

public:
	//! Get local space box
	RED_FORCE_INLINE const Box& GetBox() const { return m_box; }

	//! Get local space offset
	RED_FORCE_INLINE const Vector& GetOffset() const { return m_offset; }

	//! Get number of convex planes
	RED_FORCE_INLINE const Uint32 GetNumPlanes() const { return m_numPlanes; }

	//! Get number of vertices in convex shape
	RED_FORCE_INLINE const Uint32 GetNumVertices() const { return m_numVertices; }

	//! Get number of edges in convex shape (debug)
	RED_FORCE_INLINE const Uint32 GetNumEdges() const { return m_numEdges; }

	//! Get number of edges in convex shape (debug)
	RED_FORCE_INLINE const Uint32 GetNumFaces() const { return m_numFaces; }

	//! Get n-th plane equation
	RED_FORCE_INLINE const Vector& GetPlane(const Uint32 planeIndex) const { return m_planes[planeIndex]; }

	//! Get n-th vertex
	RED_FORCE_INLINE const Vector& GetVertex(const Uint32 vertexIndex) const { return m_vertices[vertexIndex]; }

public:
	CAreaConvex();
	~CAreaConvex();

	// Save/Load
	void Serialize( IFile& file );

	// Serialize buffers (internal)
	void SerialzieBuffers( IFile& file );

	// Generate debug display geometry
	void CompileDebugMesh( const Color& faceColor, TDynArray<DebugVertex>& outVertices, TDynArray<Uint32>& outIndices ) const;

	// Build beveled geometry
	void BuildBeveledConvex( const Float bevel, const Float bevelVertical, const Matrix& bevelSpace, TDynArray<Vector>& outPlanes ) const;

	// Generate rendering edges
	void CompileDebugEdges( const Color& faceColor, TDynArray<DebugVertex>& outVertices ) const;

	// General point overlap test, returns true if point is inside any of the convex pieces forming this shape
	Bool PointOverlap( const Vector& point ) const;

	// General box overlap test, returns true if specified box is inside any of the convex pieces forming this shape
	Bool BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const;

	// Trace a line segment across the convex shape. Returns true if the line segment intersects the convex, outputs the intersection times.
	Bool IntersectLine( const Vector& start, const Vector& end, Float& outHit0, Float& outHit1 ) const;

	// Find a closest point on the surface of the area shape, returns false if point is out of range. Returns the point itself if inside the convex.
	Bool FindClosestPoint( const Vector& point, Vector& outPoint, Float& outDistance ) const;

	// Get surface point of convex shape at given position using given trace direction.
	Bool GetSurfacePoint( const Vector& origin, const Vector& dir, Vector& outPoint ) const;

	// Debug functionality - render edges of this convex shape
	void RenderOutline( CRenderFrame* frame, const Matrix& drawMatrix, const Color& color, const Bool overlay ) const;

	// Estimate memory size consumed by data
	const Uint32 CalcMemorySize() const;

	// Cache edge special data (other edge + plane + direction)
	void CacheEdgeData();

	//! Compile a baked copy for given bake matrix, some data may be shared
	CAreaConvex* CompileBakedCopy( const Matrix& bakeMatrix ) const;

public:
	//! Build convex from list of planes and center point
	static CAreaConvex* Build( const IAreaConvexDataSource& dataSource );
};

//---------------------------------------------------------------------------

/// Helper structure used only for closest point searches
class CAreaShapeClosestPointFinder
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_AreaShapes );

	friend class CAreaShape;

private:
	//! Data calculation offset
	Vector							m_worldOffset;

	//! Reference to original data
	class CAreaShape*				m_shapeData;

	//! Transfomed data
	TDynArray< CAreaConvex* >		m_convexData;

public:
	CAreaShapeClosestPointFinder( const Matrix& localToWorld, class CAreaShape* data );
	~CAreaShapeClosestPointFinder();

	// Find a closest point on the surface of the area shape, returns false if point is out of range. Returns the point itself if inside the convex.
	Bool FindClosestPoint( const Vector& worldPoint, const Float maxSearchDistance, Vector& outPoint, Float& outDistance ) const;
};

//---------------------------------------------------------------------------

/// Group of convex shapes
class CAreaShape
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_AreaShapes );

	friend class CAreaShapeBuilder;

protected:
	//! Convex shape
	TDynArray< CAreaConvex* > m_shapes;

	//! Internal reference count
	Red::Threads::CAtomic< Int32 >	m_refCount;

public:
	//! Get number of convex pieces in this shape
	RED_FORCE_INLINE const Uint32 GetNumShapes() const { return m_shapes.Size(); }

	//! Get n-th convex piece
	RED_FORCE_INLINE const CAreaConvex* GetConvex(const Uint32 index) const { return m_shapes[index]; }

public:
	// Is empty (has no convex pieces) ?
	Bool IsEmpty() const;

	// Remove all shapes
	void Clear();

	// Add new shape
	void AddConvex( const IAreaConvexDataSource& dataSource );

	// Save/Load
	Bool Serialize( IFile& file );

	// General point overlap test, returns true if point is inside any of the convex pieces forming this shape
	Bool PointOverlap( const Vector& point ) const;

	// General box overlap test, returns true if specified box is inside any of the convex pieces forming this shape
	Bool BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const;

	// Calculate (trace) point of the surface of area shape at give local coordinates, returns false if given point is outside area boundary
	Bool GetSurfacePoint( const Vector& localPoint, const Vector& localDir, Vector& outSurfacePoint ) const;

	// Compile proxy structure for finding closest distances, this is a world space structure
	CAreaShapeClosestPointFinder* CompileDistanceSearchData( const Matrix& localToWorld ) const;

	// Compile display mesh (for debug)
	IRenderResource* CompileDebugMesh( const Float bevel, const Float bevelVertical, const Matrix& bevelSpace ) const;

	// Estimate memory used by this shape
	const Uint32 CalcMemorySize() const;

	// Add internal reference
	void AddRef();
	
	// Release internal reference
	void Release();

public:
	// Serialization helper
	static Bool Serialize(IFile& file, CAreaShape*& shape);

	// Get the special empty shape
	static CAreaShape& EMPTY();

protected:
	CAreaShape();
	~CAreaShape();
};

//---------------------------------------------------------------------------

typedef TRefCountPointer< CAreaShape > CAreaShapePtr;
