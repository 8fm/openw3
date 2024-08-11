/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once 

// Simple BSP based texture allocator
class TextureAllocator
{
protected:
	struct Node
	{
		Int32			m_Children[2];
		Uint32		m_Offset[2];
		Uint32		m_Size[2];
		bool		m_Used;

		RED_INLINE bool IsLeaf() const
		{
			return m_Children[0] == -1 && m_Children[1] == -1;
		}

		RED_INLINE Node( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
			: m_Used( false )
		{
			m_Children[0] = -1;
			m_Children[1] = -1;
			m_Offset[0] = x;
			m_Offset[1] = y;
			m_Size[0] = width;
			m_Size[1] = height;
		}

		RED_INLINE bool WillFit( Uint32 width, Uint32 height ) const
		{
			return width <= m_Size[0] && height <= m_Size[1];
		}
	};

protected:
	Uint32					m_width;
	Uint32					m_height;
	TDynArray< Node >		m_nodes;

public:
	//! Get allocator current width
	RED_INLINE Uint32 GetWidth() const { return m_width; }

	//! Get allocator current height
	RED_INLINE Uint32 GetHeight() const { return m_height; }

public:
	TextureAllocator( Uint32 width, Uint32 height )
		: m_width( 32 )
		, m_height( 32 )
	{
		m_nodes.PushBack( Node( 0, 0, width, height ) );
	};

	bool Allocate( Uint32 width, Uint32 height, Uint32& offsetX, Uint32& offsetY )
	{
		// Allocate space
		Int32 nodeIndex =  AllocateNode( 0, width, height, false );
		if ( nodeIndex == -1 )
		{
			// Try to allocate with resizing
			nodeIndex =  AllocateNode( 0, width, height, true );
			if ( nodeIndex == -1 )
			{
				return false;
			}
		}

		// Get allocated offset
		offsetX = m_nodes[ nodeIndex ].m_Offset[0];
		offsetY = m_nodes[ nodeIndex ].m_Offset[1];

		// Update size
		m_width = Max< Uint32 >( m_width, offsetX + width );
		m_height = Max< Uint32 >( m_height, offsetY + height );

		// Mark node as used
		m_nodes[ nodeIndex ].m_Used = true;
		return true;
	}

protected:
	int AllocateNode( Int32 nodeIndex, Uint32 width, Uint32 height, Bool canResize )
	{
		const Node node = m_nodes[ nodeIndex ];

		// To small
		if ( !node.WillFit( width, height ) )
		{
			return -1;
		}

		// Not a leaf, recurse
		if ( !node.IsLeaf() )
		{
			int allocatedNode = AllocateNode( node.m_Children[0], width, height, canResize );
			if ( allocatedNode == -1 )
			{
				allocatedNode = AllocateNode( node.m_Children[1], width, height, canResize );
			}
			return allocatedNode;
		}

		// Already used
		if ( node.m_Used )
		{
			return -1;
		}

		// Check if we fit without resizing
		if ( !canResize && ((node.m_Offset[0] + width > m_width) || (node.m_Offset[1] + height > m_height)) )
		{
			return -1;
		}

		// Ideal size, use
		if ( node.m_Size[0] == width && node.m_Size[1] == height )
		{
			return nodeIndex;
		}

		// Split
		Uint32 childIndex = m_nodes.Size();
		if ( node.m_Size[0] - width > node.m_Size[1] - height )
		{
			m_nodes.PushBack( Node( node.m_Offset[0], node.m_Offset[1], width, node.m_Size[1] ) );
			m_nodes.PushBack( Node( node.m_Offset[0] + width, node.m_Offset[1], node.m_Size[0] - width, node.m_Size[1] ) );
		}
		else
		{
			m_nodes.PushBack( Node( node.m_Offset[0], node.m_Offset[1], node.m_Size[0], height) );
			m_nodes.PushBack( Node( node.m_Offset[0], node.m_Offset[1]+height, node.m_Size[0], node.m_Size[1]-height ) );
		}

		// Allocate child nodes
		m_nodes[ nodeIndex ].m_Children[0] = childIndex+0;
		m_nodes[ nodeIndex ].m_Children[1] = childIndex+1;

		// Allocate from smaller split node
		return AllocateNode( childIndex, width, height, canResize );
	}
};

