//////////////////////////////////////
//          Inferno Engine          //
// Copyright (C) 2002-2006 by Dexio //
//////////////////////////////////////

#pragma once

#define POLYGON_CLIP_EPSILON		0.0001f

/// Polygon
template< typename T >
class TPolygon
{
public:
	typedef TDynArray< T, MC_RenderData > TPolygonVertices;

	TPolygonVertices	m_vertices;		//!< Polygon points
	Plane				m_plane;        //!< Polygon plane
	void*				m_userData;		//!< Polygon user data, copied				

public:
	//! Default constructor
	RED_INLINE TPolygon()
		: m_userData( NULL )
	{};

	//! Copy constructor
	RED_INLINE TPolygon( const TPolygon<T> &other )
		: m_vertices( other.m_vertices )
		, m_plane( other.m_plane )
		, m_userData( other.m_userData )
	{};

	//! Triangle
	RED_INLINE TPolygon( const T &a, const T &b, const T &c )
		: m_userData( NULL )
	{
		Set( a,b,c );
	}

	//! Quad
	RED_INLINE TPolygon( const T &a, const T &b, const T &c, const T &d )
		: m_userData( NULL )
	{
		Set( a,b,c,d );
	}

	//! Plane polygon
	RED_INLINE TPolygon( const Plane &plane, Float size=1000.0f )
		: m_userData( NULL )
	{
		Set( plane, size );
	}

	//! Any shape
	RED_INLINE TPolygon( const T *points, Uint32 numPoints )
		: m_userData( NULL )
	{
		Set( points, numPoints );
	}

	//! Any indexed shape
	RED_INLINE TPolygon( const T *points, const Uint32* indices, Uint32 numPoints )
		: m_userData( NULL )
	{
		Set( points, indices, numPoints );
	}

public:
	//! Copy from other polygon
	RED_INLINE TPolygon<T> &Set( const TPolygon &other )
	{
		m_vertices = other.m_vertices;
		m_plane = other.m_plane;
		m_userData = other.m_userData;
		return *this;
	}

	//! Set triangle
	RED_INLINE TPolygon<T> &Set( const T &a, const T &b, const T &c )
	{
		// Initialize
		m_vertices.Resize( 3 );
		m_vertices[0] = a;
		m_vertices[1] = b;
		m_vertices[2] = c;

		// Update plane equation
		RecalculatePlane();
		return *this;
	}

	//! Set quad
	RED_INLINE TPolygon<T> &Set( const T &a, const T &b, const T &c, const T &d )
	{
		// Initialize
		m_vertices.Resize( 4 );
		m_vertices[0] = a;
		m_vertices[1] = b;
		m_vertices[2] = c;
		m_vertices[3] = d;

		// Update plane equation
		RecalculatePlane();
		return *this;
	}

	//! Initialize polygon from vertex list
	RED_INLINE TPolygon<T> &Set( const T *points, Uint32 numPoints )
	{
		// Initialize
		m_vertices.Resize( numPoints );
		for ( Uint32 i=0; i<numPoints; i++ )
		{
			m_vertices[i] = points[i];
		}

		// Update plane equation
		RecalculatePlane();
		return *this;
	}

	//! Initialize polygon from indexed vertex list
	RED_INLINE TPolygon<T> &Set( const T *points, const Uint32* indices, Uint32 numPoints )
	{
		// Initialize
		m_vertices.Resize( numPoints );
		for ( Uint32 i=0; i<numPoints; i++ )
		{
			m_vertices[i] = points[ indices[i] ];
		}

		// Update plane equation
		RecalculatePlane();
		return *this;
	}

	//! Plane polygon
	RED_INLINE TPolygon &Set( const Plane &plane, Float size=1000.0f )
	{
		static Vector worldNormals[6];
		static Vector worldVertices[6][4];
		static Bool init = true;

		// Initialize internal tables
		if ( init )
		{
			init = false;

			// Init normals
			worldNormals[0] = Vector( 1.0f, 0.0f, 0.0f );
			worldNormals[1] = Vector( -1.0f, 0.0f, 0.0f );
			worldNormals[2] = Vector( 0.0f, 1.0f, 0.0f );
			worldNormals[3] = Vector( 0.0f, -1.0f, 0.0f );
			worldNormals[4] = Vector( 0.0f, 0.0f, 1.0f );
			worldNormals[5] = Vector( 0.0f, 0.0f, -1.0f );

			// Initialize faces
			for ( Uint32 i=0; i<6; i++ )
			{
				Vector up = (i<4) ? Vector(0,0,1) : Vector(1,0,0);
				Vector right = Vector::Cross( up, worldNormals[i] );
				worldVertices[i][0] = up + right;
				worldVertices[i][1] = up - right;
				worldVertices[i][2] = -up - right;
				worldVertices[i][3] = -up + right;
			}
		}

		// Find best axis
		Float bestDot=-2.0f;
		Int32 best = -1;
		for ( Uint32 i=0; i<6; i++ )
		{
			Float dot = Vector::Dot3( plane.NormalDistance, worldNormals[i] );
			if ( dot > bestDot )
			{
				bestDot = dot;
				best = i;
			}
		}

		// Set polygon plane
		m_plane = plane; 

		// Project vertices
		m_vertices.Resize( 4 );
		m_vertices[0] = T( ProjectPoint( worldVertices[best][0] * size ) );
		m_vertices[1] = T( ProjectPoint( worldVertices[best][1] * size ) );
		m_vertices[2] = T( ProjectPoint( worldVertices[best][2] * size ) );
		m_vertices[3] = T( ProjectPoint( worldVertices[best][3] * size ) );
		return *this;
	}

	//! Project point on polygon plane
	RED_INLINE const TPolygon<T> &ProjectPoint( const T &point, T&out ) const
	{
		// Start with copy
		out = point;

		// Project
		Float dist = m_plane.DistanceTo( point );
		out.Position() = point.Position() - ( m_plane.NormalDistance * dist );
		return *this;
	}

	//! Project point on polygon plane
	RED_INLINE T ProjectPoint( const T &point ) const
	{
		Float dist = m_plane.DistanceTo( point );
		return point - ( m_plane.NormalDistance * dist );
	}

	//! Add vertex to polygon
	RED_INLINE TPolygon<T> &AddVertex( const T &vert )
	{
		m_vertices.PushBack( vert );

		// Update plane equation after adding third vertex
		if ( m_vertices.Size() == 3 )
		{
			RecalculatePlane();
		}

		return *this;
	} 

	//! Clear polygon
	RED_INLINE TPolygon<T> &Clear()
	{
		m_vertices.Clear();
		return *this;
	}

	//! Get polygon size ( number of vertices )
	RED_INLINE Uint32 Size() const
	{
		return m_vertices.Size();
	}

	//! Is thia a valid polygon ?
	RED_INLINE operator Bool() const
	{
		return m_vertices.Size() >= 3;
	}  

	//! Get main axis
	RED_INLINE Int32 GetMainAxis() const
	{
		Float absX = Abs< Float >( m_plane.NormalDistance.X );
		Float absY = Abs< Float >( m_plane.NormalDistance.Y );
		Float absZ = Abs< Float >( m_plane.NormalDistance.Z );
		if ( absX > absY && absX > absZ )
		{
			return 0;
		}
		else if ( absY > absX && absY > absZ )
		{
			return 1;
		}
		else
		{
			return 2;
		}
	}  

	//! Calculate polygon bounding box
	RED_INLINE const TPolygon<T> &CalcBox( Box &box ) const
	{
		box.Clear();
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			box.AddPoint( m_vertices[i].Position() );
		}

		return *this;
	}

	//! Calculate polygon bounding box
	RED_INLINE Box CalcBox() const
	{
		Box box;
		CalcBox( box );
		return box;
	}

	//! Calculate polygon center
	RED_INLINE const TPolygon<T> &CalcCenter( Vector &center ) const
	{
		center = Vector::ZEROS;
		if ( m_vertices.Size() )
		{
			for ( Uint32 i=0; i<m_vertices.Size(); i++ )
			{
				center += m_vertices[i].Position();
			}
			center /= (Float)m_vertices.Size();
		}
		return *this;
	}

	//! Calculate polygon center
	RED_INLINE Vector CalcCenter() const
	{
		Vector center;
		CalcCenter( center );
		return center;
	}

	//! Calculate polygon extents along given axis
	RED_INLINE Bool CalcExtentsMixMax( const Vector &axis, Float &min, Float &max ) const
	{
		if ( m_vertices.Size() )
		{
			min = max = Vector::Dot3( m_vertices[0].Position(), axis );
			for ( Uint32 i=1; i<m_vertices.Size(); i++ )
			{
				Float pos = Vector::Dot3( m_vertices[i].Position(), axis );
				min = Min< Float >( pos, min );
				max = Max< Float >( pos, max );
			}
		}

		return m_vertices.Size() > 0;
	}

	//! Move polygon
	RED_INLINE TPolygon<T> &Offset( Vector &delta )
	{
		// Move vertices
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			m_vertices[i].Position() = m_vertices[i].Position() + delta;
		}

		// Update plane equation
		RecalculatePlane();
		return *this;
	}

	//! Move polygon
	RED_INLINE const TPolygon<T> &Offset( Vector &delta, TPolygon &out ) const
	{
		out.Set( *this );
		out.Offset( delta );
		return *this;
	}

	//! Transform polygon
	RED_INLINE TPolygon<T> &Transform( const Matrix &matrix )
	{
		// Transform all polygon vertices by matrix
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			m_vertices[i].Position() = matrix.TransformPoint( m_vertices[i].Position() );
		}

		// Update plane equation
		RecalculatePlane();   
		return *this;
	}

	//! Transform polygon
	RED_INLINE const TPolygon<T> &Transform( const Matrix &matrix, TPolygon &out ) const
	{
		out.Set( *this );
		out.Transform( matrix );
		return *this;
	}  

	//! Swap polygon winding
	RED_INLINE TPolygon<T> &Swap()
	{
		// Swap vertices
		for ( Uint32 i=0; i<m_vertices.Size()/2; i++ )
		{
			::Swap<T>( m_vertices[i], m_vertices[ ( m_vertices.Size() - 1 ) - i ] );
		}

		// Invert plane
		m_plane = -m_plane;
		return *this;
	}

	//! Get swap polygon
	RED_INLINE TPolygon<T> Swaped() const
	{
		TPolygon<T> out;
		out.Set( *this );
		out.Swap();
		return out;
	}

	//! Clasify point vs polygon plane
	RED_INLINE Plane::ESide Clasify( const Vector &point ) const
	{
		return m_plane.GetSide( point );
	}

	//! Clasify polygon vs polygon plane
	RED_INLINE Plane::ESide Clasify( const Plane &plane, Float epsilon=POLYGON_CLIP_EPSILON ) const
	{
		Int32 numFront=0, numBack=0;
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			Float dist = plane.DistanceTo( m_vertices[i].Position() );
			if (dist > epsilon)
			{
				if ( numBack )
				{
					return Plane::PS_Both;
				}
				numFront++;
			}
			else if ( dist < -epsilon )
			{
				if ( numFront )
				{
					return Plane::PS_Both;
				}
				numBack++;
			}
		}

		if ( !numFront && !numBack )
		{
			// Totally planar polygon
			return Plane::PS_None;
		}
		else if ( !numFront )
		{
			// Back side
			return Plane::PS_Back;   
		}
		else
		{
			// Front side
			return Plane::PS_Front;
		}
	}

public:  
	//! Split polygon
	Bool Split( const Plane &plane, TPolygon<T> *&front, TPolygon<T> *&back, Float epsilon=POLYGON_CLIP_EPSILON ) const
	{
		Float* dists = ( Float* ) RED_ALLOCA( ( Size() + 1 ) * sizeof( Float ) );
		Int32* sides = ( Int32* ) RED_ALLOCA( ( Size() + 1  ) * sizeof( Int32 ) );
		Int32 numPos=0;
		Int32 numNeg=0;

		// Clasify vertices
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			Float dist = dists[i] = plane.DistanceTo( m_vertices[i].Position() );
			if ( dist > epsilon )
			{
				sides[i] = Plane::PS_Front;
				numPos++;
			}
			else if ( dist < -epsilon )
			{
				sides[i] = Plane::PS_Back;
				numNeg++;
			}
			else
			{
				sides[i] = Plane::PS_None;
			}    
		}

		// Clear pointers
		front = NULL;
		back = NULL;   

		// Check if whole face is planar
		if ( !numPos && !numNeg )
		{
			return true;
		}

		// Check if whole face will saved
		if ( !numNeg )
		{ 
			front = new TPolygon<T>( *this );
			return true;
		}

		// Check if whole face will saved
		if ( !numPos )
		{ 
			back = new TPolygon<T>( *this );
			return true;
		}

		// Wrap
		sides[ m_vertices.Size() ] = sides[0];
		dists[ m_vertices.Size() ] = dists[0];   

		// Create both parts
		front = new TPolygon<T>();
		back = new TPolygon<T>();

		// Copy user data
		front->m_userData = m_userData;
		back->m_userData = m_userData;

		// Clip polygon
		for ( Uint32 i=0; i<m_vertices.Size(); i++ )
		{
			// Both points
			if ( sides[i] == Plane::PS_None )
			{
				front->m_vertices.PushBack( m_vertices[i] );
				back->m_vertices.PushBack( m_vertices[i] );
				continue;
			}

			// Positive side only
			if ( sides[i] == Plane::PS_Front )
			{
				front->m_vertices.PushBack( m_vertices[i] );
			}

			// Negative side only
			if ( sides[i] == Plane::PS_Back )
			{
				back->m_vertices.PushBack( m_vertices[i] );
			}

			// Not spanning
			if ( sides[i+1] == Plane::PS_None || ( sides[i+1] == sides[i] ) )
			{
				continue;
			}

			// Split
			Float frac = dists[i] / ( dists[i] - dists[i+1] );
			ASSERT( frac >= 0.0f && frac <= 1.0f );
			T vertex = Lerp<T>( frac, m_vertices[ i ], m_vertices[ (i+1)%m_vertices.Size() ] );

			// Add vertex to both polygons
			front->m_vertices.PushBack( vertex );
			back->m_vertices.PushBack( vertex );
		}

		// Copy plane equations
		front->m_plane = m_plane;
		back->m_plane = m_plane;

		// Splitted for real
		return true;   
	}

	//! Clip polygon to plane
	TPolygon<T> *Clip( const Plane &plane, Bool keepPlanar, Bool keepFront, Float epsilon = POLYGON_CLIP_EPSILON ) const
	{
		TPolygon<T> *front=NULL;
		TPolygon<T> *back=NULL;

		// Handle planar polygon case
		if ( Clasify( plane, epsilon ) == Plane::PS_None )
		{
			if ( keepPlanar )
			{
				return new TPolygon( *this );
			}
			else
			{
				return NULL;
			}
		}

		// Split polygon
		Split( plane, front, back, epsilon );

		// Delete clipped part
		if ( keepFront )
		{
			if ( back )
			{
				delete back;
			}

			return front;
		}
		else
		{
			if ( front )
			{
				delete front;
			}

			return back;
		}
	}

	//! Recalculate polygon plane
	TPolygon<T> &RecalculatePlane()
	{
		// We need at least 3 vertices
		if ( m_vertices.Size() >= 3 )
		{
			m_plane= Plane( m_vertices[0].Position(), m_vertices[1].Position(), m_vertices[2].Position() );
		}

		return *this;
	}

public:  
	//! Serialize to stream
	friend IFile &operator<<( IFile& ar, TPolygon<T> &p )
	{
		ar << p.m_plane;
		ar << p.m_vertices;
		return ar;
	}
};

// Vector based polygon
typedef TPolygon< Vector > CPolygon;
typedef TDynArray< CPolygon*, MC_RenderData > TPolygonArray;