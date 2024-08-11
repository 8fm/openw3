
#include "kdTreeTypes.h"
#include "kdTreeSearchUtils.h"
#include "kdTreeNNCollector.h"
#include "kdTreeBuilder.h"

#pragma once

template < EMemoryClass memClass >
class kdTreeAllocMemClass
{
public:
	static void* Alloc( size_t size )
	{
		return RED_MEMORY_ALLOCATE( MemoryPool_Default, memClass, size );
	}

	static void Dealloc( void* ptr )
	{
		if ( ptr )
		{
			RED_MEMORY_FREE( MemoryPool_Default, memClass, ptr );
		}
	}

	static size_t Alignment( size_t size )
	{
		return CalculateDefaultAlignment( size );
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
class TkdTree
{
	typedef TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner > TThisTree;

	template< class T, template <class> class R > friend class kdTreeBuilder;

public:
	typedef TCoord*		Point;
	typedef TCoord		Coord;
	typedef TDist		Dist;
	typedef TPointSet	PointSet;
	typedef TTreeIndex	TreeIdx;
	typedef TAlloc		Allocator;

	struct kdTreeNode
	{
		RED_INLINE kdTreeNode()
		{
			m_data.m_split.m_type_offset_cd = 0;
			m_data.m_split.m_cdVal = (Coord)0;
			m_data.m_split.m_cdBnds[0] = (Coord)0;
			m_data.m_split.m_cdBnds[1] = (Coord)0;
		}

		struct Leaf
		{
			// Type									bit 31
			// Size									bit 0..30
			Int32									m_type_size;
			TreeIdx									m_dataBucket;
		};

		struct Split
		{
			// Type									bit 31
			// Offset								bit 30..3
			// Cut dim								bit 0..2
			Int32									m_type_offset_cd;
			Coord									m_cdVal;
			Coord									m_cdBnds[ TST_Size ];
		};

		union
		{
			Leaf	m_leaf;
			Split	m_split;
		} m_data;


		// Leaf

		RED_INLINE void SetEmpty()
		{
			m_data.m_split.m_type_offset_cd = 0x70000000;
			m_data.m_leaf.m_dataBucket = -1;
		}

		RED_INLINE void SetLeaf( TreeIdx data, int size )
		{
			m_data.m_leaf.m_type_size = 0x7FFFFFFF & size;
			m_data.m_leaf.m_dataBucket = data;
		}

		RED_INLINE Bool IsLeaf() const
		{
			return m_data.m_leaf.m_type_size >= 0;
		}

		RED_INLINE Bool IsEmptyLeaf() const
		{
			return m_data.m_leaf.m_dataBucket == -1;
		}

		RED_INLINE Int32 GetLeafSize() const
		{
			return m_data.m_leaf.m_type_size & 0x7FFFFFFF;
		}

		RED_INLINE TreeIdx* GetLeafData( TreeIdx* idxbase ) const
		{
			return idxbase + m_data.m_leaf.m_dataBucket;
		}

		// Split

		RED_INLINE void SetSplit( Int32 cd, Coord val, Coord min, Coord max, Int32 rOffset )
		{
			ASSERT( cd >= 0 && cd <= 7 );
			ASSERT( rOffset >= 0 );
			ASSERT( min <= max );

			m_data.m_split.m_type_offset_cd = 0x80000000 | ( ( rOffset << 3 ) & 0x7FFFFFF8 ) | cd;
			m_data.m_split.m_cdVal = val;
			m_data.m_split.m_cdBnds[0] = min;
			m_data.m_split.m_cdBnds[1] = max;
		}

		RED_INLINE Coord GetSplitMin() const
		{
			return m_data.m_split.m_cdBnds[0];
		}

		RED_INLINE Coord GetSplitMax() const
		{
			return m_data.m_split.m_cdBnds[1];
		}

		RED_INLINE Coord GetSplitVal() const
		{
			return m_data.m_split.m_cdVal;
		}

		RED_INLINE Int32 GetSplitAxis() const
		{
			return m_data.m_split.m_type_offset_cd & 0x00000007;
		}

		RED_INLINE kdTreeNode* GetLeft() const
		{
			return reinterpret_cast<kdTreeNode*>( reinterpret_cast< kdTreePtr >( this ) + sizeof(kdTreeNode) );	
		}

		RED_INLINE kdTreeNode* GetRight() const
		{
			return reinterpret_cast<kdTreeNode*>( reinterpret_cast< kdTreePtr >( this ) + sizeof(kdTreeNode) * static_cast< size_t >( ((m_data.m_split.m_type_offset_cd & 0x7FFFFFF8)>>3) ) );
		}
	};

private:
	TkdTree()
		: m_pointsIdx( nullptr )
		, m_bndBoxLo( nullptr )
		, m_bndBoxHi( nullptr )
		, m_nodes( nullptr )
		, m_numNodes( 0 )
		, m_memSize( 0 )
		, m_isCooked( false )
	{
	}

public:
	~TkdTree()
	{
		if( !m_isCooked )		// We can only free the memory when we own it - cooked tree data comes from elsewhere
		{
			Allocator::Dealloc( m_pointsIdx );	// m_pointsIdx also marks begin of a single memory block for all the kdtree data 
		}
	}

	void CookedInit( Uint8* mem, size_t memSize, const kdTreeMediator< TThisTree >* mediator, Int32 maxNodes, Int32 bucketSize = 1 );

	// Temp - only for editor!
	RED_INLINE const kdTreeNode* GetRoot() const { return m_nodes ? &(m_nodes[0]) : NULL; }

	RED_INLINE const Point GetLoBBox() const { return m_bndBoxLo; }
	RED_INLINE const Point GetHiBBox() const { return m_bndBoxHi; }

	RED_INLINE size_t GetMemUsage() const { return m_memSize; } 
	RED_INLINE void* GetData() const { return m_pointsIdx; }

public:
	void Search( const Point q, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr, Float eps = 0.f ) const;

	Int32 SearchR( const Point q, Dist radius, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr, Float eps = 0.f ) const;

	void FastSearch( const Point q, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter = nullptr, TFilterOwner* filterOwner = nullptr, Float eps = 0.f ) const;

private:
	RED_INLINE void ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const;

	RED_INLINE void ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const;

	RED_INLINE void ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const;
	void ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const;

private:
	const PointSet*	m_pointSet;
	TreeIdx*		m_pointsIdx;

	Point			m_bndBoxLo;
	Point			m_bndBoxHi;

	Int32			m_numNodes;
	kdTreeNode*		m_nodes;

	size_t			m_memSize;
	Bool			m_isCooked;
};

#include "kdTree.inl"
