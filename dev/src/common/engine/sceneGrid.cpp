/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sceneGrid.h"
#include "sceneBeamTree.h"
#include "renderFrame.h"


Int32					CHierarchicalGrid::st_hierarchialTestIndex = 1;
Red::Threads::CMutex	CHierarchicalGrid::st_collectVisbilityLock;
Red::Threads::CMutex	CHierarchicalGrid::st_insertUpdateRemoveLock;

CHierarchicalGridNode::CHierarchicalGridNode( Int32 xSize, Int32 ySize )
{
	m_parent = NULL;
	m_children = NULL;
	m_size[0] = m_size[1] = 0;

	// Create subnodes if we have to
	if ( xSize != 0 && ySize != 0 )
	{
		Split( xSize, ySize );
	}
}

void CHierarchicalGridNode::GenerateFragments( CRenderFrame* frame, Color color, Int32 count, Float xMin, Float xMax, Float yMin, Float yMax ) const
{
	// Draw bounding box of this node
	Box box( Vector( xMin, yMin, count*1.0f ), Vector( xMax, yMax, (count+m_components.Size())*1.0f ) );
	frame->AddDebugBox( box, Matrix::IDENTITY, color );

	if ( m_children != NULL )
	{
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];

		// Generate boxes for all subnodes
		for ( Int32 y = 0; y < m_size[1]; ++y )
		{
			for ( Int32 x = 0; x < m_size[0]; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->GenerateFragments( frame, color, count + m_components.Size(), xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff );
			}
		}
	}
}

void CHierarchicalGridNode::Insert( IHierarchicalGridElement* component, Int32 level, Float xMin, Float xMax, Float yMin, Float yMax, CHierarchicalGrid* gridContext )
{
	if( m_children == NULL )
	{
		// Split this node if necessary
		if ( (Int32)( 1 + m_components.Size() ) > gridContext->m_maxComponents && level < gridContext->m_maxLevels )
		{
			// Create new subnodes, all subnodes of root are fixed to 2x2 grid, maybe we should change it in future?
			Split( 2, 2 );

			// After creating subnodes, repopulate subnodes with this node's component list
			for ( Int32 i = (Int32)m_components.Size() - 1; i >= 0; --i )
			{
				Bool inserted = InsertToSubNodes( m_components[i], level, xMin, xMax, yMin, yMax, gridContext );
				if ( inserted )
				{
					// If component is inserted in subnodes, remove it from this node's component list
					Bool removed1 = gridContext->m_componentToNodesMapping[m_components[i]].Remove( this );
					Bool removed2 = m_components.Remove( m_components[i] );
					ASSERT( removed1 == true );
					ASSERT( removed2 == true );
				}
			}
		}
		else
		{
			// This node don't have enough components, so insert it into this node's component list
			m_components.PushBack( component );
			gridContext->m_componentToNodesMapping[component].PushBack( this );
			return;
		}
	}

	// This node have subnodes so insert component into subnodes
	Bool inserted = InsertToSubNodes( component, level, xMin, xMax, yMin, yMax, gridContext );
	if ( !inserted )
	{
		// It was not inserted into subnodes, so insert it right here
		m_components.PushBack( component );
		gridContext->m_componentToNodesMapping[component].PushBack( this );
	}
}

Bool CHierarchicalGridNode::InsertToSubNodes( IHierarchicalGridElement* component, Int32 level, Float xMin, Float xMax, Float yMin, Float yMax, CHierarchicalGrid* gridContext )
{
	Box box = component->GetBoundingBox();

	Float xDiff = (xMax-xMin) / m_size[0];
	Float yDiff = (yMax-yMin) / m_size[1];
	Int32 xStart = Clamp( (Int32)((box.Min.X - xMin) / xDiff), 0, m_size[0] - 1 );
	Int32 xEnd   = Clamp( (Int32)((box.Max.X - xMin) / xDiff), 0, m_size[0] - 1 );
	Int32 yStart = Clamp( (Int32)((box.Min.Y - yMin) / yDiff), 0, m_size[1] - 1 );
	Int32 yEnd   = Clamp( (Int32)((box.Max.Y - yMin) / yDiff), 0, m_size[1] - 1 );

	// If components exist not in all subnodes we add to this component to proper ones
	if ( (xEnd - xStart + 1)*(yEnd - yStart + 1) < (m_size[0] * m_size[1]) )
	{
		for ( Int32 y = yStart; y <= yEnd; ++y )
		{
			for ( Int32 x = xStart; x <= xEnd; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->Insert( component, level+1, xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff, gridContext );
			}
		}
		// Signal caller function that we inserted components in some subnodes
		return true;
	}

	// We don't insert component if it would be in all subnodes, in that case we insert it to parent node
	return false;
}

void CHierarchicalGridNode::Remove( IHierarchicalGridElement* component, THashMap< IHierarchicalGridElement*, TDynArray< CHierarchicalGridNode* > >& componentToNodeMapping )
{
	// Remove component from node's component list and clear reverse mapping from components to nodes
	Bool removed1 = componentToNodeMapping[component].Remove( this );
	Bool removed2 = m_components.Remove( component );
	ASSERT( removed1 == true );
	ASSERT( removed2 == true );
}

void CHierarchicalGridNode::Merge( CHierarchicalGrid* gridContext )
{
	// Merging make sense only on node which has subnodes
	if ( m_size[0] != 0 && m_size[1] != 0 && m_parent != NULL )
	{
		ASSERT( m_children != NULL );

		// Count components in all subnodes
		Int32 components = 0;
		for ( Int32 y = 0; y < m_size[1]; ++y )
		{
			for ( Int32 x = 0; x < m_size[0]; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);

				// If some subnode has another subnode, don't merge, just exit
				if ( subnode->m_children != NULL )
				{
					return;
				}

				// Add this subnode components count
				components += subnode->m_components.Size();
			}
		}

		// Add this node components count
		components += m_components.Size();

		// If count is small, merge subnodes to this node
		if ( components < gridContext->m_minComponents )
		{
			for ( Int32 y = 0; y < m_size[1]; ++y )
			{
				for ( Int32 x = 0; x < m_size[0]; ++x )
				{
					CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
					for( Uint32 i = 0; i < subnode->m_components.Size(); ++i )
					{
						IHierarchicalGridElement* subnodeComponent = subnode->m_components[i];

						// Remove reverse mapping from components to nodes
						Bool removed = gridContext->m_componentToNodesMapping[subnodeComponent].Remove( subnode );
						ASSERT( removed == true );

						// Move component from subnodes to this node
						if ( m_components.PushBackUnique( subnodeComponent ) )
						{
							// Add reverse mapping from components to nodes
							gridContext->m_componentToNodesMapping[subnodeComponent].PushBack( this );
						}
					}
				}
			}

			// Delete all subnodes
			m_size[0] = m_size[1] = 0;
			delete[] m_children;
			m_children = NULL;
		}
	}
}

void CHierarchicalGridNode::Split( const Int32 xSize, const Int32 ySize )
{
	ASSERT( m_children == NULL );
	ASSERT( m_size[0] == 0 && m_size[1] == 0 );

	// Alloc new subnodes and set m_parent
	m_children = new CHierarchicalGridNode[xSize*ySize];
	for ( Int32 i = 0; i < xSize*ySize; ++i )
	{
		m_children[i].m_parent = this;
	}

	// Set number of subnodes in each direction
	m_size[0] = xSize;
	m_size[1] = ySize;
}

void CHierarchicalGridNode::Find( const Box& box, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& outComponents ) const
{
	// If this node has subnodes recurse into them
	if( m_children )
	{
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];
		Int32 xStart = Clamp( (Int32)((box.Min.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 xEnd   = Clamp( (Int32)((box.Max.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 yStart = Clamp( (Int32)((box.Min.Y - yMin) / yDiff), 0, m_size[1] - 1 );
		Int32 yEnd   = Clamp( (Int32)((box.Max.Y - yMin) / yDiff), 0, m_size[1] - 1 );

		for ( Int32 y = yStart; y <= yEnd; ++y )
		{
			for ( Int32 x = xStart; x <= xEnd; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->Find( box, xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff, outComponents );
			}
		}
	}

	// Add this node's components
	for( Uint32 i = 0; i < m_components.Size(); ++i )
	{
		if ( box.Touches( m_components[i]->GetBoundingBox() ) )
		{
			outComponents.PushBackUnique( m_components[i] );
		}
	}
}

void CHierarchicalGridNode::Find( const Box& box, Float xMin, Float xMax, Float yMin, Float yMax, THashSet< IHierarchicalGridElement* >& outComponents ) const
{
	// If this node has subnodes recurse into them
	if( m_children )
	{
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];
		Int32 xStart = Clamp( (Int32)((box.Min.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 xEnd   = Clamp( (Int32)((box.Max.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 yStart = Clamp( (Int32)((box.Min.Y - yMin) / yDiff), 0, m_size[1] - 1 );
		Int32 yEnd   = Clamp( (Int32)((box.Max.Y - yMin) / yDiff), 0, m_size[1] - 1 );

		for ( Int32 y = yStart; y <= yEnd; ++y )
		{
			for ( Int32 x = xStart; x <= xEnd; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->Find( box, xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff, outComponents );
			}
		}
	}

	// Add this node's components
	for( Uint32 i = 0; i < m_components.Size(); ++i )
	{
		if ( box.Touches( m_components[i]->GetBoundingBox() ) )
		{
			outComponents.Insert( m_components[i] );
		}
	}
}

void CHierarchicalGridNode::Find( const Vector &point, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& outComponents ) const
{
	// If this node has subnodes recurse into them
	if( m_children )
	{
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];
		Int32 xIndex = Clamp( (Int32)((point.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 yIndex = Clamp( (Int32)((point.X - xMin) / xDiff), 0, m_size[0] - 1 );

		CHierarchicalGridNode* subnode = &(m_children[xIndex + yIndex*m_size[0]]);
		subnode->Find( point, xMin+(xIndex+0)*xDiff, xMin+(xIndex+1)*xDiff, yMin+(yIndex+0)*yDiff, yMin+(yIndex+1)*yDiff, outComponents );
	}

	// Add this node's components
	for( Uint32 i = 0; i < m_components.Size(); ++i )
	{
		if ( m_components[i]->GetBoundingBox().Contains(point) )
		{
			outComponents.PushBackUnique( m_components[i] );
		}
	}
}

void CHierarchicalGridNode::FindClosest( const Vector& point, Float xMin, Float xMax, Float yMin, Float yMax, IHierarchicalGridElement*& outComponents, Float& minDistance ) const
{
	// Check if node's components are closer to the point then component found earlier
	for( Uint32 i = 0; i < m_components.Size(); ++i )
	{
		// If this component is closer then updated "closest" data
		Float distance = point.DistanceTo( m_components[i]->GetBoundingBox().CalcCenter() );
		if ( distance < minDistance )
		{
			minDistance = distance;
			outComponents = m_components[i];
		}
	}

	// Check subnodes node
	if ( m_children )
	{
		// Find which subnode contains the point
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];
		Int32 xPoint = Clamp( (Int32)((point.X - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 yPoint = Clamp( (Int32)((point.Y - yMin) / xDiff), 0, m_size[0] - 1 );

		// First check subnode that contains the point
		CHierarchicalGridNode* subnode = &(m_children[xPoint + yPoint*m_size[0]]);
		subnode->FindClosest( point, xMin+(xPoint+0)*xDiff, xMin+(xPoint+1)*xDiff, yMin+(yPoint+0)*yDiff, yMin+(yPoint+1)*yDiff, outComponents, minDistance );

		// Then check other subnodes with updated bounds
		Int32 xStart = Clamp( (Int32)(((point.X-minDistance) - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 xEnd   = Clamp( (Int32)(((point.X+minDistance) - xMin) / xDiff), 0, m_size[0] - 1 );
		Int32 yStart = Clamp( (Int32)(((point.Y-minDistance) - yMin) / yDiff), 0, m_size[1] - 1 );
		Int32 yEnd   = Clamp( (Int32)(((point.Y+minDistance) - yMin) / yDiff), 0, m_size[1] - 1 );
		for ( Int32 y = yStart; y <= yEnd; ++y )
		{
			for ( Int32 x = xStart; x <= xEnd; ++x )
			{
				// Skip subnode we have already checked (subnode which contain the point)
				if ( x != xPoint || y != yPoint )
				{
					CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
					subnode->FindClosest( point, xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff, outComponents, minDistance );
				}
			}
		}
	}
}

void CHierarchicalGridNode::CollectComponents( const CBeamTree* beamTree, Bool recurse, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const
{
	// Collect components
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		IHierarchicalGridElement* comp = m_components[i];
		if ( comp->m_hierarchialTestIndex != CHierarchicalGrid::st_hierarchialTestIndex )
		{
			// Mask this component as tested
			comp->m_hierarchialTestIndex = CHierarchicalGrid::st_hierarchialTestIndex;

			// Is component visible ?
			const Box& componentBounds = comp->GetBoundingBox();
			if ( beamTree->CheckVisible( 0, componentBounds ) != Plane::PS_Back )
			{
				visibleComponents.PushBack( comp );
			}
		}
	}

	// Recurse to sub nodes
	if ( recurse )
	{
		for ( Int32 y = 0; y < m_size[1]; ++y )
		{
			for ( Int32 x = 0; x < m_size[0]; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->CollectComponents( beamTree, recurse, visibleComponents );
			}
		}	
	}
}

void CHierarchicalGridNode::CollectVisibility( const CBeamTree* beamTree, const Uint32 beamTreeNode, Float xMin, Float xMax, Float yMin, Float yMax, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const
{
	// Check node visibility
	const Box nodeBoundingBox( Vector( xMin, yMin, -128.0f ), Vector( xMax, yMax, +1024.0f ) );
	Plane::ESide side = beamTree->CheckVisible( 0, nodeBoundingBox );

	// This node is invisible :)
	if ( side == Plane::PS_Back )
	{
		return;
	}

	// This node is in the beam tree frustum
	if ( side == Plane::PS_Front )
	{
		CollectComponents( beamTree, true, visibleComponents );
		return;
	}

	// Collect local components only
	CollectComponents( beamTree, false, visibleComponents );

	// Recurse to sub nodes
	{
		Float xDiff = (xMax-xMin) / m_size[0];
		Float yDiff = (yMax-yMin) / m_size[1];
		for ( Int32 y = 0; y < m_size[1]; ++y )
		{
			for ( Int32 x = 0; x < m_size[0]; ++x )
			{
				CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
				subnode->CollectVisibility( beamTree, beamTreeNode, xMin+(x+0)*xDiff, xMin+(x+1)*xDiff, yMin+(y+0)*yDiff, yMin+(y+1)*yDiff, visibleComponents );
			}
		}
	}
}

void CHierarchicalGridNode::AddComponentsFromThisNode( THashSet< IHierarchicalGridElement* >& visibleComponents ) const
{
	// Add all components from this node
	for ( Uint32 j = 0; j < m_components.Size(); ++j )
	{
		IHierarchicalGridElement* component = m_components[j];
		const Box& componentBounds = component->GetBoundingBox();
		visibleComponents.Insert( component );
	}
}
void CHierarchicalGridNode::AddComponentsFromThisNodeRecursive( THashSet< IHierarchicalGridElement* >& visibleComponents ) const
{
	AddComponentsFromThisNode( visibleComponents );

	// Recurse into subnodes
	for ( Int32 y = 0; y < m_size[1]; ++y )
	{
		for ( Int32 x = 0; x < m_size[0]; ++x )
		{
			CHierarchicalGridNode* subnode = &(m_children[x + y*m_size[0]]);
			subnode->AddComponentsFromThisNodeRecursive( visibleComponents );
		}
	}
}

CHierarchicalGrid::CHierarchicalGrid( const Float gridSize, const Int32 xSize, const Int32 ySize, const Int32 maxLevels, const Int32 minComponents, const Int32 maxComponents )
{
	// Copy grid's params
	m_maxLevels = maxLevels;
	m_minComponents = minComponents;
	m_maxComponents = maxComponents;

	// Setup grid's bounding box
	m_boundingBox.Clear();
	m_boundingBox.AddPoint( Vector( -gridSize, -gridSize, -gridSize ) );
	m_boundingBox.AddPoint( Vector( +gridSize, +gridSize, +gridSize ) );

	// Create root node and first level of children
	m_rootNode = new CHierarchicalGridNode( xSize, ySize );
}

CHierarchicalGrid::~CHierarchicalGrid()
{
	ASSERT( m_rootNode );
	ASSERT( m_rootNode->m_children != NULL );

	delete[] m_rootNode->m_children;
	delete m_rootNode;
	m_rootNode = NULL;
	m_boundingBox.Clear();
}

void CHierarchicalGrid::GenerateFragments( CRenderFrame* frame, Color color ) const
{
	ASSERT( m_rootNode != NULL );
	m_rootNode->GenerateFragments( frame, color, 0, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1] );
}

void CHierarchicalGrid::Insert( IHierarchicalGridElement* component )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_insertUpdateRemoveLock );

	PC_SCOPE( GridInsert );

	ASSERT( m_rootNode != NULL );
	m_rootNode->Insert( component, 0, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], this );
}


void CHierarchicalGrid::Remove( IHierarchicalGridElement* component )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_insertUpdateRemoveLock );

	PC_SCOPE( GridRemove );

	THashMap< IHierarchicalGridElement*, TDynArray< CHierarchicalGridNode* > >::iterator it;
	it = m_componentToNodesMapping.Find( component );

	// It may be happened just before attach
	if( it != m_componentToNodesMapping.End() )
	{
		TDynArray< CHierarchicalGridNode* >& nodes = (*it).m_second;

		// Get parents of nodes and later call Merge on them, unique because we don't want to merge twice the same parent
		TDynArray< CHierarchicalGridNode* > parents;
		for ( Int32 i = (Int32) nodes.Size() - 1; i >= 0; --i )
		{
			if ( nodes[i]->m_parent != NULL )
			{
				parents.PushBackUnique( nodes[i]->m_parent );
			}
		}

		// Remove component from all the nodes
		for ( Int32 i = (Int32) nodes.Size() - 1; i >= 0; --i )
		{
			nodes[i]->Remove( component, m_componentToNodesMapping );
		}

		// Merge all parents
		for ( Int32 i = (Int32) parents.Size() - 1; i >= 0; --i )
		{
			parents[i]->Merge( this );
		}

		ASSERT( m_componentToNodesMapping[component].Size() == 0 );
		m_componentToNodesMapping.Erase( it );
	}
}

void CHierarchicalGrid::Update( IHierarchicalGridElement* component )
{
	PC_SCOPE( GridUpdate );

	// TODO: Add fast update if component is in the same node as before
	Remove( component );
	Insert( component );
}

void CHierarchicalGrid::Find( const Box& box, TDynArray< IHierarchicalGridElement*>& outComponents ) const
{
	PC_SCOPE( GridFind );
	ASSERT( m_rootNode != NULL );
	m_rootNode->Find( box, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], outComponents );
}

void CHierarchicalGrid::Find( const Box& box, THashSet< IHierarchicalGridElement* >& outComponents ) const
{
	PC_SCOPE( GridFind );
	ASSERT( m_rootNode != NULL );
	m_rootNode->Find( box, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], outComponents );
}

void CHierarchicalGrid::Find( const Vector &point, TDynArray< IHierarchicalGridElement* > &outComponents ) const
{
	PC_SCOPE( GridFind );
	ASSERT( m_rootNode != NULL );
	m_rootNode->Find( point, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], outComponents );	
}

IHierarchicalGridElement* CHierarchicalGrid::FindClosest( const Vector& point ) const
{
	PC_SCOPE( GridFindClosest );
	ASSERT( m_rootNode != NULL );
	Float maxDistance = m_boundingBox.CalcSize().Mag3();
	IHierarchicalGridElement* outComponents = NULL;
	m_rootNode->FindClosest( point, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], outComponents, maxDistance );
	return outComponents;
}

void CHierarchicalGrid::CollectVisibility( const CBeamTree* beamTree, const Uint32 beamTreeNode, TDynArray< IHierarchicalGridElement* >& visibleComponents ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( st_collectVisbilityLock );

	// Bounce the index
	st_hierarchialTestIndex++;

	// Collect components
	m_rootNode->CollectVisibility( beamTree, beamTreeNode, m_boundingBox.Min.A[0], m_boundingBox.Max.A[0], m_boundingBox.Min.A[1], m_boundingBox.Max.A[1], visibleComponents );
}

