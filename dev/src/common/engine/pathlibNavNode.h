/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"
#include "pathlib.h"
#include "pathlibGraphBase.h"

namespace PathLib
{
	
class CAreaDescription;
class CHLNode;
class CNavNodesGraphBase;

////////////////////////////////////////////////////////////////////////////
// CNavGraph
// Lowest level navigation node (for final path plotting).
////////////////////////////////////////////////////////////////////////////
class CNavNode : public CPathNode
{
	friend class CNavGraph;
public:
	CNavNode()																{}
	CNavNode( const CNavNode& c )
		: CPathNode( c )													{}
	CNavNode( CNavNode&& c )
		: CPathNode( Move( c ) )											{}
	CNavNode( const Vector3& pos, Index index, AreaId areaId, CPathLibGraph& graph,  NodeFlags flags = NF_DEFAULT )
		: CPathNode( pos, index, areaId, graph, flags )						{}
	CNavNode( const Vector3& pos, Id id, AreaId areaId, CPathLibGraph& graph, NodeFlags flags = NF_DEFAULT )
		: CPathNode( pos, id, areaId, graph, flags )						{}

	CNavNode&		operator=( const CNavNode& n )							{ CPathNode::operator=( n ); return *this; }
	CNavNode&		operator=( CNavNode&& n )								{ CPathNode::operator=( Move( n ) ); return *this; }

	// in-game region id setup
	void			SetRegionId( CNavGraph& navgraph, AreaRegionId regionId );
	void			SetRegionId( CNavGraph& navgraph, CHLNode& hlNode );

	void			ClearRegionId()											{ m_region = INVALID_AREA_REGION; }

	RED_FORCE_INLINE CNavNodesGraphBase& GetNavgraph() const;
};


////////////////////////////////////////////////////////////////////////////
// CNavNodesGraphBase
// Graph of navigation nodes. Base implementation of functions effecting
// CNavNode's.
////////////////////////////////////////////////////////////////////////////
class CNavNodesGraphBase : public TPathLibGraph< CNavNode >
{
	typedef TPathLibGraph< CNavNode > Super;

protected:
	void				CalculateNodeAvailability( CAreaDescription* area, CPathNode* node, Float personalSpace );

public:
	CNavNodesGraphBase()
		: Super()															{}


	virtual CNavNode::NodesetIdx	VGetNodesetIndex() const = 0;

	void				VGenerationConnectNodes( CNavNode& targetNode, CNavNode& sourceNode, NodeFlags linkFlags = NF_DEFAULT, NavLinkCost linkCost = 0 );
	CNavNode&			VAddNode( const Vector3& position, NodeFlags nodeFlags );
	virtual Bool		VAreNodesLinkedById() const = 0;

	Bool				ConvertLinksToIds();								// modify existing graph
	Bool				ConvertLinksToPointers();
	
	// delete unused nodes
	Bool				DeleteMarked();
	void				GenerationBlockAllNodes();

	void				CompactData();

	void				WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool				ReadFromBuffer( CSimpleBufferReader& reader, CPathNode::NodesetIdx nodeSetIndex = CPathNode::INVALID_INDEX );
	void				OnPostLoad( CAreaDescription* area );

	Bool				Debug_CheckAllLinksTwoSided();
};


CNavNodesGraphBase& CNavNode::GetNavgraph() const
{
	return *static_cast< CNavNodesGraphBase* >( m_graph );
}

RED_INLINE const CNavNode* CSearchNode::AsNavNode() const
{
	return static_cast< const CNavNode* >( this );
}
RED_INLINE CNavNode* CSearchNode::ModifyNavNode()
{
	return static_cast< CNavNode* >( this );
}


};		// namespace PathLib

