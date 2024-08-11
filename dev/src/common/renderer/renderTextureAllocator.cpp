/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderTextureAllocator.h"

CRenderTextureAllocator::CRenderTextureAllocator( Uint16 width, Uint16 height )
	: m_width( width )
	, m_height( height )
{
	// Allocate initial empty node
	new ( m_nodes ) Node( 0, 0, width, height );
}

void CRenderTextureAllocator::Reset()
{
	m_nodes.ClearFast();
	new ( m_nodes ) Node( 0, 0, m_width, m_height );
}

Bool CRenderTextureAllocator::AllocateSpace( Uint16 width, Uint16 height, Uint16* offsetX, Uint16* offsetY )
{
	// Try allocating space without enlarging first
	Int32 nodeIndex = AllocateNode( 0, width, height );
	if ( nodeIndex == -1 )
	{
		// No space to allocate chunk
		return false;
	}

	// Mark node as used
	Node& node = m_nodes[ nodeIndex ];
	node.m_used = true;

	// Return allocated place
	*offsetX = node.m_offsetX;
	*offsetY = node.m_offsetY;

	// Allocated
	return true;
}

Int32 CRenderTextureAllocator::AllocateNode( Int32 nodeIndex, Uint16 width, Uint16 height )
{
	Node& node = m_nodes[ nodeIndex ];

	// Recurse to child nodes
	if ( !node.isLeaf() )
	{
		// Try in first sub tree
		Int32 allocatedNode = AllocateNode( node.m_children[0], width, height );

		// Try in the second one
		if ( allocatedNode == -1 )
		{
			allocatedNode = AllocateNode( node.m_children[1], width, height );
		}

		// Return allocated node
		return allocatedNode;
	}

	// Leaf is already used
	if ( node.m_used )
	{
		return -1;
	}

	// Leaf is to small
	if ( node.m_sizeX < width || node.m_sizeY < height )
	{
		return -1;
	}

	// Exact match
	if ( node.m_sizeX == width && node.m_sizeY == height )
	{
		return nodeIndex;
	}

	// Split the node
	m_nodes[ nodeIndex ].m_children[0] = m_nodes.Size();
	m_nodes[ nodeIndex ].m_children[1] = m_nodes.Size()+1;
	

	// Split along the best axis
	if ( node.m_sizeX - width > node.m_sizeY - height )
	{
		new ( m_nodes ) Node( m_nodes[ nodeIndex ].m_offsetX, m_nodes[ nodeIndex ].m_offsetY, width, m_nodes[ nodeIndex ].m_sizeY );
		new ( m_nodes ) Node( m_nodes[ nodeIndex ].m_offsetX + width, m_nodes[ nodeIndex ].m_offsetY, m_nodes[ nodeIndex ].m_sizeX - width, m_nodes[ nodeIndex ].m_sizeY );
	}
	else
	{
		new ( m_nodes ) Node( m_nodes[ nodeIndex ].m_offsetX, m_nodes[ nodeIndex ].m_offsetY, m_nodes[ nodeIndex ].m_sizeX, height );
		new ( m_nodes ) Node( m_nodes[ nodeIndex ].m_offsetX, m_nodes[ nodeIndex ].m_offsetY + height, m_nodes[ nodeIndex ].m_sizeX, m_nodes[ nodeIndex ].m_sizeY - height );
	}
	
	// Allocate from the smaller node
	return AllocateNode( m_nodes[ nodeIndex ].m_children[0], width, height );
}