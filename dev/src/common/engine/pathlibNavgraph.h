#pragma once

#include "pathlibHLSubGraph.h"
#include "pathlibNavNode.h"
#include "pathlibCentralNodeFinder.h"

class CNavmeshQueryData;

////////////////////////////////////////////////////////////////////////////
// NAVGRAPH is LOWEST-LEVEL pathfinding graph responsible for final
// path plotting.
////////////////////////////////////////////////////////////////////////////

namespace PathLib
{

class CAreaDescription;
class CTerrainMap;
class CNavmesh;
class CNodeSetProcessingContext;
class CObstaclesMap;
class CObstaclesDetourInfo;
class CAreaGenerationJob;
class CNavgraphNodeSet;
class CObstacleShape;
class CSpecialZonesMap;

////////////////////////////////////////////////////////////////////////////
// CNavGraph
// Navigation graph object. Given area can have multiple navigation graphs.
////////////////////////////////////////////////////////////////////////////
class CNavGraph : public CNavNodesGraphBase
{
	typedef CNavNodesGraphBase Super;

public:
	typedef TArrayMap< CPathNode::NodesetIdx, CNavgraphNodeSet* > NodeSets;

protected:
	struct SInitialNeighbourData
	{
		CNavNode::Index		m_neighbourId;
		Vector3				m_centerOfMass;
		Float				m_centerOfMassWeight;
		Vector3				m_fallbackLocation;
	};

	struct SConnectorData
	{
		struct Connector
		{
			CNavNode::Index			m_nodeIdx;
			CNavNode::Index			m_neighbourNodeIdx;
		};
		struct AreaData
		{
			AreaId					m_areaId;
			Bool					m_connected;
			Uint32					m_navgraphBinaryVersion;
			TDynArray< Connector >	m_connectors;

			void					Sort();
		};
		TDynArray< AreaData >		m_areas;
	};

	struct SCookedConnectorData
	{
		CNavNode::Index		m_nodeId;
		CNavNode::Index		m_neighbourNodeIdx;
		AreaId				m_neighbourId;

		struct Comperator
		{
			Bool operator()( SCookedConnectorData& d1, SCookedConnectorData& d2 ) const
			{
				return d1.m_nodeId < d2.m_nodeId;
			}
			Bool operator()( SCookedConnectorData& d1, CNavNode::Index idx ) const
			{
				return d1.m_nodeId < idx;
			}
		};
	};

	typedef TDynArray< SCookedConnectorData > CookedConnectors;

	CAreaDescription*								m_areaDescription;
	CCentralNodeFinder								m_nodeFinder;
	NodeSets										m_nodeSets;
	SConnectorData									m_connectors;
	Float											m_maxNodesDistance;
	Uint16											m_category;
	Bool											m_nodesAreLinkedById;
	Uint32											m_binaryVersion;
	CHLSubGraph										m_hlGraph;

	Bool		InitialGraphGeneration( CPathLibWorld* world, CTerrainMap* terrain );
	template < class TArea >
	Bool		TInitialGraphGeneration( CPathLibWorld* world, CNavmesh* navmesh, TArea* area );

	template < class TArea >
	Bool		TOptimizeGraph( CPathLibWorld* world, TArea* area );

	// move nodes away from walls
	Bool		MoveNodesToAvailablePositions();
	// revise all nodes availability - for links id representation
	Bool		GenerationCalculateLinkAvailability( CNavNode* node, CPathLink* link );
	void		GenerationCalculateNodeAvailability( CNavNode* node );
	void		GenerationCalculateNodesAvailability();

	// obstacles algorithms
	void		HandleObstaclesMap( CObstaclesMap* obstacles, const CObstaclesDetourInfo& detourInfo );
	// rubber band algorithm to improve unavailable links
	Bool		Improve();
	// rubber band algorithm link improvement
	typedef TStaticArray< Vector3, 16 > LinkImprovementPath;
	Bool		ImproveLink( const Vector3& pos1, const Vector3& pos2, LinkImprovementPath& outPath, Int32 recurensyDepth = 0 );
	// post generation - interconnect node sets
	Bool		NodeSetsInterconnection();

	Bool		MarkSeparateNodesForDeletion();

	Bool		CanMoveNode( CNavNode* node, Vector3& position, Bool linksInPointerForm );

	Bool		InitialConnectNeighbours( CNavNode::Index baseNodeIndex, const SInitialNeighbourData& data );

	void		ClearNodeFinder();
	void		ComputeNodeFinder();

	Bool		ConvertLinksToIds();											// modify existing graph
	Bool		ConvertLinksToPointers();										// prepare graph for in-game usage
	void		CalculateAllLinksCost();

	CPathNode::NodesetIdx	GetUniqueNodeSetId() const;

	void		OnNodeArrayOverflow() override;

	Bool		Debug_CheckAllLinksTwoSided();
	void		Debug_MakeAllLinksWalkable();

	Uint32		ComputeBinariesVersion();

public:
	CNavGraph( Uint32 category, CAreaDescription* area = NULL );
	~CNavGraph();

	// Find closest accessible node
	CNavNode*	FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace );
	// Special version with coherent region, notice it possibly can trigger high level graph queries
	CNavNode*	FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace, CoherentRegion regionId );
	// Find closest accessible node with some tolerance
	CNavNode*	FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace, Float toleranceRadius, Vector3& outAccessiblePosition );
	CNavNode*	FindRandomNodeAround( CAreaDescription* area, const Vector3& pos, Float searchRadius );

	//void		CollectConnectors( AreaId idDestinationArea, TDynArray< Vector3 >& outConnectorsLocations );
	//void		SetInstanceConnectors( const TDynArray< TPair< Vector3, AreaId > >& locations );

	// in game graph updating
	void		RuntimeCalculateNodeAvailability( CNavNode& node, CNodeSetProcessingContext& context );
	void		RuntimeUpdateCollisionFlags( const Box& bbox, CNodeSetProcessingContext& context );

	void						OnPostLoad( CAreaDescription* area );
	CAreaDescription*			GetArea() const								{ return m_areaDescription; }
	const CCentralNodeFinder&	GetNodeFinder() const						{ return m_nodeFinder; }
	CCentralNodeFinder&			GetNodeFinder()								{ return m_nodeFinder; }
	const NodeSets&				GetNodeSets() const							{ return m_nodeSets; }
	Uint32						GetBinariesVersion()						{ return m_binaryVersion; }

	void		Unload() override;
	void		WriteToBuffer( CSimpleBufferWriter& writer );
	Bool		ReadFromBuffer( CSimpleBufferReader& reader );

	Float		GetPersonalSpace() const;
	Float		GetMaxNodesDistance() const									{ return m_maxNodesDistance; }
	Bool		IsNodesLinkedById() const									{ return m_nodesAreLinkedById; }

	////////////////////////////////////////////////////////////////////////
	// Generation stuff
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	Bool		Generate( CPathLibWorld* world, CNavmesh* navmesh, CAreaGenerationJob* job );
	Bool		Generate( CPathLibWorld* world, CTerrainMap* terrain, CAreaGenerationJob* job );

	// Water processing
	Bool		MarkSpecialZones( CGlobalWater* water, CSpecialZonesMap* specialZones );
#endif

	Bool		ConnectWithNeighbours();
	Bool		ConnectWithNeighbour( AreaId areaId, Bool mutualAttach );
	Bool		ConnectWithNeighbour( SConnectorData::AreaData& areaData, Bool mutualAttach );
	void		DetachFromNeighbours();
	void		DetachFromNeighbour( AreaId areaId, Bool mutualDetach );
	void		DetachFromNeighbour( SConnectorData::AreaData& areaData, Bool mutualDetach );
	void		ClearConnectors();
	void		MarkConnectedWithNeighbour( AreaId neighbourAreaId, Bool connected );

	void		CompactData();
	void		DeleteMarked();

	// In-game HL-graph computation procedures
	void		ComputeCoherentRegionsMarking();

	// this interface should be used ONLY in generation subprocedures - it should never be used on running graph
	LinkBufferIndex	GenerationConnectNode( CNavNode::Index nodeId1, CNavNode::Index nodeId2, NodeFlags linkFlags = NF_DEFAULT, NavLinkCost linkCost = 0 );	// connect node assuming (and supporting) generation process running
	void		GenerationMergeNodes( CNavNode& targetNode, CNavNode& sourceNode );														// merge all internal data info targetNode and mark sourceNode for deletion
	void		MarkNodeForDeletion( CNavNode::Id id );																					// marks node for deletion
	void		MarkNodeForDeletion( CNavNode::Index idx );																				// marks base node for deletion

	// heavy graph modyfication external algorithms
	Bool		ConnectNodeList( CNavNode* nodes, Uint32 nodesCount, NodeFlags defaultLinkFlags = CT_DEFAULT, Bool ignoreNodeSets = false );	// powerful function that connect set of nodes to navgraph (assuming that input nodes are close to each other)
	Bool		CutOutWithShape( CObstacleShape* shape );

	void		ConnectNode( CNavNode::Id nodeId1, CNavNode::Id nodeId2, NodeFlags linkFlags, NavLinkCost linkCost );
	void		ConnectNode( CNavNode& node1, CNavNode& node2, NodeFlags linkFlags, NavLinkCost linkCost );
	
	Bool		TryToConnectExternalConnector( CNavNode* externalConnector, CAreaDescription* fromArea );

	static Bool	TryToCreateConnectors( CAreaDescription* area1, CAreaDescription* area2, const TDynArray< Vector3 >& locations );

	CNavgraphNodeSet*	CreateNodeSet();
	CNavgraphNodeSet*	GetNodeSet( CPathNode::NodesetIdx idx );
	void				ClearNodeSet( CPathNode::NodesetIdx idx );

	using Super::GetNode;
	const CNavNode*		GetNode( CPathNode::Id id ) const;
	CNavNode*			GetNode( CPathNode::Id id );
	CNavNode&			AddNode( const Vector3& position, NodeFlags flags = NF_DEFAULT );

	Bool				VAreNodesLinkedById() const override;
	AreaId				VGetAreaId() const override;
	CPathLibWorld*		VGetPathLibWorld() const override;
	CPathNode*			VGetPathNode( CPathNode::Id id ) override;
	CPathNode*			VGetExternalPathNode( CPathNode::Index idx, AreaId areaId ) override;
	LinkBufferIndex			VGetExtraLinksSpace() const override;
	CNavNode::NodesetIdx	VGetNodesetIndex() const override;

	CHLSubGraph&		GetHLGraph()										{ return m_hlGraph; }
	const CHLSubGraph&	GetHLGraph() const									{ return m_hlGraph; }
	Uint32				GetCategory() const									{ return m_category; }
	Uint32				GetTotalNodesCount() const;
	// End of Generation stuff
	////////////////////////////////////////////////////////////////////////

};

}		// namespace PathLib



