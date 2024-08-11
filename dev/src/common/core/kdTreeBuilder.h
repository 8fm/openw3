
#include "kdTreeTypes.h"
#include "kdTreeMediator.h"
#include "kdTreeSplitter.h"

#pragma once

//////////////////////////////////////////////////////////////////////////
template< class TTree, template <class> class Splitter = kdTreeSplitters::DefaultSplitter >
class kdTreeBuilder
{
public:
	static TTree* BuildCookedTree( Uint8* mem, size_t memSize, const kdTreeMediator< TTree >* mediator, Int32 maxNodes, Int32 bucketSize = 1 )
	{
		TTree* tree = new TTree();
		tree->CookedInit( mem, memSize, mediator, maxNodes, bucketSize );
		return tree;
	}

	static TTree* BuildTree( const kdTreeMediator< TTree >* mediator, Int32 maxNodes, Int32 bucketSize = 1 )
	{
		TTree* tree = new TTree();

		const Bool ret = mediator->GetPointSet( tree->m_pointSet );
		if ( false == ret )
		{
			delete tree;
			return nullptr;
		}

		const Int32 numPoints				= tree->m_pointSet->GetPointNum();
		const Int32 pointIdxSize			= sizeof( typename TTree::TreeIdx ) * numPoints;
		const Int32 nodesSize				= sizeof( typename TTree::kdTreeNode ) * maxNodes;
		const Int32 bboxSize				= sizeof( typename TTree::Coord ) * tree->m_pointSet->GetPointDim();
		const size_t pointIdxSizeAligned	= AlignOffset( pointIdxSize, TTree::Allocator::Alignment( pointIdxSize ) );
		const size_t nodesSizeAligned		= AlignOffset( nodesSize, TTree::Allocator::Alignment( nodesSize ) );
		const size_t bboxSizeAligned		= AlignOffset( bboxSize, TTree::Allocator::Alignment( bboxSize ) );
		tree->m_memSize						= pointIdxSizeAligned + nodesSizeAligned + ( bboxSizeAligned * 2 );

		Uint8* mem = reinterpret_cast< Uint8* >( TTree::Allocator::Alloc( tree->m_memSize ) );

		tree->m_pointsIdx = reinterpret_cast< typename TTree::TreeIdx* > ( mem );
		for ( typename TTree::TreeIdx i = 0; i < numPoints; ++i )
		{
			tree->m_pointsIdx[ i ] = i;
		}

		tree->m_numNodes = maxNodes;
		tree->m_nodes = reinterpret_cast< typename TTree::kdTreeNode* > ( mem + pointIdxSizeAligned );
		tree->m_bndBoxHi = reinterpret_cast< typename TTree::Point > ( mem + pointIdxSizeAligned + nodesSizeAligned );
		tree->m_bndBoxLo = reinterpret_cast< typename TTree::Point > ( mem + pointIdxSizeAligned + nodesSizeAligned + bboxSizeAligned );

		CalcBoxForTree( tree );

		Int32 nIdx = 0;
		BuildSubTree( nIdx, tree->m_nodes, tree->m_numNodes, *( tree->m_pointSet ), tree->m_pointsIdx, 0, numPoints, bucketSize, tree->m_bndBoxLo, tree->m_bndBoxHi );

		return tree;
	}

private:
	static void CalcBoxForTree( const TTree* tree )
	{
		const typename TTree::PointSet& pa = *( tree->m_pointSet );
		const Int32 n = pa.GetPointNum();
		const typename TTree::TreeIdx idx = 0;
		const typename TTree::TreeIdx* idxbase = tree->m_pointsIdx;
		const Int32 dim = pa.GetPointDim();

		for ( Int32 d = 0; d < dim; ++d )
		{		
			typename TTree::Coord lo_bnd = KD_POINT_VAL( 0, d );
			typename TTree::Coord hi_bnd = KD_POINT_VAL( 0, d );

			for ( Int32 i = 0; i < n; ++i ) 
			{
				if ( KD_POINT_VAL( i, d ) < lo_bnd ) 
				{
					lo_bnd = KD_POINT_VAL( i, d );
				}
				else if ( KD_POINT_VAL( i, d ) > hi_bnd ) 
				{
					hi_bnd = KD_POINT_VAL( i, d );
				}
			}

			tree->m_bndBoxLo[d] = lo_bnd;
			tree->m_bndBoxHi[d] = hi_bnd;
		}
	}

	static Int32 BuildSubTree(
		Int32&							nIdx,
		typename TTree::kdTreeNode*		nodes,
		const Int32						numNodes,
		const typename TTree::PointSet&	pa,
		typename TTree::TreeIdx*		idxbase,
		typename TTree::TreeIdx			idx,
		const Int32						n,
		const Int32						bsp,
		typename TTree::Point&			bndLo,
		typename TTree::Point&			bndHi )
	{
		if (n <= bsp)
		{		
			ASSERT( nIdx >= 0 && nIdx < numNodes );

			Int32 leafIdx = nIdx;

			if (n > 0)	
			{	
				nodes[ leafIdx ].SetLeaf( idx, n );	
			}
			else
			{
				nodes[ leafIdx ].SetEmpty();
			}

			nIdx += 1;

			return leafIdx;
		}
		else 
		{
			Int32 cd;
			typename TTree::Coord cv;
			int nLo;

			Splitter< TTree >::Split( pa, idxbase, idx, bndLo, bndHi, n, cd, cv, nLo );

			typename TTree::Coord lv = bndLo[cd];
			typename TTree::Coord hv = bndHi[cd];

			Int32 nextIdx = nIdx + 1;

			Int32 leftIdx = -1;
			Int32 rigthIdx = -1;

			{
				bndHi[cd] = cv;

				leftIdx = BuildSubTree( nextIdx, nodes, numNodes, pa, idxbase, idx, nLo, bsp, bndLo, bndHi );

				bndHi[cd] = hv;
			}

			{
				bndLo[cd] = cv;

				rigthIdx = BuildSubTree( nextIdx, nodes, numNodes, pa, idxbase, idx + nLo, n-nLo, bsp, bndLo, bndHi );

				bndLo[cd] = lv;
			}

			ASSERT( rigthIdx != -1 );
			ASSERT( leftIdx != -1 );
			ASSERT( leftIdx == nIdx + 1 );
			ASSERT( nIdx >= 0 && nIdx < numNodes );

			nodes[ nIdx ].SetSplit( cd, cv, lv, hv, rigthIdx-nIdx );

			ASSERT( nodes[ nIdx ].GetRight() );
			ASSERT( nodes[ nIdx ].GetLeft() );
			ASSERT( nodes[ nIdx ].GetLeft() == &(nodes[ leftIdx ]) );
			ASSERT( nodes[ nIdx ].GetRight() == &(nodes[ rigthIdx ]) );

			Int32 ret = nIdx;

			nIdx = nextIdx+1;

			return ret;
		}
	}
};
