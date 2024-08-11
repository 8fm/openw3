/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibObstacleDetour.h"

#include "pathlibAreaDescription.h"
#include "pathlibNavNode.h"
#include "pathlibWorld.h"


namespace PathLib
{

namespace DetourComputationAlgorithm
{
	void GenerateDetour( CAreaDescription* areaDescription, CNavNodesGraphBase* sourceGraph, CNavNodesGraphBase* targetGraph, Uint32 category, const CObstaclesDetourInfo& detourInfo )
	{
		// Sweep & prune algorithm types and subprocedures declaration

		static const Uint32 pruneDimm = 0;
		static const Uint32 searchDimm = 1;
		const Float maxNodesDistance = areaDescription->GetMaxNodesDistance();
		const Float maxNodesDistanceSq = maxNodesDistance * maxNodesDistance;

		enum ENodeType
		{
			BASE_NODE,
			DETOUR_NODE
		};
		struct CPruneEvent
		{
			Float					m_val;
			enum EType
			{
				PRUNE_ADD,
				PRUNE_REM
			}						m_eventType : 8;
			ENodeType				m_nodeType : 8;
			Uint16					m_detourNodeIdx;
			const CNavNode*			m_node;								// NOTICE: Storing pointers means that we cannot create new nodes during the algorithm

			ENodeType				GetNodeType() const					{ return m_nodeType; }
			Bool operator<( const CPruneEvent& e ) const
			{
				return
					m_val < e.m_val ? true :
					m_val > e.m_val ? false :
					m_node < e.m_node;
			}
			Bool operator>( const CPruneEvent& e ) const
			{
				return
					m_val > e.m_val ? true :
					m_val < e.m_val ? false :
					m_node > e.m_node;
			}
		};
		struct CActiveNode
		{
			Float					m_val;
			Uint16					m_detourNodeIdx;
			ENodeType				m_nodeType;
			const CNavNode*			m_node;

			CActiveNode()												{}
			CActiveNode( const CNavNode* navNode, ENodeType nodeType, Uint16 detourNodeIdx = 0xffff )
				: m_val( navNode->GetPosition().A[ searchDimm ] )
				, m_detourNodeIdx( detourNodeIdx )
				, m_nodeType( nodeType )
				, m_node( navNode )										{}

			ENodeType		GetNodeType() const				{ return m_nodeType; }
			Bool operator<( const CActiveNode& e ) const
			{
				return
					m_val < e.m_val ? true :
					m_val > e.m_val ? false :
					m_node < e.m_node;
			}
			Bool operator>( const CActiveNode& e ) const
			{
				return
					m_val > e.m_val ? true :
					m_val < e.m_val ? false :
					m_node > e.m_node;
			}
		};
		struct CDetourNodeInfo
		{
			enum EClosestNode
			{
				CLOSEST_BASE,
				CLOSEST_DETOUR,
				CLOSEST_COUNT
			};

			CDetourNodeInfo()											{}
			CDetourNodeInfo( const CNavNode* node )
				: m_node( node->GetIndex() )							{}
			CDetourNodeInfo( const CNavNode* node, Uint32 shapeIdx, Float maxNodesDistanceSq )
				: m_node( node->GetIndex() )
				, m_detourShapeIdx( shapeIdx )
			{
				for ( Uint32 i = 0; i < CLOSEST_COUNT; ++i )
				{
					m_closestNode[ i ] = CNavNode::INVALID_INDEX;
					m_closestNodeDistSq[ i ] = maxNodesDistanceSq;
				}
			}

			Bool operator<( const CDetourNodeInfo& e ) const			{ return m_node < e.m_node; }

			CNavNode::Index			m_node;
			CNavNode::Index			m_closestNode[ CLOSEST_COUNT ];
			Float					m_closestNodeDistSq[ CLOSEST_COUNT ];
			Uint32					m_detourShapeIdx;
		};
		struct AlgorithmData
		{
			typedef TSortedArray< CActiveNode > ActiveNodesList;

			AlgorithmData( CNavNodesGraphBase* navgraph, CAreaDescription* area, Float personalSpace )
				: m_this( navgraph )
				, m_area( area )
				, m_personalSpace( personalSpace )
			{}

			void FindClosestAccessibleNode( CActiveNode& activeNode, const ActiveNodesList& nodesList, CDetourNodeInfo& info, CDetourNodeInfo::EClosestNode node2Set )
			{
				if ( node2Set > 0 )
				{
					info.m_closestNodeDistSq[ node2Set ] = info.m_closestNodeDistSq[ node2Set-1 ];
				}
				auto itClosestL = ::LowerBoundIndex( nodesList.Begin(), nodesList.End(), activeNode );
				if ( itClosestL != nodesList.End() )
				{
					auto itClosestH = itClosestL+1;

					Vector3 nodePosition = activeNode.m_node->GetPosition();

					while ( true ) 
					{
						Float distL1D = FLT_MAX;
						Float distH1D = FLT_MAX;

						if ( itClosestL >= nodesList.Begin() )
						{
							distL1D = Abs(activeNode.m_val - itClosestL->m_val);
						}
						if ( itClosestH != nodesList.End() )
						{
							distH1D = Abs(itClosestH->m_val - activeNode.m_val);
						}
						Bool goH = distL1D > distH1D;
						Float dist1D = distL1D > distH1D ? distH1D : distL1D;
						if ( dist1D*dist1D >= info.m_closestNodeDistSq[ node2Set ] )
						{
							break;
						}

						const CActiveNode& testNode = goH ? *(itClosestH++) : *(itClosestL--);
						const Vector3& testPosition = testNode.m_node->GetPosition();
						Float distSq = (testNode.m_node->GetPosition().AsVector2() - nodePosition.AsVector2()).SquareMag();
						if ( distSq < info.m_closestNodeDistSq[ node2Set ] )
						{
							if ( testNode.m_detourNodeIdx == activeNode.m_detourNodeIdx )			// replaces also if ( testNode.m_node->IsConnected( *activeNode.m_node ) )
							{
								continue;
							}

							CWideLineQueryData queryData( CT_IGNORE_OTHER_AREAS, nodePosition, testPosition, m_personalSpace );
							if ( m_area->VSpatialQuery( queryData ) )
							{
								info.m_closestNodeDistSq[ node2Set ] = distSq;
								info.m_closestNode[ node2Set ] = testNode.m_node->GetIndex();
							}
						}
					}
				}
			}

			void IterateEvents( TDynArray< CPruneEvent >& events, TDynArray< CDetourNodeInfo >& detourNodesInfo, Float personalSpace )
			{
				// process events loop
				for ( Uint32 eventId = 0, eventsCount = events.Size(); eventId < eventsCount; ++eventId )
				{
					const CPruneEvent& e = events[ eventId ];
					if ( e.m_eventType == CPruneEvent::PRUNE_ADD )
					{
						CActiveNode activeNode( e.m_node, e.m_nodeType, e.m_detourNodeIdx );
						if ( e.GetNodeType() == DETOUR_NODE )
						{
							CDetourNodeInfo& info = detourNodesInfo[ e.m_detourNodeIdx ];

							// Find closest connection
							FindClosestAccessibleNode( activeNode, m_activeBaseNodes, info, CDetourNodeInfo::CLOSEST_BASE );

							FindClosestAccessibleNode( activeNode, m_activeDetourNodes, info, CDetourNodeInfo::CLOSEST_DETOUR );

							m_activeDetourNodes.Insert( activeNode );
						}
						else
						{
							m_activeBaseNodes.Insert( activeNode );
						}
					}
					else
					{
						CActiveNode activeNode( e.m_node, e.m_nodeType );
						auto& nodeList = (e.GetNodeType() == DETOUR_NODE) ? m_activeDetourNodes : m_activeBaseNodes;
						auto itFind = nodeList.Find( activeNode );
						ASSERT( itFind != nodeList.End() );
						nodeList.Erase( itFind );
					}
				}
				ASSERT( m_activeBaseNodes.Empty() );
				ASSERT( m_activeDetourNodes.Empty() );
			}

			CNavNodesGraphBase*						m_this;
			CAreaDescription*						m_area;
			ActiveNodesList							m_activeBaseNodes;
			ActiveNodesList							m_activeDetourNodes;
			Float									m_personalSpace;

		};

		// declare run time data
		CPathLibWorld&							pathlib	= areaDescription->GetPathLib();
		Float									personalSpace = pathlib.GetGlobalSettings().GetCategoryPersonalSpace( category );
		TDynArray< CPruneEvent >				events;
		TSortedArray< CDetourNodeInfo >			detourNodesInfo;
		AlgorithmData							algorithm( sourceGraph, areaDescription, personalSpace );


		// Cool. Now create base nodes & connections for obstacles detours
		Uint32 detourShapes = detourInfo.DetousShapesCount();
		for ( Uint32 shapeIdx = 0; shapeIdx < detourShapes; ++shapeIdx )
		{
			Uint32 shapeVerts = detourInfo.DetourShapeVertsCount( shapeIdx );
			// spawn detour nodes
			TStaticArray< CNavNode::Index, MAX_OBSTACLE_DETOUR_LEN > detourNodes;
			detourNodes.Resize( shapeVerts );
			for ( Uint32 vertId = 0; vertId < shapeVerts; ++ vertId )
			{
				const Vector3& currVert = detourInfo.DetourShapeVert( shapeIdx, vertId );
				CNavNode& detourNode = targetGraph->VAddNode( currVert, NF_DEFAULT | NF_IS_OBSTACLE_DETOUR_NODE );
				detourNodes[ vertId ] = detourNode.GetIndex();
				detourNodesInfo.PushBack( CDetourNodeInfo( &detourNode, shapeIdx, maxNodesDistanceSq ) );
			}

			// spawn detour links
			if ( shapeVerts == 2 )
			{
				const Vector3& prevVert = detourInfo.DetourShapeVert( shapeIdx, 0 );
				const Vector3& currVert = detourInfo.DetourShapeVert( shapeIdx, 1 );
				CWideLineQueryData query( CT_NO_ENDPOINT_TEST, currVert, prevVert, personalSpace );
				if ( areaDescription->VSpatialQuery( query ) )
				{
					CNavNode* detourNode0 = targetGraph->GetNode( detourNodes[ 0 ] );
					CNavNode* detourNode1 = targetGraph->GetNode( detourNodes[ 1 ] );
					targetGraph->VGenerationConnectNodes( *detourNode0, *detourNode1, NF_IS_OBSTACLE_DETOUR_NODE );
				}

			}
			else if ( shapeVerts > 1 )
			{
				Vector3 prevVert = detourInfo.DetourShapeVert( shapeIdx, shapeVerts-1 );
				CNavNode::Index prevIdx = detourNodes[ shapeVerts-1 ];
				for ( Uint32 vertId = 0; vertId < shapeVerts; ++ vertId )
				{
					const Vector3& currVert = detourInfo.DetourShapeVert( shapeIdx, vertId );
					CNavNode::Index currIdx = detourNodes[ vertId ];
					CWideLineQueryData query( CT_NO_ENDPOINT_TEST, currVert, prevVert, personalSpace );
					if ( areaDescription->VSpatialQuery( query ) )
					{
						CNavNode* detourNode0 = targetGraph->GetNode( currIdx );
						CNavNode* detourNode1 = targetGraph->GetNode( prevIdx );
						targetGraph->VGenerationConnectNodes( *detourNode0, *detourNode1, NF_IS_OBSTACLE_DETOUR_NODE );
					}
					prevIdx = currIdx;
					prevVert = currVert;
				}
			}
		}

		ASSERT( IsSorted( detourNodesInfo.Begin(), detourNodesInfo.End(), Less< CDetourNodeInfo >() ) );

		// True shit. Now we must connect detours with graph.
		// We use two-way sweep and prune algorithm to find closest node pairs.

		// reserve data
		Bool isSameGraph = ( sourceGraph == targetGraph );
		const CNavNodesGraphBase::NodesArray& sourceNodesArray = sourceGraph->GetNodesArray();
		const CNavNodesGraphBase::NodesArray& targetNodesArray = targetGraph->GetNodesArray();
		Uint32 eventsCount = isSameGraph
			? sourceNodesArray.Size() * 2
			: (sourceNodesArray.Size() + targetNodesArray.Size()) * 2;
		events.Reserve( eventsCount );
		detourNodesInfo.Reserve( detourInfo.VertsCount() );

		auto funProcessNode =
			[ isSameGraph, maxNodesDistance, &events, &detourNodesInfo ] ( const CNavNode& node, ENodeType nodeType )
		{
			if ( node.HaveAnyFlag( NF_MARKED_FOR_DELETION ) )
			{
				return;
			}
			CPruneEvent e;
			e.m_val = node.GetPosition().A[ pruneDimm ];
			e.m_eventType = CPruneEvent::PRUNE_ADD;
			e.m_nodeType = isSameGraph
				? ( node.HaveFlag( NF_IS_OBSTACLE_DETOUR_NODE ) ? DETOUR_NODE : BASE_NODE )
				: nodeType;
			e.m_node = &node;
			if ( e.GetNodeType() == DETOUR_NODE )
			{
				CDetourNodeInfo info( &node );
				auto itFind = detourNodesInfo.Find( info );
				ASSERT ( itFind != detourNodesInfo.End() );
				e.m_detourNodeIdx = static_cast< Uint16 >( itFind - detourNodesInfo.Begin() );
			}
			else
			{
				e.m_detourNodeIdx = 0xffff;
			}
			events.PushBack( e );
			e.m_val += maxNodesDistance;
			e.m_eventType = CPruneEvent::PRUNE_REM;
			events.PushBack( e );
		};

		// spawn events by iterating all nodes
		for ( const CNavNode& node : sourceNodesArray )
		{
			funProcessNode( node, BASE_NODE );
		}

		if ( !isSameGraph )
		{
			for ( const CNavNode& node : targetNodesArray )
			{
				funProcessNode( node, DETOUR_NODE );
			}
		}

		// sort events
		::Sort( events.Begin(), events.End(), Less< CPruneEvent >() );

		algorithm.IterateEvents( events, detourNodesInfo, personalSpace );

		// correct events (only positions)
		for( auto it = events.Begin(), end = events.End(); it != end; ++it )
		{
			CPruneEvent& e = *it;
			// removal events should be moved
			if ( e.m_eventType == CPruneEvent::PRUNE_REM )
			{
				e.m_val = e.m_node->GetPosition().A[ pruneDimm ] - maxNodesDistance;
			}
		}

		::Sort( events.Begin(), events.End(), Greater< CPruneEvent >() );

		algorithm.IterateEvents(  events, detourNodesInfo, personalSpace );

		// spawn connections

		struct CDetourBestConnection
		{
			CNavNode::Index	m_detourNode;
			Float			m_distSq;
		};

		THashMap< CNavNode::Id, CDetourBestConnection > bestConnection;
		for( auto it = detourNodesInfo.Begin(), end = detourNodesInfo.End(); it != end; )
		{
			// only one connection with each node from each detour
			Uint32 shapeIdx = it->m_detourShapeIdx;
			do
			{
				const CDetourNodeInfo& info = *it;
				for ( Uint32 i = 0; i < CDetourNodeInfo::CLOSEST_COUNT; ++i )
				{
					CNavNode::NodesetIdx nodeSetIdx =
						(i == CDetourNodeInfo::CLOSEST_BASE)
						? sourceGraph->VGetNodesetIndex()
						: targetGraph->VGetNodesetIndex();
					if ( info.m_closestNode[ i ] != CNavNode::INVALID_INDEX )
					{
						CNavNode::Id nodeId;
						nodeId.m_index = info.m_closestNode[ i ];
						nodeId.m_nodeSetIndex = nodeSetIdx;
						auto itFind = bestConnection.Find( nodeId );
						if ( itFind == bestConnection.End() )
						{
							CDetourBestConnection c;
							c.m_detourNode = info.m_node;
							c.m_distSq = info.m_closestNodeDistSq[ i ];
							bestConnection.Insert( nodeId, c );
						}
						else
						{
							if ( info.m_closestNodeDistSq[ i ] < itFind->m_second.m_distSq )
							{
								itFind->m_second.m_distSq = info.m_closestNodeDistSq[ i ];
								itFind->m_second.m_detourNode = info.m_node;
							}
						}
					}
				}

				++it;
			}
			while ( it != end && it->m_detourShapeIdx == shapeIdx );

			// connect shape detour with navgraph
			for( auto it = bestConnection.Begin(), end = bestConnection.End(); it != end; ++it )
			{
				CNavNode::Id baseNodeIdx = it->m_first;
				CNavNode::Index detourNodeIdx = it->m_second.m_detourNode;
				
				CNavNode* baseNode = static_cast< CNavNode* >( sourceGraph->VGetPathNode( baseNodeIdx ) );
				CNavNode* detourNode = targetGraph->GetNode( detourNodeIdx );

				if ( !baseNode->IsConnected( *detourNode ) )
				{
					targetGraph->VGenerationConnectNodes( *baseNode, *detourNode, NF_IS_OBSTACLE_DETOUR_NODE );
				}
			}

			bestConnection.ClearFast();
		}
	}
};




};