/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNavNode.h"

#include "pathlibAreaDescription.h"
#include "pathlibHLSubGraph.h"
#include "pathlibNavgraph.h"
#include "pathlibSimpleBuffers.h"

#include "pathlibGraphBase.inl"

//#define DEBUG_NODESETS_ATTACHMENT_MECHANISM

#ifdef DEBUG_NODESETS_ATTACHMENT_MECHANISM
	#include "pathlibNodeSet.h"
#endif


namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CNavNode
////////////////////////////////////////////////////////////////////////////
void CNavNode::SetRegionId( CNavGraph& navgraph, AreaRegionId regionId )
{
	if ( m_region == regionId )
	{
		return;
	}

#ifdef DEBUG_NODESETS_ATTACHMENT_MECHANISM
	if ( m_id.m_nodeSetIndex != INVALID_INDEX )
	{
		CNavgraphNodeSet* nodeSet = navgraph.GetNodeSet( m_id.m_nodeSetIndex );
		if ( !nodeSet->IsAttached() )
		{
			PATHLIB_LOG( TXT("Node from detached nodeset is becoming attached to nodefinder!") );
		}
	}
#endif

	CHLSubGraph& hlGraph = navgraph.GetHLGraph();

	{
		Bool wasDetached = true;
		if ( m_region != INVALID_AREA_REGION )
		{
			CHLNode* node = hlGraph.FindHLNode( m_region );
			if ( node )
			{
				node->RemoveNode( *this );
				wasDetached = false;
			}
		}
		if ( wasDetached )
		{
			navgraph.GetNodeFinder().GetDetachedNodesMap().RemoveDynamicElement( this );
		}
	}
	
	
	
	m_region = regionId;

	{
		if ( m_region != INVALID_AREA_REGION )
		{
			CHLNode& node = hlGraph.RequestHLNode( m_region );
			node.CollectNode( *this );
		}
		else
		{
			navgraph.GetNodeFinder().GetDetachedNodesMap().AddDynamicElement( this );
		}
	}
	
}

void CNavNode::SetRegionId( CNavGraph& navgraph, CHLNode& hlNode )
{
	AreaRegionId regionId = hlNode.GetAreaRegionId();
	if ( m_region == regionId )
	{
		return;
	}

#ifdef DEBUG_NODESETS_ATTACHMENT_MECHANISM
	if ( m_id.m_nodeSetIndex != INVALID_INDEX )
	{
		CNavgraphNodeSet* nodeSet = navgraph.GetNodeSet( m_id.m_nodeSetIndex );
		if ( !nodeSet->IsAttached() )
		{
			PATHLIB_LOG( TXT("Node from detached nodeset is becoming attached to nodefinder!") );
		}
	}
#endif

	CHLSubGraph& hlGraph = navgraph.GetHLGraph();

	{
		Bool wasDetached = true;
		if ( m_region != INVALID_AREA_REGION )
		{
			CHLNode* node = hlGraph.FindHLNode( m_region );
			if ( node )
			{
				node->RemoveNode( *this );
				wasDetached = false;
			}
		}
		if ( wasDetached )
		{
			navgraph.GetNodeFinder().GetDetachedNodesMap().RemoveDynamicElement( this );
		}
	}

	m_region = regionId;

	hlNode.CollectNode( *this );

}

////////////////////////////////////////////////////////////////////////////
// CNavNodesGraphBase
////////////////////////////////////////////////////////////////////////////
void CNavNodesGraphBase::VGenerationConnectNodes( CNavNode& node1, CNavNode& node2, NodeFlags linkFlags , NavLinkCost linkCost )
{
	if ( linkCost == 0 && ( linkFlags & NF_IS_GHOST_LINK ) == 0 )
	{
		linkCost = CalculateLinkCost( ( node1.GetPosition().AsVector2() - node2.GetPosition().AsVector2() ).Mag() );
	}

	if ( VAreNodesLinkedById() )
	{
		node2.GetNavgraph().AddLink( node2, CPathLink( node1.GetIndex(), node1.GetNodesetIndex(), linkFlags, linkCost ) );
		node1.GetNavgraph().AddLink( node1, CPathLink( node2.GetIndex(), node2.GetNodesetIndex(), linkFlags, linkCost ) );
	}
	else
	{
		node2.GetNavgraph().AddLink( node2, CPathLink( &node1, linkFlags, linkCost ) );
		node1.GetNavgraph().AddLink( node1, CPathLink( &node2, linkFlags, linkCost ) );
	}
}
CNavNode& CNavNodesGraphBase::VAddNode( const Vector3& position, NodeFlags nodeFlags )
{
	AreaId areaId = VGetAreaId();
	CNavNode::NodesetIdx idx = VGetNodesetIndex();
	if ( idx != CNavNode::INVALID_INDEX )
	{
		nodeFlags |= NF_IS_IN_NODESET;
	}
	return Super::AddNode( position, areaId, idx, nodeFlags );
}

void CNavNodesGraphBase::CalculateNodeAvailability( CAreaDescription* area, CPathNode* node, Float personalSpace )
{
	NodeFlags nodeFlags = node->GetFlags() | NF_BLOCKED;

	for ( LinksIterator itLinks( *node ); itLinks; ++itLinks )
	{
		CPathLink& link = *itLinks;

		CPathNode* destinationNode = link.GetDestination();
		CPathLinkModifier modifier( *node, link );

		CWideLineQueryData query( CT_NO_ENDPOINT_TEST, node->GetPosition(), destinationNode->GetPosition(), personalSpace );
		if ( link.HaveFlag( NF_IS_GHOST_LINK ) || area->VSpatialQuery( query ) )
		{
			modifier.ClearFlags( NF_BLOCKED );
			nodeFlags &= ~NF_BLOCKED;
		}
		else
		{
			modifier.AddFlags( NF_BLOCKED );
		}
	}

	node->SetFlags( nodeFlags );
}

Bool CNavNodesGraphBase::ConvertLinksToIds()
{
	return Super::ConvertLinksToIds();
}

Bool CNavNodesGraphBase::DeleteMarked()
{
	struct Implementation
	{
		enum { PRE_NODE_TRASHING = false };

		//SConnectorData&		m_connectors;

		//Implementation( SConnectorData& connectors )
		//	: m_connectors( connectors ) {}

		void PreNodeTrashing( CNavNode& node )
		{

		}
		void PreDeletion()
		{

		}
		void PostDeletion()
		{

		}
		void PreNodeIndexChange( CNavNode& originalNode, CNavNode::Index shiftedIndex )
		{
			// fix connectors list
			ASSERT( !originalNode.HaveFlag( NF_CONNECTOR ) );
			//if ( originalNode.HaveFlag( NF_CONNECTOR ) )
			//{
			//	for ( auto itArea = m_connectors.Begin(), endArea = m_connectors.end(); itArea != endArea; +itArea )
			//	{
			//	}
			//	CNavNode::Index currIndex = originalNode.GetIndex();
			//	auto itFind = ::LowerBound( m_connectors.Begin(), m_connectors.End(), currIndex, SCookedConnectorData::Comperator() );
			//	ASSERT( itFind != m_connectors.End() && itFind->m_nodeId == currIndex );
			//	itFind->m_nodeId = shiftedIndex;
			//}
		}

	} implementation;//( m_connectorsLegacy );

	return Super::DeleteMarked( implementation );
}

void CNavNodesGraphBase::GenerationBlockAllNodes()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		node.AddFlagsToLinks( NF_BLOCKED );
	}
}

void CNavNodesGraphBase::CompactData()
{
	m_nodes.Shrink();
}

Bool CNavNodesGraphBase::ConvertLinksToPointers()
{
	return Super::ConvertLinksToPointers();
}

void CNavNodesGraphBase::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );
}
Bool CNavNodesGraphBase::ReadFromBuffer( CSimpleBufferReader& reader, CPathNode::NodesetIdx nodeSetIndex )
{
	return Super::ReadFromBuffer( reader, nodeSetIndex );
}
void CNavNodesGraphBase::OnPostLoad( CAreaDescription* area )
{
	
}


Bool CNavNodesGraphBase::Debug_CheckAllLinksTwoSided()
{
	return Super::Debug_CheckAllLinksTwoSided();
}


};		// namespace PathLib

