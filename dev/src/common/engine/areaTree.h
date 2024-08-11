/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#if 0
// Forward class declaration
class CAreaTree;
class CAreaTreeNode;
class CAreaTreeToken;
typedef TFragPool< CAreaTreeToken*, CAreaTreeNode* > CAreaFragPool;
typedef TFragElem< CAreaTreeToken*, CAreaTreeNode* > CAreaFrag;

/// Token for component in the area tree
class CAreaTreeToken
{
public:
	CBoundedComponent*	m_component;			//!< Drawable component this token is for
	CAreaFrag*			m_areaFrags;			//!< Element fragments in area tree
	Uint32				m_numAreaNodes;			//!< Number of area nodes this element is in
	Bool				m_singleAreaNode;		//!< This element was inserted as single area node
	Bool				m_collectedInGrab;		//!< Collected in a grab operation
	Uint32				m_renderAreaQueryIndex;	//!< Query index for area tree

public:
	//! Constructor
	RED_INLINE CAreaTreeToken( CBoundedComponent* component )
		: m_areaFrags( NULL )
		, m_numAreaNodes( 0 )
		, m_singleAreaNode( FALSE )
		, m_collectedInGrab( FALSE )
		, m_renderAreaQueryIndex( 0 )
		, m_component( component )
	{};
};

/// Area tree bounds
class CAreaTreeBounds
{
public:
	Vector		m_center;		//!< Bounds center
	Float		m_extent;		//!< Bounds extents

public:
	//! Default constructor
	RED_INLINE CAreaTreeBounds()
		: m_extent( 0.0f )
	{};

	//! Construct from box
	RED_INLINE CAreaTreeBounds( const Vector &center, Float extent )
		: m_center( center )
		, m_extent(extent )
	{};

	//! Construct from parent bounds and child index
	RED_INLINE CAreaTreeBounds( const CAreaTreeBounds &parentBounds, Int32 childIndex )
	{
		// Child has half the box size
		m_extent = parentBounds.m_extent * 0.5f;

		// Calculate new center
		m_center.A[0] = parentBounds.m_center.A[0] + (((childIndex & 1) << 1) - 1) * m_extent;
		m_center.A[1] = parentBounds.m_center.A[1] + (((childIndex & 2)     ) - 1) * m_extent;
		m_center.A[2] = parentBounds.m_center.A[2] + (((childIndex & 4) >> 1) - 1) * m_extent;
	}

	//! Returns true if we are inside given box
	RED_INLINE Bool IsInside( const Box &box ) const
	{
		if ( m_center.A[0] - m_extent < box.Min.A[0] || m_center.A[0] + m_extent > box.Max.A[0] )
		{
			return false;
		}
		else if ( m_center.A[1] - m_extent < box.Min.A[1] || m_center.A[1] + m_extent > box.Max.A[1] )
		{
			return false;
		}
		else if ( m_center.A[2] - m_extent < box.Min.A[2] || m_center.A[2] + m_extent > box.Max.A[2] )
		{
			return false;
		}
		else
		{
			return true;
		}
	};

	//! Get bounding box
	RED_INLINE Box ToBox() const
	{
		return Box( m_center, m_extent );
	}
};

/// Area tree node
class CAreaTreeNode
{
protected:
	CAreaTreeNode*		m_children;		//!< Child nodes
	CAreaFrag*			m_frags;		//!< Object fragments

public:
	CAreaTreeNode();
	~CAreaTreeNode();

	//! Remove all elements from this node, should be leaf node
	void RemoveAllElements();

	//! Count elements stored in this node
	Uint32 GetElementCount() const;

	//! Collect elements from this node
	void GetElements( Uint32 queryIndex, TDynArray< CBoundedComponent* >& components ) const;

	//! Collect all tokens in this node and below
	void GetTokens( TDynArray< CAreaTreeToken* >& tokens ) const;

	//! Collect elements overlapping given box
	void GetIntersectingElements( const Box &box, Uint32 queryIndex, TDynArray< CBoundedComponent* >& components, const CAreaTreeBounds &bounds ) const;

	//! Collect elements overlapping given box
	void GetIntersectingElements( const CFrustum &frustum, Uint32 queryIndex, TDynArray< CBoundedComponent* >& components, const CAreaTreeBounds &bounds ) const;

	//! Single node inserter
	void SingleNodeInsert( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds );

	//! Multiple node inserter
	Bool MultiNodeInsert( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds );

	//! Store element in this node
	void StoreElement( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds );

protected:	
	//! Create array of children node indices that this box overlaps
	static Uint32 FindChildren( const CAreaTreeBounds &parentBounds, const Box &testBox, Uint32* childIndices )
	{
		// Calculate node center
		const Vector &center = parentBounds.m_center;

		// Generate indices list
		Uint32 childCount = 0;
		if ( testBox.Max.A[0] > center.A[0] ) // XMAX
		{ 
			if ( testBox.Max.A[1] > center.A[1] ) // YMAX
			{
				if ( testBox.Max.A[2] > center.A[2] ) // ZMAX
				{
					childIndices[ childCount++ ] = 7;
				}

				if ( testBox.Min.A[2] <= center.A[2] )
				{
					childIndices[ childCount++ ] = 3;
				}
			}

			if ( testBox.Min.A[1] <= center.A[1] ) // YMIN
			{
				if ( testBox.Max.A[2] > center.A[2] ) // ZMAX
				{
					childIndices[ childCount++ ] = 5;
				}
				if ( testBox.Min.A[2] <= center.A[2] ) // ZMIN
				{
					childIndices[ childCount++ ] = 1;
				}
			}
		}

		if ( testBox.Min.A[0] <= center.A[0] ) // XMIN
		{ 
			if ( testBox.Max.A[1] > center.A[1] ) // YMAX
			{
				if ( testBox.Max.A[2] > center.A[2] ) // ZMAX
				{
					childIndices[ childCount++ ] = 6;
				}

				if ( testBox.Min.A[2] <= center.A[2] ) // ZMIN
				{
					childIndices[ childCount++ ] = 2;
				}
			}

			if ( testBox.Min.A[1] <= center.A[1] ) // YMIN
			{
				if ( testBox.Max.A[2] > center.A[2] ) // ZMAX
				{
					childIndices[ childCount++ ] = 4;
				}

				if ( testBox.Min.A[2] <= center.A[2] ) // ZMIN
				{
					childIndices[ childCount++ ] = 0;
				}
			}
		}

		return childCount;
	}

	//! Returns which child node 'testBox' would fit into.
	static Int32 FindChild( const CAreaTreeBounds &parentBounds, const Box& testBox ) 
	{
		INT result = 0;

		// Calculate node center
		const Vector &center = parentBounds.m_center;

		// X axis
		if ( testBox.Min.A[0] > center.A[0] )
		{
			result |= 1;
		}
		else if ( testBox.Max.A[0] > center.A[0] )
		{
			return -1;
		}

		// Y axis
		if ( testBox.Min.A[1] > center.A[1] )
		{
			result |= 2;
		}
		else if ( testBox.Max.A[1] > center.A[1] )
		{
			return -1;
		}

		// Z axis
		if ( testBox.Min.A[2] > center.A[2] )
		{
			result |= 4;
		}
		else if ( testBox.Max.A[2] > center.A[2] )
		{
			return -1;
		}

		return result;
	}

};

/// AABB based area tree for scene component management
class CAreaTree
{
	friend class CAreaTreeNode;

private:
	CAreaFragPool					m_fragPool;		//!< Pool of fragments
	CAreaTreeNode*					m_root;			//!< Area tree
	TDynArray< CAreaTreeToken* >	m_failed;		//!< Component that because of the tree split are in to many nodes
	mutable Uint32					m_queryIndex;	//!< Query index for area components
	mutable Red::Threads::CMutex	m_lock;			//!< Master lock for adding/removing elements

public:
	//! Constructor
	CAreaTree();

	//! Destructor
	~CAreaTree();

	//! Insert/Remove component from area tree
	void UpdateElement( CBoundedComponent* remove, CBoundedComponent* add );

	//! Collect elements inside box
	void CollectElements( const Box& frustum, TDynArray< CBoundedComponent* > &elements ) const;

	//! Collect elements inside frustum
	void CollectElements( const CFrustum &frustum, TDynArray< CBoundedComponent* > &elements ) const;
};
#endif