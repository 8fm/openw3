/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

/**
 * This template can be used to create a vertex array by adding individual
 * triangles and quads to it.  The Vertex template parameter is expected to
 * contain x, y and z fields (lowercase) and be usable with a THashMap
 * (CalcHash and operator== overrides must be defined for the type where it
 * is used).  Check stripeComponent.cpp as an example where this is used.
 */
template < typename Vertex, typename Index=Uint32 >
class TEasyMeshBuilder
{
private:
	TDynArray< Vertex, MC_Temporary >			m_vertices;
	TDynArray< Index, MC_Temporary >			m_indices;
	THashMap< Vertex, Index, DefaultHashFunc< Vertex >, DefaultEqualFunc< Vertex >, MC_Temporary >		m_indexMap;
	Box											m_boundingBox;

	Index GetVertex( const Vertex& v )
	{
		Index* index = m_indexMap.FindPtr( v );
		if ( index )
		{
			return *index;
		}
		m_indexMap.Insert( v, Index( m_vertices.Size() ) );
		m_vertices.PushBack( v );
		m_boundingBox.AddPoint( Vector( v.x, v.y, v.z ) );
		return Index( m_vertices.Size() - 1 );
	}

	void AddVertex( const Vertex& v )
	{
		m_indices.PushBack( GetVertex( v ) );
	}

public:
	RED_INLINE const Vertex* GetVertexArray() const { return m_vertices.TypedData(); }
	RED_INLINE const Index* GetIndexArray() const { return m_indices.TypedData(); }
	RED_INLINE Uint32 GetVertexCount() const { return m_vertices.Size(); }
	RED_INLINE Uint32 GetIndexCount() const { return m_indices.Size(); }
	RED_INLINE const Box& GetBoundingBox() const { return m_boundingBox; }

	// Start adding geometry
	void Begin()
	{
		m_vertices.Clear();
		m_indices.Clear();
		m_indexMap.Clear();
		m_boundingBox.Clear();
	}

	// Finish adding geometry
	void End()
	{
		m_indexMap.Clear();
	}

	// Add a single triangle
	void Triangle( const Vertex& a, const Vertex& b, const Vertex& c )
	{
		AddVertex( a );
		AddVertex( b );
		AddVertex( c );
	}

	// Add a quad (made up of two triangles)
	void Quad( const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d )
	{
		// Choose triangle arrangement which produces more balanced triangle areas to avoid
		// quads made up from a thin and a fat triangle wherever possible.  Note that this
		// only uses XY and ignores Z
		Vector2 va( a.x, a.y ), vb( b.x, b.y ), vc( c.x, c.y ), vd( d.x, d.y );
		Float case1 = MAbs( MathUtils::GeometryUtils::TriangleArea2D( va, vb, vc ) - MathUtils::GeometryUtils::TriangleArea2D( va, vc, vd ) );
		Float case2 = MAbs( MathUtils::GeometryUtils::TriangleArea2D( va, vb, vd ) - MathUtils::GeometryUtils::TriangleArea2D( vb, vc, vd ) );
		if ( case1 < case2 )
		{
			Triangle( a, b, c );
			Triangle( a, c, d );
		}
		else
		{
			Triangle( a, b, d );
			Triangle( b, c, d );
		}
	}

	// Add multiple triangles
	void Triangles( const Vertex* vertices, Uint32 count )
	{
		for ( Uint32 i=0; i < count; ++i,vertices += 3 )
		{
			Triangle( vertices[0], vertices[1], vertices[2] );
		}
	}

	// Transform the mesh using the given matrix
	void Transform( const Matrix& matrix )
	{
		for ( auto it=m_vertices.Begin(); it != m_vertices.End(); ++it )
		{
			Vector v( (*it).x, (*it).y, (*it).z );
			v = matrix.TransformPoint( v );
			(*it).x = v.X;
			(*it).y = v.Y;
			(*it).z = v.Z;
		}
	}
};

