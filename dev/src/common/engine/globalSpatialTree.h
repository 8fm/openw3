/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "integerVector.h"
#include "nodeAllocator.h"
#include "renderFrame.h"

//---------------------------------------------------------------------------
//
// An optimized Quad tree based tree structure that uses integer coordinates (to accomodate better for large world sizes)
//
// The main optimization factor is that the leafs and nodes store the actual bounding boxes
// of the geometry inside the nodes so better broad phase and queries performance can be achieved.
//
// The top level of the tree consists of the NxN grid instead of a normal hierarchy.
// This saves a lot of unnecessary splits at the top level and makes the tree depth smaller.
//
// Access and storage is done through tokens.
//
//---------------------------------------------------------------------------

template< typename T, 
	Red::MemoryFramework::MemoryClass MemClass = MC_GlobalSpatialTree, 
	RED_CONTAINER_POOL_TYPE MemPool = MemoryPool_Default >
class TGlobalSpatialTree
{	
	/// General size of the top level grid cell (in integer units)
	const static Int32 GRID_CELL_SIZE = 256 * 1024; // around 1024 meters
	const static Int32 GRID_CELL_COUNT = 32; // 32 grid cells in one row
	const static Int32 GRID_CELL_OFFSET = (GRID_CELL_COUNT/2) * GRID_CELL_SIZE;

	/// Maximum depth of the tree
	const static Uint32 MAX_DEPTH = 8; // this makes the smallest cell around 4x4 meters

	/// Number of children per node - 4 for QuadTree, 8 for OccTree
	const static Uint32 NUM_CHILDREN = 4;

public:
	struct Node;
	struct TokenLink;
	struct Token;

public:
	/// Internal token
	struct Token
	{
		// World space bounds
		IntegerBox m_worldBounds;

		// User data (pointer to user related data structure)
		T* m_userData;

		// First node we are in
		TokenLink* m_nodeList;
	};

	/// Reference to a token inside the node
	struct TokenLink
	{
		// Actual referenced token
		Token* m_token;

		// Actual node
		Node* m_node;

		// Linked list of tokens in the node
		TokenLink* m_nextToken;
		TokenLink** m_prevToken;

		// Linked list of nodes given token is in
		TokenLink* m_nextNode;
		TokenLink** m_prevNode;
	};

	/// Basic node
	struct Node
	{
		// Bounding box of content (always at offset 0, do not move)
		IntegerBox m_contentBox;

		// Tokens stored at this node
		TokenLink* m_tokenList;

		// Parent node
		Node* m_parent;

		// Allocated child nodes (always in group of NUM_CHILDREN)
		Node* m_children;

		// Does the node (or any subnode) contain any elements
		Uint32 m_hasElements:1;

		// Does this node content box is not up to date ?
		Uint32 m_hasDirtyBounds:1;
	};

private:
	//! Root nodes for reach grid cell, preallocated
	Node m_gridNodes[GRID_CELL_COUNT * GRID_CELL_COUNT];
	Bool m_gridNodesDirtyFlag[GRID_CELL_COUNT * GRID_CELL_COUNT];

	//! List of grid cells that require update
	typedef TDynArray<Uint32> TGridCellUpdateList;
	TGridCellUpdateList m_cellUpdateList;

	//! Memory allocator for tree nodes, watch out for the BulkCount = NUM_CHILDREN
	typedef TNodeAllocator< Node, TDefaultPageAllocator< MemClass, MemPool >, 16, NUM_CHILDREN > TNodesAllocator;
	TNodesAllocator m_nodeAllocator;

	//! Memory allocator for convex pieces
	typedef TNodeAllocator< TokenLink, TDefaultPageAllocator< MemClass, MemPool >, 4 > TConvexLinkAllocator;
	TConvexLinkAllocator m_linkAllocator;

	//! Memory allocator for the tokens
	typedef TNodeAllocator< Token, TDefaultPageAllocator< MemClass, MemPool >, 4 > TTokenAllocator;
	TTokenAllocator m_tokenAllocator;

public:
	TGlobalSpatialTree()
	{
		Red::System::MemoryZero(m_gridNodes, sizeof(m_gridNodes));
		Red::System::MemoryZero(m_gridNodesDirtyFlag, sizeof(m_gridNodesDirtyFlag));
	}

	//! Insert an object into the tree, requires you to keep the token around
	Token* Insert(const IntegerBox& worldBounds, T* userData)
	{
		// Allocate token object
		Token* token = m_tokenAllocator.Allocate();
		token->m_worldBounds = worldBounds;
		token->m_nodeList = NULL;
		token->m_userData = userData;

		// Get the range of grid nodes affected by the world bounds size
		// TODO: this can (and should) be converted into SSE math, especially because of the clamping
		const Int32 gridMinX = Clamp<Int32>((worldBounds.Min.X.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMinY = Clamp<Int32>((worldBounds.Min.Y.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMaxX = Clamp<Int32>((worldBounds.Max.X.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMaxY = Clamp<Int32>((worldBounds.Max.Y.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);

		// Size of the inserted data (determines how far we will split the tree)
		const Uint32 tokenSize = Max(worldBounds.Max.X - worldBounds.Min.X, 
			worldBounds.Max.Y - worldBounds.Min.Y).ToInt32();

		// Process all affected grid nodes
		for (Int32 y=gridMinY; y<=gridMaxY; ++y)
		{
			for (Int32 x=gridMinX; x<=gridMaxX; ++x)
			{
				// Calculate grid node bounding box, only XY matters
				IntegerBox gridCellBox;
				gridCellBox.Min.X = IntegerUnit(x * GRID_CELL_SIZE - GRID_CELL_OFFSET);
				gridCellBox.Min.Y = IntegerUnit(y * GRID_CELL_SIZE - GRID_CELL_OFFSET);
				gridCellBox.Min.Z = IntegerUnit(INT_MIN);
				gridCellBox.Max.X = gridCellBox.Min.X + IntegerUnit(GRID_CELL_SIZE);
				gridCellBox.Max.Y = gridCellBox.Min.Y + IntegerUnit(GRID_CELL_SIZE);
				gridCellBox.Max.Z = IntegerUnit(INT_MAX);

				// Insert the token into the quad tree at the grid level, this may split the node
				const Uint32 gridNodeIndex = x + y * GRID_CELL_COUNT;
				if (InsertIntoNode(&m_gridNodes[ gridNodeIndex ], token, tokenSize, gridCellBox, 0))
				{
					// Grid cell got modified, mark it as dirty
					if (!m_gridNodesDirtyFlag[ gridNodeIndex ])
					{
						m_gridNodesDirtyFlag[ gridNodeIndex ] = 1;
						m_cellUpdateList.PushBack(gridNodeIndex);
					}
				}
			}
		}

		// Return inserted token
		return token;
	}

	//! Remove a convex from the tree
	void Remove(Token* token)
	{
		// Unlink token from all nodes it it in
		TokenLink* cur = token->m_nodeList;
		while (cur != NULL)
		{
			// Mark all nodes on the way as dirty
			Node* node = cur->m_node;
			while (NULL != node)
			{
				// Already marked
				if (node->m_hasDirtyBounds)
				{
					break;
				}

				// Root node
				if (node->m_parent == NULL)
				{
					// Mark the grid cell as dirty
					const Uint32 gridCellIndex = (const Uint32)( node - &m_gridNodes[0] );
					ASSERT(gridCellIndex < ARRAY_COUNT(m_gridNodes));
					
					if (!m_gridNodesDirtyFlag[ gridCellIndex ])
					{
						m_gridNodesDirtyFlag[ gridCellIndex ] = 1;
						m_cellUpdateList.PushBack( gridCellIndex );
					}
				}

				// Mark as dirty and go to the parent node
				node->m_hasDirtyBounds = true;
				node = node->m_parent;
			}

			// Release the link to the pool
			TokenLink* next = cur->m_nextNode;
			ReleaseLink(cur);
			cur = next;
		}

		// Make sure it's cleared
		ASSERT(NULL == token->m_nodeList);
		m_tokenAllocator.Free( token );
	}

	//! Perform structure update on the dirty nodes
	void Update()
	{
		// Rebuild only the dirty cells
		for (Uint32 i=0; i<m_cellUpdateList.Size(); ++i)
		{
			const Uint32 gridNodeIndex = m_cellUpdateList[i];

			// Reset dirty flag
			ASSERT(m_gridNodesDirtyFlag[ gridNodeIndex ] == 1);
			m_gridNodesDirtyFlag[ gridNodeIndex ] = 0;

			// Refit bounding boxes on modified nodes
			UpdateContentBoxes(&m_gridNodes[ gridNodeIndex ]);
		}

		// Reset update list
		m_cellUpdateList.ClearFast();
	}

	//! Collect objects (by user data) from given area
	template< class Func >
	void CollectObjects(const IntegerBox& worldBounds, Func& functor) const
	{
		// Get the range of grid nodes affected by the world bounds size
		// TODO: this can (and should) be converted into SSE math, especially because of the clamping
		const Int32 gridMinX = Clamp<Int32>((worldBounds.Min.X.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMinY = Clamp<Int32>((worldBounds.Min.Y.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMaxX = Clamp<Int32>((worldBounds.Max.X.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);
		const Int32 gridMaxY = Clamp<Int32>((worldBounds.Max.Y.ToInt32() + GRID_CELL_OFFSET) / GRID_CELL_SIZE, 0, GRID_CELL_COUNT-1);

		// Process all affected grid cells
		for (Int32 y=gridMinY; y<=gridMaxY; ++y)
		{
			for (Int32 x=gridMinX; x<=gridMaxX; ++x)
			{
				const Uint32 gridNodeIndex = x + y * GRID_CELL_COUNT;
				CollectObjectFromNode(&m_gridNodes[ gridNodeIndex ], worldBounds, functor);
			}
		}
	}

	//! Render the active tree
	void Render( class CRenderFrame* frame )
	{
		// Process all affected grid cells
		for (Int32 y=0; y<GRID_CELL_COUNT; ++y)
		{
			for (Int32 x=0; x<GRID_CELL_COUNT; ++x)
			{
				// Calculate grid node bounding box, only XY matters
				IntegerBox gridCellBox;
				gridCellBox.Min.X = IntegerUnit(x * GRID_CELL_SIZE - GRID_CELL_OFFSET);
				gridCellBox.Min.Y = IntegerUnit(y * GRID_CELL_SIZE - GRID_CELL_OFFSET);
				gridCellBox.Min.Z = IntegerUnit(-50.0f);
				gridCellBox.Max.X = gridCellBox.Min.X + IntegerUnit(GRID_CELL_SIZE);
				gridCellBox.Max.Y = gridCellBox.Min.Y + IntegerUnit(GRID_CELL_SIZE);
				gridCellBox.Max.Z = IntegerUnit(50.0f);

				// Render cell
				const Uint32 gridNodeIndex = x + y * GRID_CELL_COUNT;
				RenderNode(&m_gridNodes[ gridNodeIndex ], frame, gridCellBox);
			}
		}
	}

private:
	//! Create a link between the token and give node
	void AddTokenToNode(Node* node, Token* token)
	{
		// Allocate link info
		TokenLink* link = m_linkAllocator.Allocate();
		link->m_token = token;
		link->m_node = node;

		// Link into the list of tokens in the node
		if (node->m_tokenList) node->m_tokenList->m_prevToken = &link->m_nextToken;
		link->m_prevToken = &node->m_tokenList;
		link->m_nextToken = node->m_tokenList;
		node->m_tokenList = link;

		// Link into the list of nodes given token is in
		if (token->m_nodeList) token->m_nodeList->m_prevNode = &link->m_nextNode;
		link->m_prevNode = &token->m_nodeList;
		link->m_nextNode = token->m_nodeList;
		token->m_nodeList = link;

		// Mark node as having dirty content box
		node->m_hasElements = 1;
		node->m_hasDirtyBounds = 1;
	}

	//! Unlink and free the link structure
	void ReleaseLink(TokenLink* link)
	{
		// Relink the token list pointers
		*link->m_prevToken = link->m_nextToken;
		if (link->m_nextToken) link->m_nextToken->m_prevToken = link->m_prevToken;

		// Relink the node list pointers
		*link->m_prevNode = link->m_nextNode;
		if (link->m_nextNode) link->m_nextNode->m_prevNode = link->m_prevNode;

		// Clean the link data
		link->m_node = NULL;
		link->m_token = NULL;

		// Return link to the allocator
		m_linkAllocator.Free(link);
	}

	//! Calculate child box
	void CalcChildBox2D(const IntegerBox& nodeBox, const Uint32 childIndex, IntegerBox& outChildBox)
	{
		const IntegerVector4 half = (nodeBox.Min + nodeBox.Max) / 2;

		outChildBox.Min.Z = nodeBox.Min.Z;
		outChildBox.Max.Z = nodeBox.Max.Z;

		if (childIndex & 1)
		{
			outChildBox.Min.X = half.X;
			outChildBox.Max.X = nodeBox.Max.X;
		}
		else
		{
			outChildBox.Min.X = nodeBox.Min.X;
			outChildBox.Max.X = half.X;
		}

		if (childIndex & 2)
		{
			outChildBox.Min.Y = half.Y;
			outChildBox.Max.Y = nodeBox.Max.Y;
		}
		else
		{
			outChildBox.Min.Y = nodeBox.Min.Y;
			outChildBox.Max.Y = half.Y;
		}
	}

	//! Insert a token into the tree
	Bool InsertIntoNode(Node* node, Token* token, const Uint32 tokenSize, const IntegerBox& nodeBox, const Uint32 currentDepth)
	{
		// We are not yet at the depth limit, try to insert the token into the child nodes
		if (currentDepth < MAX_DEPTH)
		{
			// We can try to fit the parts into the nodes
			const Uint32 nodeLevelSize = (nodeBox.Max.X - nodeBox.Min.X).ToInt32();
			if (tokenSize < nodeLevelSize)
			{
				// Node has no child nodes yet, create them
				if (NULL == node->m_children)
				{
					node->m_children = m_nodeAllocator.Allocate();
					Red::System::MemoryZero(node->m_children, sizeof(Node)*NUM_CHILDREN);

					// Set the parent pointer
					for (Uint32 i=0; i<NUM_CHILDREN; ++i)
					{
						node->m_children[i].m_parent = node;
					}
				}

				// Insert into overlapping nodes		
				Bool wasInserted = false;
				for (Uint32 i=0; i<NUM_CHILDREN; ++i)
				{
					IntegerBox childBox;
					CalcChildBox2D(nodeBox, i, childBox);

					if (childBox.Touches(token->m_worldBounds))
					{
						if (InsertIntoNode(&node->m_children[i], token, tokenSize, childBox, currentDepth+1))
						{
							wasInserted = true;
						}
					}
				}

				// Mark the node as dirty
				if (wasInserted)
				{
					node->m_hasDirtyBounds = 1;
				}

				// Return true if the node was inserted in at least one child node
				return wasInserted;
			}
		}

		// Insert at this level
		AddTokenToNode(node, token);
		return true;		
	}

	// Update the nodes with dirty bounding boxes
	void UpdateContentBoxes(Node* node)
	{
		Bool hasElements = (node->m_tokenList != NULL);

		// Calculate local content box
		IntegerBox box;
		for (TokenLink* cur = node->m_tokenList; 
			cur != NULL; cur = cur->m_nextToken)
		{
			box.AddBox(cur->m_token->m_worldBounds);
		}

		// Recurse to child nodes (only the dirty ones)
		if (NULL != node->m_children)
		{
			for (Uint32 i=0; i<NUM_CHILDREN; ++i)
			{
				Node* childNode = &node->m_children[i];
				if (childNode->m_hasDirtyBounds)
				{
					UpdateContentBoxes(childNode);
				}

				// Merge the "has elements" flag (early exit)
				if (childNode->m_hasElements)
				{
					// Merge with the content boxes from child nodes		
					box.AddBox(childNode->m_contentBox);
					hasElements = true;
				}
			}
		}		

		// Reset dirty flags and setup new content box
		node->m_contentBox = box;
		node->m_hasDirtyBounds = 0;

		// Set new "has elements" flag
		node->m_hasElements = hasElements;
	}

	//! Render tree node
	void RenderNode(const Node* node, class CRenderFrame* frame, const IntegerBox& nodeBox)
	{
		// node is empty, do not draw
		if ( !node->m_hasElements )
		{
			return;
		}

		// draw the node box
		{
			Box drawBox;
			drawBox.Min = nodeBox.Min.ToVector();
			drawBox.Max = nodeBox.Max.ToVector();
			
			drawBox.Min.Z = node->m_contentBox.Min.Z.ToFloat() - 5.0f;
			drawBox.Max.Z = node->m_contentBox.Max.Z.ToFloat() + 5.0f;

			frame->AddDebugBox( drawBox, Matrix::IDENTITY, Color::BROWN, false );
		}

		// draw the content box and stats
		if ( NULL != node->m_tokenList )
		{
			Box drawBox;
			drawBox.Min = node->m_contentBox.Min.ToVector();
			drawBox.Max = node->m_contentBox.Max.ToVector();
			frame->AddDebugBox( drawBox, Matrix::IDENTITY, Color::WHITE, false );

			// count number of fragments
			Uint32 numFrags = 0;
			const Vector boxCenter = nodeBox.CalcCenter().ToVector();
			for (TokenLink* cur = node->m_tokenList; 
				cur != NULL; cur = cur->m_nextToken)
			{
				const Vector convexPos = cur->m_token->m_worldBounds.CalcCenter().ToVector();
				frame->AddDebugLine( boxCenter, convexPos, Color::GREEN, false );

				++numFrags;
			}

			// print number of fragments in the node
			frame->AddDebugText(boxCenter, String::Printf( TXT("%d"), numFrags ), 0, 0, true);
		}

		// recurse
		if ( NULL != node->m_children )
		{
			for (Uint32 i=0; i<NUM_CHILDREN; ++i)
			{
				IntegerBox childBox;
				CalcChildBox2D(nodeBox, i, childBox);

				RenderNode(&node->m_children[i], frame, childBox);
			}
		}
	}

	//! Collect objects from node using provided functor
	template< typename Func >
	void CollectObjectFromNode(const Node* node, const IntegerBox& worldBounds, Func& functor) const
	{
		// Recurse to child nodes if overlap is detected
		if (NULL != node->m_children)
		{
			for (Uint32 i=0; i<NUM_CHILDREN; ++i)
			{
				const Node* childNode = &node->m_children[i];
				if (childNode->m_hasElements)
				{
					if (childNode->m_contentBox.Touches(worldBounds))
					{
						CollectObjectFromNode(childNode, worldBounds, functor);
					}
				}
			}
		}

		// Process local elements
		for (const TokenLink* cur = node->m_tokenList; 
			cur != NULL; cur = cur->m_nextToken)
		{
			const Token* token = cur->m_token;
			if (token->m_worldBounds.Touches(worldBounds))
			{
				functor.Collect(token->m_userData);
			}
		}
	}
};
