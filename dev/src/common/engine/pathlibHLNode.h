/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraph.h"
#include "pathlibNodeFinder.h"

namespace PathLib
{

class CHLGraphBase;
class CHLSubGraph;

////////////////////////////////////////////////////////////////////////////
// High level graph node. One node represent coherent component of
// navigation graph.
class CHLNode : public CPathNode
{
	typedef CPathNode Super;
protected:
	Box											m_boundingArea;
	Int32										m_navNodesCount;
	Bool										m_nodeCollectionInvalidated;
	Bool										m_connectionsInvalidated;
	NodeFlags									m_requiredFlags;
	CNodeFinder									m_nodeFinder;

	void						ModifyBoundings( const CNavNode& node, Float nodeIndex );
public:
	CHLNode()																{}

	CHLNode( AreaId areaId, CHLGraphBase& subGraph, AreaRegionId areaRegionId );

	CHLNode( const CHLNode& hlNode )
		: Super( hlNode )
		, m_boundingArea( hlNode.m_boundingArea )
		, m_navNodesCount( hlNode.m_navNodesCount )
		, m_nodeCollectionInvalidated( hlNode.m_nodeCollectionInvalidated )
		, m_connectionsInvalidated( hlNode.m_connectionsInvalidated )
		, m_requiredFlags( hlNode.m_requiredFlags )
		, m_nodeFinder( hlNode.m_nodeFinder )								{}

	CHLNode( CHLNode&& hlNode )
		: Super( Move( hlNode ) )
		, m_boundingArea( hlNode.m_boundingArea )
		, m_navNodesCount( hlNode.m_navNodesCount )
		, m_nodeCollectionInvalidated( hlNode.m_nodeCollectionInvalidated )
		, m_connectionsInvalidated( hlNode.m_connectionsInvalidated )
		, m_requiredFlags( hlNode.m_requiredFlags )
		, m_nodeFinder( Move( hlNode.m_nodeFinder ) )						{}

	~CHLNode()																{}

	CHLNode&					operator=( CHLNode&& hlNode );

	RED_INLINE CoherentRegion	GetRegionId() const							{ return CoherentRegion( GetIndex() ); }

	void						PreNodesCollection();

	void						CollectNode( CNavNode& navnode );
	void						RemoveNode( CNavNode& navnode );

	void						WriteToBuffer( CSimpleBufferWriter& writer, LinkBufferIndex& linksCountAccumulator ) const;
	Bool						ReadFromBuffer( CSimpleBufferReader& reader, CPathLibGraph& graph, AreaId areaId, NodesetIdx nodesetIdx, Index idx, LinkBufferIndex& linksCountAccumulator );

	void						PreCenteralInitialization( CNavGraph& navgraph );
	void						CentralInitializationBBoxUpdate( CNavNode& node );
	void						CentralInitilzationComputeCelMap();
	void						CentralInitializationCollectNode( CNavNode& node );
	void						PostCentralInitialization();

	NodeFlags					GetForbiddenFlags() const					{ return NFG_FORBIDDEN_ALWAYS | ( NFG_BREAKS_COHERENT_REGION & (~m_requiredFlags) ); }
	NodeFlags					GetRequiredFlags() const					{ return m_requiredFlags; }

	CNodeFinder&				GetNodeFinder()								{ return m_nodeFinder; }
	const CNodeFinder&			GetNodeFinder() const						{ return m_nodeFinder; }

	void						InvalidateLinkage()							{ m_connectionsInvalidated = true; }
	void						MarkLinkageValid()							{ m_connectionsInvalidated = false; }

	Bool						IsNodeListInvalidated() const				{ return m_nodeCollectionInvalidated; }
	Bool						IsNodeLinkageInvalidated() const			{ return m_connectionsInvalidated; }
	Int32						GetNavnodesCount() const					{ return m_navNodesCount; }
};

};			// namespace PathLib