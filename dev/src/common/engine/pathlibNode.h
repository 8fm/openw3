/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "pathlibConst.h"
#include "pathlib.h"

class CPathLibWorld;
class CSimpleBufferWriter;
class CSimpleBufferReader;


////////////////////////////////////////////////////////////////////////////
// PATHLIB GRAPH is abstract interface for pathfinding graph - that
// could be low level - or higher level one.
////////////////////////////////////////////////////////////////////////////


namespace PathLib
{

class CPathLink;
class CPathLibGraph;
class CPathNode;
class CNavGraph;
class CNavNode;
class LinksErasableIterator;

////////////////////////////////////////////////////////////////////////////
// CSearchNode
// Class that represent all data needed be search engine.
// Currently there is no support for multithreading. If we would like to do
// some, we would have to detach this structure from node, and store it
// externally as some thread context data.
////////////////////////////////////////////////////////////////////////////
class CSearchNode
{
	friend class CSearchEngine;
public:
	typedef Uint32		HeapIndex;

	static const HeapIndex INVALID_HEAP_INDEX								= 0xffffffff;
protected:
	// runtime search data
	CSearchNode*		m_previousNode;																									// 8
	PathCost			m_currentCost;										// Cost to get to this node from start						// 12
	PathCost			m_totalCost;										// Cost from start + heuristic cost to end					// 16

	Uint16				m_markerVisited;																								// 18
	Uint16				m_markerDestination;																							// 20

	HeapIndex			m_heapIndex;																									// 24
public:
	struct Marking
	{
		friend class CSearchEngine;
	private:
		Uint16			m_marker;

	public:
		Marking()											{} // Notice empty default constructor
		Marking( Uint16 marker )
			: m_marker( marker )							{}
		Marking( CPathLibWorld& pathlib );


		void			Mark( CSearchNode& node ) const						{ node.m_markerVisited = m_marker; }
		Bool			CheckMarking( CSearchNode& node ) const				{ return node.m_markerVisited == m_marker; }

		Bool			Debug_IsMarkerValid( CPathLibWorld& pathlib );
	};


	CSearchNode()
		: m_markerVisited( 0xffff )
		, m_markerDestination( 0xffff )
		, m_heapIndex( INVALID_HEAP_INDEX )									{}
	CSearchNode( const CSearchNode& otherNode )								// NOTICE: we don't really care about coping this data, as we cant do while search is running - and we don't care at other times
		: m_markerVisited( 0xffff )
		, m_markerDestination( 0xffff )
		, m_heapIndex( INVALID_HEAP_INDEX )									{}

	static RED_INLINE PathCost	TotalCost( PathCost currentCost, PathCost heuristicCost )
	{
		return currentCost + heuristicCost;
	}
	RED_INLINE void				Set( PathCost currentCost, PathCost heuristicCost, CSearchNode* previousNode = NULL )
	{
		m_previousNode = previousNode; 
		m_currentCost = currentCost; 
		m_totalCost = TotalCost( currentCost, heuristicCost ); 
	}
	RED_INLINE void				Update( PathCost currentCost, CSearchNode* previousNode )
	{
		Int32 costDiff = currentCost - m_currentCost;

		m_previousNode = previousNode;
		m_currentCost = currentCost;
		m_totalCost += costDiff;
	}

	// from CSearchNode we have only const interface to CPathNode
	RED_INLINE CNavNode*		ModifyNavNode();
	RED_INLINE const CNavNode*	AsNavNode() const;
	RED_INLINE CPathNode*		ModifyPathNode();
	RED_INLINE const CPathNode*	AsPathNode() const;
	RED_INLINE PathCost			GetTotalCost() const					{ return m_totalCost; }
	RED_INLINE PathCost			GetCurrentCost() const					{ return m_currentCost; }
	RED_INLINE PathCost			GetHeuristicCost() const				{ return m_totalCost - m_currentCost; }
	RED_INLINE CSearchNode*		GetPreviousNode() const					{ return m_previousNode; }
	RED_INLINE HeapIndex		GetHeapIndex() const					{ return m_heapIndex; }
	RED_INLINE const Vector3&	GetPosition() const;

	RED_INLINE void				SetHeapIndex( HeapIndex index )			{ m_heapIndex = index; }
	RED_INLINE void				SwapHeapIndex( CSearchNode* node )		{ ::Swap( m_heapIndex, node->m_heapIndex ); }
	RED_INLINE void				SetVisited( Uint16 marker )				{ m_markerVisited = marker; }
};

////////////////////////////////////////////////////////////////////////////
// CPathNode
// PathLib node. Currently its base for HL pathfinding node, and it got all
// data needed by navigation graph.
////////////////////////////////////////////////////////////////////////////
class CPathNode : public CSearchNode
{
	friend class CPathLibGraph;
	friend class LinksErasableIterator;
public:
	typedef Uint32 Index;
	typedef Uint32 NodesetIdx;
	static const Index INVALID_INDEX = 0xffffffff;
	typedef LinkBufferIndex tLinks;
	struct Id
	{
		static const Id INVALID;
		static const Id VALUE_MIN;
		static const Id VALUE_MAX;
		union
		{
			struct 
			{
				Index			m_index;
				NodesetIdx		m_nodeSetIndex;
			};
			Uint64				m_id;
		};

		Bool operator<( const Id& id ) const								{ return m_id < id.m_id; }
		Bool operator>( const Id& id ) const								{ return m_id > id.m_id; }
		Bool operator==( const Id& id ) const								{ return m_id == id.m_id; }
		Bool operator!=( const Id& id ) const								{ return m_id != id.m_id; }
		void Set( const Id& id )											{ m_id = id.m_id; }
		Uint32 CalcHash() const												{ return GetHash( m_id ); }

		static Id Create( Index idx, NodesetIdx nodeSetIdx )				{ Id id; id.m_index = idx; id.m_nodeSetIndex = nodeSetIdx; return id; }
	};
protected:
	// 24
	Id					m_id;												// 32
	NodeFlags			m_flags;											// 34
	AreaRegionId		m_region;											// 36
	tLinks				m_links;											// 40
	Vector3				m_position;											// 52
	AreaId				m_areaId;											// 54 // is unnessessary now, since we do have reference to graph, but as we are allready loosing some space due to alignment, lets keep it for fast access.
																			// 56 // 2 unused
	CPathLibGraph*		m_graph;											// 64

public:
	CPathNode()																{}
	CPathNode( const CPathNode& c )
		: m_id( c.m_id )
		, m_flags( c.m_flags )
		, m_region( c.m_region )
		, m_links( c.m_links )
		, m_position( c.m_position )
		, m_areaId( c.m_areaId )
		, m_graph( c.m_graph )												{}
	CPathNode( CPathNode&& c )
		: m_id( c.m_id )
		, m_flags( c.m_flags )
		, m_region( c.m_region )
		, m_links( c.m_links )
		, m_position( c.m_position )
		, m_areaId( c.m_areaId )
		, m_graph( c.m_graph )												{}
	CPathNode( const Vector3& pos, Index index, AreaId areaId, CPathLibGraph& graph, NodeFlags flags = NF_DEFAULT, AreaRegionId regionId = INVALID_AREA_REGION )
		: m_flags( flags )
		, m_region( regionId )
		, m_links( INVALID_LINK_BUFFER_INDEX )	
		, m_position( pos )	
		, m_areaId( areaId )
		, m_graph( &graph )													{ m_id.m_index = index; m_id.m_nodeSetIndex = INVALID_INDEX; }
	CPathNode( const Vector3& pos, Id id, AreaId areaId, CPathLibGraph& graph, NodeFlags flags = NF_DEFAULT, AreaRegionId regionId = INVALID_AREA_REGION )
		: m_id( id )
		, m_flags( flags )
		, m_region( regionId )
		, m_links( INVALID_LINK_BUFFER_INDEX )
		, m_position( pos )
		, m_areaId( areaId )
		, m_graph( &graph )													{}

	CPathNode&					operator=( const CPathNode& n );

	RED_INLINE Index			GetIndex() const							{ return m_id.m_index; }
	RED_INLINE NodesetIdx		GetNodesetIndex() const						{ return m_id.m_nodeSetIndex; }
	RED_INLINE Id				GetFullId() const							{ return m_id; }
	RED_INLINE NodeFlags		GetFlags() const							{ return m_flags; }
	RED_INLINE AreaId			GetAreaId() const							{ return m_areaId; }
	RED_INLINE CoherentRegion	GetRegionId() const							{ return MakeCoherentRegion( m_areaId, m_region ); }
	RED_INLINE AreaRegionId		GetAreaRegionId() const						{ return m_region; }
	//RED_INLINE void			SetRegionId( CoherentRegion id )			{ m_region = id; }
	RED_INLINE const Vector3&	GetPosition() const							{ return m_position; }
	RED_INLINE void				SetPosition( const Vector3& v )				{ m_position = v; }
	RED_INLINE void				InternalChangeIndex( Index idx )			{ m_id.m_index = idx; }
	RED_INLINE CPathLibGraph&	GetGraph() const							{ return *m_graph; }

	RED_INLINE Bool				HaveFlag(ENodeFlags flag) const				{ return (m_flags & flag) != 0; }
	RED_INLINE Bool				HaveAnyFlag(NodeFlags flags) const			{ return (m_flags & flags) != 0; }
	RED_INLINE Bool				HaveAllFlags(NodeFlags flags) const			{ return (m_flags & flags) == flags; }

	RED_INLINE void				AddFlags(NodeFlags flags)					{ m_flags |= flags; }
	RED_INLINE void				ClearFlags(NodeFlags flags)					{ m_flags &= ~flags; }
	RED_INLINE void				SetFlags(NodeFlags flags)					{ m_flags = flags; }

	void						AddFlagsToLinks(NodeFlags flags);
	void						ClearFlagsAtLinks(NodeFlags flags);

	const CPathLink*			GetLinkTo(const CPathNode* destination) const;
	CPathLink*					GetLinkTo(const CPathNode* destination);

	tLinks						GetLinksArray() const						{ return m_links; }
	Bool						IsUnlinked() const							{ return m_links == INVALID_LINK_BUFFER_INDEX; }
	void						AddLink(LinkBufferIndex idx);

	Bool						IsConnected(const CPathNode& node) const	{ return IsConnected( node.m_id ); }
	Bool						IsConnected(Index idx) const				{ Id nodeId; nodeId.m_index = idx; nodeId.m_nodeSetIndex = INVALID_INDEX; return IsConnected( nodeId ); }
	Bool						IsConnected(Id nodeId) const;

	void						EraseLink( LinksErasableIterator& it );								// erase that supports links id form
	void						Unlink();															// erase all links
	void						Unlink( NodeFlags byFlag );											// erase all links that match given flag

	//void						Serialize( IFile& file );
	void						WriteToBuffer( CSimpleBufferWriter& writer, LinkBufferIndex& linksCountAccumulator ) const;
	Bool						ReadFromBuffer( CSimpleBufferReader& reader, CPathLibGraph& graph, AreaId areaId, NodesetIdx nodeSetIdx, Index index, LinkBufferIndex& linksCountAccumulator );
};

////////////////////////////////////////////////////////////////////////////
// CPathLink
// Link data. Links are stored inside navnodes, so they usually exist in
// two exact copies (except for one sided). This generally makes search
// faster and miraculously decrease system memory usage - as having
// pointers in both nodes + some structure to hold links is more costly
// that storing this same data twice.
// Also notice that navlink represents all data needed by low-level
// navgraph, and for high level graph rest of the data are stored inside
// the node (and accessed with m_highLevelLinkIndex).
////////////////////////////////////////////////////////////////////////////
class CPathLink
{
	// modification interface available ONLY from CNavLinkModifier
	// for safety reasons (nav links basically always exist in pairs).
	friend class CPathLinkModifier;
	friend class CPathLibGraph;
	friend class CPathNode;
	friend class CLinksBuffer;
	friend class LinksIterator;
	friend class LinksErasableIterator;
	friend class ConstLinksIterator;
protected:
	typedef Int16 CustomLinkIndex;
	typedef Int16 HighLevelLinkIndex;

	union
	{
		CPathNode*					m_destination;							// run-time form
		CPathNode::Id				m_destinationId;						// serialization "safe" form
	};																													// 8

	NodeFlags						m_flags;																			// 10
	NavLinkCost						m_cost;																				// 12
private:
	Uint32							m_next;																				// 16
public:
	CPathLink()																{}
	CPathLink( const CPathLink& c )
		: m_destination( c.m_destination )
		, m_flags( c.m_flags )
		, m_cost( c.m_cost )
		, m_next( INVALID_LINK_BUFFER_INDEX )								{}
	CPathLink( CPathNode* destination, NodeFlags flags, NavLinkCost cost )
		: m_destination( destination )
		, m_flags( flags )
		, m_cost( cost )
		, m_next( INVALID_LINK_BUFFER_INDEX )								{}
	CPathLink( CPathNode::Index destinationIndex, CPathNode::NodesetIdx destinationNodeSet, NodeFlags flags = 0, NavLinkCost cost = 0 )
		: m_flags( NF_DESTINATION_IS_ID | flags )
		, m_cost( cost )
		, m_next( INVALID_LINK_BUFFER_INDEX )								{ m_destinationId.m_index = destinationIndex; m_destinationId.m_nodeSetIndex = destinationNodeSet; }
	// notice usage of default constructors
	RED_INLINE NavLinkCost			GetCost() const							{ return m_cost; }
	RED_INLINE Bool					HaveFlag(ENodeFlags flag) const			{ return (m_flags & flag) != 0; }
	RED_INLINE Bool					HaveAnyFlag(NodeFlags flags) const		{ return (m_flags & flags) != 0; }
	RED_INLINE Bool					HaveAllFlags(NodeFlags flags) const		{ return (m_flags & flags) == flags; }
	RED_INLINE NodeFlags			GetFlags() const						{ return m_flags; }
	RED_INLINE Bool					IsCustomLink() const					{ return (m_flags & NF_IS_CUSTOM_LINK) != 0; }

	RED_INLINE CPathNode*			GetDestination() const					{ ASSERT( (m_flags & NF_DESTINATION_IS_ID) == 0 ); return m_destination; }
	RED_INLINE CPathNode::Id		GetDestinationId() const				{ ASSERT( m_flags & NF_DESTINATION_IS_ID ); return m_destinationId; }

	//void							Serialize( IFile& file );
	void							WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool							ReadFromBuffer( CSimpleBufferReader& reader );
};

////////////////////////////////////////////////////////////////////////////
// CNavLinkModifier
// Temporary object used to protect link modyfications - so we always
// modify them both, keeping their inner state consistant.
////////////////////////////////////////////////////////////////////////////
class CPathLinkModifier
{
protected:
	CPathLink*		m_link1;
	CPathLink*		m_link2;
public:
	CPathLinkModifier( CPathNode& node, CPathLink& link );

	void			SetCost( NavLinkCost cost );
	void			AddFlags( NodeFlags flags );
	void			ClearFlags( NodeFlags flags );
	void			SetFlags( NodeFlags flags );
	Bool			Erase();
	Bool			GenerationErase( CPathLibGraph* graph );
	void			ChangeNodeIndex( CPathNode::Index idFrom, CPathNode::Index itTo );

	void			ConvertToPointers( CPathLibGraph* graph );
	void			ConvertToIds( CPathLibGraph* graph );

	void			ConvertConnectorToPointers( CPathLibGraph* graph );
	void			ConvertConnectorToIds( CPathLibGraph* graph );

	Bool			IsDoubleSided() const									{ return m_link1 && m_link2; }
};



};			// namespace PathLib