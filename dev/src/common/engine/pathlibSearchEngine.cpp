#include "build.h"
#include "pathlibSearchEngine.h"

#include "abstractPathRater.h"
#include "pathlibAreaDescription.h"
#include "pathlibHLNode.h"
#include "pathlibNavgraph.h"
#include "pathlibWorld.h"
#include "pathlibUtils.h"

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CConsistenctComputeData
////////////////////////////////////////////////////////////////////////////
CRegionFloodFillData::CRegionFloodFillData( CSearchNode* baseNode, CNavGraph* navgraph, AreaRegionId regionColor, NodeFlags forbiddenFlags, NodeFlags requiredFlags )
	: m_regionColor( regionColor )
{
	m_start = baseNode;
	m_flagsForbidden = forbiddenFlags | NF_CONNECTOR;
	m_requiredFlags = requiredFlags;
	m_graph = navgraph;	
}

////////////////////////////////////////////////////////////////////////////
// CSearchEngine
////////////////////////////////////////////////////////////////////////////
CSearchEngine::CSearchEngine()
	: m_marker( 0 )
{

}

RED_INLINE void CSearchEngine::InitializeAstarStartingNode( CSearchNode* startNode )
{
	startNode->Set( 0, 0 );		// its impossible to come back to starting node

	m_activeNodes.Push( startNode );

	startNode->m_markerVisited = m_marker;
}

template < class TImplementation >
RED_INLINE Bool CSearchEngine::TAstar( CBrowseData& data, TImplementation& implementation )
{
	implementation.Initialize();

	// Process A*
	do
	{
		CSearchNode* currentNode = m_activeNodes.Pop();

		implementation.StartNode( currentNode );

		if ( implementation.CheckSuccess( currentNode ) )
		{
			implementation.OnSuccess( currentNode );
			m_activeNodes.Clear();
			return true;
		}

		// lets store node cost
		PathCost currentCost = currentNode->GetCurrentCost();
		const CPathNode* pathNode = currentNode->AsPathNode();
		// generate successors
		for ( ConstLinksIterator it( pathNode->GetGraph().GetLinksBuffer(), pathNode->GetLinksArray() ); it; ++it )
		{
			const CPathLink& link = *it;

			if( !implementation.AcceptLink( link ) )
			{
				continue;
			}

			CSearchNode* newNode = link.GetDestination();
			PathCost newNodeCurrentCost = currentCost + implementation.GetLinkCost( link );

			if ( !implementation.AcceptCost( newNodeCurrentCost ) )
			{
				continue;
			}
			// trashable code
			//if ( link.IsCustomLink() )
			//{
			//	//NavLinkCost linkCost;
			//	//CCustomLink* customLink = data.m_graph->GetCustomLink( link.GetCustomLinkIndex() );
			//	//if ( !customLink->IsAccessible( data, linkCost ) )
			//	//{
			//	//	continue;
			//	//}
			//	//newNode = customLink->GetNext( data.m_graph, currentNode );
			//}

			// check if the node was visited b4
			if( newNode->m_markerVisited == m_marker )
			{
				if( newNode->GetCurrentCost() <= newNodeCurrentCost)
				{
					continue;
				}

				// should be a relatively rare case
				
				newNode->Update( newNodeCurrentCost, currentNode );

				if ( newNode->GetHeapIndex() != CSearchNode::INVALID_HEAP_INDEX )
				{
					m_activeNodes.Update( newNode );
				}
				else
				{
					// should never happen as long as heuristic function is valid
					m_activeNodes.Push( newNode );
				}
			}
			else
			{
				PathCost newNodeHeuristicCost = implementation.NodeHeuristicCost( newNode );
				// we got new visited node
				newNode->m_markerVisited = m_marker;
				newNode->Set( newNodeCurrentCost, newNodeHeuristicCost, currentNode );
				m_activeNodes.Push( newNode );
			}
		}
	}
	while( !m_activeNodes.Empty() );

	return false;
}

Bool CSearchEngine::FindPath( CQueryData& data )
{
	struct Implementation : public Red::System::NonCopyable
	{
		Implementation( CSearchEngine& me, CQueryData& data )
			: m_me( me )
			, m_data( data )
			, m_destinationPosition( data.m_destination->GetPosition() )
		{}
		RED_INLINE void Initialize()
		{
			m_marker = m_me.m_marker;
			m_data.m_destination->m_markerDestination = m_marker;
			m_me.InitializeAstarStartingNode( m_data.m_start );
		}
		RED_INLINE Bool AcceptLink( const CPathLink& link )
		{
			return m_data.CheckFlags( link.GetFlags() );
		}
		RED_INLINE Bool AcceptCost( PathCost cost )
		{
			return true;
		}
		RED_INLINE void StartNode( CSearchNode* currentNode )
		{

		}
		RED_INLINE Bool CheckSuccess( const CSearchNode* currentNode )
		{
			return currentNode->m_markerDestination == m_marker;
		}

		RED_INLINE PathCost GetLinkCost( const CPathLink& link )
		{
			return link.GetCost();
		}
		RED_INLINE PathCost NodeHeuristicCost( CSearchNode* node )
		{
			return CalculatePathCost( (node->GetPosition() - m_destinationPosition).Mag() );
		}

		void OnSuccess( CSearchNode* currentNode )
		{
			CSearchNode* path[ 4096 ];
			Uint32 pathLength = 1;
			path[ 0 ] = currentNode;
			while( (currentNode = currentNode->GetPreviousNode()) != NULL )
			{
				path[ pathLength++ ] = currentNode;
			}
			m_data.m_path.ResizeFast( pathLength );
			for( Uint32 i=0; i<pathLength; i++ )
			{
				const CPathNode *p = path[ pathLength - i - 1 ]->AsPathNode();
				m_data.m_path[ i ] = p;
			}
		}

		CSearchEngine&		m_me;
		CQueryData&			m_data;
		Uint16				m_marker;
		const Vector3		m_destinationPosition;
	};

#ifdef PATHLIB_SUPPORT_PATH_RATER
	struct PathRaterImplementation : public Implementation
	{
		typedef Implementation Super;
		PathRaterImplementation( CSearchEngine& me, CQueryData& data )
			: Implementation( me, data )
			, m_pathRater( *data.m_pathRater )
		{}
		RED_INLINE void StartNode( CSearchNode* currentNode )
		{
			m_currentNodePos = currentNode->GetPosition();
		}
		RED_INLINE void Initialize()
		{
			Super::Initialize();

			m_pathRater.PathFindingStarted();
		}
		RED_INLINE PathCost GetLinkCost( const CPathLink& link ) const
		{
			return m_pathRater.CountRealRate( m_currentNodePos, &link );
		}
		RED_INLINE PathCost NodeHeuristicCost( CSearchNode* node )
		{
			return m_pathRater.CountHeurusticRate( node->GetPosition(), m_destinationPosition );
		}
		IPathRater&			m_pathRater;
		Vector3				m_currentNodePos;
	};
#endif

	PC_SCOPE( AI_FindPath );

	++m_marker;

#ifdef PATHLIB_SUPPORT_PATH_RATER
	if ( !data.m_pathRater )
	{
		Implementation implementation( *this, data );
		return TAstar( data, implementation );
	}
	else
	{
		PathRaterImplementation implementation( *this, data );
		return TAstar( data, implementation );
	}
#else
	Implementation implementation( *this, data );
	return TAstar( data, implementation );
#endif
}

Bool CSearchEngine::FindPathToClosestSpot( CFindClosestSpotQueryData& data )
{
	struct Implementation : public Red::System::NonCopyable
	{
		Implementation( CSearchEngine& me, CFindClosestSpotQueryData& data )
			: m_me( me )
			, m_data( data )
			, m_bestSoFar( nullptr )
			, m_lookForConnectors( data.m_lookForConnectors )
			, m_destinationPosition( data.m_destinationPosition )
		{}
		RED_INLINE void Initialize()
		{
			// store marker
			m_marker = m_me.m_marker;
			// compute heuristic cost of start point
			Float distanceLimit = Min( m_data.m_distanceLimit, ( m_destinationPosition - m_data.m_start->AsPathNode()->GetPosition() ).Mag() );
			PathCost heuristicCost = CalculatePathCost( distanceLimit );
			m_bestSoFarCost = heuristicCost;

			// setup starting node
			m_data.m_start->Set( 0, heuristicCost );
			m_data.m_start->m_markerVisited = m_marker;
			m_me.m_activeNodes.Push( m_data.m_start );
		}
		RED_INLINE Bool AcceptLink( const CPathLink& link )
		{
			return m_data.CheckFlags( link.GetFlags() );
		}
		RED_INLINE Bool AcceptCost( PathCost cost )
		{
			return true;
		}
		RED_INLINE void StartNode( CSearchNode* currentNode )
		{
			PathCost heuristicCost = currentNode->GetHeuristicCost();
			if ( heuristicCost < m_bestSoFarCost )
			{
				m_bestSoFarCost = heuristicCost;
				m_bestSoFar = currentNode;
			}
		}
		RED_INLINE Bool CheckSuccess( CSearchNode* currentNode )
		{
			if ( m_lookForConnectors )
			{
				const CPathNode* pathNode = currentNode->AsPathNode();
				if ( pathNode->HaveAnyFlag( NF_CONNECTOR ) )
				{
					Bool hasLink = false;

					for ( ConstLinksIterator it( *pathNode ); it; ++it )
					{
						if ( it->HaveAnyFlag( NF_CONNECTOR ) )
						{
							hasLink = true;
							break;
						}
					}

					// connector to not-yet streamed area
					if ( !hasLink )
					{
						m_bestSoFar = currentNode;
						m_bestSoFarCost = 0;
						return true;
					}
				}
			}
			
			return false;
		}

		RED_INLINE PathCost GetLinkCost( const CPathLink& link )
		{
			return link.GetCost();
		}
		RED_INLINE PathCost NodeHeuristicCost( CSearchNode* node )
		{
			return CalculatePathCost( (node->GetPosition() - m_destinationPosition).Mag() );
		}

		void OnSuccess( CSearchNode* currentNode )
		{
			m_bestSoFar = currentNode;
			
		}

		CSearchEngine&						m_me;
		CFindClosestSpotQueryData&			m_data;
		CSearchNode*						m_bestSoFar;
		Bool								m_lookForConnectors;
		Uint16								m_marker;
		PathCost							m_bestSoFarCost;
		const Vector3						m_destinationPosition;
	};

	PC_SCOPE( AI_FindPathToClosestSpot );

	++m_marker;
	Implementation implementation( *this, data );
	TAstar( data, implementation );

	if ( !implementation.m_bestSoFar )
	{
		return false;
	}

	CSearchNode* currentNode = implementation.m_bestSoFar;
	CSearchNode* path[ 4096 ];
	Uint32 pathLength = 1;
	path[ 0 ] = currentNode;
	while( ( currentNode = currentNode->GetPreviousNode() ) != NULL )
	{
		path[ pathLength++ ] = currentNode;
	}
	data.m_path.ResizeFast( pathLength );
	for( Uint32 i=0; i<pathLength; i++ )
	{
		const CPathNode* p = path[ pathLength - i - 1 ]->AsPathNode();
		data.m_path[ i ] = p;
	}

	return true;
}

Uint16 CSearchEngine::QueryMultipleDestinations( CMultiQueryData& data )
{
	struct Implementation : public Red::System::NonCopyable
	{
		Implementation( CSearchEngine& me, CMultiQueryData& data )
			: m_me( me )
			, m_data( data )
		{
			
		}

		RED_INLINE void Initialize()
		{
			m_marker = m_me.m_marker;

			m_me.InitializeAstarStartingNode( m_data.m_start );
			for ( auto it = m_data.m_originsList.Begin(), end = m_data.m_originsList.End(); it != end; ++it )
			{
				CSearchNode* node = *it;
				m_me.InitializeAstarStartingNode( node );
			}

			m_centerOfMass.Set( 0.f, 0.f, 0.f );
			Float destRatio = 1.f / Float( m_data.m_destinationList.Size() );
			for ( auto it = m_data.m_destinationList.Begin(), end = m_data.m_destinationList.End(); it != end; ++it )
			{
				CSearchNode* node = *it;
				node->m_markerDestination = m_marker;
				m_centerOfMass += node->GetPosition() * destRatio;
			}
			m_nodesToFind = m_data.m_destinationList.Size();
		}

		RED_INLINE Bool AcceptLink( const CPathLink& link )
		{
			return m_data.CheckFlags( link.GetFlags() );
		}
		RED_INLINE Bool AcceptCost( PathCost cost )
		{
			return cost <= m_data.m_searchLimit;
		}
		RED_INLINE void StartNode( CSearchNode* currentNode )
		{

		}
		RED_INLINE Bool CheckSuccess( const CSearchNode* currentNode )
		{
			if ( currentNode->m_markerDestination == m_marker )
			{
				if ( m_data.m_breakIfAnyNodeIsFound )
				{
					return true;
				}
				if ( --m_nodesToFind == 0 )
				{
					return true;
				}
				ComputeCenterOfMass();
			}
			return false;
		}

		RED_INLINE void OnSuccess( const CSearchNode* currentNode )
		{

		}

		RED_INLINE PathCost GetLinkCost( const CPathLink& link )
		{
			return link.GetCost();
		}
		RED_INLINE PathCost NodeHeuristicCost( CSearchNode* node )
		{
			const Vector3& pos = node->GetPosition();
			Float xDiff = pos.X - m_centerOfMass.X;
			Float yDiff = pos.Y - m_centerOfMass.Y;
			return CalculatePathCost( Max( xDiff, yDiff ) );
		}

		Bool ComputeCenterOfMass()
		{
			m_centerOfMass.Set( 0.f, 0.f, 0.f );
			Float nodesCount = 0.f;
			for ( auto it = m_data.m_destinationList.Begin(), end = m_data.m_destinationList.End(); it != end; ++it )
			{
				CSearchNode* node = *it;
				if ( node->m_markerVisited != m_marker )
				{
					Float newNodesCount = nodesCount + 1;
					m_centerOfMass *= nodesCount / newNodesCount;
					m_centerOfMass += node->GetPosition() * ( 1.f / newNodesCount );
					nodesCount = newNodesCount;
				}
			}
			return nodesCount > 0.f;
		}



		CSearchEngine&		m_me;
		CMultiQueryData&	m_data;
		Uint16				m_marker;
		Int16				m_nodesToFind;
		Vector3				m_centerOfMass;
		
	};

	PC_SCOPE( AI_QueryMultipleDestinations );

	++m_marker;

	Implementation implementation( *this, data );
	TAstar( data, implementation );
	return m_marker;
}

// Its much simpler logic than normal astar, so we do implementation 'by-hand'
void CSearchEngine::RegionFloodFill( CRegionFloodFillData& data, CNavGraph* navgraph, CHLNode* hlNode, Bool internalDontModifyMarker )
{
	if ( !internalDontModifyMarker )
	{
		++m_marker;
	}

	CSearchNode* startNode = data.m_start;
	AreaRegionId regionColor = data.m_regionColor;

	hlNode->PreNodesCollection();

	startNode->ModifyNavNode()->SetRegionId( *navgraph, *hlNode );
	m_processingList.PushBack( startNode );
	startNode->SetVisited( m_marker );
	
	// Process wide search
	while( !m_processingList.Empty() )
	{
		CSearchNode* currentNode = m_processingList.PopBackFast();

		for ( ConstLinksIterator it( *currentNode->AsPathNode() ); it; ++it )
		{
			const CPathLink& link = *it;

			if( !data.CheckFlags( link.GetFlags() ) )
			{
				continue;
			}

			// Check if node was visited b4
			CNavNode* newNode = static_cast< CNavNode* >( link.GetDestination() );
			if( newNode->m_markerVisited != m_marker )
			{
				newNode->SetRegionId( *navgraph, *hlNode );
				m_processingList.PushBack( newNode );
				newNode->SetVisited( m_marker );
			}
		}
	}
}

void CSearchEngine::ComputeRegionConnection( CRegionFloodFillData& data, CNavGraph* navgraph, CHLNode* hlNode )
{
	++m_marker;

	CHLSubGraph& hlGraph = navgraph->GetHLGraph();

	CSearchNode* startNode = data.m_start;
	AreaId currArea = navgraph->GetArea()->GetId();
	AreaRegionId regionColor = data.m_regionColor;
	NodeFlags forbiddenFlags = data.m_flagsForbidden & (~NF_CONNECTOR);

	// reset node links, now we will start to redo them
	startNode->SetVisited( m_marker );
	m_processingList.PushBack( startNode );

	// Process wide search
	while( !m_processingList.Empty() )
	{
		CSearchNode* currentNode = m_processingList.PopBackFast();

		for ( ConstLinksIterator it( *currentNode->AsPathNode() ); it; ++it )
		{
			const CPathLink& link = *it;

			// forbidden flags test
			if ( link.HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
			{
				continue;
			}

			CNavNode* newNode = static_cast< CNavNode* >( link.GetDestination() );

			if ( newNode->GetAreaRegionId() != regionColor || newNode->GetAreaId() != currArea )
			{
				hlGraph.ConnectRegions( regionColor, newNode->GetAreaId(), newNode->GetAreaRegionId(), link.GetFlags() );
			}
			else
			{
				// required flags test
				if ( ( (link.GetFlags() & data.m_requiredFlags) != data.m_requiredFlags ) )
				{
					continue;
				}
				if ( link.HaveAnyFlag( forbiddenFlags ) )
				{
					continue;
				}

				// Check if node was visited b4
				if( newNode->m_markerVisited != m_marker )
				{
					m_processingList.PushBack( newNode );
					newNode->SetVisited( m_marker );
				}
			}
		}
	}
	hlNode->MarkLinkageValid();
}

Bool CSearchEngine::RegionsUpdate( CRegionUpdateData& data )
{
	struct Implementation : public Red::System::NonCopyable
	{
		CSearchEngine&		m_me;
		NodeFlags			m_forbiddenFlags;
		NodeFlags			m_requiredFlags;
		AreaRegionId		m_regionId;
		Vector3				m_centerOfMass;
		CNavGraph&			m_navgraph;
		CHLNode&			m_hlNode;
		Uint32&				m_nodesToProcessLeft;
		CNavNode*			m_startingNode;

		Implementation( CSearchEngine& me, NodeFlags forbiddenFlags, NodeFlags requiredFlags, AreaRegionId regionId, Vector3 centerOfMass, CNavGraph& navgraph, CHLNode& hlNode, Uint32& nodesToProcessLeft, CNavNode* startingNode )
			: m_me( me )
			, m_forbiddenFlags( forbiddenFlags )
			, m_requiredFlags( requiredFlags )
			, m_regionId( regionId )
			, m_centerOfMass( centerOfMass )
			, m_navgraph( navgraph )
			, m_hlNode( hlNode )
			, m_nodesToProcessLeft( nodesToProcessLeft )
			, m_startingNode( startingNode )
		{}
		RED_INLINE void Initialize()
		{
			m_me.InitializeAstarStartingNode( m_startingNode );
		}
		RED_INLINE Bool AcceptLink( const CPathLink& link )
		{
			return !link.HaveAnyFlag( m_forbiddenFlags ) && ( link.GetFlags() & m_requiredFlags ) == m_requiredFlags;
		}
		RED_INLINE Bool AcceptCost( PathCost cost )
		{
			return true;
		}
		RED_INLINE void StartNode( CSearchNode* currentNode )
		{
			CNavNode* navnode = currentNode->ModifyNavNode();
			navnode->SetRegionId( m_navgraph, m_hlNode );
			// never visit again
			currentNode->m_currentCost = 0;

		}
		RED_INLINE Bool CheckSuccess( const CSearchNode* currentNode )
		{
			if  ( currentNode->m_markerDestination == m_me.m_marker )
			{
				ASSERT( m_nodesToProcessLeft > 0 );
				if ( (--m_nodesToProcessLeft) == 0 )
				{
					return true;
				}
				
			}
			return false;
		}

		RED_INLINE PathCost GetLinkCost( const CPathLink& link )
		{
			return link.GetCost();
		}
		RED_INLINE PathCost NodeHeuristicCost( CSearchNode* node )
		{
			return CalculatePathCost( (node->GetPosition() - m_centerOfMass).Mag() );
		}

		void OnSuccess( CSearchNode* currentNode )
		{
			
		}
	};

	++m_marker;

	CNavGraph& navgraph = data.m_graph;

	Vector3 centerOfMass( 0.f, 0.f, 0.f );
	Float nodesCount = 0.f;
	for ( auto it = data.m_nodeList.Begin(), end = data.m_nodeList.End(); it != end; ++it )
	{
		CNavNode* node = *it;

		// ignore unaccessible nodes
		if ( node->HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
		{
			node->SetRegionId( navgraph, INVALID_AREA_REGION );
			continue;
		}

		node->m_markerDestination = m_marker;
		
		nodesCount += 1.f;
		Float ratio = 1.f / nodesCount;

		centerOfMass *= 1.f - ratio;
		centerOfMass += node->GetPosition() * ratio;
	}

	Uint32 nodesToProcessLeft = Uint32( nodesCount );

	CHLSubGraph& hlGraph = navgraph.GetHLGraph();

	//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );

	AreaRegionId prevRegion = INVALID_AREA_REGION;

	for ( auto it = data.m_nodeList.Begin(), end = data.m_nodeList.End(); it != end; ++it )
	{
		CNavNode* node = *it;

		// check if node was already updated - in this case it already has proper region assigned
		if ( node->m_markerVisited == m_marker )
		{
			continue;
		}

		// ignore unaccessible nodes
		if ( node->HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
		{
			continue;
		}

		NodeFlags specialFlags = node->GetFlags() & NFG_BREAKS_COHERENT_REGION;
		NodeFlags forbiddenFlags = ( NFG_BREAKS_COHERENT_REGION & (~specialFlags) ) | NF_CONNECTOR;


		// see current region id
		AreaRegionId currRegionId = node->GetAreaRegionId();
		// check if current region was already updated
		CHLNode* hlNode = nullptr;
		if ( currRegionId == prevRegion || currRegionId == INVALID_AREA_REGION )
		{
			currRegionId = hlGraph.GetNextUniqueRegionId();
			hlNode = &hlGraph.AddHLNode( currRegionId );

			//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );

			// here we are creating a new region. So this time we need to run a flood fill algorithm to mark it completely
			CRegionFloodFillData floodFillData( node, &navgraph, currRegionId, forbiddenFlags, specialFlags );
			RegionFloodFill( floodFillData, &navgraph, hlNode, true );

			//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );
		}
		else
		{
			hlNode = hlGraph.FindHLNode( currRegionId );
			// in theory its sanity check, shouldn't be the case. But some deprecated data might lead to this issue probably.
			if ( !hlNode )
			{
				hlNode = &hlGraph.AddHLNode( currRegionId );
			}
			prevRegion = currRegionId;

			//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );

			// in that situation we are trying to update existing area region. Lets do Astar as we might have pretty results if graph doesn't break consistency
			CRegionFloodFillData searchData( node, &navgraph, currRegionId, forbiddenFlags, specialFlags );

			Implementation implementation( *this, forbiddenFlags, specialFlags, currRegionId, centerOfMass, navgraph, *hlNode, nodesToProcessLeft, node );

			TAstar( searchData, implementation );

			//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );
		}

		if ( hlNode )
		{
			hlNode->InvalidateLinkage();
			// we unlink node
			hlNode->Unlink();

			//PATHLIB_ASSERT( hlGraph.Debug_HLCheckAllLinksTwoSided() );
		}
		

		if ( nodesToProcessLeft == 0 )
		{
			break;
		}
	}
	return true;
}

CSearchNode::Marking CSearchEngine::SpreadMarking( CBrowseData& data )
{
	// TODO: There is lots of common code with flood fill (as it is same algorithm). It should be templated and refactorized.
	CSearchNode::Marking marking( ++m_marker );

	CSearchNode* startNode = data.m_start;

	m_processingList.PushBack( startNode );
	startNode->m_markerVisited = m_marker;

	// Process wide search
	while( !m_processingList.Empty() )
	{
		CSearchNode* currentNode = m_processingList.PopBackFast();

		for ( ConstLinksIterator it( *currentNode->AsPathNode() ); it; ++it )
		{
			const CPathLink& link = *it;
			
			if( !data.CheckFlags( link.GetFlags() ) )
			{
				continue;
			}

			// Check if node was visited b4
			CNavNode* newNode = static_cast< CNavNode* >( link.GetDestination() );
			if( newNode->m_markerVisited != m_marker )
			{
				newNode->m_markerVisited = m_marker;
				m_processingList.PushBack( newNode );
			}
		}
	}
	return marking;
}

Float CSearchEngine::ComputeOptimizedPathLength( CPathLibWorld& pathlib, const Vector3& destination, const Vector3& origin, CSearchNode* destinationNode, Float personalSpace, CollisionFlags colFlags )
{
	Float accumulator = 0.f;

	const CPathNode* pathNode = destinationNode->AsPathNode();

	Vector3 lastPos = destination;
	CAreaDescription* lastArea = pathlib.GetAreaDescription( pathNode->GetAreaId() );
	PathLib::AreaId cachedArea = pathNode->GetAreaId();

	
	// find furthest reachable pathNode
	while ( true )
	{
		CSearchNode* prevSearchNode = pathNode->GetPreviousNode();
		// check if we are at the origin
		if ( prevSearchNode == nullptr )
		{
			// last node is accessible from last position
			if ( pathlib.TestLine( cachedArea, origin, lastPos, personalSpace, colFlags ) )
			{
				accumulator += ( origin - lastPos ).Mag();
			}
			else
			{
				const Vector3& pathNodePos = pathNode->GetPosition();
				NavUtils::SBinSearchContext context( lastArea, lastPos, pathNodePos, origin, personalSpace, 1.f, colFlags );
				Vector3 midPoint = ::FunctionalBinarySearch( pathNodePos, origin, context );

				accumulator += ( midPoint - lastPos ).Mag();
				accumulator += ( midPoint - origin ).Mag();
			}

			break;
		}

		const CPathNode* nextNode = prevSearchNode->AsPathNode();

		CWideLineQueryData::MultiArea query( CWideLineQueryData( colFlags, lastPos, nextNode->GetPosition(), personalSpace ) );
		if ( !lastArea->TMultiAreaQuery( query ) )
		{
			const Vector3& pathNodePos = pathNode->GetPosition();
			const Vector3& nextNodePos = nextNode->GetPosition();
			NavUtils::SBinSearchContext context( lastArea, lastPos, pathNodePos, nextNodePos, personalSpace, 1.f, colFlags );
			Vector3 midPoint = ::FunctionalBinarySearch( pathNodePos, nextNodePos, context );
			accumulator +=  ( midPoint - lastPos ).Mag();
			lastPos = midPoint;
		}
		
		pathNode = nextNode;
	}

	return accumulator;
}

};		// namespace PathLib



