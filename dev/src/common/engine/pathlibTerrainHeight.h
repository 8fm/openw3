/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CSimpleBufferReader;
class CSimpleBufferWriter;

namespace PathLib
{

class CTerrainAreaDescription;
class CTerrainHeightComputationContext;

// Terrain quad tree
class CTerrainHeight
{
protected:
	// node data
	struct Leaf
	{
		enum ECorner
		{
			DIR_EAST				= FLAG( 0 ),
			DIR_NORTH				= FLAG( 1 ),

			CORNER_SW				= 0,
			CORNER_SE				= DIR_EAST,
			CORNER_NW				= DIR_NORTH,
			CORNER_NE				= DIR_EAST | DIR_NORTH
		};

		Float				m_height[ 4 ];
	};
	struct Branch
	{
		static const Uint32 MASK_LEAF = 0x80000000;

		enum EChild
		{
			CHILD_EAST				= FLAG( 0 ),
			CHILD_NORTH				= FLAG( 1 ),

			CHILD_SW				= 0,
			CHILD_SE				= CHILD_EAST,
			CHILD_NW				= CHILD_NORTH,
			CHILD_NE				= CHILD_EAST | CHILD_NORTH
		};

		Uint32				m_children[ 4 ];
	};

	struct Node
	{
		union
		{
			Branch			m_branch;
			Leaf			m_leaf;
		};
	};

	enum ENeighbor
	{
		NEIGHBOR_SN				= FLAG( 0 ),					// in other words: horizontal vs vertical
		NEIGHBOR_NE				= FLAG( 1 ),					// in other words: lesser or higher value

		NEIGHBOR_WEST			= 0,
		NEIGHBOR_SOUTH			= NEIGHBOR_SN,
		NEIGHBOR_EAST			= NEIGHBOR_NE,
		NEIGHBOR_NORTH			= NEIGHBOR_SN | NEIGHBOR_NE,
	};

	typedef TDynArray< Node, MC_PathLib > NodeCollection;

	NodeCollection	m_nodes;
	Float			m_tileSize;

	// manipulation data
	static RED_FORCE_INLINE Bool			IsLeaf( const Branch& b, Uint32 child )										{ return ( b.m_children[ child ] & Branch::MASK_LEAF ) != 0; }
	static RED_FORCE_INLINE const Leaf*		GetLeaf( const NodeCollection& col, const Branch& b, Uint32 child  )		{ ASSERT( IsLeaf( b, child ) ); return &col[ b.m_children[ child ] & (~Branch::MASK_LEAF) ].m_leaf; }
	static RED_FORCE_INLINE const Branch*	GetBranch( const NodeCollection& col, const Branch& b, Uint32 child  )		{ ASSERT( !IsLeaf( b, child ) ); return &col[ b.m_children[ child ] ].m_branch; }

	RED_FORCE_INLINE const Branch*			BeginQuery() const															{ return &m_nodes[ 0 ].m_branch; }

	static RED_FORCE_INLINE Float			ComputeHeightAtLeaf( const Leaf& leaf, const Vector2& leafPos, Float leafSize );
	static RED_FORCE_INLINE Float			ComputeHeightAtLeaf( const Leaf& leaf, const Vector2& leafPos, Float leafSize, Bool* useNeighborHeight, Float* neighborHeight );
public:

	CTerrainHeight();
	~CTerrainHeight();

	void		WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool		ReadFromBuffer( CSimpleBufferReader& reader );

	void		OnPostLoad( CTerrainAreaDescription* terrain );

	Float		ComputeHeightSmooth( const Vector2& localPos ) const;
	Float		ComputeHeightFast( const Vector2& localPos ) const;

	Bool		Construct( CTerrainAreaDescription* terrain );

	IRenderResource* GenerateDebugMesh( const Vector2& terrainCorner, Float tileSize ) const;
};

};

