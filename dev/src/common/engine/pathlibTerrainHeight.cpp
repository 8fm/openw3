/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibTerrainHeight.h"

#include "pathlibSimpleBuffers.h"
#include "pathlibTerrain.h"
#include "pathlibTerrainInfo.h"
#include "pathlibWorld.h"
#include "renderer.h"
#include "renderVertices.h"

namespace PathLib
{

RED_FORCE_INLINE Float CTerrainHeight::ComputeHeightAtLeaf( const Leaf& leaf, const Vector2& leafPos, Float leafSize )
{
	Float ratioWE = leafPos.X / leafSize;
	Float ratioSN = leafPos.Y / leafSize;

	Float hW = leaf.m_height[ Leaf::CORNER_SW ] + ratioSN * ( leaf.m_height[ Leaf::CORNER_NW ] - leaf.m_height[ Leaf::CORNER_SW ] );
	Float hE = leaf.m_height[ Leaf::CORNER_SE ] + ratioSN * ( leaf.m_height[ Leaf::CORNER_NE ] - leaf.m_height[ Leaf::CORNER_SE ] );
	Float hS = leaf.m_height[ Leaf::CORNER_SW ] + ratioWE * ( leaf.m_height[ Leaf::CORNER_SE ] - leaf.m_height[ Leaf::CORNER_SW ] );
	Float hN = leaf.m_height[ Leaf::CORNER_NW ] + ratioWE * ( leaf.m_height[ Leaf::CORNER_NE ] - leaf.m_height[ Leaf::CORNER_NW ] );
	
	Float hWE = hW + ratioWE * (hE - hW);
	Float hSN = hS + ratioSN * (hN - hS);
	
	return ( hWE + hSN ) * 0.5f;
}

RED_FORCE_INLINE Float CTerrainHeight::ComputeHeightAtLeaf( const Leaf& leaf, const Vector2& leafPos, Float leafSize, Bool* useNeighborHeight, Float* neighborHeight )
{
	struct Local
	{
		static Float ComputeRatio( Float alpha, Float beta )
		{
			Float betaT = Abs( beta - 0.5f );
			if ( betaT > 0.5f - NumericLimits< Float >::Epsilon() )
			{
				return 0.f;
			}
			return 1.f - ( (alpha*2.f) / (1.f - betaT * 2.f) );
		}
	};

	Float ratioWE = leafPos.X / leafSize;
	Float ratioSN = leafPos.Y / leafSize;

	Float h = ComputeHeightAtLeaf( leaf, leafPos, leafSize );

	// actually we can get to only one 'smoothout' zone, so if we hit any - we can return from function
	if ( useNeighborHeight[ NEIGHBOR_WEST ] && ratioWE < 0.5f )
	{
		Float ratio = Local::ComputeRatio( ratioWE, ratioSN );
		if ( ratio > 0.f )
		{
			return h + ( neighborHeight[ NEIGHBOR_WEST ] - h ) * ratio;
		}
	}
	if ( useNeighborHeight[ NEIGHBOR_SOUTH ] && ratioSN < 0.5f )
	{
		Float ratio = Local::ComputeRatio( ratioSN, ratioWE );
		if ( ratio > 0.f )
		{
			return h + ( neighborHeight[ NEIGHBOR_SOUTH ] - h ) * ratio;
		}
	}
	if ( useNeighborHeight[ NEIGHBOR_EAST ] && ratioWE > 0.5f )
	{
		Float ratio = Local::ComputeRatio( 1.f - ratioWE, ratioSN );
		if ( ratio > 0.f )
		{
			return h + ( neighborHeight[ NEIGHBOR_EAST ] - h ) * ratio;
		}
	}
	if ( useNeighborHeight[ NEIGHBOR_NORTH ] && ratioSN > 0.5f )
	{
		Float ratio = Local::ComputeRatio( 1.f - ratioSN, ratioWE );
		if ( ratio > 0.f )
		{
			return h + ( neighborHeight[ NEIGHBOR_NORTH ] - h ) * ratio;
		}
	}

	return h;
}


CTerrainHeight::CTerrainHeight()
	: m_tileSize( 1.f )
{

}

CTerrainHeight::~CTerrainHeight()
{

}

void CTerrainHeight::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	writer.SmartPut( m_nodes );
}

Bool CTerrainHeight::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.SmartGet( m_nodes ) )
	{
		return false;
	}

	return true;
}

void CTerrainHeight::OnPostLoad( CTerrainAreaDescription* terrain )
{
	m_tileSize = terrain->GetPathLib().GetTerrainInfo().GetTileSize();
	// Fail-safe. If data are empty populate them with dummy data.
	if ( m_nodes.Empty() )
	{
		m_nodes.Grow( 5 );
		m_nodes[ 0 ].m_branch.m_children[ 0 ] = Branch::MASK_LEAF | 1;
		m_nodes[ 0 ].m_branch.m_children[ 1 ] = Branch::MASK_LEAF | 2;
		m_nodes[ 0 ].m_branch.m_children[ 2 ] = Branch::MASK_LEAF | 3;
		m_nodes[ 0 ].m_branch.m_children[ 3 ] = Branch::MASK_LEAF | 4;

		for ( Uint32 i = 1; i < 5; ++i )
		{
			for ( Uint32 j = 0; j < 4; ++j )
			{
				m_nodes[ i ].m_leaf.m_height[ j ] = 0.f;
			}
		}
	}
}


Float CTerrainHeight::ComputeHeightSmooth( const Vector2& localPos ) const
{
	const Branch* branch = BeginQuery();
	const Leaf* leaf = nullptr;

	const Branch* neighbor[ 4 ] =
	{
		nullptr, nullptr, nullptr, nullptr
	};

	// keep position in node space all time
	Vector2 nodePos = localPos;

	// sanity clamp
	nodePos.X = Clamp( nodePos.X, 0.f, m_tileSize );
	nodePos.Y = Clamp( nodePos.Y, 0.f, m_tileSize );

	Float nodeSize = m_tileSize;
	while ( true )
	{
		ENeighbor nWE = NEIGHBOR_EAST;
		ENeighbor nSN = NEIGHBOR_NORTH;
		
		// compute midpoint
		Float halfSize = nodeSize * 0.5f;

		// compute next child index
		Uint32 nextChild = 0;

		// compare local node position with midpoint
		if ( nodePos.X > halfSize )
		{
			nextChild = Branch::CHILD_EAST;
			nodePos.X -= halfSize;
			nWE = NEIGHBOR_WEST;
		}
		if ( nodePos.Y > halfSize )
		{
			nextChild |= Branch::CHILD_NORTH;
			nodePos.Y -= halfSize;
			nSN = NEIGHBOR_SOUTH;
		}

		// go one level deeper
		nodeSize = halfSize;

		// update neighbor nodes
		Uint32 neighborWE = (nextChild & Branch::CHILD_NORTH) | (nextChild ^ Branch::CHILD_EAST);
		neighbor[ nWE ] = 
			IsLeaf( *branch, neighborWE )
			? nullptr
			: GetBranch( m_nodes, *branch, neighborWE );

		Uint32 neighborSN = (nextChild & Branch::CHILD_EAST) | (nextChild ^ Branch::CHILD_NORTH);
		neighbor[ nSN ] = 
			IsLeaf( *branch, neighborSN )
			? nullptr
			: GetBranch( m_nodes, *branch, neighborSN );

		Uint32 nNegWE = nWE ^ NEIGHBOR_NE;
		Uint32 nNegSN = nSN ^ NEIGHBOR_NE;
		
		if ( neighbor[ nNegWE ] )
		{
			Uint32 nextNeighborChild = 0;

			// add 'north' component if we are going 'north' with current child
			nextNeighborChild = nextChild & Branch::CHILD_NORTH;

			// add 'east' component if we are inspecting 'west' child
			if ( ( nNegWE & NEIGHBOR_NE ) == 0 )
			{
				nextNeighborChild |= Branch::CHILD_EAST;
			}
			neighbor[ nNegWE ] =
				IsLeaf( *neighbor[ nNegWE ], nextNeighborChild )
				? nullptr
				: GetBranch( m_nodes, *neighbor[ nNegWE ], nextNeighborChild );
		}

		if ( neighbor[ nNegSN ] )
		{
			Uint32 nextNeighborChild = 0;

			// add 'east' component if we are going 'east' with current child
			nextNeighborChild = nextChild & Branch::CHILD_EAST;

			// add 'north' component if we are inspecting 'south' child
			if ( ( nNegSN & NEIGHBOR_NE ) == 0 )
			{
				nextNeighborChild |= Branch::CHILD_NORTH;
			}
			neighbor[ nNegSN ] =
				IsLeaf( *neighbor[ nNegSN ], nextNeighborChild )
				? nullptr
				: GetBranch( m_nodes, *neighbor[ nNegSN ], nextNeighborChild );
		}

		// we encountered leaf node
		if ( IsLeaf( *branch, nextChild ) )
		{
			leaf = GetLeaf( m_nodes, *branch, nextChild );
			break;
		}

		// loop for next branch node
		branch = GetBranch( m_nodes, *branch, nextChild );
	}

	// determine neighbour nodes heights
	Bool useNeighborHeight[ 4 ];
	Float neighborHeight[ 4 ];

	for ( Uint32 n = 0; n < 4; ++n )
	{
		useNeighborHeight[ n ] = false;

		if ( neighbor[ n ] == nullptr )
		{
			continue;
		}

		Uint32 childComputationBase = 0;

		if ( n == NEIGHBOR_SOUTH )
		{
			childComputationBase = Branch::CHILD_NORTH;
		}
		else if ( n == NEIGHBOR_WEST )
		{
			childComputationBase = Branch::CHILD_EAST;
		}

		Uint32 axisTest =						// which axis to test on vectors
			(n & NEIGHBOR_SN)
			? 0									// test X
			: 1;								// test Y

		Uint32 axisFlag =						// axis flag
			(n & NEIGHBOR_SN)
			? Branch::CHILD_EAST
			: Branch::CHILD_NORTH;


		Vector2 neighborNodePos = nodePos;
		Float neighborNodeSize = nodeSize;
		// progress down the node until we find a leaf
		const Leaf* neighbourLeaf = nullptr;
		const Branch* neighbourBranch = neighbor[ n ];
		do
		{
			// compute midpoint
			Float halfSize = neighborNodeSize * 0.5f;

			// compute next child index
			Uint32 nextChild = childComputationBase;

			// compare local node position with midpoint
			if ( neighborNodePos.A[ axisTest ] > halfSize )
			{
				nextChild |= axisFlag & 0xff;
				neighborNodePos.A[ axisTest ] -= halfSize;
			}

			neighborNodeSize = halfSize;

			// we encountered leaf node
			if ( IsLeaf( *neighbourBranch, nextChild ) )
			{
				neighbourLeaf = GetLeaf( m_nodes, *neighbourBranch, nextChild );
			}
			else
			{
				neighbourBranch = GetBranch( m_nodes, *neighbourBranch, nextChild ) ;
			}
		}
		while( !neighbourLeaf );
		
		Uint32 alpha = 0;
		Uint32 betaFlag =
			( n & NEIGHBOR_SN )
			? Leaf::DIR_EAST
			: Leaf::DIR_NORTH;

		if ( ( n & NEIGHBOR_NE ) == 0 )
		{
			alpha = 0x3 & (~betaFlag);
		}

		Float a = neighbourLeaf->m_height[ alpha ];
		Float b = neighbourLeaf->m_height[ alpha | betaFlag ];

		neighborHeight[ n ] = a + ( b - a ) * ( neighborNodePos.A[ axisTest ] / neighborNodeSize );
		useNeighborHeight[ n ] = true;
	}

	return ComputeHeightAtLeaf( *leaf, nodePos, nodeSize, useNeighborHeight, neighborHeight );
}

Float CTerrainHeight::ComputeHeightFast( const Vector2& localPos ) const
{
	const Branch* branch = BeginQuery();
	const Leaf* leaf = nullptr;

	// keep position in node space all time
	Vector2 nodePos = localPos;

	// sanity clamp
	nodePos.X = Clamp( nodePos.X, 0.f, m_tileSize );
	nodePos.Y = Clamp( nodePos.Y, 0.f, m_tileSize );

	Float nodeSize = m_tileSize;
	while ( true )
	{
		// compute midpoint
		Float halfSize = nodeSize * 0.5f;

		// compute next child index
		Uint32 nextChild = 0;

		// compare local node position with midpoint
		if ( nodePos.X > halfSize )
		{
			nextChild = Branch::CHILD_EAST;
			nodePos.X -= halfSize;
		}
		if ( nodePos.Y > halfSize )
		{
			nextChild |= Branch::CHILD_NORTH;
			nodePos.Y -= halfSize;
		}

		// go one level deeper
		nodeSize = halfSize;

		// we encountered leaf node
		if ( IsLeaf( *branch, nextChild ) )
		{
			leaf = GetLeaf( m_nodes, *branch, nextChild );
			break;
		}

		// loop for next branch node
		branch = GetBranch( m_nodes, *branch, nextChild );
	}

	return ComputeHeightAtLeaf( *leaf, nodePos, nodeSize );
}

Bool CTerrainHeight::Construct( CTerrainAreaDescription* terrain )
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	struct Algorithm
	{
		Algorithm( CTerrainAreaDescription* terrain )
			: m_terrainMap( terrain->GetTerrainMap() )
			, m_terrainConfig( m_terrainMap->GetTerrainInfo() )
			, m_terrainHeight( terrain->GetHeightData() )
			, m_maxInaccuracy( terrain->GetPathLib().GetGlobalSettings().GetTerrainHeightApproximationRange() )
		{
		}

		void ComputeLeaf( Leaf& leaf, Uint32 minX, Uint32 maxX, Uint32 minY, Uint32 maxY )
		{
			// clamp max values
			Uint32 vertsCount = m_terrainConfig->GetTilesResolution();
			maxX = Min( maxX, vertsCount-1 );
			maxY = Min( maxY, vertsCount-1 );

			leaf.m_height[ Leaf::CORNER_SW ] = m_terrainHeight.GetVertHeight( minX, minY );
			leaf.m_height[ Leaf::CORNER_SE ] = m_terrainHeight.GetVertHeight( maxX, minY );
			leaf.m_height[ Leaf::CORNER_NW ] = m_terrainHeight.GetVertHeight( minX, maxY );
			leaf.m_height[ Leaf::CORNER_NE ] = m_terrainHeight.GetVertHeight( maxX, maxY );
		}

		Bool IsLeafOk( Leaf& leaf, Uint32 minX, Uint32 maxX, Uint32 minY, Uint32 maxY, Float nodeSize )
		{
			Float quadSize = m_terrainConfig->GetQuadSize();
			for ( Uint32 y = minY; y < maxY; ++y )
			{
				for ( Uint32 x = minX; x < maxX; ++x)
				{
					CTerrainMap::eQuadState quadState = m_terrainMap->GetQuadState( m_terrainMap->GetQuadIndex( x, y ) );

					if ( quadState == CTerrainMap::QUAD_BLOCK_ALL || quadState == CTerrainMap::QUAD_INSTANCE )
					{
						continue;
					}

					Float vertsHeight[ 4 ];
					m_terrainHeight.GetTexelVertsHeight( x, y, vertsHeight );

					Vector2 leafPosLow(
						Float( x - minX ) * quadSize,
						Float( y - minY ) * quadSize
						);

					Vector2 leafPosHigh(
						Float( x+1 - minX ) * quadSize,
						Float( y+1 - minY ) * quadSize
						);

					{
						Float approxZ = ComputeHeightAtLeaf( leaf, Vector2( leafPosLow.X, leafPosLow.Y ), nodeSize );
						if ( Abs( vertsHeight[ 0 ] - approxZ ) > m_maxInaccuracy )
						{
							return false;
						}
					}

					{
						Float approxZ = ComputeHeightAtLeaf( leaf, Vector2( leafPosHigh.X, leafPosLow.Y ), nodeSize );
						if ( Abs( vertsHeight[ 1 ] - approxZ ) > m_maxInaccuracy )
						{
							return false;
						}
					}

					{
						Float approxZ = ComputeHeightAtLeaf( leaf, Vector2( leafPosLow.X, leafPosHigh.Y ), nodeSize );
						if ( Abs( vertsHeight[ 2 ] - approxZ ) > m_maxInaccuracy )
						{
							return false;
						}
					}

					{
						Float approxZ = ComputeHeightAtLeaf( leaf, Vector2( leafPosHigh.X, leafPosHigh.Y ), nodeSize );
						if ( Abs( vertsHeight[ 3 ] - approxZ ) > m_maxInaccuracy )
						{
							return false;
						}
					}
				}
			}
			return true;
		}

		Uint32 ComputeRec( NodeCollection& collection, Uint32 minX, Uint32 maxX, Uint32 minY, Uint32 maxY, Float nodeSize )
		{
			// remember node index to return it afterwards
			Uint32 ret = collection.Size();

			// add new node
			collection.PushBack( Node() );
			Node& node = collection.Back();

			// compute return node as a leaf
			ComputeLeaf( node.m_leaf, minX, maxX, minY, maxY );

			// check if leaf is precise enough
			if ( !IsLeafOk( node.m_leaf, minX, maxX, minY, maxY, nodeSize ) )
			{
				// convert to branch node
				ASSERT( ( maxX - minX ) > 1 );
				Uint32 halfX = minX + (maxX - minX) / 2;
				Uint32 halfY = minY + (maxY - minY) / 2;

				Float halfSize  = nodeSize * 0.5f;

				// recursively compute branch nodes NOTICE: it may resize collection, thus invalidating node reference
				Uint32 childSW = ComputeRec( collection, minX, halfX, minY, halfY, halfSize );
				Uint32 childSE = ComputeRec( collection, halfX, maxX, minY, halfY, halfSize );
				Uint32 childNW = ComputeRec( collection, minX, halfX, halfY, maxY, halfSize );
				Uint32 childNE = ComputeRec( collection, halfX, maxX, halfY, maxY, halfSize );
				Branch& b = collection[ ret ].m_branch;
			
				b.m_children[ Branch::CHILD_SW ] = childSW;
				b.m_children[ Branch::CHILD_SE ] = childSE;
				b.m_children[ Branch::CHILD_NW ] = childNW;
				b.m_children[ Branch::CHILD_NE ] = childNE;
			}
			else
			{
				// return leaf node
				ret |= Branch::MASK_LEAF;
			}

			return ret;
		}

		void Compute( NodeCollection& collection )
		{
			Float tileSize = m_terrainConfig->GetTileSize();

			collection.ClearFast();
			collection.PushBack( Node() );
			Node& node = collection.Back();

			Uint32 min = 0;
			Uint32 max = m_terrainConfig->GetTilesResolution();
			Uint32 half = max / 2 + (max & 1);

			Float halfSize  = tileSize * 0.5f;

			Uint32 childSW = ComputeRec( collection, min, half, min, half, halfSize );
			Uint32 childSE = ComputeRec( collection, half, max, min, half, halfSize );
			Uint32 childNW = ComputeRec( collection, min, half, half, max, halfSize );
			Uint32 childNE = ComputeRec( collection, half, max, half, max, halfSize );
			Branch& b = collection[ 0 ].m_branch;

			b.m_children[ Branch::CHILD_SW ] = childSW;
			b.m_children[ Branch::CHILD_SE ] = childSE;
			b.m_children[ Branch::CHILD_NW ] = childNW;
			b.m_children[ Branch::CHILD_NE ] = childNE;
		}

		const CTerrainMap* m_terrainMap;
		const CTerrainInfo* m_terrainConfig;
		const CTerrainHeightComputationContext& m_terrainHeight;
		const Float m_maxInaccuracy;
	};

	Algorithm algorithm( terrain );

	m_tileSize = algorithm.m_terrainConfig->GetTileSize();
	
	algorithm.Compute( m_nodes );

	return true;
#else
	return false;
#endif
}

IRenderResource* CTerrainHeight::GenerateDebugMesh( const Vector2& terrainCorner, Float tileSize ) const
{
	struct Algorithm
	{
		Algorithm( const NodeCollection& collection )
			: m_nodes( collection ) {}

		void GenerateLeaf( const Leaf& l, const Vector2& posMin, Float nodeSize )
		{
			const Color borderColor( 0, 0, 255, 160 );
			const Color centerColor( 0, 160, 255, 160 );

			Uint32 baseVertIndex = m_vertices.Size();
			m_vertices.Grow( 5 );

			Float avgHeight =
				( l.m_height[ 0 ] + l.m_height[ 1 ] + l.m_height[ 2 ] + l.m_height[ 3 ] ) * 0.25f;

			// corners
			m_vertices[ baseVertIndex+0 ] = DebugVertex( Vector( posMin.X, posMin.Y, l.m_height[ Leaf::CORNER_SW ] ), borderColor );
			m_vertices[ baseVertIndex+1 ] = DebugVertex( Vector( posMin.X + nodeSize, posMin.Y, l.m_height[ Leaf::CORNER_SE ] ), borderColor );
			m_vertices[ baseVertIndex+2 ] = DebugVertex( Vector( posMin.X + nodeSize, posMin.Y + nodeSize, l.m_height[ Leaf::CORNER_NE ] ), borderColor );
			m_vertices[ baseVertIndex+3 ] = DebugVertex( Vector( posMin.X, posMin.Y + nodeSize, l.m_height[ Leaf::CORNER_NW ] ), borderColor );
			// midpoint
			m_vertices[ baseVertIndex+4 ] = DebugVertex( Vector( posMin.X + nodeSize * 0.5f, posMin.Y + nodeSize * 0.5f, avgHeight ), centerColor );

			Uint32 baseIndice = m_indices.Size();
			m_indices.Grow( 12 );

			m_indices[ baseIndice+ 0 ] = baseVertIndex+1;
			m_indices[ baseIndice+ 1 ] = baseVertIndex+0;
			m_indices[ baseIndice+ 2 ] = baseVertIndex+4;
			m_indices[ baseIndice+ 3 ] = baseVertIndex+2;
			m_indices[ baseIndice+ 4 ] = baseVertIndex+1;
			m_indices[ baseIndice+ 5 ] = baseVertIndex+4;
			m_indices[ baseIndice+ 6 ] = baseVertIndex+3;
			m_indices[ baseIndice+ 7 ] = baseVertIndex+2;
			m_indices[ baseIndice+ 8 ] = baseVertIndex+4;
			m_indices[ baseIndice+ 9 ] = baseVertIndex+0;
			m_indices[ baseIndice+10 ] = baseVertIndex+3;
			m_indices[ baseIndice+11 ] = baseVertIndex+4;

		}

		void ComputeRec( const Branch& b, const Vector2& posMin, Float nodeSize )
		{
			Float halfSize = nodeSize * 0.5f;
			for ( Uint32 i = 0; i < 4; ++i )
			{
				Vector2 corner = posMin;
				if ( i & Branch::CHILD_NORTH )
				{
					corner.Y += halfSize;
				}
				if ( i & Branch::CHILD_EAST )
				{
					corner.X += halfSize;
				}


				if ( IsLeaf( b, i ) )
				{
					GenerateLeaf( *GetLeaf( m_nodes, b, i ), corner, halfSize );
				}
				else
				{
					ComputeRec( *GetBranch( m_nodes, b, i ), corner, halfSize );
				}
			}
		}

		const NodeCollection&		m_nodes;
		TDynArray< Uint32 >			m_indices;
		TDynArray< DebugVertex >	m_vertices;
	};

	if ( m_nodes.Empty() )
	{
		return nullptr;
	}

	Algorithm alg( m_nodes );

	alg.ComputeRec( *BeginQuery(), terrainCorner, tileSize );

	return GRender->UploadDebugMesh( alg.m_vertices, alg.m_indices );
}


};		// namespace PathLib