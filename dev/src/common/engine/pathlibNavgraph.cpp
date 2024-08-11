#include "build.h"
#include "pathlibNavgraph.h"

#include "../redSystem/crc.h"

#include "baseEngine.h"
#include "globalWater.h"
#include "pathlibAreaDescription.h"
#include "pathlibAreaProcessingJob.h"
#include "pathlibCookerData.h"
#include "pathlibConnectorsBin.h"
#include "pathlibHLGraph.h"
#include "pathlibNavmesh.h"
#include "pathlibNavmeshArea.h"
#include "pathlibNodeFinder.h"
#include "pathlibNodeSet.h"
#include "pathlibObstacleDetour.h"
#include "pathlibObstacleShape.h"
#include "pathlibObstaclesMap.h"
#include "pathlibNodeSetRequestsList.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibSpecialZoneMap.h"
#include "pathlibTerrain.h"
#include "pathlibWorld.h"

#include "pathlibGraphBase.inl"
#include "pathlibNavgraphHelper.inl"

#define F_SQRT2  (1.414213562373095f)


namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// CNavGraph::SConnectorData
////////////////////////////////////////////////////////////////////////////
void CNavGraph::SConnectorData::AreaData::Sort()
{
	struct Pred
	{
		Bool operator()( Connector& c0, Connector& c1 ) const
		{
			return c0.m_nodeIdx < c1.m_nodeIdx;
		}
	} pred;
	::Sort( m_connectors.Begin(), m_connectors.End(), pred );
}

////////////////////////////////////////////////////////////////////////////
// CNavGraph
////////////////////////////////////////////////////////////////////////////
void CNavGraph::ConnectNode( CNavNode& node1, CNavNode& node2, NodeFlags linkFlags, NavLinkCost linkCost )
{
	ASSERT( !node1.IsConnected( node2 ) && &node1 != &node2 );
	ASSERT( ( node1.HaveFlag( NF_IS_IN_NODESET ) || node2.HaveFlag( NF_IS_IN_NODESET ) ) == ((linkFlags & NF_IS_IN_NODESET) != 0) );

	AddLink( node2, CPathLink( &node1, linkFlags, linkCost ) );
	AddLink( node1, CPathLink( &node2, linkFlags, linkCost ) );
}
void CNavGraph::ConnectNode( CNavNode::Id nodeId1, CNavNode::Id nodeId2, NodeFlags linkFlags, NavLinkCost linkCost )
{
	CNavNode* node1 = GetNode( nodeId1 );
	CNavNode* node2 = GetNode( nodeId2 );

	ConnectNode( *node1, *node2, linkFlags, linkCost );
}

LinkBufferIndex CNavGraph::GenerationConnectNode( CNavNode::Index nodeId1, CNavNode::Index nodeId2, NodeFlags linkFlags, NavLinkCost linkCost )
{
	CNavNode* node1 = GetNode( nodeId1 );
	CNavNode* node2 = GetNode( nodeId2 );

	ASSERT( node1 != node2 && !node1->IsConnected( *node2 ) && !node1->HaveFlag( NF_MARKED_FOR_DELETION ) && !node2->HaveFlag( NF_MARKED_FOR_DELETION ) );

	AddLink( *node2, CPathLink( nodeId1, CNavNode::INVALID_INDEX, linkFlags, linkCost ) );
	return AddLink( *node1, CPathLink( nodeId2, CNavNode::INVALID_INDEX, linkFlags, linkCost ) );
}

void CNavGraph::GenerationMergeNodes( CNavNode& targetNode, CNavNode& sourceNode )
{
	for ( ConstLinksIterator it( sourceNode ); it; ++it )
	{
		const CPathLink& link = *it;
		if ( link.HaveAnyFlag( NF_MARKED_FOR_DELETION ) )
		{
			continue;
		}

		if ( link.GetDestinationId() == targetNode.GetFullId() )
		{
			continue;
		}
		if ( targetNode.IsConnected( link.GetDestinationId() ) )
		{
			continue;
		}
		LinkBufferIndex linkIndex = GenerationConnectNode( targetNode.GetIndex(), link.GetDestinationId().m_index );
		CPathLink& newLink = m_links.Get( linkIndex );
		CPathLinkModifier modifier( targetNode, newLink );
		modifier.SetFlags( link.GetFlags() );
	}
	MarkNodeForDeletion( sourceNode.GetIndex() );
}

Bool CNavGraph::InitialGraphGeneration( CPathLibWorld* world, CTerrainMap* terrain )
{
	enum eEdge
	{
		EDGE_TOP,
		EDGE_RIGHT,
		EDGE_BOTTOM,
		EDGE_LEFT,
		EDGES
	};
	static const Int16 EDGE_NORMALS[EDGES][2] =
	{
		{ 0, 1 },
		{ 1, 0 },
		{ 0, -1 },
		{ -1, 0 },
	};
	struct Coords
	{
		Coords() {}
		RED_INLINE Coords( Int32 x, Int32 y )
			: m_x( Int16(x) ), m_y( Int16(y) ) {}

		RED_INLINE void operator+=( eEdge edge )
		{
			m_x += EDGE_NORMALS[ edge ][ 0 ];
			m_y += EDGE_NORMALS[ edge ][ 1 ];
		}
		RED_INLINE Coords operator+( eEdge edge ) const
		{
			Coords c;
			c.m_x = m_x + EDGE_NORMALS[ edge ][ 0 ];
			c.m_y = m_y + EDGE_NORMALS[ edge ][ 1 ];
			return c;
		}

		RED_INLINE Bool CheckBoundings( Int32 tileResolution )
		{
			return m_x >= 0 && m_y >= 0 && m_x < tileResolution && m_y < tileResolution;
		}

		union
		{
			Int32			m_val;
			struct 
			{
				Int16		m_x;
				Int16		m_y;
			};
		};
		
	};

	// collect data
	CTerrainAreaDescription* area = m_areaDescription->AsTerrainArea();
	const CTerrainInfo* terrainInfo = terrain->GetTerrainInfo();
	Uint32 tileResolution = terrainInfo->GetTilesResolution();
	Int32 maxRegionSize = Int32( m_maxNodesDistance / terrainInfo->GetQuadSize() );
	// set helper 2d array of points
	TDynArray< CNavNode::Index > processedPoints( tileResolution * tileResolution );
	Red::System::MemorySet( processedPoints.Data(), 0xffffffff, processedPoints.DataSize() );
	// declare base algorithm variables
	const Uint32 maxNodes = 0x7fffffff;
	const CNavNode::Index MASK_NEIGHBOURTEST = 0x80000000;
	// and other helpers
	TDynArray< Coords > processingQuads;
	// neighbour connecting algorithm data
	TDynArray< SInitialNeighbourData > neigbours;
	for ( Uint32 y = 0; y < tileResolution; ++y )
	{
		for ( Uint32 x = 0; x < tileResolution; ++x )
		{
			if ( processedPoints[ y*tileResolution + x ] != CNavNode::INVALID_INDEX )
			{
				continue;
			}
			CTerrainMap::QuadIndex baseQuadIndex = terrain->GetQuadIndex( x, y );
			CTerrainMap::eQuadState quadState = terrain->GetQuadState( baseQuadIndex );
			if ( quadState != CTerrainMap::QUAD_FREE )
			{
				continue;
			}
			CNavNode::Index currentMarker = CNavNode::Index(m_nodes.Size());
			processedPoints[ y*tileResolution + x ] = currentMarker;
			// start region
			Bool successfullyExtendedRegion;
			Vector2 centralPoint;
				
			// This part of algorithm is pure heuristic.
			// We only allow one direction to be double in length of other
			Int32 regionWidth = 1;
			Int32 regionHeight = 1;

			// extend rectangular region
			{
				// discrete rectangle
				Int32 regionRectangleMinX = x;
				Int32 regionRectangleMaxX = x;
				Int32 regionRectangleMinY = y;
				Int32 regionRectangleMaxY = y;
				Bool edgeIsAvailable[ EDGES ];
				for ( Uint32 i = 0; i < EDGES; ++i )
					edgeIsAvailable[ i ] = true;

				do 
				{
					auto funCheckPosition =
						[ this, terrain, &processedPoints, tileResolution ] ( Int32 x, Int32 y ) -> Bool
						{
							if ( processedPoints[ y * tileResolution + x ] != CNavNode::INVALID_INDEX )
								return false;
							CTerrainMap::eQuadState pointState = terrain->GetQuadState( terrain->GetQuadIndex( x, y ) );
							return pointState == CTerrainMap::QUAD_FREE;
						};

					successfullyExtendedRegion = false;
					
					if ( edgeIsAvailable[EDGE_RIGHT] && regionWidth <= regionHeight << 1 && regionWidth < maxRegionSize )
					{
						if ( regionRectangleMaxX+1 >= Int32(tileResolution) )
						{
							edgeIsAvailable[EDGE_RIGHT] = false;
						}
						else
						{
							for( Int32 y = regionRectangleMinY; y <= regionRectangleMaxY; ++y )
							{
								if ( !funCheckPosition( regionRectangleMaxX + 1, y ) )
								{
									edgeIsAvailable[EDGE_RIGHT] = false;
									break;
								}
							}
							if ( edgeIsAvailable[EDGE_RIGHT] )
							{
								++regionRectangleMaxX;
								++regionWidth;
								successfullyExtendedRegion = true;
							}
						}
					}
					if ( edgeIsAvailable[EDGE_BOTTOM] && regionHeight <= regionWidth << 1 && regionHeight < maxRegionSize )
					{
						if ( regionRectangleMinY <= 0 )
						{
							edgeIsAvailable[EDGE_BOTTOM] = false;
						}
						else
						{
							for( Int32 x = regionRectangleMinX; x <= regionRectangleMaxX; ++x )
							{
								if ( !funCheckPosition( x, regionRectangleMinY - 1 ) )
								{
									edgeIsAvailable[EDGE_BOTTOM] = false;
									break;
								}
							}
							if ( edgeIsAvailable[EDGE_BOTTOM] )
							{
								--regionRectangleMinY;
								++regionHeight;
								successfullyExtendedRegion = true;
							}
						}
					}
					if ( edgeIsAvailable[EDGE_LEFT] && regionWidth <= regionHeight << 1 && regionWidth < maxRegionSize )
					{
						if ( regionRectangleMinX <= 0 )
						{
							edgeIsAvailable[EDGE_LEFT] = false;
						}
						else
						{
							for( Int32 y = regionRectangleMinY; y <= regionRectangleMaxY; ++y )
							{
								if ( !funCheckPosition( regionRectangleMinX - 1, y ) )
								{
									edgeIsAvailable[EDGE_LEFT] = false;
									break;
								}
							}
							if ( edgeIsAvailable[EDGE_LEFT] )
							{
								--regionRectangleMinX;
								++regionWidth;
								successfullyExtendedRegion = true;
							}
						}
					}
					if ( edgeIsAvailable[EDGE_TOP] && regionHeight <= regionWidth << 1 && regionHeight < maxRegionSize  )
					{
						if ( regionRectangleMaxY+1 >= Int32(tileResolution) )
						{
							edgeIsAvailable[EDGE_TOP] = false;
						}
						else
						{
							for( Int32 x = regionRectangleMinX; x <= regionRectangleMaxX; ++x )
							{
								if ( !funCheckPosition( x, regionRectangleMaxY + 1 ) )
								{
									edgeIsAvailable[EDGE_TOP] = false;
									break;
								}
							}
							if ( edgeIsAvailable[EDGE_TOP] )
							{
								++regionRectangleMaxY;
								++regionHeight;
								successfullyExtendedRegion = true;
							}
						}
					}

					

				} while (successfullyExtendedRegion);

				// mark computed rectangle
				centralPoint =
					( terrain->GetQuadCenterLocal(regionRectangleMinX,regionRectangleMinY)
					+ terrain->GetQuadCenterLocal(regionRectangleMaxX,regionRectangleMaxY) )
					* 0.5f;

				for ( Int32 y = regionRectangleMinY; y <= regionRectangleMaxY; ++y )
				{
					for ( Int32 x = regionRectangleMinX; x <= regionRectangleMaxX; ++x )
					{
						processedPoints[ y*tileResolution + x ] = currentMarker;
					}
				}

				// ehhhh... I cannot attach funMove pointer as argument without linking to std::function, and I would like to do this shit other way around
				auto funCheckExtendRegion = [&] ( Int32 x, Int32 y ) -> Bool
				{
					Uint32 processedPointsIdx = y*tileResolution + x;
					if ( processedPoints[ processedPointsIdx ] != CNavNode::INVALID_INDEX )
						return false;

					CTerrainMap::eQuadState pointState = terrain->GetQuadState( terrain->GetQuadIndex( x, y ) );
					if ( pointState == CTerrainMap::QUAD_BLOCK_ALL )
						return false;

					Vector2 pointPosition = terrain->GetQuadCenterLocal( x, y );
					CLineQueryData query( CT_DEFAULT, Vector3( centralPoint ), Vector3( pointPosition ) );
					if ( !terrain->SpatialQuery( query ) )
						return false;

					// accept point
					processedPoints[ processedPointsIdx ] = currentMarker;
					return true;
				};

				auto funMoveUp = [funCheckExtendRegion] ( Int32 x, Int32 y, Int32 extensionLimit )
					{
						while( true )
						{
							if ( ++y > extensionLimit )
								return;
							if ( !funCheckExtendRegion( x, y ) )
								return;
						}
						
					};
				auto funMoveRight = [funCheckExtendRegion] ( Int32 x, Int32 y, Int32 extensionLimit )
					{
						while( true )
						{
							if ( ++x > extensionLimit )
								return;
							if ( !funCheckExtendRegion( x, y ) )
								return;
						}
						
					};
				auto funMoveDown = [funCheckExtendRegion] ( Int32 x, Int32 y, Int32 extensionLimit )
					{
						while( true )
						{
							if ( --y < extensionLimit )
								return;
							if ( !funCheckExtendRegion( x, y ) )
								return;
						}
					};
				auto funMoveLeft = [funCheckExtendRegion] ( Int32 x, Int32 y, Int32 extensionLimit )
					{
						while( true )
						{
							if ( --x < extensionLimit )
								return;
							if ( !funCheckExtendRegion( x, y ) )
								return;
						}
					};
				
				for ( Int32 x = regionRectangleMinX; x <= regionRectangleMaxX; ++x )
				{
					// This part of algorithm is pure heuristic.
					// We only allow one direction to be double in length of other
					Int32 maxSteps = Min( regionWidth - regionHeight / 2, maxRegionSize/2 - regionWidth );
					funMoveUp( x, regionRectangleMaxY, Min( regionRectangleMaxY + maxSteps, Int32(tileResolution)-1 ) );
					funMoveDown( x, regionRectangleMinY, Max( regionRectangleMinY - maxSteps, 0 ) );
				}

				for ( Int32 y = regionRectangleMinY; y <= regionRectangleMaxY; ++y )
				{
					Int32 maxSteps = Min( regionHeight - regionWidth / 2, maxRegionSize/2 - regionHeight );
					funMoveRight( regionRectangleMaxX, y, Min( regionRectangleMaxX + maxSteps, Int32(tileResolution)-1 ) );
					funMoveLeft( regionRectangleMinX, y, Max( regionRectangleMinX - maxSteps, 0 ) );
				}
			}
			
			if ( m_nodes.Size() >= maxNodes )
			{
				PATHLIB_ERROR( TXT("Too many generated nodes! Nodes count hit its limit!\n" ) );
				return false;
			}
			// now create node and connect it with existing ones
			Vector3 nodePos( centralPoint.X, centralPoint.Y, terrain->ComputeHeight( centralPoint ) );
			area->LocalToWorld( nodePos );

			AddNode( nodePos, NF_DEFAULT );
			// find neighbour nodes
			// we use wide search over processedPoints
			processingQuads.PushBack( Coords(x,y) );
			processedPoints[ y*tileResolution + x ] = currentMarker | MASK_NEIGHBOURTEST;
			while ( !processingQuads.Empty() )
			{
				Coords coords = processingQuads.PopBackFast();
				for ( Uint32 dir = 0; dir < EDGES; ++dir )
				{
					Coords tryPos = coords + eEdge(dir);
					if ( !tryPos.CheckBoundings( tileResolution ) )
					{
						continue;
					}
					CNavNode::Index index = processedPoints[ tryPos.m_y*tileResolution + tryPos.m_x ];
					if ( index == currentMarker )
					{
						processedPoints[ tryPos.m_y*tileResolution + tryPos.m_x ] = currentMarker | MASK_NEIGHBOURTEST;
						processingQuads.PushBack( tryPos );
					}
					else if ( index == (currentMarker | MASK_NEIGHBOURTEST) || index == 0xffffffff )
					{
						continue;
					}
					else
					{
						// found a neighbour
						index = index & (~MASK_NEIGHBOURTEST);
						Vector2 edgePoint =
							(terrain->GetQuadCenterLocal( coords.m_x, coords.m_y ) +
							terrain->GetQuadCenterLocal( tryPos.m_x, tryPos.m_y )) * 0.5f;
						// find its corresponding struct
						Bool neighbourAllreadyFound = false;
						for ( Uint32 i = 0, n = neigbours.Size(); i != n; ++i )
						{
							if ( neigbours[ i ].m_neighbourId == index )
							{
								auto& data = neigbours[ i ];
								// just modify center of mass data
								data.m_centerOfMass.AsVector2() =
									data.m_centerOfMass.AsVector2() * (data.m_centerOfMassWeight/(data.m_centerOfMassWeight+1.f)) +
									(edgePoint / (data.m_centerOfMassWeight+1.f));
								data.m_centerOfMassWeight += 1.f;
								neighbourAllreadyFound = true;
								break;
							}
						}
						if ( !neighbourAllreadyFound )
						{
							SInitialNeighbourData data;
							data.m_neighbourId = index;
							data.m_centerOfMass = edgePoint;
							data.m_centerOfMassWeight = 1.f;
							data.m_fallbackLocation.AsVector2() = edgePoint;
							data.m_fallbackLocation.Z = terrain->ComputeHeight( edgePoint );
							neigbours.PushBack( data );
						}
					}
				}
			}
			for ( Uint32 i = 0, n = neigbours.Size(); i != n; ++i )
			{
				// calculate input points Z
				neigbours[ i ].m_centerOfMass.Z = terrain->ComputeHeight( neigbours[ i ].m_centerOfMass.AsVector2() );
				neigbours[ i ].m_fallbackLocation.Z = terrain->ComputeHeight( neigbours[ i ].m_fallbackLocation.AsVector2() );
				
				// translate input points to world coordinates
				area->LocalToWorld( neigbours[ i ].m_centerOfMass );
				area->LocalToWorld( neigbours[ i ].m_fallbackLocation );
				
				InitialConnectNeighbours( currentMarker, neigbours[ i ] );
			}
			neigbours.Clear();
		}
	}
	//{
	//	// TODO: Tmpshit debug code
	//	Uint32 sz = tileResolution * tileResolution;
	//	m_colors.Resize( sz );
	//	for ( Uint32 i = 0; i < sz; ++i )
	//	{
	//		m_colors[ i ] = Uint8(processedPoints[ i ] & (~MASK_NEIGHBOURTEST));
	//	}
	//}
	
	return !m_nodes.Empty();
}
template < class TArea >
Bool CNavGraph::TInitialGraphGeneration( CPathLibWorld* world, CNavmesh* navmesh, TArea* area )
{
	// create dual graph
	CNavmesh::TriangleIndex triCount = navmesh->GetTrianglesCount();
	m_nodes.ResizeFast( triCount );
	for ( CNavmesh::TriangleIndex tri = 0; tri < triCount; ++tri )
	{
		Vector3 triVerts[ 3 ];
		navmesh->GetTriangleVerts( tri, triVerts );
		Vector3 pos = (triVerts[ 0 ] + triVerts[ 1 ] + triVerts[ 2 ]) * (1.f / 3.f);
		area->LocalToWorld( pos );
		m_nodes[ tri ] = CNavNode( pos, tri, area->GetId(), *this, NF_DEFAULT );
		CNavmesh::TriangleIndex neighbours[ 3 ];
		navmesh->GetTriangleNeighbours( tri, neighbours );

		for( Uint32 i = 0; i < 3; ++i )
		{
			if ( !CNavmesh::IsEdge( neighbours[ i ] ) )
			{
				if ( neighbours[ i ] == tri )
				{
					PATHLIB_ERROR( TXT("INVALID NAVMESH! TRIANGLE IS SELF-CONNECTED!!! WTF?!\n") );
					continue;
				}
				AddLink( m_nodes[ tri ], CPathLink( neighbours[ i ], CNavNode::INVALID_INDEX ) );
			}
		}
	}

	if( !Debug_CheckAllLinksTwoSided() )
	{
		PATHLIB_ERROR( TXT("INVALID NAVMESH! ONE SIDED CONNECTIONS DETECTED! NAVGRAPH GENERATION FAILED!\n") );
		return false;
	}

	return !m_nodes.Empty();
}

template < class TArea >
Bool CNavGraph::TOptimizeGraph( CPathLibWorld* world, TArea* area )
{
	struct Collector : public CNodeFinder::Handler
	{
		Vector3						m_pos;
		Float						m_distSq;
		TDynArray< CPathNode* >&	m_nodesInVicinity;

		Collector( const Vector3& pos, Float dist, TDynArray< CPathNode* >&	nodesInVicinity )
			: m_pos( pos )
			, m_distSq( dist*dist )
			, m_nodesInVicinity( nodesInVicinity ) {}

		void Handle( CNavNode& node ) override
		{
			if ( ( node.GetPosition() - m_pos ).SquareMag() < m_distSq )
			{
				m_nodesInVicinity.PushBack( &node );
			}
		}
	};

	struct NodeInfo
	{
		CPathNode*			m_node;
		Vector3				m_originalPosition;
	};

	TDynArray< CPathNode* > nodesInVicinity;
	TDynArray< NodeInfo > nodesInSet;

	Float personalSpace = GetPersonalSpace();
	Float maxNodesDistance = area->GetMaxNodesDistance();
	Bool dirty = false;

	CCentralNodeFinder::RegionAcceptor region = CCentralNodeFinder::AnyRegion( true, false );

	for ( Uint32 i = 0, n = m_nodes.Size(); i < n; ++i )
	{
		CNavNode& node = m_nodes[ i ];
		if ( node.HaveAnyFlag( NFG_PROCESSED | NF_BLOCKED ) )
			continue;

		Bool touched = false;
		Vector3 newPosition = node.GetPosition();

		{
			NodeInfo c;
			c.m_node = &node;
			c.m_originalPosition = newPosition;
			nodesInSet.PushBack(c);
		}

		Box testBBox( Vector( newPosition ), maxNodesDistance / 2.f );

		Collector collector( newPosition, maxNodesDistance / 2.f, nodesInVicinity );

		m_nodeFinder.IterateNodes( testBBox, collector, region, 0 );

		for ( auto it = nodesInVicinity.Begin(), end = nodesInVicinity.End(); it != end; ++it )
		{
			CNavNode* checkNode = static_cast< CNavNode* >( *it );
			if ( checkNode->HaveAnyFlag( NFG_PROCESSED | NF_BLOCKED ) || checkNode == &node )
				continue;

			Vector3 testNodeCurrentPos = checkNode->GetPosition();
			// lets pick the new position
			Vector3 tryPosition = newPosition * (Float(nodesInSet.Size()) / Float(nodesInSet.Size()+1)) + (testNodeCurrentPos / float(nodesInSet.Size()+1));
			if ( !area->ComputeHeight( tryPosition, tryPosition.Z ) )
				continue;
			// and try if its not cancelling any existing links
			if ( !CanMoveNode( checkNode, tryPosition, false ) )
				continue;
			if ( !CanMoveNode( &node, tryPosition, false  ) )
				continue;

			Bool acceptPosition = true;
			for ( auto itCheck = nodesInSet.Begin(), end = nodesInSet.End(); itCheck != end; itCheck++)
			{
				CWideLineQueryData query( CT_DEFAULT, itCheck->m_originalPosition, tryPosition, personalSpace );
				if ( !area->SpatialQuery( query ) )
				{
					acceptPosition = false;
					break;
				}
			}
			if ( !acceptPosition )
			{
				continue;
			}

			touched = true;
			newPosition = tryPosition;
				
			{
				NodeInfo c;
				c.m_node = checkNode;
				c.m_originalPosition = testNodeCurrentPos;
				nodesInSet.PushBack(c);
			}
			// merge nodes
			GenerationMergeNodes( node, *checkNode );
			ASSERT( CanMoveNode( &node, newPosition, false ) );
			checkNode->AddFlags( NF_PROCESSED );
		}
		if ( touched )
		{
			node.AddFlags( NF_PROCESSED );
			dirty = true;
			ASSERT( CanMoveNode( &node, newPosition, false ) );
			//m_nodeFinder.RemoveDynamicElement( &node );
			// this basically breaks node finder, but we don't care, since node won't be used anymore, and we will invalidate nodefinder soon after
			node.SetPosition( newPosition );
			//m_nodeFinder.AddDynamicElement( &node );
			GenerationCalculateNodeAvailability( &node );	
		}
		nodesInSet.ClearFast();
		nodesInVicinity.ClearFast();
	}

	if ( dirty )
	{
		m_nodeFinder.Invalidate();
		ASSERT( Debug_CheckAllLinksTwoSided() );
		DeleteMarked();
		for ( Uint32 i = 0, n = m_nodes.Size(); i < n; ++i )
		{
			CNavNode& node = m_nodes[ i ];
			node.ClearFlags( NF_PROCESSED );
		}
	}

	return dirty;
}

void CNavGraph::MarkNodeForDeletion( CNavNode::Id id )
{
	CNavNode* node = GetNode( id );
	node->AddFlags( NF_MARKED_FOR_DELETION );
	// clear all node links instantly (no need to wait for anything
	for ( LinksErasableIterator it( *node ); it; )
	{
		node->EraseLink( it );
	}
}

void CNavGraph::MarkNodeForDeletion( CNavNode::Index idx )
{
	CNavNode* node = GetNode( idx );
	node->AddFlags( NF_MARKED_FOR_DELETION );
	// clear all node links instantly (no need to wait for anything
	for ( LinksErasableIterator it( *node ); it; )
	{
		node->EraseLink( it );
	}
}


Bool CNavGraph::MarkSeparateNodesForDeletion()
{
	Bool dirty = false;
	Bool isLinkedById = VAreNodesLinkedById();
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		if ( node.HaveAnyFlag( NF_BLOCKED ) || ( node.IsUnlinked() && !node.HaveFlag( NF_CONNECTOR ) ) )
		{
			MarkNodeForDeletion( node.GetIndex() );

			dirty = true;
		}
		else
		{
			for ( LinksErasableIterator it( node ); it; )
			{
				CPathLink& link = *it;
				if ( link.HaveAnyFlag( NF_BLOCKED ) )
				{
					node.EraseLink( it );
				}
				else
				{
					++it;
				}
			}
		}
	}
	return dirty;
}

Bool CNavGraph::ConnectNodeList( CNavNode* nodes, Uint32 nodesCount, NodeFlags defaultLinkFlags, Bool ignoreNodeSets )
{
	static const Uint32 MAX_INPUT_LEN = 512;

	ASSERT( !m_nodesAreLinkedById, TXT("Function doesn't support links in id form") );

	Box testBox( Box::RESET_STATE );
	for( Uint32 i = 0; i < nodesCount; ++i )
	{
		testBox.AddPoint( nodes[ i ].GetPosition() );
		nodes[ i ].AddFlags( NF_PROCESSED );
	}

	testBox.Min.AsVector3() -= Vector3( m_maxNodesDistance, m_maxNodesDistance, m_maxNodesDistance );
	testBox.Max.AsVector3() += Vector3( m_maxNodesDistance, m_maxNodesDistance, m_maxNodesDistance );

	struct Handler : public CNodeFinder::Handler
	{
		CNavGraph*									m_navgraph;
		CNavNode*									m_nodes;
		Uint32										m_nodesCount;
		CNavNode::NodesetIdx						m_ignoreNodeSets;
		TDynArray< TPair< CNavNode::Id, Uint32 > >	m_connectorsList;
		Float										m_personalSpace;

		Handler( CNavGraph* navgraph, CNavNode*	nodes, Uint32 nodesCount, Bool ignoreNodeSets )
			: m_navgraph( navgraph )
			, m_nodes( nodes )
			, m_nodesCount( nodesCount )
			, m_ignoreNodeSets( ignoreNodeSets )
			, m_personalSpace( navgraph->GetPersonalSpace() ) {}

		void Handle( CNavNode& node ) override
		{
			if ( m_ignoreNodeSets && node.GetNodesetIndex() != CNavNode::INVALID_INDEX )
			{
				return;
			}

			// check if node can connect to any detour point
			Vector3 nodePosition = node.GetPosition();

			Uint32 bestDetourPoint = 0xffff;
			Float closestDetourPointDistSq = FLT_MAX;
			for ( Uint32 i = 0; i != m_nodesCount; ++i )
			{
				Float distSq = (nodePosition - m_nodes[ i ].GetPosition()).SquareMag();
				if ( distSq < closestDetourPointDistSq )
				{
					CWideLineQueryData query( CT_IGNORE_OTHER_AREAS, nodePosition, m_nodes[ i ].GetPosition(), m_personalSpace );
					if ( m_navgraph->GetArea()->VSpatialQuery( query ) )
					{
						closestDetourPointDistSq = distSq;
						bestDetourPoint = i;
					}
				}
			}

			if ( bestDetourPoint != 0xffff )
			{
				m_connectorsList.PushBack( TPair< CNavNode::Id, Uint32 >( node.GetFullId(), bestDetourPoint ) );
			}
		}

	} collector( this, nodes, nodesCount, ignoreNodeSets );

	m_nodeFinder.IterateNodes( testBox, collector, CCentralNodeFinder::AnyRegion( true, false ), NF_MARKED_FOR_DELETION | NF_PROCESSED );

	for( Uint32 i = 0; i < nodesCount; ++i )
	{
		nodes[ i ].ClearFlags( NF_PROCESSED );
	}

	// now compute closest connector nodes
	// initialize data
	TStaticArray< TPair< CNavNode::Id, Float >, MAX_INPUT_LEN > closestConnectors;
	closestConnectors.Resize( nodesCount );
	for ( auto it = closestConnectors.Begin(), end = closestConnectors.End(); it != end; ++it )
	{
		(*it) = TPair< CNavNode::Id, Float >( CNavNode::Id::INVALID, FLT_MAX );
	}

	// compute closest connectors
	for ( auto it = collector.m_connectorsList.Begin(), end = collector.m_connectorsList.End(); it != end; ++it )
	{
		CNavNode* connectorNode = GetNode( it->m_first );
		CNavNode* detourNode = &nodes[ it->m_second ];
		Float distSq = (connectorNode->GetPosition() - detourNode->GetPosition()).SquareMag();
		if ( distSq < closestConnectors[ it->m_second ].m_second )
		{
			closestConnectors[ it->m_second ].m_second = distSq;
			closestConnectors[ it->m_second ].m_first = it->m_first;
		}
	}


	Bool ret = false;
	// connect detour with connectors
	for ( Uint32 i = 0; i != nodesCount; ++i )
	{
		if ( closestConnectors[ i ].m_first != CNavNode::Id::INVALID )
		{
			Float distSq = closestConnectors[ i ].m_second;

			ConnectNode( closestConnectors[ i ].m_first, nodes[ i ].GetFullId(), defaultLinkFlags, CalculateLinkCost( sqrt( distSq ) ) );
			ret = true;
		}
	}

	return ret;
}

Bool CNavGraph::CutOutWithShape( CObstacleShape* shape )
{
	Box testBox( shape->GetBBoxMin(), shape->GetBBoxMax() );
	m_areaDescription->VLocalToWorld( testBox );
	testBox.Min.AsVector3() -= Vector3( m_maxNodesDistance, m_maxNodesDistance, m_maxNodesDistance );
	testBox.Max.AsVector3() += Vector3( m_maxNodesDistance, m_maxNodesDistance, m_maxNodesDistance );
	
	struct Handler : public CNodeFinder::Handler
	{
		CNavGraph*		m_navgraph;
		CObstacleShape* m_shape;
		CAreaDescription* m_areaDescription;
		Float			m_personalSpace;
		Bool			m_ret;

		Handler( CNavGraph* navgraph, CObstacleShape* shape )
			: m_navgraph( navgraph )
			, m_shape( shape )
			, m_areaDescription( m_navgraph->GetArea() )
			, m_personalSpace( navgraph->GetPersonalSpace() )
			, m_ret( false ) {}

		void Handle( CNavNode& node ) override
		{
			// check if node collides with our shape - if so mark it for deletion
			Vector3 nodePosition = node.GetPosition();
			Vector3 localNodePosition = nodePosition;
			m_areaDescription->VWorldToLocal( localNodePosition );
			{
				CCircleQueryData query( CT_DEFAULT, localNodePosition, m_personalSpace );
				Vector3 testBoundings[2];
				query.ComputeBBox( testBoundings );
				if ( m_shape->TestBoundings( testBoundings[ 0 ], testBoundings[ 1 ] ) && !m_shape->VSpatialQuery( query, testBoundings ) )
				{
					m_navgraph->MarkNodeForDeletion( node.GetFullId() );
					m_ret = true;
					return;
				}
			}

			// delete links that collide with our shape
			for ( LinksErasableIterator it( node ); it; )
			{
				CPathLink& link = *it;
				CPathNode* destNode = link.HaveFlag( NF_DESTINATION_IS_ID ) ? m_navgraph->GetNode( link.GetDestinationId() ) : link.GetDestination();
				Vector3 localDestNodePosition = destNode->GetPosition();
				m_areaDescription->VWorldToLocal( localDestNodePosition );
				CWideLineQueryData query( CT_DEFAULT, localNodePosition, localDestNodePosition, m_personalSpace );
				Vector3 testBoundings[2];
				query.ComputeBBox( testBoundings );
				if ( m_shape->TestBoundings( testBoundings[ 0 ], testBoundings[ 1 ] ) && !m_shape->VSpatialQuery( query, testBoundings ) )
				{
					node.EraseLink( it );
					m_ret = true;
				}
				else
				{
					++it;
				}
			}
		}

		
	} handler( this, shape );

	m_nodeFinder.IterateNodes( testBox, handler, CCentralNodeFinder::AnyRegion( true ), NF_MARKED_FOR_DELETION );

	return handler.m_ret;
}


//void CNavGraph::RecomputeNodesHeight( CTerrainAreaDescription* terrainArea )
//{
//	for ( Uint32 i = 0, n = m_nodes.Size(); i != n; ++i )
//	{
//		const Vector3& nodePos = m_nodes[ i ].GetPosition();
//		if ( nodePos.Z == CTerrainAreaDescription::HEIGHT_UNSET )
//		{
//			Float z;
//			terrainArea->ComputeHeight( nodePos.AsVector2(), z );
//			m_nodes[ i ].SetPosition( Vector3( nodePos.X, nodePos.Y, z ) );
//		}
//	}
//}

void CNavGraph::DeleteMarked()
{
	if ( Super::DeleteMarked() )
	{
		if ( m_nodeFinder.IsInitialized() )
		{
			ClearNodeFinder();
			ComputeNodeFinder();
		}
	}
}

Bool CNavGraph::MoveNodesToAvailablePositions()
{
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );
	// WALL REPULTION TEST CONSTANTS
	static const Uint32 MAX_TRIES = 12;

	// PREDEFINED DIRECTIONS TEST CONSTANTS
	//static const Int32 DIRECTIONS = 8;
	//static const Vector2 DIRECTION[DIRECTIONS] =
	//{
	//	Vector2(1,0), Vector2(0,1), Vector2(-1,0), Vector2(0,-1),
	//	Vector2(F_SQRT2,F_SQRT2), Vector2(F_SQRT2,-F_SQRT2), Vector2(-F_SQRT2,F_SQRT2), Vector2(-F_SQRT2,-F_SQRT2)
	//};
	//static const Float ALG_STEPS = 4;
	//const Float ALG_DIST = F_SQRT2 * personalSpace;


	//Bool bDirAvailable[DIRECTIONS];
	Uint32 nSize = m_nodes.Size();
	for ( Uint32 nodeIndex = 0; nodeIndex < nSize; ++nodeIndex )
	{
		CNavNode* node = &m_nodes[ nodeIndex ];
		if ( !node->HaveAnyFlag( NF_BLOCKED ) )
			continue;
		Vector3 originalPosition = node->GetPosition();
		
		////////////////////////////////////////////////////////////////////
		// Optimize location of node - wall repultion
		if ( m_areaDescription->VTestLocation( node->GetPosition(), CT_DEFAULT ) )			// start with initial test
		{
			Bool bSuccess = false;
			Vector3 tryPos = originalPosition;
			// Try to move node away from closest wall
			for( Uint32 tryCount = 0; tryCount < MAX_TRIES; ++tryCount )
			{
				CClosestObstacleCircleQueryData query( CT_DEFAULT, tryPos, personalSpace );
				m_areaDescription->VSpatialQuery( query );
				if ( query.HasHit() )
				{
					Float distanceToObstacle = sqrt( query.m_closestDistSq );

					Vector2 vecOut = tryPos.AsVector2() - query.m_pointOut.AsVector2();
					vecOut.Normalize();
					vecOut *= personalSpace - distanceToObstacle + 0.05f;
					Vector3 vecPos = tryPos;
					vecPos.AsVector2() += vecOut;
					if ( !m_areaDescription->VComputeHeightFrom( vecPos.AsVector2(), tryPos, vecPos.Z ) )
					{
						break;
					}

					if ( CanMoveNode( node, vecPos, false ) )
					{
						tryPos = vecPos;
					}
					else
					{
						break;
					}
				}
				else
				{
					bSuccess = true;
					break;
				}
			}
			if ( bSuccess )
			{
				node->SetPosition( tryPos );
				GenerationCalculateNodeAvailability( node );
			}
		}
		
		/*
		// Try to move node in some predefined direction
		if ( !bSuccess )
		{
			for (Int32 i = 0; i < DIRECTIONS; ++i)
				bDirAvailable[i] = true;
			for (Float nStep = 0; nStep < ALG_STEPS; ++nStep)
			{
				Float fDist = ((nStep + 1.f) / ALG_STEPS) * ALG_DIST;

				for (Int32 nDirection = 0; nDirection < DIRECTIONS; ++nDirection)
				{
					if (!bDirAvailable[nDirection])
						continue;

					Vector3 tryPos = originalPosition + (DIRECTION[nDirection] * fDist);
					if (!CanMoveNode(node,tryPos,&originalPosition))
					{
						bDirAvailable[nDirection] = false;
						continue;
					}

					if (m_pArea->TestLocation(tryPos,personalSpace,GetDefaultCollisionFlags()))
					{
						nStep = ALG_STEPS;	// Break both loops
						MoveNode(node, tryPos);
						break;
					}
				}
			}
		}*/
	}

	return true;
}


Bool CNavGraph::GenerationCalculateLinkAvailability( CNavNode* node, CPathLink* link )
{
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );
	CPathNode* destinationNode = GetNode( link->GetDestinationId() );

	CPathLinkModifier modifier( *node, *link );
	CWideLineQueryData query( CT_NO_ENDPOINT_TEST | CT_NO_OBSTACLES, node->GetPosition(), destinationNode->GetPosition(), personalSpace );
	if ( m_areaDescription->VSpatialQuery( query ) )
	{
		modifier.ClearFlags( NF_BLOCKED );
		return true;
	}
	else
	{
		modifier.AddFlags( NF_BLOCKED );
		return false;
	}
}


void CNavGraph::GenerationCalculateNodeAvailability( CNavNode* node )
{
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );

	NodeFlags nodeFlags = node->GetFlags() | NF_BLOCKED;

	for ( LinksIterator itLinks( *node ); itLinks; ++itLinks )
	{
		CPathLink& link = *itLinks;
		CPathNode* destinationNode = GetNode( link.GetDestinationId() );

		if ( destinationNode->HaveFlag( NF_DETACHED ) )
		{
			continue;
		}
		
		CPathLinkModifier modifier( *node, link );
		CWideLineQueryData query( CT_NO_ENDPOINT_TEST | CT_NO_OBSTACLES, node->GetPosition(), destinationNode->GetPosition(), personalSpace );
		if ( m_areaDescription->VSpatialQuery( query ) )
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

void CNavGraph::GenerationCalculateNodesAvailability()
{
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );

	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode* node = &(*it);

		NodeFlags nodeFlags = node->GetFlags() | NF_BLOCKED;

		for ( LinksIterator itLinks( *node ); itLinks; ++itLinks )
		{
			CPathLink& link = *itLinks;

			CPathNode* destinationNode = GetNode( link.GetDestinationId() );
			if ( destinationNode->GetIndex() < node->GetIndex() )
			{
				if ( !link.HaveAnyFlag( NF_BLOCKED ) )
				{
					nodeFlags &= ~NF_BLOCKED;
				}
				// link must be already processed
				continue;
			}
			CPathLinkModifier modifier( *node, link );
			CWideLineQueryData query( CT_NO_ENDPOINT_TEST | CT_NO_OBSTACLES, node->GetPosition(), destinationNode->GetPosition(), personalSpace );
			if (  link.HaveFlag( NF_IS_GHOST_LINK ) || m_areaDescription->VSpatialQuery( query ) )
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
		if ( node->HaveFlag( NF_BLOCKED ) )
		{
			node->SetRegionId( *this, INVALID_AREA_REGION );
		}
	}
}

void CNavGraph::RuntimeCalculateNodeAvailability( CNavNode& node, CNodeSetProcessingContext& context )
{
	const auto& marking = context.GetMarking();
	if ( marking.CheckMarking( node ) )
	{
		return;
	}

	CAreaDescription* area = m_areaDescription;
	Float personalSpace = GetPersonalSpace();
	Bool hasUpdated = false;

	NodeFlags nodeFlags = node.GetFlags() | NF_BLOCKED;

	for ( LinksIterator itLinks( node ); itLinks; ++itLinks )
	{
		CPathLink& link = *itLinks;

		CNavNode* destinationNode = static_cast< CNavNode* >( link.GetDestination() );

		if ( marking.CheckMarking( *destinationNode ) || destinationNode->HaveFlag( NF_DETACHED ) )
		{
			continue;
		}

		Bool isBlocked;
		CWideLineQueryData query( CT_NO_ENDPOINT_TEST | CT_IGNORE_OTHER_AREAS, node.GetPosition(), destinationNode->GetPosition(), personalSpace );
		if ( link.HaveAnyFlag( NF_IS_GHOST_LINK | NF_CONNECTOR ) || area->VSpatialQuery( query ) )
		{
			isBlocked = false;
			nodeFlags &= ~NF_BLOCKED;
		}
		else
		{
			isBlocked = true;
		}

		// check if anything has changed
		if ( link.HaveFlag( NF_BLOCKED ) == isBlocked )
		{
			continue;
		}

		hasUpdated = true;
		context.UpdateNodeConsistency( destinationNode );

		// modify link
		CPathLinkModifier modifier( node, link );
		if ( isBlocked )
		{
			modifier.AddFlags( NF_BLOCKED );
		}
		else
		{
			modifier.ClearFlags( NF_BLOCKED );
			if ( !link.HaveFlag( NF_IS_ONE_SIDED ) )
			{
				link.GetDestination()->ClearFlags( NF_BLOCKED );
			}
		}
	}

	if ( hasUpdated )
	{
		context.UpdateNodeConsistency( &node );
		node.SetFlags( nodeFlags );
	}
	
	marking.Mark( node );
}

void CNavGraph::RuntimeUpdateCollisionFlags( const Box& bbox, CNodeSetProcessingContext& context )
{
	struct Handler : public CNodeFinder::Handler
	{
		CNavGraph*						m_navgraph;
		CNodeSetProcessingContext&		m_context;
		Box								m_testBox;

		Handler( CNavGraph* navgraph, CNodeSetProcessingContext& context, const Box& bbox )
			: m_navgraph( navgraph ), m_context( context )
		{
			Float maxNodesDistance = m_navgraph->GetMaxNodesDistance();

			m_testBox = bbox;
			m_testBox.Min.AsVector3() -= Vector3( maxNodesDistance, maxNodesDistance, maxNodesDistance );
			m_testBox.Max.AsVector3() += Vector3( maxNodesDistance, maxNodesDistance, maxNodesDistance );
		}

		void operator()( CNavNode& node ) const
		{
			if ( m_testBox.Contains( node.GetPosition() ) )
			{
				m_navgraph->RuntimeCalculateNodeAvailability( node, m_context );
			}
		}

		void Handle( CNavNode& node ) override
		{
			m_navgraph->RuntimeCalculateNodeAvailability( node, m_context );
		}
	};

	Handler handler( this, context, bbox );
	
	m_nodeFinder.IterateNodes( handler.m_testBox, handler, CCentralNodeFinder::AnyRegion( true, false ), NF_IS_GHOST_LINK );
}

void CNavGraph::HandleObstaclesMap( CObstaclesMap* obstacles, const CObstaclesDetourInfo& detourInfo )
{
	// Sweep & prune algorithm types and subprocedures declaration
	static const Uint32 pruneDimm = 0;
	static const Uint32 searchDimm = 1;
	const Float maxNodesDistanceSq = m_maxNodesDistance * m_maxNodesDistance;

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
		Uint16					m_detourNodeIdx;
		CNavNode*				m_node;								// NOTICE: Storing pointers means that we cannot create new nodes during the algorithm

		ENodeType				GetNodeType() const					{ return m_node->HaveFlag( NF_IS_OBSTACLE_DETOUR_NODE ) ? DETOUR_NODE : BASE_NODE; }
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
		CNavNode*				m_node;

		CActiveNode()												{}
		CActiveNode( CNavNode* navNode, Uint16 detourNodeIdx = 0xffff )
			: m_val( navNode->GetPosition().A[ searchDimm ] )
			, m_detourNodeIdx( detourNodeIdx )
			, m_node( navNode )										{}

		ENodeType				GetNodeType() const					{ return m_node->HaveFlag( NF_IS_OBSTACLE_DETOUR_NODE ) ? DETOUR_NODE : BASE_NODE; }
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
		CDetourNodeInfo( CNavNode* node )
			: m_node( node->GetIndex() )							{}
		CDetourNodeInfo( CNavNode* node, Uint32 shapeIdx, Float maxNodesDistanceSq )
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

		AlgorithmData( CNavGraph* navgraph, Float personalSpace )
			: m_this( navgraph )
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
						if ( m_this->m_areaDescription->VSpatialQuery( queryData ) )
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
					CActiveNode activeNode( e.m_node, e.m_detourNodeIdx );
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
					CActiveNode activeNode( e.m_node );
					auto& nodeList = (e.GetNodeType() == DETOUR_NODE) ? m_activeDetourNodes : m_activeBaseNodes;
					auto itFind = nodeList.Find( activeNode );
					ASSERT( itFind != nodeList.End() );
					nodeList.Erase( itFind );
				}
			}
			ASSERT( m_activeBaseNodes.Empty() );
			ASSERT( m_activeDetourNodes.Empty() );
		}

		CNavGraph*								m_this;
		ActiveNodesList							m_activeBaseNodes;
		ActiveNodesList							m_activeDetourNodes;
		Float									m_personalSpace;

	};

	// declare run time data
	Float									personalSpace = GetPersonalSpace();
	TDynArray< CPruneEvent >				events;
	TSortedArray< CDetourNodeInfo >			detourNodesInfo;
	AlgorithmData							algorithm( this, personalSpace );


	for ( CNavNode::Index nodeIdx = 0, nodesCount = CNavNode::Index( m_nodes.Size() ); nodeIdx < nodesCount; ++nodeIdx )
	{
		CNavNode& node = m_nodes[ nodeIdx ];
		if ( node.HaveFlag( NF_MARKED_FOR_DELETION ) )
		{
			continue;
		}
		Vector3 nodePos = node.GetPosition();
		m_areaDescription->VWorldToLocal( nodePos );
		CCircleQueryData nodeQuery( CT_DEFAULT, nodePos, personalSpace );

		// erase nodes blocked by obstacles
		if ( !obstacles->TSpatialQuery( nodeQuery ) )
		{
			MarkNodeForDeletion( nodeIdx );
			continue;
		}

		// erase blocked links
		for ( LinksErasableIterator itLinks( node ); itLinks; )
		{
			CPathLink& link = *itLinks;

			if ( link.GetDestinationId().m_index < nodeIdx )
			{
				// ommit two side tests
				++itLinks;
				continue;
			}
			CNavNode* targetNode = GetNode( link.GetDestinationId() );

			Vector3 targetNodePos = targetNode->GetPosition();
			m_areaDescription->VWorldToLocal( targetNodePos );

			CWideLineQueryData lineQuery( CT_DEFAULT, nodePos, targetNodePos, personalSpace );
			if ( !obstacles->TSpatialQuery( lineQuery ) )
			{
				node.EraseLink( itLinks );
			}
			else
			{
				++itLinks;
			}
		}
	}

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
			CNavNode& detourNode = AddNode( currVert, NF_DEFAULT | NF_IS_OBSTACLE_DETOUR_NODE );
			detourNodes[ vertId ] = detourNode.m_id.m_index;
			detourNodesInfo.PushBack( CDetourNodeInfo( &detourNode, shapeIdx, maxNodesDistanceSq ) );
		}
		
		// spawn detour links
		if ( shapeVerts == 2 )
		{
			const Vector3& prevVert = detourInfo.DetourShapeVert( shapeIdx, 0 );
			const Vector3& currVert = detourInfo.DetourShapeVert( shapeIdx, 1 );
			CWideLineQueryData query( CT_NO_ENDPOINT_TEST, currVert, prevVert, personalSpace );
			if ( m_areaDescription->VSpatialQuery( query ) )
			{
				GenerationConnectNode( detourNodes[ 0 ], detourNodes[ 1 ] );
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
				if ( m_areaDescription->VSpatialQuery( query ) )
				{
					GenerationConnectNode( currIdx, prevIdx );
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
	events.Reserve( m_nodes.Size() * 2 );
	detourNodesInfo.Reserve( detourInfo.VertsCount() );

	// spawn events by iterating all nodes
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		if ( node.HaveAnyFlag( NF_MARKED_FOR_DELETION ) )
		{
			continue;
		}
		CPruneEvent e;
		e.m_val = node.GetPosition().A[ pruneDimm ];
		e.m_eventType = CPruneEvent::PRUNE_ADD;
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
		e.m_val += m_maxNodesDistance;
		e.m_eventType = CPruneEvent::PRUNE_REM;
		events.PushBack( e );
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
			e.m_val = e.m_node->GetPosition().A[ pruneDimm ] - m_maxNodesDistance;
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

	TArrayMap< CNavNode::Index, CDetourBestConnection > bestConnection;
	for( auto it = detourNodesInfo.Begin(), end = detourNodesInfo.End(); it != end; )
	{
		// only one connection with each node from each detour
		Uint32 shapeIdx = it->m_detourShapeIdx;
		do
		{
			const CDetourNodeInfo& info = *it;
			for ( Uint32 i = 0; i < CDetourNodeInfo::CLOSEST_COUNT; ++i )
			{
				if ( info.m_closestNode[ i ] != CNavNode::INVALID_INDEX )
				{
					CNavNode::Index closestNode = info.m_closestNode[ i ];
					auto itFind = bestConnection.Find( closestNode );
					if ( itFind == bestConnection.End() )
					{
						CDetourBestConnection c;
						c.m_detourNode = info.m_node;
						c.m_distSq = info.m_closestNodeDistSq[ i ];
						bestConnection.Insert( closestNode, c );
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
			CNavNode::Index baseNodeIdx = it->m_first;
			CNavNode::Index detourNodeIdx = it->m_second.m_detourNode;
			CNavNode* baseNode = GetNode( baseNodeIdx );
			if ( !baseNode->IsConnected( detourNodeIdx ) )
			{
				GenerationConnectNode( baseNodeIdx, detourNodeIdx );
			}
		}

		bestConnection.ClearFast();
	}
}


Bool CNavGraph::Improve()
{
	TDynArray< CNavNode::Index > linksTargets;
	linksTargets.Reserve( 16 );

	// We try to use rubber band algorithm to make unaccessible links accessible.
	// To do this we will add transitional nodes that will modify links and move them "away from walls".
	for ( CPathNode::Index nodeIdx = 0, nodesCount = CPathNode::Index( m_nodes.Size() ); nodeIdx < nodesCount; ++nodeIdx )
	{
		for ( LinksIterator it(  m_nodes[ nodeIdx ] ); it; ++it )
		{
			CPathLink& link = *it;
			linksTargets.PushBack( link.GetDestinationId().m_index );
		}

		Bool nodeIsBlocked = true;

		// iteration is very tricky because we are accessing dynarray that can be resized during process
		for ( auto it = linksTargets.Begin(), end = linksTargets.End(); it != end; ++it )
		{
			Uint32 linkIdx = 0;
			for ( LinksIterator itLinks( m_nodes[ nodeIdx ] ); ; ++itLinks )
			{
				ASSERT( itLinks );
				if ( itLinks->GetDestinationId().m_index == *it )
				{
					linkIdx = itLinks.GetIndex();
					break;
				}
			}
			
			CPathNode::Index destinationNodeIndex;
			{
				CPathLink& link =  m_links.Get( linkIdx );

				if ( !link.HaveAnyFlag( NF_BLOCKED ) )
				{
					nodeIsBlocked = false;
					continue;
				}
				ASSERT( link.GetDestinationId().m_nodeSetIndex == CPathNode::INVALID_INDEX );
				destinationNodeIndex = link.GetDestinationId().m_index;
				if ( destinationNodeIndex < nodeIdx )
				{
					// link must have been already processed
					continue;
				}
			}

			CPathNode* destinationNode = GetNode( destinationNodeIndex );

			LinkImprovementPath linkImprovementPath;
			if ( ImproveLink( m_nodes[ nodeIdx ].GetPosition(), destinationNode->GetPosition(), linkImprovementPath ) )
			{
				// algorithm is successful. Create nodes and link them
				// this is very tricky part. Since we will be adding nodes its important to notice points when pointers to links/nodes could be invalidated.
				
				// first break initial link
				{
					CPathLinkModifier modifier( m_nodes[ nodeIdx ], m_links.Get( linkIdx ) );
					modifier.GenerationErase( this );
				}
				
				// now create new nodes and link them
				{
					CPathNode::Index prevNodeIdx = nodeIdx;
					for ( auto it = linkImprovementPath.Begin(), end = linkImprovementPath.End(); it != end; ++it )
					{
						CPathNode::Index newNodeIndex = AddNode( *it, NF_DEFAULT ).GetIndex();
						GenerationConnectNode( prevNodeIdx, newNodeIndex );
						prevNodeIdx = newNodeIndex;
					}
					GenerationConnectNode( prevNodeIdx, destinationNodeIndex );
				}
			}
		}
		linksTargets.ClearFast();

		// Improvement algorithm can only help
		if ( !nodeIsBlocked )
		{
			m_nodes[ nodeIdx ].ClearFlags( NF_BLOCKED );
		}
	}

	return true;
}

Bool CNavGraph::ImproveLink( const Vector3& pos1, const Vector3& pos2, LinkImprovementPath& outPath, Int32 reservedNodes )
{
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );

	CClosestObstacleWideLineQueryData query( CT_NO_ENDPOINT_TEST, pos1, pos2, personalSpace );
	m_areaDescription->VSpatialQuery( query );

	// check if link is accessible
	if ( !query.HasHit() )
	{
		return true;
	}

	// test if we run out of nodes (some may have reserved place for themselves but are not yet inserted to output vector)
	if ( outPath.Capacity() <= outPath.Size() + reservedNodes )
	{
		return false;
	}

	// if line segment touches obstacle we cant really do anything
	if ( query.m_closestDistSq < NumericLimits< Float >::Epsilon() )
	{
		return false;
	}

	// Compute possibly safe position (rubber band vertex)
	Float obstacleDistance = sqrt( query.m_closestDistSq );

	Vector2 moveVec = query.m_closestPointOnSegment.AsVector2() - query.m_closestGeometryPoint.AsVector2();
	//moveVec.Normalize();
	//moveVec *= personalSpace - obstacleDistance + 0.01f;
	moveVec *=
		(personalSpace - obstacleDistance + 0.01f )			// safe distance
		/ obstacleDistance;									// normalization factor

	Vector3 safePoint = query.m_closestPointOnSegment;
	safePoint.AsVector2() += moveVec;

	if ( !m_areaDescription->VComputeHeight( safePoint, safePoint.Z ) )
	{
		// TODO: Handle this relatively rare case
		// Vector3 linePosition = query.m_closestPointOnSegment;
		// m_areaDescription->VComputeHeightFrom( linePosition.AsVector2(), pos1, linePosition.Z );
		// ...
		return false;
	}

	// test if safe point is usable
	CCircleQueryData safePointQuery( CT_DEFAULT, safePoint, personalSpace );
	if ( !m_areaDescription->VSpatialQuery( safePointQuery ) )
	{
		// this is most common case of rubber band algorithm failure. Basically it means that we can't walk around obstacle because some other obstacles are making traversal too tight.
		return false;
	}
	// we reserve place for our node
	if ( !ImproveLink( pos1, safePoint, outPath, reservedNodes+1 ) )
	{
		return false;
	}
	ASSERT( outPath.Capacity() > outPath.Size() );
	//if ( outPath.Capacity() <= outPath.Size() )
	//{
	//	return false;
	//}
	outPath.PushBack( safePoint );
	if ( !ImproveLink( safePoint, pos2, outPath, reservedNodes ) )
	{
		return false;
	}
	return true;
}

Bool CNavGraph::NodeSetsInterconnection()
{
	// TODO: make it better
	
	Bool dirty = false;
	Float maxNodesDistanceSq = m_maxNodesDistance*m_maxNodesDistance;
	// naive n^2 approach
	for ( auto it1 = m_nodeSets.Begin(), end = m_nodeSets.End(); it1 != end; ++it1 )
	{
		for ( auto it2 = it1+1; it2 != end; ++it2 )
		{
			CNavgraphNodeSet* nodeSet1 = it1->m_second;
			CNavgraphNodeSet* nodeSet2 = it2->m_second;

			// distance test - check if two node sets has any chance to connect to each other
			const Box& bb1 = nodeSet1->GetBBox();
			const Box& bb2 = nodeSet2->GetBBox();

			if ( bb1.SquaredDistance( bb2 ) > maxNodesDistanceSq )
			{
				continue;
			}

			// try to connect both node sets
			if ( nodeSet1->TryConnecting( nodeSet2 ) )
			{
				dirty = true;
			}
		}
	}

	if ( dirty )
	{
		++m_version;
	}

	return true;
}

Bool CNavGraph::CanMoveNode( CNavNode* node, Vector3& position, Bool linksInPointerForm )
{
	const Vector3& nodePosition = node->GetPosition();
	Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );

	if ( node->HaveAnyFlag( NF_BLOCKED ) )
	{
		CLineQueryData query( CT_DEFAULT, position, nodePosition );
		if ( !m_areaDescription->VSpatialQuery( query ) )
		{
			return false;
		}
	}
	else
	{
		CWideLineQueryData query( CT_DEFAULT, position, nodePosition, personalSpace );
		if ( !m_areaDescription->VSpatialQuery( query ) )
		{
			return false;
		}
	}
	

	for( ConstLinksIterator it( *node ); it; ++it )
	{
		const CPathLink& link = *it;
		CPathNode* nextNode = linksInPointerForm ? link.GetDestination() : GetNode( link.GetDestinationId() );
		if ( link.HaveAnyFlag( NF_BLOCKED ) )
		{
			CLineQueryData query( CT_DEFAULT, nextNode->GetPosition(), position );
			if ( !m_areaDescription->VSpatialQuery( query ) )
				return false;
		}
		else
		{
			CWideLineQueryData query( CT_DEFAULT, nextNode->GetPosition(), position, personalSpace );
			if ( !m_areaDescription->VSpatialQuery( query ) )
				return false;
		}
			
		
	}

	return true;
}

Bool CNavGraph::InitialConnectNeighbours( CNavNode::Index baseNodeIndex, const SInitialNeighbourData& data )
{
	Vector3 pos1, pos2;

	CNavNode::Index idx1 = baseNodeIndex;
	CNavNode::Index idx2 = data.m_neighbourId;

	{
		CNavNode* node1 = GetNode( idx1 );
		CNavNode* node2 = GetNode( idx2 );

		pos1 = node1->GetPosition();
		pos2 = node2->GetPosition();
	}
	
	CLineQueryData query( CT_NO_ENDPOINT_TEST, pos1, pos2 );
	if ( m_areaDescription->VSpatialQuery( query ) )
	{
		// simple and direct connection
		CNavNode* node1 = GetNode( idx1 );
		CNavNode* node2 = GetNode( idx2 );

		AddLink( *node1, CPathLink( idx2, CNavNode::INVALID_INDEX ) );
		AddLink( *node2, CPathLink( idx1, CNavNode::INVALID_INDEX ) );
	}
	else
	{
		// need to put additional node inside
		Vector3 position = data.m_centerOfMass;
		CLineQueryData query1( CT_NO_ENDPOINT_TEST, position, pos1 );
		CLineQueryData query2( CT_NO_ENDPOINT_TEST, position, pos2 );
		if ( !m_areaDescription->VTestLocation( position, CT_DEFAULT ) || 
			!m_areaDescription->VSpatialQuery( query1 ) || !m_areaDescription->VSpatialQuery( query2 )
			)
		{
			position = data.m_fallbackLocation;
			if ( !m_areaDescription->VTestLocation( position, CT_DEFAULT ) )
			{
				PATHLIB_ERROR( TXT("Cannot create connector node at location %f, %f, %f!\n"), position.X, position.Y, position.Z );
				return false;
			}
		}
		// TODO: handle this rare case
		//ASSERT( m_areaDescription->VTestLine( pos1, position, CT_DEFAULT ) && m_areaDescription->VTestLine( pos2, position, CT_DEFAULT ) );
		CNavNode* connectorNode = &AddNode( position, NF_DEFAULT );
		CNavNode* node1 = GetNode( idx1 );
		CNavNode* node2 = GetNode( idx2 );
		CNavNode::Index connectorNodeIndex = connectorNode->GetIndex();
		AddLink( *node1, CPathLink( connectorNodeIndex, CNavNode::INVALID_INDEX ) );
		AddLink( *connectorNode, CPathLink( idx1, CNavNode::INVALID_INDEX ) );
		AddLink( *connectorNode, CPathLink( idx2, CNavNode::INVALID_INDEX ) );
		AddLink( *node2, CPathLink( connectorNodeIndex, CNavNode::INVALID_INDEX ) );
	}
	return true;
}

void CNavGraph::ClearNodeFinder()
{
	m_nodeFinder.Clear();
}

void CNavGraph::ComputeNodeFinder()
{
	if ( !m_nodeFinder.IsInitialized() )
	{
		m_nodeFinder.Initialize();
	}
}

Bool CNavGraph::ConvertLinksToIds()
{
	m_nodesAreLinkedById = true;

	// first run - make all links run on pointers
	Super::ConvertLinksToIds();

	for ( auto itNodeSets = m_nodeSets.Begin(), endNodeSets = m_nodeSets.End(); itNodeSets != endNodeSets; ++itNodeSets )
	{
		CNavgraphNodeSet* nodeSet = itNodeSets->m_second;
		nodeSet->ConvertLinksToIds();
	}

	return true;
}

Bool CNavGraph::ConvertLinksToPointers()
{
	// first run - make all links run on pointers
	Super::ConvertLinksToPointers();
	
	for ( auto itNodeSets = m_nodeSets.Begin(), endNodeSets = m_nodeSets.End(); itNodeSets != endNodeSets; ++itNodeSets )
	{
		CNavgraphNodeSet* nodeSet = itNodeSets->m_second;
		nodeSet->ConvertLinksToPointers();
	}

	m_nodesAreLinkedById = false;

	return true;
}

void CNavGraph::CompactData()
{
	Bool nodesWereById = m_nodesAreLinkedById;
	if ( !nodesWereById )
	{
		ConvertLinksToIds();
	}

	m_nodeFinder.CompactData();

	Super::CompactData();

	for ( auto itNodeSets = m_nodeSets.Begin(), endNodeSets = m_nodeSets.End(); itNodeSets != endNodeSets; ++itNodeSets )
	{
		CNavgraphNodeSet* nodeSet = itNodeSets->m_second;
		nodeSet->CompactData();
	}

	if ( !nodesWereById )
	{
		ConvertLinksToPointers();
	}
}

void CNavGraph::CalculateAllLinksCost()
{
	// first run - make all links run on pointers
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		CNavNode& node = *itNodes;
		for ( LinksIterator itLinks( node ); itLinks; ++itLinks )
		{
			CPathLink& link = *itLinks;
			// custom links has custom costs
			if ( link.HaveFlag( NF_IS_CUSTOM_LINK ) )
			{
				continue;
			}
			// check if link was processed already
			if ( node.GetFullId() < link.GetDestination()->GetFullId() )
			{
				Float costMult = 1.f;
				if ( link.HaveAnyFlag( NF_ROUGH_TERRAIN ) )
				{
					costMult = 2.f;
				}
				// calculate link cost
				CPathLinkModifier modifier( node, link );
				modifier.SetCost( CalculateLinkCost( (node.GetPosition() - link.GetDestination()->GetPosition()).Mag() * costMult ) );
			}
		}
	}
}

Uint32 CNavGraph::ComputeBinariesVersion()
{
	// To compute binaries version, we just virtually save everything and compare binaries hash.
	// NOTICE: Wasteful approach, but we don't care much as its cooker only stuff, and we are far from being main offender here.
	TDynArray< Int8 > buffer;
	CSimpleBufferWriter writer( buffer, CAreaNavgraphs::RES_VERSION );
	WriteToBuffer( writer );

	Red::System::CRC32 crc;
	return crc.Calculate( buffer.Data(), Uint32( buffer.DataSize() ) );
}

Bool CNavGraph::VAreNodesLinkedById() const
{
	return m_nodesAreLinkedById;
}

AreaId CNavGraph::VGetAreaId() const
{
	return m_areaDescription->GetId();
}
CNavNode& CNavGraph::AddNode( const Vector3& position, NodeFlags flags )
{
	CNavNode& navNode = Super::AddNode( position, m_areaDescription->GetId(), flags );
	if ( m_nodeFinder.IsInitialized() )
	{
		m_nodeFinder.AddDynamicElement( &navNode );
	}
	return navNode;
}
void CNavGraph::OnNodeArrayOverflow()
{
	m_nodeFinder.Clear();
}

CPathLibWorld* CNavGraph::VGetPathLibWorld() const
{
	return &m_areaDescription->GetPathLib();
}

Bool CNavGraph::Debug_CheckAllLinksTwoSided()
{
	if ( !Super::Debug_CheckAllLinksTwoSided() )
	{
		return false;
	}

	for ( auto it = m_nodeSets.Begin(), end = m_nodeSets.End(); it != end; ++it )
	{
		if ( !it->m_second->Debug_CheckAllLinksTwoSided() )
		{
			return false;
		}
	}
	return true;
}

void CNavGraph::Debug_MakeAllLinksWalkable()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		CNavNode& node = *it;
		for ( LinksIterator itLinks( node ); itLinks; ++itLinks )
		{
			CPathLink& link = *itLinks;
			CPathLinkModifier modifier( node, link );
			modifier.ClearFlags( NF_BLOCKED );
		}
	}
}

CNavNode* CNavGraph::FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace )
{
	return m_nodeFinder.FindClosestNodeWithLinetest( pos, m_maxNodesDistance, personalSpace, CCentralNodeFinder::AnyRegion( false, true ) );
}
CNavNode* CNavGraph::FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace, CoherentRegion regionId )
{
	return m_nodeFinder.FindClosestNodeWithLinetest( pos, m_maxNodesDistance, personalSpace, CCentralNodeFinder::SpecyficRegion( AreaRegionIdFromCoherentRegion( regionId ) ) );
}
CNavNode* CNavGraph::FindClosestAccessibleNode( CAreaDescription* area, const Vector3& pos, Float personalSpace, Float toleranceRadius, Vector3& outAccessiblePosition )
{
	return m_nodeFinder.FindClosestNodeWithLinetestAndTolerance( pos, toleranceRadius, personalSpace, outAccessiblePosition, CCentralNodeFinder::AnyRegion( false, false ) );
}

CNavNode* CNavGraph::FindRandomNodeAround( CAreaDescription* area, const Vector3& pos, Float searchRadius )
{
	return m_nodeFinder.FindClosestNode( pos, searchRadius, CCentralNodeFinder::AnyRegion( false, true ) );
}

CNavGraph::CNavGraph( Uint32 category, CAreaDescription* area )
	: Super()
	, m_areaDescription( area )
	, m_nodeFinder( *this )
	, m_category( Uint16( category ) )
	, m_nodesAreLinkedById( true )
	, m_binaryVersion( INVALID_BINARIES_VERSION )
	, m_hlGraph( *this )
{
	m_maxNodesDistance = area ? area->GetMaxNodesDistance() : -1.f;
}

CNavGraph::~CNavGraph()
{
	for ( auto it = m_nodeSets.Begin(), end = m_nodeSets.End(); it != end; ++it )
	{
		delete it->m_second;
	}
}

//void CNavGraph::CollectConnectors( AreaId idDestinationArea, TDynArray< Vector3 >& outConnectorsLocations )
//{
//	for ( auto itArea = m_connectors.Begin(), endArea = m_connectors.End(); itArea != endArea; ++itArea )
//	{
//		const SConnectorData::AreaData& areaData = *itArea;
//		if ( areaData.m_areaId == idDestinationArea )
//		{
//			for ( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
//			{
//				CNavNode* node = GetNode( itCon->m_nodeIdx );
//				outConnectorsLocations.PushBack( node->GetPosition() );
//			}
//		}
//	}
//}
//
//void CNavGraph::SetInstanceConnectors( const TDynArray< TPair< Vector3, AreaId > >& locations )
//{
//	ConvertLinksToIds();
//
//	if ( !GIsCooker )
//	{
//		Bool deletedConnectors = false;
//		// delete all instance connector nodes
//		for ( Uint32 i = 0; i < m_connectorsLegacy.Size(); )
//		{
//			const auto& data = m_connectorsLegacy[ i ];
//			if ( (data.m_neighbourId & CAreaDescription::ID_MASK_TERRAIN) == 0 )
//			{
//				// delete connection
//				MarkNodeForDeletion( data.m_nodeId );
//				m_connectorsLegacy.RemoveAtFast( i );
//				deletedConnectors = true;
//			}
//			else
//			{
//				++i;
//			}
//		}
//		if ( deletedConnectors )
//		{
//			DeleteMarked();
//		}
//	}
//
//	const Float personalSpace = m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );
//	// spawn new instance connector nodes
//	for ( auto it = locations.Begin(), end = locations.End(); it != end; ++it )
//	{
//		const Vector3& position = (*it).m_first;
//		CCircleQueryData query( CT_IGNORE_OTHER_AREAS, position, personalSpace );
//		if ( m_areaDescription->VSpatialQuery( query ) )
//		{
//			CNodeFinder::OutputVector connectableNodes;
//
//			if ( m_nodeFinder.FindNClosestNodesWithLinetest( position, GetMaxNodesDistance(), personalSpace, 3, connectableNodes, CCentralNodeFinder::AnyRegion( true, false ), CT_IGNORE_OTHER_AREAS, NFG_FORBIDDEN_BY_DEFAULT | NF_IS_IN_NODESET ) )
//			{
//				CNavNode& node = AddNode( position, NF_CONNECTOR );
//
//				SCookedConnectorData data;
//				data.m_nodeId = node.GetIndex();
//				data.m_neighbourId = (*it).m_second;
//				data.m_neighbourNodeIdx = CNavNode::INVALID_INDEX;
//				m_connectorsLegacy.PushBack( data );
//
//				for ( auto it = connectableNodes.Begin(), end = connectableNodes.End(); it != end; ++it )
//				{
//					ASSERT( (*it).m_nodeSetIndex == CNavNode::INVALID_INDEX );
//					GenerationConnectNode( node.GetIndex(), (*it).m_index );
//				}
//			}
//		}
//	}
//	Sort( m_connectorsLegacy.Begin(), m_connectorsLegacy.End(), SCookedConnectorData::Comperator() );
//
//	ConvertLinksToPointers();
//	
//	ComputeNodeFinder();
//
//	++m_version;
//}

Bool CNavGraph::TryToConnectExternalConnector( CNavNode* externalConnector, CAreaDescription* fromArea )
{
	Float personalSpace = GetPersonalSpace();

	Float closestConnectorSq = (2 * m_areaDescription->GetMaxNodesDistance());
	closestConnectorSq *= closestConnectorSq;

	AreaId idFromArea = fromArea->GetId();

	CNavNode* internalConnector = NULL;
	Vector3 externalPosition = externalConnector->GetPosition();
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		const auto& areaData = *itArea;
		if ( areaData.m_areaId != idFromArea )
		{
			continue;
		}

		for( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
		{
			const auto& data = *itCon;
			CNavNode* node = GetNode( data.m_nodeIdx );
			Float distSq = (externalPosition - node->GetPosition()).SquareMag();
			if ( distSq < closestConnectorSq )
			{
				CWideLineQueryData query1( CT_IGNORE_OTHER_AREAS, node->GetPosition(), externalPosition, personalSpace );
				CWideLineQueryData query2( CT_IGNORE_OTHER_AREAS, externalPosition, node->GetPosition(), personalSpace );
				if ( m_areaDescription->VSpatialQuery( query1 ) && fromArea->VSpatialQuery( query2 ) )
				{
					closestConnectorSq = distSq;
					internalConnector = node;
				}
			}
		}
	}
	//for ( auto it = m_connectorsLegacy.Begin(), end = m_connectorsLegacy.End(); it != end; ++it )
	//{
	//	const auto& data = *it;
	//	if ( data.m_neighbourId == idFromArea )
	//	{
	//		CNavNode* node = GetNode( data.m_nodeId );
	//		Float distSq = (externalPosition - node->GetPosition()).SquareMag();
	//		if ( distSq < closestConnectorSq )
	//		{
	//			CWideLineQueryData query1( CT_IGNORE_OTHER_AREAS, node->GetPosition(), externalPosition, personalSpace );
	//			CWideLineQueryData query2( CT_IGNORE_OTHER_AREAS, externalPosition, node->GetPosition(), personalSpace );
	//			if ( m_areaDescription->VSpatialQuery( query1 ) && fromArea->VSpatialQuery( query2 ) )
	//			{
	//				closestConnectorSq = distSq;
	//				internalConnector = node;
	//			}
	//		}
	//	}
	//}
	if ( internalConnector )
	{
		NavLinkCost linkCost = CalculateLinkCost( sqrt( closestConnectorSq ) );
		AddLink( *externalConnector, CPathLink( internalConnector, NF_CONNECTOR, linkCost ) );
		AddLink( *internalConnector, CPathLink( externalConnector, NF_CONNECTOR, linkCost ) );
		return true;
	}
	return false;
}

Bool CNavGraph::TryToCreateConnectors( CAreaDescription* area1, CAreaDescription* area2, const TDynArray< Vector3 >& locations )
{
	struct Local
	{
		static Bool ConnectNodes( const Vector3& position, Float personalSpace, CNavGraph* g1, CNavGraph* g2, SConnectorData::AreaData& areaData1, SConnectorData::AreaData& areaData2, Bool failIfNotConnected, CNavNode** outNode1 = nullptr, CNavNode** outNode2 = nullptr )
		{
			CNodeFinder::OutputVector connectableNodes1;
			CNodeFinder::OutputVector connectableNodes2;

			CCentralNodeFinder::RegionAcceptor region = CCentralNodeFinder::AnyRegion( true, false );

			if ( (!g1->GetNodeFinder().FindNClosestNodesWithLinetest( position, g1->GetMaxNodesDistance(), personalSpace, 3, connectableNodes1, region, CT_IGNORE_OTHER_AREAS, NFG_FORBIDDEN_BY_DEFAULT | NF_IS_IN_NODESET ) && failIfNotConnected )
				|| (!g2->GetNodeFinder().FindNClosestNodesWithLinetest( position, g2->GetMaxNodesDistance(), personalSpace, 3, connectableNodes2, region, CT_IGNORE_OTHER_AREAS, NFG_FORBIDDEN_BY_DEFAULT | NF_IS_IN_NODESET ) && failIfNotConnected ) )
			{
				return false;
			}

			// create new nodes
			CNavNode& node1 =
				( outNode1 && (*outNode1)->GetNodesetIndex() == CNavNode::INVALID_INDEX )
				? **outNode1
				: g1->AddNode( position, NF_CONNECTOR );
			CNavNode& node2 =
				( outNode2 && (*outNode2)->GetNodesetIndex() == CNavNode::INVALID_INDEX )
				? **outNode2
				: g2->AddNode( position, NF_CONNECTOR );

			// connect new nodes to existing navgraphs
			for ( auto it = connectableNodes1.Begin(), end = connectableNodes1.End(); it != end; ++it )
			{
				CNavNode* destNode = g1->GetNode( *it );

				NavLinkCost cost = CalculateLinkCost( (position - destNode->GetPosition()).Mag() );
				g1->GenerationConnectNode( node1.GetIndex(), (*it).m_index, NF_DEFAULT, cost );
			}
			for ( auto it = connectableNodes2.Begin(), end = connectableNodes2.End(); it != end; ++it )
			{
				CNavNode* destNode = g2->GetNode( *it );

				NavLinkCost cost = CalculateLinkCost( (position - destNode->GetPosition()).Mag() );
				g2->GenerationConnectNode( node2.GetIndex(), (*it).m_index, NF_DEFAULT, cost );
			}

			// add connectors
			{
				SConnectorData::Connector c;

				c.m_nodeIdx = node1.GetIndex();
				c.m_neighbourNodeIdx = node2.GetIndex();

				areaData1.m_connectors.PushBack( c );
			}
			{
				SConnectorData::Connector c;

				c.m_nodeIdx = node2.GetIndex();
				c.m_neighbourNodeIdx = node1.GetIndex();

				areaData2.m_connectors.PushBack( c );
			}

			if ( outNode1 )
			{
				*outNode1 = &node1;
			}
			if ( outNode2 )
			{
				*outNode2 = &node2;
			}
			return true;
		}
	};

	CGlobalConnectorsBin* globalData = area1->GetPathLib().GetCookingContext()->GetPathlibCookerData()->GetGlobalConnections();

	Bool success = false;
	for ( Uint32 category = 0; category < MAX_ACTOR_CATEGORIES; ++category )
	{
		CNavGraph* g1 = area1->GetNavigationGraph( category );
		if ( !g1 )
		{
			continue;
		}
		CNavGraph* g2 = area2->GetNavigationGraph( category );
		if ( !g2 )
		{
			continue;
		}

		SConnectorData::AreaData areaData1;
		SConnectorData::AreaData areaData2;

		areaData1.m_areaId = area2->GetId();
		areaData1.m_navgraphBinaryVersion = g2->GetBinariesVersion();
		areaData1.m_connected = false;

		areaData2.m_areaId = area1->GetId();
		areaData2.m_navgraphBinaryVersion = g1->GetBinariesVersion();
		areaData2.m_connected = false;

		Float personalSpace = g1->GetPersonalSpace();

		g1->ConvertLinksToIds();
		g2->ConvertLinksToIds();

		for ( auto it = locations.Begin(), end = locations.End(); it != end; ++it )
		{
			const Vector3& position = *it;

			// check if connector location is accessible in both areas
			{
				CCircleQueryData query( CT_IGNORE_OTHER_AREAS, position, personalSpace );
				if ( !area1->VSpatialQuery( query ) )
				{
					continue;
				}
			}
			{
				CCircleQueryData query( CT_IGNORE_OTHER_AREAS, position, personalSpace );
				if ( !area2->VSpatialQuery( query ) )
				{
					continue;
				}
			}

			if ( !Local::ConnectNodes( position, personalSpace, g1, g2, areaData1, areaData2, true ) )
			{
				continue;
			}
			
			// yay we did it
			success = true;
		}

		// now lets go through nodesets as they can contain possible connections
		CGlobalConnectorsBin::Iterator it1( *globalData, category, area1->GetId(), area2->GetId() );
		CGlobalConnectorsBin::Iterator it2( *globalData, category, area2->GetId(), area1->GetId() );

		while ( it1 )
		{
			CGlobalConnectorsBin::Connection& con1 = *it1;

			if ( con1.m_category != category )
			{
				continue;
			}

			CNavNode* node1 = g1->GetNode( con1.m_idFrom );
			ASSERT( node1 );
			const Vector3& pos1 = node1->GetPosition();

			while ( it2 )
			{
				CGlobalConnectorsBin::Connection& con2 = *it2;

				if ( con2.m_category != category )
				{
					continue;
				}

				CNavNode* node2 = g2->GetNode( con2.m_idFrom );
				ASSERT( node2 );
				const Vector3& pos2 = node2->GetPosition();

				if ( ( pos1 - pos2 ).SquareMag() < 0.1f )
				{
					CNavNode* createdNode1 = node1;
					CNavNode* createdNode2 = node2;

					Local::ConnectNodes( pos1, personalSpace, g1, g2, areaData1, areaData2, false, &createdNode1, &createdNode2 );

					if ( node1 != createdNode1 )
					{
						g1->VGenerationConnectNodes( *node1, *createdNode1, NF_IS_GHOST_LINK, 0 );
					}
					if ( node2 != createdNode2 )
					{
						g2->VGenerationConnectNodes( *node2, *createdNode2, NF_IS_GHOST_LINK, 0 );
					}

					break;
				}

				++it2;
			}

			it2.Reset();

			++it1;
		}

		if ( !areaData1.m_connectors.Empty() )
		{
			areaData1.Sort();
			g1->m_connectors.m_areas.PushBack( areaData1 );
		}

		if ( !areaData2.m_connectors.Empty() )
		{
			areaData2.Sort();
			g2->m_connectors.m_areas.PushBack( areaData2 );
		}

		g1->ConvertLinksToPointers();
		g2->ConvertLinksToPointers();
	}

	return success;
}

CNavgraphNodeSet* CNavGraph::CreateNodeSet()
{
	CPathNode::NodesetIdx idx = GetUniqueNodeSetId();
	CNavgraphNodeSet* nodeSet = new CNavgraphNodeSet( this, idx );
	// optimization - reserve some default nodes count, so we won't hit reallocation too much that would invalidate nodefinder
	nodeSet->ReserveNodes( 32 );
	m_nodeSets.Insert( idx, nodeSet );
	return nodeSet;
}
CNavgraphNodeSet* CNavGraph::GetNodeSet( CPathNode::NodesetIdx idx )
{
	auto itFind = m_nodeSets.Find( idx );
	if ( itFind != m_nodeSets.End() )
	{
		return itFind->m_second;
	}
	return NULL;
}
void CNavGraph::ClearNodeSet( CPathNode::NodesetIdx idx )
{
	auto itFind = m_nodeSets.Find( idx );
	if ( itFind != m_nodeSets.End() )
	{
		CNavgraphNodeSet* nodeSet = itFind->m_second;
		Bool wasAttached = nodeSet->IsAttached();
		nodeSet->Clear();
		delete nodeSet;
		m_nodeSets.Erase( itFind );
		if ( wasAttached )
		{
			m_nodeFinder.Invalidate();
		}
	}
}

const CNavNode* CNavGraph::GetNode( CPathNode::Id id ) const
{
	if ( id.m_nodeSetIndex != CPathNode::INVALID_INDEX )
	{
		auto itFind = m_nodeSets.Find( id.m_nodeSetIndex );
		if ( itFind == m_nodeSets.End() )
		{
			return NULL;
		}
		return itFind->m_second->GetNode( id.m_index );
	}
	return &m_nodes[ id.m_index ];
}

CNavNode* CNavGraph::GetNode( CPathNode::Id id )
{
	if ( id.m_nodeSetIndex != CPathNode::INVALID_INDEX )
	{
		auto itFind = m_nodeSets.Find( id.m_nodeSetIndex );
		if ( itFind == m_nodeSets.End() )
		{
			return NULL;
		}
		return itFind->m_second->GetNode( id.m_index );
	}
	return &m_nodes[ id.m_index ];
}

CPathNode* CNavGraph::VGetExternalPathNode( CPathNode::Index idx, AreaId areaId )
{
	CAreaDescription* extArea = m_areaDescription->GetPathLib().GetAreaDescription( areaId );
	if ( !extArea )
	{
		return nullptr;
	}
	CNavGraph* extGraph = extArea->GetNavigationGraph( m_category );
	if ( !extGraph )
	{
		return nullptr;
	}

	return extGraph->GetNode( idx );
}

LinkBufferIndex CNavGraph::VGetExtraLinksSpace() const
{
	return 128;
}

CNavNode::NodesetIdx CNavGraph::VGetNodesetIndex() const
{
	return CNavNode::INVALID_INDEX;
}

CPathNode* CNavGraph::VGetPathNode( CPathNode::Id id )
{
	return GetNode( id );
}

Uint32 CNavGraph::GetTotalNodesCount() const
{
	Uint32 totalCount = m_nodes.Size();
	for ( auto it = m_nodeSets.Begin(), end = m_nodeSets.End(); it != end; ++it )
	{
		totalCount += it->m_second->GetNodesArray().Size();
	}
	return totalCount;
}

CPathNode::NodesetIdx CNavGraph::GetUniqueNodeSetId() const
{
	for ( ;; )
	{
		CNavgraphNodeSet::Id id = GEngine->GetRandomNumberGenerator().Get< CNavgraphNodeSet::Id >();
		auto itFind = m_nodeSets.Find( id );
		if ( itFind == m_nodeSets.End() )
		{
			return id;
		}
	}
	ASSUME( false );
}

Bool CNavGraph::ConnectWithNeighbour( SConnectorData::AreaData& areaData, Bool mutualAttach )
{
	Bool ret = false;

	CPathLibWorld& pathlib = m_areaDescription->GetPathLib();
	CAreaDescription* area = pathlib.GetAreaDescription( areaData.m_areaId );

	if ( area && !area->IsBeingLoaded() )
	{
		CNavGraph* otherAreaNavgraph = area->GetNavigationGraph( m_category );
		if ( otherAreaNavgraph )
		{
			areaData.m_connected = true;

			if( otherAreaNavgraph->GetBinariesVersion() != INVALID_BINARIES_VERSION && otherAreaNavgraph->GetBinariesVersion() == areaData.m_navgraphBinaryVersion )
			{
				// all cooked! performance friendly connection method
				for( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
				{
					const auto& connectorData = *itCon;

					CNavNode* connectorNode = GetNode( connectorData.m_nodeIdx );
					Bool isConnected = false;
					for( LinksIterator itLinks( *connectorNode ); itLinks; ++itLinks )
					{
						if ( (*itLinks).HaveAnyFlag( NF_CONNECTOR ) )
						{
							isConnected = true;
							break;
						}
					}
					if ( isConnected )
					{
						ret = true;
						continue;
					}

					CNavNode* externalNode = otherAreaNavgraph->GetNode( connectorData.m_neighbourNodeIdx );
					NavLinkCost linkCost = 1;
					AddLink( *externalNode, CPathLink( connectorNode, NF_CONNECTOR, linkCost ) );
					AddLink( *connectorNode, CPathLink( externalNode, NF_CONNECTOR, linkCost ) );

					if ( connectorNode->GetAreaRegionId() != INVALID_AREA_REGION && externalNode->GetAreaRegionId() != INVALID_AREA_REGION )
					{
						m_hlGraph.ConnectRegions( connectorNode->GetAreaRegionId(), area->GetId(), externalNode->GetAreaRegionId(), NF_CONNECTOR );
					}

					ret  = true;
				}

				if ( mutualAttach )
				{
					otherAreaNavgraph->MarkConnectedWithNeighbour( m_areaDescription->GetId(), true );
				}
			}
			else
			{
				// no cooked data. switch to fallback algorithm version
				for( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
				{
					const auto& connectorData = *itCon;

					CNavNode* connectorNode = GetNode( connectorData.m_nodeIdx );
					Bool isConnected = false;
					for( LinksIterator itLinks( *connectorNode ); itLinks; ++itLinks )
					{
						if ( (*itLinks).HaveAnyFlag( NF_CONNECTOR ) )
						{
							isConnected = true;
							break;
						}
					}
					if ( isConnected )
					{
						ret = true;
						continue;
					}

					// try to connect 2 graphs
					ret = otherAreaNavgraph->TryToConnectExternalConnector( connectorNode, m_areaDescription ) || ret;
				}

				if ( mutualAttach )
				{
					otherAreaNavgraph->ConnectWithNeighbour( m_areaDescription->GetId(), false );
				}
			}
#ifndef RED_FINAL_BUILD
			MarkVersionDirty();
			otherAreaNavgraph->MarkVersionDirty();
#endif
		}
	}

	return ret;
}

Bool CNavGraph::ConnectWithNeighbour( AreaId areaId, Bool mutualAttach )
{
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		SConnectorData::AreaData& areaData = *itArea;

		if ( areaData.m_areaId != areaId )
		{
			continue;
		}
		return ConnectWithNeighbour( areaData, mutualAttach );
	}
	return false;
}



Bool CNavGraph::ConnectWithNeighbours()
{
	Bool ret = false;
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		SConnectorData::AreaData& areaData = *itArea;

		if ( areaData.m_connected )
		{
			continue;
		}

		ConnectWithNeighbour( areaData, true );
	}

	return ret;
}

void CNavGraph::DetachFromNeighbour( SConnectorData::AreaData& areaData, Bool mutualDetach )
{
	CPathLibWorld& pathlib = m_areaDescription->GetPathLib();

	areaData.m_connected = false;

	for( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
	{
		const auto& connectorData = *itCon;

		CNavNode* node = GetNode( connectorData.m_nodeIdx );
		for( LinksErasableIterator it( *node ); it; )
		{
			if ( (*it).HaveFlag( NF_CONNECTOR ) )
			{
				node->EraseLink( it );
			}
			else
			{
				++it;
			}
		}
	}

	if ( mutualDetach )
	{
		CAreaDescription* area = pathlib.GetAreaDescription( areaData.m_areaId );

		if ( area && !area->IsBeingLoaded() )
		{
			CNavGraph* otherAreaNavgraph = area->GetNavigationGraph( m_category );
			if ( otherAreaNavgraph )
			{
			
				otherAreaNavgraph->DetachFromNeighbour( m_areaDescription->GetId(), false );
			}
		}
	}
}

void CNavGraph::DetachFromNeighbour( AreaId areaId, Bool mutualDetach )
{
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		SConnectorData::AreaData& areaData = *itArea;

		if ( areaData.m_areaId == areaId )
		{
			DetachFromNeighbour( areaData, mutualDetach );
			break;
		}
	}

}

void CNavGraph::DetachFromNeighbours()
{
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		auto& areaData = *itArea;

		if ( !areaData.m_connected )
		{
			continue;
		}

		DetachFromNeighbour( areaData, true );
	}

	m_hlGraph.UnlinkFromHLGraph();
}

void CNavGraph::ClearConnectors()
{
	if ( m_connectors.m_areas.Empty() )
	{
		return;
	}
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		const auto& areaData = *itArea;
		for( auto itCon = areaData.m_connectors.Begin(), endCon = areaData.m_connectors.End(); itCon != endCon; ++itCon )
		{
			const auto& connectorData = *itCon;

		MarkNodeForDeletion( connectorData.m_nodeIdx );
		}
	}
	m_connectors.m_areas.Clear();

	ConvertLinksToIds();
	DeleteMarked();
	ConvertLinksToPointers();
	// TODO: should or should not ComputeNodeFinder();
}

void CNavGraph::MarkConnectedWithNeighbour( AreaId neighbourAreaId, Bool connected )
{
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		if ( itArea->m_areaId == neighbourAreaId )
		{
			itArea->m_connected = connected;
			break;
		}
	}
}

void CNavGraph::Unload()
{
	for ( auto it = m_nodeSets.Begin(), end = m_nodeSets.End(); it != end; ++it )
	{
		CNavgraphNodeSet* nodeSet = it->m_second;
		nodeSet->Clear();
		delete nodeSet;
	}
	m_nodeSets.ClearFast();

	Super::Unload();

	m_connectors.m_areas.ClearFast();
}

void CNavGraph::WriteToBuffer( CSimpleBufferWriter& writer )
{
	writer.Put( m_binaryVersion );

	Uint16 areasCount = Uint16( m_connectors.m_areas.Size() );
	writer.Put( areasCount );
	for ( auto itArea = m_connectors.m_areas.Begin(), endArea = m_connectors.m_areas.End(); itArea != endArea; ++itArea )
	{
		const auto& areaData = *itArea;
		writer.Put( areaData.m_areaId );
		writer.Put( areaData.m_navgraphBinaryVersion );
		writer.SmartPut( areaData.m_connectors );
	}

	Super::WriteToBuffer( writer );

	CNavgraphNodeSet::Id nodeSetCount = CNavgraphNodeSet::Id( m_nodeSets.Size() );
	writer.Put( nodeSetCount );
	for ( CNavgraphNodeSet::Id i = 0; i < nodeSetCount; ++i )
	{
		CNavgraphNodeSet::Id id = m_nodeSets[ i ].m_first;
		writer.Put( id );
		m_nodeSets[ i ].m_second->WriteToBuffer( writer );
	}

	// save coherent regions
	m_hlGraph.WriteHLToBuffer( writer );
}
Bool CNavGraph::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.Get( m_binaryVersion ) )
	{
		return false;
	}

	Uint16 areasCount;
	if ( !reader.Get( areasCount ) )
	{
		return false;
	}

	m_connectors.m_areas.Resize( areasCount );

	for ( Uint16 i = 0; i < areasCount; ++i )
	{
		SConnectorData::AreaData& areaData = m_connectors.m_areas[ i ];
		if (
			!reader.Get( areaData.m_areaId ) ||
			!reader.Get( areaData.m_navgraphBinaryVersion ) ||
			!reader.SmartGet( areaData.m_connectors ) )
		{
			return false;
		}
		areaData.m_connected = false;
	}
	
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	CNavgraphNodeSet::Id nodeSetCount;
	if  ( !reader.Get( nodeSetCount ) )
	{
		return false;
	}
	
	for ( CNavgraphNodeSet::Id i = 0; i < nodeSetCount; ++i )
	{
		CNavgraphNodeSet::Id id;
		if ( !reader.Get( id ) )
		{
			return false;
		}
		
		CNavgraphNodeSet* nodeSet = new CNavgraphNodeSet( this, id );
		if ( !nodeSet->ReadFromBuffer( reader ) )
		{
			delete nodeSet;
			return false;
		}
		m_nodeSets.Insert( id, nodeSet );
	}

	ConvertLinksToPointers();

	if ( !m_hlGraph.ReadHLFromBuffer( reader ) )
	{
		return false;
	}
	
	return true;
}

void CNavGraph::OnPostLoad( CAreaDescription* area )
{
	Super::OnPostLoad( area );

	m_areaDescription = area;
	m_maxNodesDistance = area->GetMaxNodesDistance();

	CNavgraphNodeSet::Id nodeSetCount = CNavgraphNodeSet::Id( m_nodeSets.Size() );
	for ( CNavgraphNodeSet::Id i = 0; i < nodeSetCount; ++i )
	{
		m_nodeSets[ i ].m_second->OnPostLoad( area );
	}

	m_hlGraph.OnHLPostLoad();

	ComputeNodeFinder();
}

Float CNavGraph::GetPersonalSpace() const
{
	return m_areaDescription->GetPathLib().GetGlobalSettings().GetCategoryPersonalSpace( m_category );
}

void CNavGraph::ComputeCoherentRegionsMarking()
{
	struct ComputeRegions : public Red::System::NonCopyable
	{
		CNavGraph* m_me;
		CHLSubGraph& m_hlGraph;
		CSearchEngine& m_searchEngine;

		ComputeRegions( CNavGraph* me, CHLSubGraph& hlGraph, CSearchEngine& searchEngine )
			: m_me( me ), m_hlGraph( hlGraph ), m_searchEngine( searchEngine ) {}

		void operator()( CNavNode& node )
		{
			if ( node.HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
			{
				return;
			}
			if ( node.GetRegionId() == INVALID_COHERENT_REGION )
			{
				AreaRegionId regionId = m_hlGraph.GetNextUniqueRegionId();

				NodeFlags forbiddenFlags = NFG_BREAKS_COHERENT_REGION;
				NodeFlags specialFlags = 0;

				if ( node.HaveAnyFlag( NFG_BREAKS_COHERENT_REGION ) )
				{
					specialFlags = node.GetFlags() & ( NFG_BREAKS_COHERENT_REGION & ( ~NFG_FORBIDDEN_ALWAYS ) );
					forbiddenFlags &= ~specialFlags;
				}

				forbiddenFlags |= NF_CONNECTOR;

				CHLNode& hlNode = m_hlGraph.AddHLNode( regionId );

				CRegionFloodFillData paintData( &node, m_me, regionId, forbiddenFlags, specialFlags );

				m_searchEngine.RegionFloodFill( paintData, m_me, &hlNode );
			}
		}
	};

	struct ConnectRegions : public Red::System::NonCopyable
	{
		CNavGraph* m_me;
		CHLSubGraph& m_hlGraph;

		ConnectRegions( CNavGraph* me, CHLSubGraph& hlGraph )
			: m_me( me ), m_hlGraph( hlGraph ) {}

		void operator()( CNavNode& node )
		{
			if ( node.HaveAnyFlag( NFG_FORBIDDEN_ALWAYS ) )
			{
				return;
			}

			AreaRegionId currRegion = node.GetAreaRegionId();
			if ( currRegion == INVALID_AREA_REGION )
			{
				// ignore for unaccessible nodes
				return;
			}

			// iterate all connections looking for connections with different regions
			for ( LinksIterator it( node ); it; ++it )
			{
				const CPathLink& link = *it;
				const CPathNode* destNode = link.GetDestination();
				AreaRegionId destRegion = destNode->GetAreaRegionId();
				// check if h
				if ( destRegion == currRegion || destRegion == INVALID_AREA_REGION )
				{
					continue;
				}

				// if link is ever acceptable
				if ( link.HaveAnyFlag( NFG_FORBIDDEN_ALWAYS | NF_CONNECTOR ) )
				{
					continue;
				}
				m_hlGraph.ConnectRegions( currRegion, destRegion, link.GetFlags() );
			}
		}
	};

	CPathLibWorld& pathlib = m_areaDescription->GetPathLib();
	CHLSubGraph& hlGraph = m_hlGraph;
	CSearchEngine& searchEngine = pathlib.GetSearchEngine();

	{
		ComputeRegions f( this, hlGraph, searchEngine );

		NavgraphHelper::ForAllNodes( *this, f, true );
	}

	

	{
		ConnectRegions f( this, hlGraph );

		NavgraphHelper::ForAllNodes( *this, f, true );
	}
}

#ifndef NO_EDITOR_PATHLIB_SUPPORT
Bool CNavGraph::MarkSpecialZones( CGlobalWater* water, CSpecialZonesMap* specialZones )
{
	struct Fun : public Red::System::NonCopyable
	{
		CNavGraph*				m_this;
		CGlobalWater*			m_water;
		CSpecialZonesMap*		m_specialZones;

		Fun( CNavGraph* me, CGlobalWater* water, CSpecialZonesMap* specialZones )
			: m_this( me )
			, m_water( water )
			, m_specialZones( specialZones ) {}

		void GlobalWaterComputation( CNavNode& node )
		{
			Vector3 nodePos = node.GetPosition();
			Float nodeWaterLevel = m_water->GetWaterLevelBasic( nodePos.X, nodePos.Y );

			if ( nodePos.Z < nodeWaterLevel - 0.1f )
			{
				node.AddFlags( NF_ROUGH_TERRAIN );
				node.AddFlagsToLinks( NF_ROUGH_TERRAIN );
				return;
			}

			Bool isEveryLinkInWater = true;

			for ( LinksIterator it( node ); it; ++it )
			{
				CPathLink& link = *it;

				if ( link.HaveFlag( NF_ROUGH_TERRAIN ) )
				{
					continue;
				}

				if ( link.HaveAnyFlag( NF_IS_GHOST_LINK | NF_IS_CUSTOM_LINK ) )
				{
					isEveryLinkInWater = false;
					continue;
				}

				Bool isLinkInWater = false;

				Vector3 destinationPos = link.GetDestination()->GetPosition();

				// water test
				const Float SAMPLES_DIST = 0.75f;

				Vector3 diff = destinationPos - nodePos;

				Float linkLength = diff.AsVector2().Mag();

				Float samples = ceilf( linkLength / SAMPLES_DIST );
				for ( Float i = 0; i < samples; ++i )
				{
					Float ratio = i / samples;

					Vector samplePos;
					samplePos.AsVector3() = nodePos + diff * ratio;
					if ( !m_this->m_areaDescription->VComputeHeight( samplePos.AsVector3(), samplePos.Z ) )
					{
						m_this->m_areaDescription->VComputeHeightFrom( samplePos.AsVector2(), nodePos, samplePos.Z );
					}

					Float nodeWaterLevel = m_water->GetWaterLevelBasic( samplePos.X, samplePos.Y );
					if ( samplePos.Z < nodeWaterLevel - 0.1f )
					{
						isLinkInWater = true;
						break;
					}
				}

				if ( isLinkInWater )
				{
					CPathLinkModifier modifier( node, link );
					modifier.AddFlags( NF_ROUGH_TERRAIN );
				}
				else
				{
					isEveryLinkInWater = false;
				}

			}
			if ( isEveryLinkInWater )
			{
				node.AddFlags( NF_ROUGH_TERRAIN );
			}
		}

		void operator()( CNavNode& node )
		{
			// water computation
			if ( m_water )
			{
				GlobalWaterComputation( node );
			}

			Vector3 nodePos = node.GetPosition();

			// special zones computation
			{
				NodeFlags clearFlags;
				NodeFlags forceFlags;
				if ( m_specialZones->QueryPosition( nodePos, clearFlags, forceFlags ) )
				{
					if ( forceFlags )
					{
						node.AddFlags( forceFlags );
						node.AddFlagsToLinks( forceFlags );
					}
					if ( clearFlags )
					{
						node.ClearFlags( clearFlags );
						node.ClearFlagsAtLinks( clearFlags );
					}
				}
			}

			for ( LinksIterator it( node ); it; ++it )
			{
				CPathLink& link = *it;

				if ( link.HaveAnyFlag( NF_IS_GHOST_LINK | NF_IS_CUSTOM_LINK ) )
				{
					continue;
				}

				Vector3 destinationPos = link.GetDestination()->GetPosition();
				{
					NodeFlags clearFlags;
					NodeFlags forceFlags;
					if ( m_specialZones->QueryLine( nodePos, destinationPos, clearFlags, forceFlags ) )
					{
						CPathLinkModifier modifier( node, link );
						if ( forceFlags )
						{
							modifier.AddFlags( forceFlags );
						}
						if ( clearFlags )
						{
							modifier.ClearFlags( clearFlags );
						}
					}
				}
			}
		}
	};


	Fun f( this, water, specialZones );

	NavgraphHelper::ForAllNodes( *this, f, false );

	CalculateAllLinksCost();

	return true;
}



Bool CNavGraph::Generate( CPathLibWorld* world, CNavmesh* navmesh, CAreaGenerationJob* job )
{
	static const Float InitialGraphGenerationProgress = 0.3f;
	static const Float ImproveProgress = 0.45f;
	static const Float OptimizationProgress = 0.89f;

	m_maxNodesDistance = m_areaDescription->GetMaxNodesDistance();
	
	m_nodesAreLinkedById = true;

	CObstaclesMap* obstacles = m_areaDescription->GetObstaclesMap();
	if ( obstacles )
	{
		obstacles->OnGraphClearance( this );
	}
	

	if ( m_areaDescription->AsNavmeshArea()->IsUsingTransformation() )
	{
		if ( !TInitialGraphGeneration( world, navmesh, m_areaDescription->AsTransformedNavmeshArea() ) )
			return false;
	}
	else
	{
		if ( !TInitialGraphGeneration( world, navmesh, m_areaDescription->AsNavmeshArea() ) )
			return false;
	}

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( InitialGraphGenerationProgress );
	}

	GenerationCalculateNodesAvailability();

	MoveNodesToAvailablePositions();

	if ( !Improve() )
	{
		return false;
	}

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( ImproveProgress );
	}

	ComputeNodeFinder();
	if ( obstacles )
	{
		obstacles->RepopulateArea( false, true );
	}

	PATHLIB_LOG( TXT("Initial graph creation. Initial node count: %d\n"), m_nodes.Size() );

	if ( m_areaDescription->AsNavmeshArea()->IsUsingTransformation() )
	{
		TOptimizeGraph( world, m_areaDescription->AsTransformedNavmeshArea() );
	}
	else
	{
		TOptimizeGraph( world, m_areaDescription->AsNavmeshArea() );
	}
	PATHLIB_LOG( TXT("Navgraph optimalization done. Node count: %d\n"), m_nodes.Size() );

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( OptimizationProgress );
	}
	
	ClearNodeFinder();

	if ( MarkSeparateNodesForDeletion() )
	{
		DeleteMarked();
	}

	PATHLIB_LOG( TXT("Navgraph generated. Final node count: %d\n"), m_nodes.Size() );

	if ( !ConvertLinksToPointers() )
	{
		return false;
	}

	CalculateAllLinksCost();

	ComputeNodeFinder();

	if ( obstacles )
	{
		obstacles->NavgraphInjection( this );
		obstacles->RepopulateArea( false, false );
		obstacles->PostGraphGeneration( this );

		ClearNodeFinder();
		ComputeNodeFinder();
	}

	CNavModyficationMap* navMods = m_areaDescription->GetMetalinks();
	if ( navMods )
	{
		navMods->OnGraphGeneration( this );
	}

	NodeSetsInterconnection();

	if ( job )
	{
		job->SetTaskProgress( 1.f );
	}

	m_binaryVersion = ComputeBinariesVersion();

	++m_version;

	return true;
}
Bool CNavGraph::Generate( CPathLibWorld* world, CTerrainMap* terrain, CAreaGenerationJob* job )
{
	static const Float InitialGraphGenerationProgress = 0.5f;
	static const Float ImproveProgress = 0.6f;
	static const Float ObstaclesProgress = 0.8f;

	m_maxNodesDistance = m_areaDescription->GetMaxNodesDistance();

	m_nodesAreLinkedById = true;
	Float personalSpace = GetPersonalSpace();

	CObstaclesMap* obstacles = m_areaDescription->GetObstaclesMap();
	if ( obstacles )
	{
		obstacles->OnGraphClearance( this );
	}

	if ( !InitialGraphGeneration( world, terrain ) )
		return false;

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( InitialGraphGenerationProgress );
	}

	GenerationCalculateNodesAvailability();

	MoveNodesToAvailablePositions();

	// Improve graph unaccessible nodes and links
	if ( !Improve() )
	{
		return false;
	}

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( ImproveProgress );
	}

	if ( obstacles )
	{
		CObstaclesDetourInfo detourInfo;
		obstacles->RepopulateArea( false, true );
		obstacles->GenerateDetourInfo( detourInfo, personalSpace, false );
		HandleObstaclesMap( obstacles, detourInfo );
		DeleteMarked();
	}

	if ( job )
	{
		if ( job->ShouldTerminate() )
		{
			return false;
		}
		job->SetTaskProgress( ObstaclesProgress );
	}

	ClearNodeFinder();

	if ( MarkSeparateNodesForDeletion() )
	{
		DeleteMarked();
	}

	if ( !ConvertLinksToPointers() )
	{
		return false;
	}

	CalculateAllLinksCost();

	ComputeNodeFinder();

	if ( obstacles )
	{
		obstacles->NavgraphInjection( this );
		obstacles->RepopulateArea( false, false );
		obstacles->PostGraphGeneration( this );

		ClearNodeFinder();
		ComputeNodeFinder();
	}

	CNavModyficationMap* navMods = m_areaDescription->GetMetalinks();
	if ( navMods )
	{
		navMods->OnGraphGeneration( this );
	}

	NodeSetsInterconnection();

	if ( job )
	{
		job->SetTaskProgress( 1.f );
	}

	m_binaryVersion = ComputeBinariesVersion();

	++m_version;

	return true;
}

#endif

};		// namespace PathLib
