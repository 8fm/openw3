/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibHLSubGraph.h"

#include "pathlibAreaDescription.h"
#include "pathlibHLGraph.h"
#include "pathlibNavgraph.h"
#include "pathlibSearchEngine.h"
#include "pathlibWorld.h"

#include "pathlibNavgraphHelper.inl"

namespace PathLib
{

CPathNode* CHLSubGraph::VGetExternalPathNode( CPathNode::Index idx, AreaId areaId )
{
	CAreaDescription* area = m_navgraph.GetArea()->GetPathLib().GetAreaDescription( areaId );
	if ( !area )
	{
		return nullptr;
	}
	CNavGraph* navgraph = area->GetNavigationGraph( m_navgraph.GetCategory() );
	if ( !navgraph )
	{
		return nullptr;
	}
	return navgraph->GetHLGraph().GetHLNode( idx );
}

CPathLibWorld* CHLSubGraph::VGetPathLibWorld() const
{
	return &m_navgraph.GetArea()->GetPathLib();
}
AreaId CHLSubGraph::VGetAreaId() const
{
	return m_navgraph.GetArea()->GetId();
}

CHLNode& CHLSubGraph::AddHLNode( AreaRegionId areaRegionId )
{
	AreaId areaId = GetNavgraph().GetArea()->GetId();
	return Super::AddNode( areaId, areaRegionId );
}
CHLNode* CHLSubGraph::FindHLNode( AreaRegionId areaRegionId )
{
	return Super::FindNode( areaRegionId );
}

CHLNode& CHLSubGraph::RequestHLNode( AreaRegionId areaRegionId )
{
	CHLNode* hlNode = Super::FindNode( areaRegionId );

	if ( hlNode )
	{
		return *hlNode;
	}

	return AddHLNode( areaRegionId );
}

void CHLSubGraph::ConnectRegions( CHLNode& n0, CHLNode& n1, NodeFlags nodeFlags )
{
	PATHLIB_ASSERT( &n0 != &n1, TXT("PathLib! Trying to connect hl node with itself.") );
	PATHLIB_ASSERT( (n0.GetAreaId() == n1.GetAreaId()) || ( (nodeFlags & NF_CONNECTOR) != 0 ));
	CPathLink* link = n0.GetLinkTo( &n1 );
	if ( !link )
	{
		LinkBufferIndex idx = AddLink( n0, CPathLink( &n1, NFG_BREAKS_COHERENT_REGION, 1 ) );
		if ( ( nodeFlags & NF_IS_ONE_SIDED ) == 0 )
		{
			AddLink( n1, CPathLink( &n0, NFG_BREAKS_COHERENT_REGION, 1 ) );
		}

		link = &m_links.Get( idx );
	}

	NodeFlags flagsToClear = (~nodeFlags) & NFG_BREAKS_COHERENT_REGION;
	CPathLinkModifier modifier( n0, *link );
	modifier.ClearFlags( flagsToClear );
}

void CHLSubGraph::ConnectRegions( AreaRegionId region0, AreaRegionId region1, NodeFlags nodeFlags )
{
	CHLNode* node0 = FindHLNode( region0 );
	CHLNode* node1 = FindHLNode( region1 );

	if ( !node0 || !node1 )
	{
		return;
	}

	ConnectRegions( *node0, *node1, nodeFlags );

}

void CHLSubGraph::ConnectRegions( AreaRegionId region, AreaId destAreaId, AreaRegionId destRegion, NodeFlags nodeFlags )
{
	CNavGraph& navgraph = GetNavgraph();
	if ( destAreaId == navgraph.GetArea()->GetId() )
	{
		ConnectRegions( region, destRegion, nodeFlags );
	}

	nodeFlags |= NF_CONNECTOR;

	CHLNode* node0 = FindHLNode( region );
	if ( !node0 )
	{
		return;
	}
	CHLNode* node1 = HLGraph::FindNode( navgraph.GetArea()->GetPathLib(), navgraph.GetCategory(), MakeCoherentRegion( destAreaId, destRegion ) );
	if ( !node1 )
	{
		return;
	}

	ConnectRegions( *node0, *node1, nodeFlags );
}

void CHLSubGraph::UnlinkFromHLGraph()
{
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		CHLNode& node = *itNodes;
		node.Unlink( NF_CONNECTOR );
	}
	PATHLIB_ASSERT( Debug_CheckAllLinksTwoSided() );
}

void CHLSubGraph::CleanLegacyNodes()
{
	PATHLIB_ASSERT( Debug_CheckAllLinksTwoSided() );
	Bool isDirty = false;
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		CHLNode& node = *itNodes;
		if ( node.GetNavnodesCount() <= 0 )
		{
			isDirty = true;
			MarkNodeForDeletion( node );
		}
	}
	if ( isDirty )
	{
		DeleteMarked();
		PATHLIB_ASSERT( Debug_CheckAllLinksTwoSided() );
	}
}

void CHLSubGraph::UpdateLinkage()
{
	struct Functor
	{
		CHLSubGraph&			m_hlGraph;
		CSearchEngine&			m_searchEngine;
		Uint32					m_subGraphsToUpdate;

		Functor( CHLSubGraph& hl, CSearchEngine& searchEngine, Uint32 subGraphToUpdate )
			: m_hlGraph( hl )
			, m_searchEngine( searchEngine )
			, m_subGraphsToUpdate( subGraphToUpdate ) {}

		Bool operator()( CNavNode& navNode )
		{
			AreaRegionId regionId = navNode.GetAreaRegionId();
			if ( regionId != INVALID_AREA_REGION )
			{
				CHLNode* hlNode = m_hlGraph.FindHLNode( regionId );
				if ( hlNode && hlNode->IsNodeLinkageInvalidated() )
				{
					CRegionFloodFillData queryData( &navNode, &m_hlGraph.m_navgraph, regionId, hlNode->GetForbiddenFlags(), hlNode->GetRequiredFlags() );
					m_searchEngine.ComputeRegionConnection( queryData, &m_hlGraph.m_navgraph, hlNode );
					if ( (--m_subGraphsToUpdate) == 0 )
					{
						return false;
					}
				}
			}
			return true;
			
		}
	};


	Uint32 subGraphsToUpdate = 0;

	CSearchEngine& searchEngine = m_navgraph.GetArea()->GetPathLib().GetSearchEngine();
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		CHLNode& hlNode = *itNodes;
		if ( hlNode.IsNodeLinkageInvalidated() )
		{
			++subGraphsToUpdate;
		}
	}

	if( subGraphsToUpdate == 0 )
	{
		return;
	}

	Functor functor( *this, searchEngine, subGraphsToUpdate );

	NavgraphHelper::ForAllNodesBreakable( m_navgraph, functor, true );
	

	PATHLIB_ASSERT( Debug_CheckAllLinksTwoSided() );

}

Bool CHLSubGraph::Debug_HLCheckAllLinksTwoSided()
{
	return Super::Debug_CheckAllLinksTwoSided();
}

};			// namespace PathLib

