#include "build.h"
#include "pathlibNodeSet.h"

#include "pathlibAreaDescription.h"
#include "pathlibNavgraph.h"
#include "pathlibObstacleMapper.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibNodeSetRequestsList.h"
#include "pathlibWorld.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////
// CNodeSet
////////////////////////////////////////////////////////////////////////

CNavgraphNodeSet::~CNavgraphNodeSet()
{
	if ( m_owner )
	{
		m_owner->NodeSetDeleted( m_navgraph->GetCategory() );
	}
}

void CNavgraphNodeSet::Unlink()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		for (LinksErasableIterator itLink( node ); itLink; )
		{
			if ( itLink->GetDestination()->GetNodesetIndex() != m_id )
			{
				node.EraseLink( itLink );
			}
			else
			{
				++itLink;
			}
		}
	}
}
Bool CNavgraphNodeSet::VAreNodesLinkedById() const
{
	return m_navgraph->IsNodesLinkedById();
}
AreaId CNavgraphNodeSet::VGetAreaId() const
{
	return m_navgraph->GetArea()->GetId();
}
CPathLibWorld* CNavgraphNodeSet::VGetPathLibWorld() const
{
	return m_navgraph->VGetPathLibWorld();
}
CNavNode::NodesetIdx CNavgraphNodeSet::VGetNodesetIndex() const
{
	return m_id;
}
LinkBufferIndex CNavgraphNodeSet::VGetExtraLinksSpace() const
{
	return 4;
}

void CNavgraphNodeSet::PreNodeArrayOverflow()
{
	ConvertLinksToIds();
}

void CNavgraphNodeSet::OnNodeArrayOverflow()
{
	ConvertLinksToPointers();
	m_navgraph->GetNodeFinder().Invalidate();
}
void CNavgraphNodeSet::Clear()
{
	Unlink();
	m_nodes.ClearFast();
}

CNavNode& CNavgraphNodeSet::AddNode( const Vector3& position,NodeFlags flags )
{
	m_bbox.AddPoint( position );

	return Super::AddNode( position, m_navgraph->GetArea()->GetId(), m_id, flags );
}

void CNavgraphNodeSet::PreAttach( CNavGraph* navgraph, CNodeSetProcessingContext& context )
{
	ASSERT( !m_isAttached );
	const CSearchNode::Marking& marking = context.GetMarking();

	for( CNavNode& node : m_nodes )
	{
		node.ClearFlags( NF_DETACHED );
	}
	m_links.ClearFlagsToEveryLink( NF_DETACHED );

	// revise internal flags
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		navgraph->RuntimeCalculateNodeAvailability( node, context );
		marking.Mark( node );
	}
}
void CNavgraphNodeSet::PreDetach( CNavGraph* navgraph, CNodeSetProcessingContext& context )
{
	ASSERT( m_isAttached );
	const CSearchNode::Marking& marking = context.GetMarking();

	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		node.AddFlagsToLinks( NF_BLOCKED );
		marking.Mark( node );
	}
}

void CNavgraphNodeSet::Attach( CNavGraph* navgraph, CNodeSetProcessingContext& context )
{
	ASSERT( !m_isAttached );

	CAreaDescription* area = navgraph->GetArea();

	CCentralNodeFinder& nf = navgraph->GetNodeFinder();
	// revise internal flags
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		nf.AddDynamicElement( &node );
	}

	m_isAttached = true;
}

void CNavgraphNodeSet::Detach( CNavGraph* navgraph, CNodeSetProcessingContext& context )
{
	ASSERT( m_isAttached );

	CCentralNodeFinder& nf = navgraph->GetNodeFinder();
	// block ur links
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		nf.RemoveDynamicElement( &node );
	}

	for( CNavNode& node : m_nodes )
	{
		node.AddFlags( NF_DETACHED );
	}
	m_links.AddFlagsToEveryLink( NF_DETACHED );

	m_isAttached = false;
}
CNavNode* CNavgraphNodeSet::GetNode( CPathNode::Id id )
{
	if ( id.m_nodeSetIndex == m_id )
	{
		return &m_nodes[ id.m_index ];
	}
	return m_navgraph->GetNode( id );
}
CPathNode* CNavgraphNodeSet::VGetPathNode( CPathNode::Id id )
{
	return GetNode( id );
}

void CNavgraphNodeSet::ConnectNodes( CNavNode& nodeSetNode, CNavNode& destinationNode, NodeFlags linkFlags, NavLinkCost linkCost )
{
	return m_navgraph->ConnectNode( nodeSetNode, destinationNode, linkFlags | NF_IS_IN_NODESET, linkCost );
}

void CNavgraphNodeSet::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	writer.Put( m_bbox );
}
Bool CNavgraphNodeSet::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader, m_id ) )
	{
		return false;
	}

	if ( !reader.Get( m_bbox ) )
	{
		return false;
	}

	for( CNavNode& node : m_nodes )
	{
		node.AddFlags( NF_DETACHED );
	}
	m_links.AddFlagsToEveryLink( NF_DETACHED );

	return true;
}

Bool CNavgraphNodeSet::TryConnecting( CNavgraphNodeSet* nodeSet )
{
	struct Local
	{
		static CNavNode* FindClosestNode( CNavgraphNodeSet* nodeSet, const Vector3& pos )
		{
			Float closestDistSq = FLT_MAX;
			CNavNode* closestNode = NULL;
			for ( auto it = nodeSet->m_nodes.Begin(), end = nodeSet->m_nodes.End(); it != end; ++it )
			{
				CNavNode* node = &(*it);
				Float distSq = (node->GetPosition() - pos).SquareMag();
				if ( distSq < closestDistSq )
				{
					closestDistSq = distSq;
					closestNode = node;
				}
			}
			return closestNode;
		}
	};

	if ( !m_owner || !nodeSet->m_owner )
	{
		return false;
	}

	CAreaDescription* area = m_navgraph->GetArea();
	Float personalSpace = m_navgraph->GetPersonalSpace();
	
	m_owner->GenerationOnPreNodeSetConnection( m_navgraph );
	nodeSet->m_owner->GenerationOnPreNodeSetConnection( m_navgraph );

	////////////////////////////////////////////////////////////////////
	// TODO: Better implementation currently its very simple, but it doesn't cover multiple cases
	////////////////////////////////////////////////////////////////////
	CNavNode* closestNode1 = Local::FindClosestNode( this, nodeSet->GetBBox().GetMassCenter() );
	CNavNode* closestNode2 = Local::FindClosestNode( nodeSet, GetBBox().GetMassCenter() );

	Bool connected = false;

	if ( closestNode1 && closestNode2 && !closestNode1->IsConnected( *closestNode2 ) )
	{
		CWideLineQueryData query( CT_IGNORE_OTHER_AREAS, closestNode1->GetPosition(), closestNode2->GetPosition(), personalSpace );
		if( area->VSpatialQuery( query ) )
		{
			ConnectNodes( *closestNode1, *closestNode2, NF_BLOCKED, CalculateLinkCost( (closestNode1->GetPosition() - closestNode2->GetPosition()).Mag() ) );
			connected = true;
		}
	}


	m_owner->GenerationOnPostNodeSetConnection( m_navgraph );
	nodeSet->m_owner->GenerationOnPostNodeSetConnection( m_navgraph );

	// find nodeset center of mass
	return connected;
}

Bool CNavgraphNodeSet::ConnectNodeSet()
{
	/*struct CollectNodes : public CNodeFinder::Handler
	{
		CAreaDescription*								m_area;
		CNavGraph*										m_navgraph;
		Float											m_personalSpace;
		CObstacleShape*									m_shape;
		TDynArray< TPair< CNavNode::Id, Uint32 > >		m_detourConnectors;
		const ObstacleDetour&							m_detour;

		CollectNodes( CNavGraph* navgraph, CObstacleShape* shape, const ObstacleDetour&	detour )
			: m_area( navgraph->GetArea() )
			, m_navgraph( navgraph )
			, m_personalSpace( navgraph->GetPersonalSpace() )
			, m_shape( shape )
			, m_detour( detour )
			, m_functor( functor ) {}

		void Handle( CNavNode& node ) override
		{
			// check if node collides with our shape - if so mark it for deletion
			const Vector3& nodePosition = node.GetPosition();
			Vector3 localNodePosition = nodePosition;
			m_area->VWorldToLocal( localNodePosition );
			{
				CCircleQueryData query( CT_DEFAULT, localNodePosition, m_personalSpace );
				Vector3 testBoundings[2];
				query.ComputeBBox( testBoundings );
				if ( m_shape->TestBoundings( testBoundings[ 0 ], testBoundings[ 1 ] ) && !m_shape->VSpatialQuery( query, testBoundings ) )
				{
					m_functor.NodeIsBlocked( &node );
					return;
				}
			}

			// delete links that collide with our shape
			auto& linksArray = node.GetLinksArray();
			for ( Int32 i = linksArray.Size()-1; i >= 0; --i )
			{
				CPathLink& link = linksArray[ i ];
				CPathNode* destNode = link.HaveFlag( NF_DESTINATION_IS_ID ) ? m_navgraph->GetNode( link.GetDestinationId() ) : link.GetDestination();
				Vector3 localDestNodePosition = destNode->GetPosition();
				m_area->VWorldToLocal( localDestNodePosition );
				CWideLineQueryData query( CT_DEFAULT, localNodePosition, localDestNodePosition, m_personalSpace );
				Vector3 testBoundings[2];
				query.ComputeBBox( testBoundings );
				if ( m_shape->TestBoundings( testBoundings[ 0 ], testBoundings[ 1 ] ) && !m_shape->VSpatialQuery( query, testBoundings ) )
				{
					m_functor.ConnectionIsBlocked( &node, linksArray.Begin() + i );
				}
			}

			// check if node can connect to any detour point
			Uint32 bestDetourPoint = 0xffff;
			Float closestDetourPointDistSq = FLT_MAX;
			for ( Uint32 i = 0, n = m_detour.Size(); i != n; ++i )
			{
				Float distSq = (nodePosition - m_detour[ i ]).SquareMag();
				if ( distSq < closestDetourPointDistSq )
				{
					CWideLineQueryData query( CT_IGNORE_OTHER_AREAS, nodePosition, m_detour[ i ], m_personalSpace );
					if ( m_area->VSpatialQuery( query ) )
					{
						closestDetourPointDistSq = distSq;
						bestDetourPoint = i;
					}
				}
			}

			if ( bestDetourPoint != 0xffff )
			{
				m_detourConnectors.PushBack( TPair< CNavNode::Id, Uint32 >( node.GetFullId(), bestDetourPoint ) );
			}
		}
	};*/

	return true;
}


////////////////////////////////////////////////////////////////////////
// CNodeSetPack
////////////////////////////////////////////////////////////////////////
INodeSetPack::INodeSetPack()
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		m_nodeSetsIndexes[ i ] = CPathNode::INVALID_INDEX;
	}
}
INodeSetPack::~INodeSetPack()
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_nodeSets[ i ] )
		{
			m_nodeSets[ i ]->SetOwner( nullptr );
		}
	}
}


CNavgraphNodeSet* INodeSetPack::RegisterNodeSet( CNavGraph* navgraph )
{
	Uint32 category = navgraph->GetCategory();
	CNavgraphNodeSet* nodeSet = m_nodeSets[ category ];
	if ( !nodeSet )
	{
		nodeSet = navgraph->CreateNodeSet();
		nodeSet->SetOwner( this );
		m_nodeSetsIndexes[ category ] = nodeSet->GetId();
		m_nodeSets[ category ] = nodeSet;
	}
	else
	{
		nodeSet->Clear();
	}
	
	return nodeSet;
}

CNavgraphNodeSet* INodeSetPack::GetNodeSet( CNavGraph* navgraph ) const
{
	Uint32 category = navgraph->GetCategory();
	return m_nodeSets[ category ];
}


void INodeSetPack::ClearNodeSet( CNavGraph* navgraph )
{
	Uint32 category = navgraph->GetCategory();
	if ( m_nodeSetsIndexes[ category ] != CNavgraphNodeSet::INVALID_ID )
	{
		if ( m_nodeSets[ category ] != NULL )
		{
			m_nodeSets[ category ]->SetOwner( NULL );
			m_nodeSets[ category ] = NULL;
		}
		navgraph->ClearNodeSet( m_nodeSetsIndexes[ category ] );
		m_nodeSetsIndexes[ category ] = CNavgraphNodeSet::INVALID_ID;
	}
}	

void INodeSetPack::Attach( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	CAreaNavgraphsRes* areaNavgraphs = area->GetNavgraphs();
	if ( !areaNavgraphs )
	{
		return;
	}
	CObstaclesMapper* obstaclesMapper = area->GetPathLib().GetObstaclesMapper();
	for ( Uint16 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		CNavGraph* navgraph = areaNavgraphs->GetGraph( i );
		if ( navgraph )
		{
			if ( m_nodeSets[ i ] )
			{
				context.AddNodeSetAttachRequest( area->GetId(), i, m_nodeSets[ i ]->GetId() );
				ASSERT( navgraph->GetNodeSet( m_nodeSets[ i ]->GetId() ) != nullptr );
				//m_nodeSets[ i ]->Attach( navgraph );
			}
		}
	}
}

void INodeSetPack::Detach( CAreaDescription* area, CComponentRuntimeProcessingContext& context )
{
	CAreaNavgraphsRes* areaNavgraphs = area->GetNavgraphs();
	if ( !areaNavgraphs )
	{
		return;
	}
	for ( Uint16 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		CNavGraph* navgraph = areaNavgraphs->GetGraph( i );
		if ( navgraph )
		{
			if ( m_nodeSets[ i ] )
			{
				context.AddNodeSetDetachRequest( area->GetId(), i, m_nodeSets[ i ]->GetId() );
			}
		}
	}
}

void INodeSetPack::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		writer.Put( m_nodeSetsIndexes[ i ] );
	}
}
Bool INodeSetPack::ReadFromBuffer( CSimpleBufferReader& reader )
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( !reader.Get( m_nodeSetsIndexes[ i ] ) )
		{
			return false;
		}
	}
	return true;
}

void INodeSetPack::OnPostLoad( CAreaDescription* area )
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_nodeSetsIndexes[ i ] == CNavgraphNodeSet::INVALID_ID )
		{
			continue;
		}
		CNavGraph* navgraph = area->GetNavigationGraph( i );
		if ( navgraph )
		{
			m_nodeSets[ i ] = navgraph->GetNodeSet( m_nodeSetsIndexes[ i ] );
			if ( m_nodeSets[ i ] )
			{
				m_nodeSets[ i ]->SetOwner( this );
			}
		}
	}
}

void INodeSetPack::Clear( CAreaDescription* area )
{
	for ( Uint32 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		if ( m_nodeSetsIndexes[ i ] == CNavgraphNodeSet::INVALID_ID )
		{
			continue;
		}
		CNavGraph* navgraph = area->GetNavigationGraph( i );
		if ( navgraph )
		{
			navgraph->ClearNodeSet( m_nodeSetsIndexes[ i ] );
		}
		m_nodeSetsIndexes[ i ] = CNavgraphNodeSet::INVALID_ID;
		m_nodeSets[ i ] = NULL;
	}
}

void INodeSetPack::NodeSetDeleted( Uint32 category )
{
	m_nodeSets[ category ] = NULL;
}

CNavModyfication* INodeSetPack::AsNavModyfication()
{
	return NULL;
}
Bool INodeSetPack::IsUpdatingNodeCollisionOnAttach()
{
	return false;
}
void INodeSetPack::GenerationOnPreNodeSetConnection( CNavGraph* navgraph )
{

}
void INodeSetPack::GenerationOnPostNodeSetConnection( CNavGraph* navgraph )
{

}

};			// namespace PathLib

