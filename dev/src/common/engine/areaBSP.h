/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Area BSP tree
class CAreaBSP
{
public:
	// BSP Polygon
	struct Polygon
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );

		TDynArray< Vector, MC_RenderData >	m_points;
		Plane								m_plane;
		Bool								m_wasSplitter;
		Int32									m_sourceEdge;

		// Create from plane
		Polygon( const Plane& plane );

		// Create from area edge
		Polygon( const Vector& vertexA, const Vector& vertexB, Float height, Int32 edge );

		// Create from three vertices
		Polygon( const Vector& vertexA, const Vector& vertexB, const Vector& vertexC );

		// Create from vertex list
		Polygon( const TDynArray< Vector, MC_RenderData >& vertices, const Plane& plane, Bool wasSplitter, Int32 edge );

		// Test vs plane
		Plane::ESide TestSide( const Plane& plane ) const;

		// Split by plane, destroys original
		void Split( const Plane& plane, Polygon*& front, Polygon*& back );
	};

protected:
	// SAT collision context
	struct CollisionContext
	{
		Vector		m_boxCenter;		// Center of the box in local space
		Vector		m_boxExtents;		// Extents
		Vector		m_boxAxes[3];		// Axes

		// Calculate extents along ray
		void CalculateExtents( const Vector& normal, Float &a, Float& b ) const;
	};

	// SAT trace context
	struct TraceContext
	{
		Vector		m_traceStart;		// Start of trace
		Vector		m_traceEnd;			// End of trace
		Vector		m_boxExtents;		// Extents
		Vector		m_boxAxes[3];		// Axes

		// Calculate extents along ray
		void CalculateExtents( const Vector& normal, Float &extents ) const;
	};

	/// SAT trace results
	struct TraceResults
	{
		Float		m_impactTime;
		Float		m_exitTime;
		Vector		m_impactNormal;
		Vector		m_exitNormal;
	};

	// BSP Node
	struct Node
	{
		Int16		m_parent;
		Int16		m_plane;
		Int16		m_children[2];

		// Constructor
		inline Node()
			: m_parent( -1 )
			, m_plane( -1 )
		{
			m_children[0] = -1;
			m_children[1] = -1;
		};

		// Serialization
		friend IFile& operator<<( IFile& file, Node& node )
		{
			file << node.m_parent;
			file << node.m_plane;
			file << node.m_children[0];
			file << node.m_children[1];
			return file;
		}
	};

public:
	// Convex
	struct Convex
	{
		TDynArray< Vector, MC_RenderData >		m_vertices;
		TDynArray< Vector, MC_RenderData >		m_axes;

		// Build convex from list of planes
		Bool Build( CAreaBSP& bsp, const TDynArray< Plane >& planes );

		// Calculate extents along ray
		void CalculateExtents( const Vector& normal, Float &a, Float& b ) const;

		// Test SAT overlap with box in given axis
		Bool TestOverlapInAxis( const Vector& normal, const CollisionContext& context ) const;

		// Test SAT overlap with box
		Bool TestOverlap( const CollisionContext& context ) const;

		// Test SAT overlap with box in given axis
		Bool TraceAxis( const Vector& normal, const TraceContext& context, TraceResults& results ) const;

		// Test SAT trace with box
		Bool Trace( const TraceContext& context, TraceResults& results ) const;

		// Serialization
		friend IFile& operator<<( IFile& file, Convex& convex )
		{
			file << convex.m_vertices;
			file << convex.m_axes;
			return file;
		}
	};

protected:
	TDynArray< Plane, MC_RenderData >	m_planes;		// BSP planes
	TDynArray< Convex, MC_RenderData >	m_convexes;		// BSP leafs	
	TDynArray< Node, MC_RenderData >	m_nodes;		// BSP tree nodes
	Int16								m_root;			// BSP root node

public:
	//! Get the convex list
	RED_INLINE const TDynArray< Convex, MC_RenderData > &GetConvexes() const { return m_convexes; }

public:
	CAreaBSP();
	~CAreaBSP();

	// Is BSP valid ?
	Bool IsValid() const;

	// Compile from area vertices
	void Compile( const TDynArray< Vector >& localVertices, Float height );

	// Test if point is inside area BSP
	Bool PointOverlap( const Vector& point ) const;

	// Test if AABB touches area BSP
	Bool BoxOverlap( const Vector& center, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ ) const;

	Bool Intersects( const Vector &start, const Vector &end ) const;

	// Trace AABB through the BSP
	Bool BoxTrace( const Vector& start, const Vector& end, const Vector& extents, const Vector& axisX, const Vector& axisY, const Vector& axisZ, Float& hitTime, Vector& hitNormal ) const;

	// Serialize
	void Serialize( IFile& file );

	// Clip polygon to the BSP tree, leaves the part that's inside the tree
	Bool ClipToTree( Polygon* polygon, TDynArray< Polygon* >& outPolygons ) const;

protected:
	Int16 IndexPlane( const Plane& plane );
	Int16 CompileBSP( Int16 parent, const TDynArray< Polygon* >& polygons, TDynArray< Plane >& planeList );
	Polygon* FindSplitter( const TDynArray< Polygon* >& polygons );
	void SplitPolygons( const TDynArray< Polygon* >& polygons, Polygon* splitter, TDynArray< Polygon* >& frontPolygons, TDynArray< Polygon* >& backPolygons );
	void PrintNode( Uint32 level, Int16 node );
	Bool CheckBoxOverlapNode( Int16 nodeIndex, CollisionContext& context ) const;
	Bool ClipToNode( Int16 node, Polygon* polygon, TDynArray< Polygon* >& outPolygons ) const;
};
	