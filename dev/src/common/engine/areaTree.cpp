///////////////////////////////////////////////////
// Penthouse builder system by Dex.
///////////////////////////////////////////////////

#include "build.h"
#include "areaTree.h"

#if 0
#define MAX_NODES_PER_ELEMENT	128
#define MAX_ELEMENTS_PER_NODE	8
#define MIN_NODE_SIZE			64

// World bounds
static const CAreaTreeBounds WorldBounds( Vector::ZEROS, 4096.0f );

CAreaTreeNode::CAreaTreeNode()
	: m_frags( NULL )
	, m_children( NULL )
{
}

CAreaTreeNode::~CAreaTreeNode()
{
	// Delete child nodes
	if ( m_children )
	{
		delete [] m_children;
		m_children = NULL;
	}
}

void CAreaTreeNode::RemoveAllElements()
{
	ASSERT( !m_children );

	// Collect all elements from this node	
	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		// Get element
		CAreaTreeToken* token = cur->GetObjectA();
		//ASSERT( cur->GetObjectB() == this );

		// Remove this node from node count
		token->m_numAreaNodes--;
	}

	// Clear list of primitives in this node
	if ( m_frags )
	{
		m_frags->FreeListA();
		ASSERT( m_frags == NULL );
	}
}

Uint32 CAreaTreeNode::GetElementCount() const
{
	Uint32 count=0;

	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		count++;
	}

	return count;
}

void CAreaTreeNode::GetElements( Uint32 queryIndex, TDynArray< CBoundedComponent* >& components ) const
{
	// Collect all elements from this node	
	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		// Get element
		CAreaTreeToken* token = cur->GetObjectA();
		//ASSERT( cur->GetObjectB() == this );

		// Add to list
		CBoundedComponent* component = token->m_component;
		if ( component && token->m_renderAreaQueryIndex != queryIndex )
		{
			token->m_renderAreaQueryIndex = queryIndex;
			components.PushBack( component );
		}
	}

	// Recurse to child nodes
	if ( m_children )
	{
		for ( INT i=0; i<8; i++ )
		{
			m_children[i].GetElements( queryIndex, components );
		}
	}
}

void CAreaTreeNode::GetTokens( TDynArray< CAreaTreeToken* >& tokens ) const
{
	// Collect all elements from this node	
	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		// Get element
		CAreaTreeToken* token = cur->GetObjectA();
		//ASSERT( cur->GetObjectB() == this );

		// Add to list
		if ( !token->m_collectedInGrab )
		{
			token->m_collectedInGrab = true;
			tokens.PushBack( token );
		}
	}

	// Recurse to child nodes
	if ( m_children )
	{
		for ( INT i=0; i<8; i++ )
		{
			m_children[i].GetTokens( tokens );
		}
	}
}

void CAreaTreeNode::GetIntersectingElements( const Box &box, Uint32 queryIndex, TDynArray< CBoundedComponent* >& components, const CAreaTreeBounds &bounds ) const
{
	// Test all elements from this node	
	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		// Get element
		CAreaTreeToken* token = cur->GetObjectA();
		//ASSERT( cur->GetObjectB() == this );

		// Test only once per query
		if ( token->m_renderAreaQueryIndex != queryIndex )
		{
			// Mark as tested
			token->m_renderAreaQueryIndex = queryIndex;

			// Test 
			CBoundedComponent* element = token->m_component;
			if ( element && element->GetBoundingBox().Touches( box ) )
			{
				components.PushBack( element );
			}
		}
	}

	// Recurse
	if ( m_children )
	{
		// Collect touched children
		Uint32 childIndices[8];
		Uint32 numChildren = FindChildren( bounds, box, childIndices );

		// Recurse to touched children
		for ( Uint32 i=0; i<numChildren; i++ )
		{
			Int32 childIndex = childIndices[ i ];
			CAreaTreeBounds childBounds( bounds, childIndex );

			if ( childBounds.IsInside( box ))
			{
				// If we are totally enclosed in test box we don't need intersection for elements contained inside child node
				m_children[ childIndex ].GetElements( queryIndex, components );
			}
			else
			{
				// We are only partially enclosed by test box
				m_children[ childIndex ].GetIntersectingElements( box, queryIndex, components, childBounds );
			}
		}
	}
}

void CAreaTreeNode::GetIntersectingElements( const CFrustum &frustum, Uint32 queryIndex, TDynArray< CBoundedComponent* >& components, const CAreaTreeBounds &bounds ) const
{
	// Test all elements from this node	
	for ( CAreaFrag *cur=m_frags; cur; cur=cur->GetNextFragA() )
	{
		// Get element
		CAreaTreeToken* token = cur->GetObjectA();
		//ASSERT( cur->GetObjectB() == this );

		// Test only once per query
		if ( token->m_renderAreaQueryIndex != queryIndex )
		{
			// Mark as tested
			token->m_renderAreaQueryIndex = queryIndex;

			// Test bounding box
			CBoundedComponent* component = token->m_component;
			if ( frustum.TestBox( component->GetBoundingBox() ) )
			{
				components.PushBack( component );
			}
		}
	}

	// Recurse
	if ( m_children )
	{
		for ( Int32 i=0; i<8; i++ )
		{
			// Get bounds of child node
			CAreaTreeBounds childBounds( bounds, i );
			Box nodeBox( childBounds.m_center, childBounds.m_extent );

			// Test it with frustum
			Int32 testCode = frustum.TestBox( nodeBox );

			// Whole node is inside the frustum
			if ( testCode == 1 )
			{
				// Collect all elements !
				m_children[ i ].GetElements( queryIndex, components );
			}

			// Intersection...
			else if ( testCode == -1 )
			{
				// Collect only visible elements
				m_children[ i ].GetIntersectingElements( frustum, queryIndex, components, childBounds );
			}
		}
	}
}

void CAreaTreeNode::SingleNodeInsert( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds )
{
	// Find child index
	Int32 childIndex = FindChild( bounds, token->m_component->GetBoundingBox() );
	if ( !m_children || childIndex == -1 )
	{
		// Already at leaf or element does not fit inside any child node
		StoreElement( tree, token, bounds );
	}
	else
	{
		// Fit to child node
		CAreaTreeBounds childBounds( bounds, childIndex );
		m_children[ childIndex ].SingleNodeInsert( tree, token, childBounds );
	}
}

Bool CAreaTreeNode::MultiNodeInsert( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds )
{
	// If there are no children, or this primitives bounding box completely 
	// contains this nodes bounding box, store the actor at this node.
	if ( !m_children || bounds.IsInside( token->m_component->GetBoundingBox() ) )
	{
		if ( token->m_numAreaNodes >= MAX_NODES_PER_ELEMENT )
		{
			// We are in too many nodes
			return false;
		}
		else
		{				
			// Store inside this node
			StoreElement( tree, token, bounds );

			// Successful store
			return true;
		}
	}
	else
	{
		// Collect touched children
		Uint32 childIndices[8];
		Uint32 numChildren = FindChildren( bounds, token->m_component->GetBoundingBox(), childIndices );

		// Recurse to touched children
		for ( Uint32 i=0; i<numChildren; i++ )
		{
			Int32 childIndex = childIndices[ i ];
			CAreaTreeBounds childBounds( bounds, childIndex );

			// Try to insert into child node
			if ( !m_children[ childIndex ].MultiNodeInsert( tree, token, childBounds ) )
			{
				// We failed to insert (element in too many nodes)
				return false;
			}
		}

		// Inserted
		return true;
	}
}

void CAreaTreeNode::StoreElement( CAreaTree *tree, CAreaTreeToken* token, const CAreaTreeBounds &bounds )
{
	// Split node if it holds to many elements
	if ( !m_children && ( 0.5f * bounds.m_extent >= MIN_NODE_SIZE ) && GetElementCount() >= MAX_ELEMENTS_PER_NODE )
	{	
		// Extract tokens stored in this node
		TDynArray< CAreaTreeToken* > tokens;
		GetTokens( tokens );

		// We need to re add all elements previously stored in this node + new element
		tokens.PushBack( token );

		// Remove all elements from this node
		RemoveAllElements();

		// Allocate memory for children nodes.
		m_children = new CAreaTreeNode[ 8 ];

		// Insert element into child nodes
		for ( Uint32 i=0; i<tokens.Size(); i++ )
		{
			CAreaTreeToken* token = tokens[i];
			token->m_collectedInGrab = false;

			// Then re-check it against this node, which will then check against children.
			if ( token->m_singleAreaNode )
			{
				// Insert once again as single node element
				SingleNodeInsert( tree, token, bounds );
			}
			else
			{
				// Try to insert into multiple nodes
				if ( !MultiNodeInsert( tree, token, bounds ))
				{
					// Add to list of elements that failed adding
					tree->m_failed.PushBack( token );
				}
			}
		}
	}
	else
	{
		// Add this element here using element fragment
		CAreaFrag *frag = tree->m_fragPool.AllocateFrag();
		frag->LinkA( token, m_frags );
		//frag->LinkB( this, token->m_areaFrags );

		// Bump node counter
		token->m_numAreaNodes++;
	}
}

CAreaTree::CAreaTree()
	: m_fragPool( 8192 )
	, m_root( NULL )
{
	// Create root node
	m_root = new CAreaTreeNode;
}

CAreaTree::~CAreaTree()
{
	// Delete tree
	if ( m_root )
	{
		delete m_root;
		m_root = NULL;
	}
}

void CAreaTree::UpdateElement( CBoundedComponent* remove, CBoundedComponent* add )
{
	ASSERT( remove || add );

	// Lock the array access
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Remove existing node
	if ( remove )
	{
		// Find the token
		CAreaTreeToken* token = remove->m_areaToken;
		if ( token )
		{
			// Remove element from all nodes it's in
			ASSERT( token->m_component == remove );
			//token->m_areaFrags->FreeListB();
			ASSERT( token->m_areaFrags == NULL );

			// Cleanup the data
			token->m_numAreaNodes = 0;
			token->m_singleAreaNode = false;

			// Unlink if not reused
			if ( add != remove )
			{
				// Cleanup the link
				remove->m_areaToken = NULL;

				// Delete the token
				delete token;
			}
		}
	}

	// Add new shit
	if ( add )
	{
		// Safety
		ASSERT( m_failed.Size() == 0 );

		// Get existing token
		CAreaTreeToken* token = add->m_areaToken;
		if ( !token )
		{
			// Create new token
			token = new CAreaTreeToken( add );
			add->m_areaToken = token;
		}

		// Try to insert into multiple nodes
		if ( !m_root->MultiNodeInsert( this, token, WorldBounds ) )
		{
			// Fallback insert into single node
			m_root->SingleNodeInsert( this, token, WorldBounds );
			token->m_singleAreaNode = true;
		}

		// Sometimes because of splitting we have tokens that are placed in to many single nodes, merge them
		if ( m_failed.Size() )
		{
			// Reinsert failed nodes
			for ( Uint32 i=0; i<m_failed.Size(); i++ )
			{
				m_root->SingleNodeInsert( this, m_failed[i], WorldBounds );
			}

			// Clear list
			m_failed.ClearFast();
		}
	}
}

void CAreaTree::CollectElements( const Box &box, TDynArray< CBoundedComponent* > &elements ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Count queries
	m_queryIndex++;

	// Collect elements
	m_root->GetIntersectingElements( box, m_queryIndex, elements, WorldBounds );
}

void CAreaTree::CollectElements( const CFrustum &frustum, TDynArray< CBoundedComponent* > &elements ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_lock );

	// Count queries
	m_queryIndex++;

	// Collect elements
	m_root->GetIntersectingElements( frustum, m_queryIndex, elements, WorldBounds );
}
#endif