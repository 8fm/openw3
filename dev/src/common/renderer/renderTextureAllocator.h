/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Texture space allocator
class CRenderTextureAllocator
{
protected:
	// BSP node
	struct Node
	{
		Bool	m_used;
		Int32		m_children[2];
		Uint16	m_offsetX;
		Uint16	m_offsetY;
		Uint16	m_sizeX;
		Uint16	m_sizeY;

		// Constructor
		RED_INLINE Node( Uint16 offsetX, Uint16 offsetY, Uint16 width, Uint16 height )
			: m_offsetX( offsetX )
			, m_offsetY( offsetY )
			, m_sizeX( width )
			, m_sizeY( height )
			, m_used( false )
		{
			m_children[0] = -1;
			m_children[1] = -1;
		}

		// Is a leaf ?
		RED_INLINE bool isLeaf() const
		{
			return ( m_children[0] == -1 ) && ( m_children[1] == -1 );
		}
	};

protected:
	TDynArray< Node >	m_nodes;		// Nodes
	Uint16				m_width;
	Uint16				m_height;

public:
	CRenderTextureAllocator( Uint16 width, Uint16 height );

	//! Reset space allocation
	void Reset();

	//! Allocate texture space
	Bool AllocateSpace( Uint16 width, Uint16 height, Uint16* offsetX, Uint16* offsetY );

protected:
	Int32 AllocateNode( Int32 nodeIndex, Uint16 width, Uint16 height );
};
