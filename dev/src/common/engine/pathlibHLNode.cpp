/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibHLNode.h"

#include "pathlibHLSubGraph.h"
#include "pathlibNavNode.h"
#include "pathlibSimpleBuffers.h"


namespace PathLib
{

CHLNode::CHLNode( AreaId areaId, CHLGraphBase& subGraph, AreaRegionId areaRegionId )
	: CPathNode( Vector3( 0.f, 0.f, 0.f ), INVALID_INDEX, areaId, subGraph, 0, areaRegionId )
	, m_boundingArea( Box::RESET_STATE )
	, m_navNodesCount( 0 )
	, m_nodeCollectionInvalidated( false )
	, m_connectionsInvalidated( true )
	, m_requiredFlags( 0 )
{}

void CHLNode::ModifyBoundings( const CNavNode& node, Float nodeIndex )
{
	const Vector3& nodePos = node.GetPosition();
	if ( !m_boundingArea.Contains( nodePos ) )
	{
		m_boundingArea.AddPoint( nodePos );
		m_nodeCollectionInvalidated = true;
	}

	Float nodeRatio = 1.f / (nodeIndex + 1.f);
	Float currRatio = 1.f - nodeRatio;
	
	m_position *= currRatio;
	m_position += nodePos * nodeRatio;
}
CHLNode& CHLNode::operator=( CHLNode&& hlNode )
{
	Super::operator=( Move( hlNode ) );

	m_boundingArea = hlNode.m_boundingArea;
	m_navNodesCount = hlNode.m_navNodesCount;
	m_nodeCollectionInvalidated = hlNode.m_nodeCollectionInvalidated;
	m_connectionsInvalidated = hlNode.m_connectionsInvalidated;
	m_requiredFlags = hlNode.m_requiredFlags;
	m_nodeFinder = Move( hlNode.m_nodeFinder );

	return *this;
}
void CHLNode::PreNodesCollection()
{
	m_nodeCollectionInvalidated = false;
	m_boundingArea.Clear();
	m_position.Set( 0.f, 0.f, 0.f );
	m_navNodesCount = 0;						// this might be problematic since, nodes are about to automatically register themselves to hlnode
}

void CHLNode::CollectNode( CNavNode& navnode )
{
	InvalidateLinkage();

	if ( !m_nodeCollectionInvalidated )
	{
		ModifyBoundings( navnode, Float( m_navNodesCount ) );
	}

	if ( m_nodeFinder.IsInitialized() )
	{
		m_nodeFinder.AddDynamicElement( &navnode );
	}
	else
	{
		m_nodeCollectionInvalidated = true;
	}

	++m_navNodesCount;
}
void CHLNode::RemoveNode( CNavNode& navnode )
{
	InvalidateLinkage();

	--m_navNodesCount;

	if ( m_nodeFinder.IsInitialized() )
	{
		m_nodeFinder.RemoveDynamicElement( &navnode );
	}

	m_nodeCollectionInvalidated = true;
}

void CHLNode::WriteToBuffer( CSimpleBufferWriter& writer, LinkBufferIndex& linksCountAccumulator ) const
{
	writer.Put( m_boundingArea );
	writer.Put( m_navNodesCount );

	Super::WriteToBuffer( writer, linksCountAccumulator );
}
Bool CHLNode::ReadFromBuffer( CSimpleBufferReader& reader, CPathLibGraph& graph, AreaId areaId, NodesetIdx nodesetIdx, Index idx, LinkBufferIndex& linksCountAccumulator )
{
	if ( !reader.Get( m_boundingArea )
		|| !reader.Get( m_navNodesCount ) )
	{
		return false;
	}
	
	return Super::ReadFromBuffer( reader, graph, areaId, nodesetIdx, idx, linksCountAccumulator );
}

void CHLNode::PreCenteralInitialization( CNavGraph& navgraph )
{
	if ( m_nodeCollectionInvalidated )
	{
		m_boundingArea.Clear();
		m_position.Set( 0.f, 0.f, 0.f );
		m_navNodesCount = 0;
		m_nodeFinder.Invalidate();
	}
	m_nodeFinder.PreCenteralInitialization( navgraph, GetAreaRegionId());
}
void CHLNode::CentralInitializationBBoxUpdate( CNavNode& node )
{
	if ( m_nodeCollectionInvalidated )
	{
		ModifyBoundings( node, Float( m_navNodesCount ) );
		++m_navNodesCount;
	}
	m_nodeFinder.CentralInitializationBBoxUpdate( node );
}
void CHLNode::CentralInitilzationComputeCelMap()
{
	m_nodeFinder.CentralInitilzationComputeCelMap();
}
void CHLNode::CentralInitializationCollectNode( CNavNode& node )
{
	m_nodeFinder.CentralInitializationCollectNode( node );
}
void CHLNode::PostCentralInitialization()
{
	m_nodeCollectionInvalidated = false;
	m_nodeFinder.PostCentralInitialization();
}


};			// namespace PathLib