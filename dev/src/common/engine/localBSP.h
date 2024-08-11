/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "polygon.h"

/// Planar case solver
class IPlanarSolver
{
public:
	virtual ~IPlanarSolver() {}
	//! Solve planar case	
	virtual Plane::ESide Solve( const Plane& polygonPlane, const Plane &plane ) = 0;
};

/// Solid leaf BSP for sectors/brushes/other stuff
class CLocalBSP
{
private:  
	/// Leaf type
	enum ELeafType
	{
		LEAF_Inside,
		LEAF_Outside,
	};

	//! Construction face
	struct Face
	{
		DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );

		CPolygon*	m_polygon;			//!< Source polygon
		Bool		m_wasSplitter;		//!< Was this face splitted

		//! Constructor
		RED_INLINE Face( CPolygon *poly, Bool wasSplitter )
			: m_polygon( poly )
			, m_wasSplitter( wasSplitter )
		{};

		//! Destructor
		RED_INLINE ~Face()
		{
			delete m_polygon;
			m_polygon = NULL;
		}

		//! Split face, deletes original polygon, face is no longer useable
		void Split( const Plane &plane, Face *&front, Face *&back );
	};

	//! BSP Node
	struct Node
	{
		Plane	m_orgPlane;			//!< Original plane (for nodes)
		Plane	m_plane;			//!< Transformed plane
		Int16	m_children[2];		//!< Children (for nodes)
		Int32		m_leafType;			//!< Leaf type

		//! Default constructor
		RED_INLINE Node()
		{
			m_children[0] = -1;
			m_children[1] = -1;
			m_leafType = LEAF_Inside;
		}

		//! Is this a leaf ?
		RED_INLINE Bool IsLeaf() const
		{
			return m_children[0] == -1 && m_children[1] == -1;
		}

		//! Serialization
		friend RED_INLINE IFile& operator<<( IFile& ar, Node& node )
		{
			ar << node.m_children[0];
			ar << node.m_children[1];
			ar << node.m_leafType;
			ar << node.m_orgPlane;
			return ar;
		}
	};

private:
	TDynArray< Node >	m_nodes;			//!< BSP nodes

public:
	//! Print tree structure
	void Print() const;  

	//! Build tree from polygons
	void Build( const TDynArray< CPolygon* > &faces );

	//! Add global clipping plane
	void AddClipPlane( const Plane &plane, Bool outsideZone );

	//! Transform BSP planes
	void Transform( const Matrix& matrix );

public:
	//! Split polygon
	template< class T >
	void SplitNode( Int32 nodeIndex, T* poly, TDynArray< T* > &insideFrags, TDynArray< T* > &outsideFrags, IPlanarSolver *solver ) const
	{
		ASSERT( nodeIndex != -1 );

		// Get node
		const Node& node = m_nodes[ nodeIndex ];

		// In leaf
		if ( node.IsLeaf() )
		{ 
			// Add polygon to leaf lists
			if ( node.m_leafType == LEAF_Outside )
			{
				outsideFrags.PushBack( poly );
			}
			else if ( node.m_leafType == LEAF_Inside )
			{
				insideFrags.PushBack( poly );
			}
			else
			{
				// WTF
				delete poly;
			}    

			// We're done
			return;   
		}

		// Clasify poly
		const Plane &plane = node.m_plane;
		Plane::ESide side = poly->Clasify( plane );

		// Planar case, resolve
		if ( side == Plane::PS_None )
		{
			// Use solver if we have one
			if ( solver )
			{
				side = solver->Solve( poly->m_plane, plane );
			}
			else
			{
				// Use simple logic, assign polygons to either front or back list
				Float dot = Vector::Dot3( plane.NormalDistance, poly->m_plane.NormalDistance );
				side = dot > 0.0f ? Plane::PS_Front : Plane::PS_Back;
			}
		}

		// If solver decided to leave face on plane so send it to both sides
		if ( side == Plane::PS_None )
		{
			TDynArray< T* > tempFrags;

			// Clip to front side
			SplitNode( node.m_children[0], poly, insideFrags, tempFrags, solver );

			// Clip all generated fragments to back side
			for ( Uint32 i=0; i<tempFrags.Size(); i++ )
			{
				SplitNode( node.m_children[1], tempFrags[i], insideFrags, outsideFrags, solver );
			}         
		}

		// Splitted by node plane
		if ( side == Plane::PS_Both )
		{
			T *frontPoly = NULL;
			T *backPoly = NULL;

			// Split polygon
			poly->Split( plane, frontPoly, backPoly );
			ASSERT( frontPoly );
			ASSERT( backPoly );

			// Send to both sides
			TDynArray< T* > realInsideFrags, realOutsideFrags;
			SplitNode( node.m_children[0], frontPoly, realInsideFrags, realOutsideFrags, solver );
			SplitNode( node.m_children[1], backPoly, realInsideFrags, realOutsideFrags, solver );

			// No real inside fragments, use original polygon
			if ( !realInsideFrags.Size() )
			{
				// Delete clipped outside frags
				realOutsideFrags.ClearPtr();

				// Use source polygon
				outsideFrags.PushBack( poly );
			}
			else if ( !realOutsideFrags.Size() )
			{
				// Delete clipped inside frags
				realInsideFrags.ClearPtr();

				// Use source polygon
				insideFrags.PushBack( poly );
			}
			else
			{
				// Use splitted polygons
				outsideFrags.PushBack( realOutsideFrags );
				insideFrags.PushBack( realInsideFrags );

				// Delete original polygon
				delete poly;
			}
		}

		// Simple cases
		if ( side == Plane::PS_Front )
		{
			SplitNode( node.m_children[0], poly, insideFrags, outsideFrags, solver );
		}
		else if ( side == Plane::PS_Back )
		{
			SplitNode( node.m_children[1], poly, insideFrags, outsideFrags, solver );
		}
	}

	//! Split polygon into solid and empty leaf parts, used for CSG
	template< class T >
	void SplitPoly( T* poly, TDynArray< T* > &insideFrags, TDynArray< T* > &outsideFrags, IPlanarSolver *solver ) const
	{
		ASSERT( poly );

		// Copy input polygon   
		T *copy = new T( *poly );

		// Split polygon using BSP
		TDynArray< T* > realInsideFrags, realOutsideFrags;
		SplitNode( 0, poly, realInsideFrags, realOutsideFrags, solver );

		// Nothing created ( strange... )
		if ( !realInsideFrags.Size() && !realOutsideFrags.Size() )
		{
			delete copy;
			return;
		}

		// No fragments inside BSP
		if ( !realInsideFrags.Size() )
		{
			// Delete generated fragments
			for ( Uint32 i=0; i<realOutsideFrags.Size(); i++ )
			{
				delete realOutsideFrags[i];
			}

			// Use original polygon instead of sliced one
			outsideFrags.PushBack( copy );
			return;  
		}

		// No fragments outside BSP
		if ( !realOutsideFrags.Size() )
		{
			// Delete generated fragments
			for ( Uint32 i=0; i<realInsideFrags.Size(); i++ )
			{
				delete realInsideFrags[i];
			}

			// Use original polygon instead of sliced one
			insideFrags.PushBack( copy );
			return;  
		}  

		// Copy outside fragments
		outsideFrags = realOutsideFrags;
		insideFrags = realInsideFrags;

		// Delete temporary polygon copy
		delete copy;
	}

public:
	//! Serialize
	friend IFile& operator<<( IFile& file, CLocalBSP& bsp )
	{
		file << bsp.m_nodes;
		return file;
	}

private:
	//! Build BSP node
	Int32 CompileNode( const TDynArray< Face* > &faces );

	//! Print node
	void PrintNode( Int32 node, Int32 level ) const;
};

