#pragma once

#include "../core/mathUtils.h"

#include "baseEngine.h"

RED_WARNING_PUSH()
// Shut up u stupid compiler. Trained professionals are here.
RED_DISABLE_WARNING_MSC( 4200 ) // nonstandard extension used : zero-sized array in struct/union


namespace PathLib
{

namespace BinTree
{

////////////////////////////////////////////////////////////////////////////
// CBinTreeNode
// Binary tree nodes
////////////////////////////////////////////////////////////////////////////
template < class Element > struct TBinTreeLeafNode;
template < class Element > struct TBinTreeBranchNode;

struct BinTreeNodeBase
{
	BinTreeNodeBase( Bool isLeaf )
		: m_isLeaf( isLeaf )												{}

	Bool							m_isLeaf;
	Int8							m_cutAxis;								// BranchNode member - moved here to optimize space usage
	Uint16							m_elementsCount;						// LeafNode member - also moved here to optimize space
};

template < class Element >
struct TBinTreeNode : public BinTreeNodeBase
{
	typedef TBinTreeNode< Element > CBinTreeNode;
	typedef TBinTreeLeafNode< Element > CBinTreeLeafNode;
	typedef TBinTreeBranchNode< Element > CBinTreeBranchNode;
	typedef Element ElementType;

	TBinTreeNode( Bool isLeaf )
		: BinTreeNodeBase( isLeaf )											{}

	Uint32 ComputeSize() const;												// data size computation

	Bool ContainsElement( Element ind ) const;								// debug function

	TBinTreeLeafNode< Element >* AsLeafNode();
	TBinTreeBranchNode< Element >* AsBranchNode();

	const TBinTreeLeafNode< Element >* AsLeafNode() const;
	const TBinTreeBranchNode< Element >* AsBranchNode() const;
};

template < class Element >
struct TBinTreeLeafNode : public TBinTreeNode< Element >
{
	using TBinTreeNode< Element >::m_elementsCount;

	TBinTreeLeafNode( Uint32 elementsCount )
		: TBinTreeNode< Element >( true )									{ m_elementsCount = Uint16( elementsCount ); }
	~TBinTreeLeafNode()														{}
	Element							m_elements[];
};

template < class Element >
struct TBinTreeBranchNode : public TBinTreeNode< Element >
{
	TBinTreeBranchNode()
		: TBinTreeNode< Element >( false )
		, m_cutPoint( -666.666f )
	{
		m_childNodes[ 0 ] = 0;
		m_childNodes[ 1 ] = 0;
		this->m_cutAxis = 0;
	}
	TBinTreeBranchNode( TBinTreeNode< Element >* p1, TBinTreeNode< Element >* p2, Float cutPoint, Int32 axis )
		: TBinTreeNode< Element >( false )
		, m_cutPoint( cutPoint )
	{
		m_childNodes[ 0 ] = Uint32( PtrOffset< TBinTreeNode< Element > >( p1, this ) );
		m_childNodes[ 1 ] = Uint32( PtrOffset< TBinTreeNode< Element > >( p2, this ) );
		this->m_cutAxis = Int8(axis);
	}

	void InitializeNode( TBinTreeNode< Element >* p1, TBinTreeNode< Element >* p2, Float cutPoint, Int32 axis )
	{
		m_cutPoint = cutPoint;
		m_childNodes[ 0 ] = Uint32( PtrOffset< TBinTreeNode< Element > >( p1, this ) );
		m_childNodes[ 1 ] = Uint32( PtrOffset< TBinTreeNode< Element > >( p2, this ) );
		this->m_cutAxis = Int8(axis);
	}
	~TBinTreeBranchNode()
	{
		if ( HasChild( 0 ) )
			ChildNode( 0 )->Delete();
		if ( HasChild( 1 ) )
			ChildNode( 1 )->Delete();
	}
	RED_INLINE Bool				HasChild( Uint32 i  )  const			{ return m_childNodes[ i ] != 0; }
	RED_INLINE TBinTreeNode< Element >*
									ChildNode( Uint32 i ) const				{ return static_cast< TBinTreeNode< Element >* >( OffsetPtr( const_cast< TBinTreeBranchNode* >( this ), m_childNodes[ i ] ) ); }

	Uint32							m_childNodes[2];
	Float							m_cutPoint;
};

template < class Element >
TBinTreeLeafNode< Element >* TBinTreeNode< Element >::AsLeafNode() { ASSERT( m_isLeaf ); return static_cast< TBinTreeLeafNode< Element >* >( this ); }
template < class Element >
TBinTreeBranchNode< Element >* TBinTreeNode< Element >::AsBranchNode() { ASSERT( !m_isLeaf ); return static_cast< TBinTreeBranchNode< Element >* >( this ); }

template < class Element >
const TBinTreeLeafNode< Element >* TBinTreeNode< Element >::AsLeafNode() const { ASSERT( m_isLeaf ); return static_cast< const TBinTreeLeafNode< Element >* >( this ); }
template < class Element >
const TBinTreeBranchNode< Element >* TBinTreeNode< Element >::AsBranchNode() const { ASSERT( !m_isLeaf ); return static_cast< const TBinTreeBranchNode< Element >* >( this ); }

template < class Element >
Uint32 TBinTreeNode< Element >::ComputeSize() const
{
	if ( m_isLeaf )
	{
		const CBinTreeLeafNode* me = static_cast< const CBinTreeLeafNode* >( this );
		return sizeof( TBinTreeLeafNode< Element > ) + ( sizeof( Element ) * me->m_elementsCount );
	}
	else
	{
		const CBinTreeBranchNode* me = static_cast< const CBinTreeBranchNode* >( this );
		Uint32 size1 = me->HasChild( 0 ) ? me->ChildNode( 0 )->ComputeSize() : 0;
		Uint32 size2 = me->HasChild( 1 ) ? me->ChildNode( 1 )->ComputeSize() : 0;
		return sizeof( TBinTreeBranchNode< Element > ) + size1 + size2;
	}
}

template < class Element >
Bool TBinTreeNode< Element >::ContainsElement( Element ind ) const
{
	if ( m_isLeaf )
	{
		const CBinTreeLeafNode* me = static_cast< const CBinTreeLeafNode* >( this );
		for( Uint32 i = 0; i < me->m_elementsCount; ++i )
		{
			if ( me->m_elements[ i ] == ind )
				return true;
		}
		return false;
	}
	else
	{
		const CBinTreeBranchNode* me = static_cast< const CBinTreeBranchNode* >( this );
		return (me->HasChild( 0 ) && me->ChildNode( 0 )->ContainsElement( ind )) || (me->HasChild( 1 ) && me->ChildNode( 1 )->ContainsElement( ind ));
	}
}

// it is a set of separate functions because otherwise we couldn't partially specialize function template of class template
template < class CBinTreeNode, class Functor >
RED_INLINE void IterateElementsInBoundings( const CBinTreeNode* node, const Vector2& bboxMin, const Vector2& bboxMax, Functor& functor )
{
	struct Local : public Red::System::NonCopyable
	{
		Local( const Vector2& bboxMin, const Vector2& bboxMax, Functor& functor )
			: m_bboxMin( bboxMin )
			, m_bboxMax( bboxMax )
			, m_functor( functor ) {}
		void LeafNodeTest( const typename CBinTreeNode::CBinTreeLeafNode* leafNode )
		{
			for (Uint32 i = 0; i < leafNode->m_elementsCount; ++i)
			{
				m_functor( leafNode->m_elements[ i ] );
			}
		}
		void BranchNodeTestRec( const typename CBinTreeNode::CBinTreeBranchNode* branchNode, const Vector2& branchBBoxMin, const Vector2& branchBBoxMax )
		{
			Int32 axis = branchNode->m_cutAxis;
			if ( MathUtils::GeometryUtils::RangeOverlap1D( m_bboxMin.A[ axis ], m_bboxMax.A[ axis ], branchBBoxMin.A[ axis ], branchNode->m_cutPoint ) )
			{
				CBinTreeNode* node = branchNode->ChildNode( 0 );
				if ( node->m_isLeaf )
				{
					LeafNodeTest( static_cast< typename CBinTreeNode::CBinTreeLeafNode* >( node ) );
				}
				else
				{
					Vector2 bboxMax = branchBBoxMax;
					bboxMax.A[ axis ] = branchNode->m_cutPoint;
					BranchNodeTestRec( static_cast< typename CBinTreeNode::CBinTreeBranchNode* >( node ), branchBBoxMin, bboxMax );
				}
			}
			if ( MathUtils::GeometryUtils::RangeOverlap1D( m_bboxMin.A[ axis ], m_bboxMax.A[ axis ], branchNode->m_cutPoint, branchBBoxMax.A[ axis ] ) )
			{
				CBinTreeNode* node = branchNode->ChildNode( 1 );
				if ( node->m_isLeaf )
				{
					LeafNodeTest( static_cast< typename CBinTreeNode::CBinTreeLeafNode* >( node ) );
				}
				else
				{
					Vector2 bboxMin = branchBBoxMin;
					bboxMin.A[ axis ] = branchNode->m_cutPoint;
					BranchNodeTestRec( static_cast< typename CBinTreeNode::CBinTreeBranchNode* >( node ), bboxMin, branchBBoxMax );
				}
			}
		}
		Vector2							m_bboxMin;
		Vector2							m_bboxMax;
		Functor&						m_functor;
	} local( bboxMin, bboxMax, functor );
	if ( node->m_isLeaf )
	{
		local.LeafNodeTest( node->AsLeafNode() );
	}
	else
	{
		Vector2 branchBBoxMin( -FLT_MAX, -FLT_MAX );
		Vector2 branchBBoxMax( FLT_MAX, FLT_MAX );

		local.BranchNodeTestRec( node->AsBranchNode(), branchBBoxMin, branchBBoxMax );
	}
}


template < class CBinTreeNode, class Functor >
RED_INLINE static typename CBinTreeNode::ElementType FindBinTreeClosestElement( const CBinTreeNode* node, const Vector2& pos, Functor& acceptor, Float& maxDistSq )
{
	struct Local : public Red::System::NonCopyable
	{
		Local( const Vector2& pos, Functor& acceptor, Float& maxDistSq )
			: m_pos( pos )
			, m_acceptor( acceptor )
			, m_maxDistSq( maxDistSq )
			, m_ret( Functor::InvalidElement() )
		{}
		void LeafNodeTest( const typename CBinTreeNode::CBinTreeLeafNode* leafNode )
		{
			for (Uint32 i = 0; i < leafNode->m_elementsCount; ++i)
			{
				Float distSq = m_acceptor( leafNode->m_elements[ i ] );
				if ( distSq < m_maxDistSq )
				{
					m_maxDistSq = distSq;
					m_ret = leafNode->m_elements[ i ];
				}
			}
		}
		void NodeTestRec( const CBinTreeNode* node, Vector2* regionRect )
		{
			if ( node->m_isLeaf )
			{
				LeafNodeTest( node->AsLeafNode() );
				return;
			}

			const typename CBinTreeNode::CBinTreeBranchNode* branchNode = node->AsBranchNode();
			Uint32 cutAxis = branchNode->m_cutAxis;
			// determine closer child
			Uint32 closerChild = m_pos.A[ cutAxis ] <= branchNode->m_cutPoint ? 0 : 1;
			Uint32 farChild = !closerChild;

			// test closer child
			{
				Float prevBounding = regionRect[ farChild ].A[ cutAxis ];
				regionRect[ farChild ].A[ cutAxis ] = branchNode->m_cutPoint;
				NodeTestRec( branchNode->ChildNode( closerChild ), regionRect );
				regionRect[ farChild ].A[ cutAxis ] = prevBounding;
			}

			// test other child
			{
				// try to skip the test
				Vector2 closestPoint;
				MathUtils::GeometryUtils::ClosestPointToRectangle2D( regionRect[ 0 ], regionRect[ 1 ], m_pos, closestPoint );
				Float distSq = (closestPoint - m_pos).SquareMag();

				if ( distSq >= m_maxDistSq )
				{
					return;
				}

				// do test
				Float prevBounding = regionRect[ closerChild ].A[ cutAxis ];
				regionRect[ closerChild ].A[ cutAxis ] = branchNode->m_cutPoint;
				NodeTestRec( branchNode->ChildNode( farChild ), regionRect );
				regionRect[ closerChild ].A[ cutAxis ] = prevBounding;
			}
		}
		// initial recursive test is a little different - and so 
		void NodeInitialTestRec( const CBinTreeNode* node, Vector2* regionRect )
		{
			if ( node->m_isLeaf )
			{
				LeafNodeTest( node->AsLeafNode() );
				return;
			}
			const typename CBinTreeNode::CBinTreeBranchNode* branchNode = node->AsBranchNode();
			Uint32 cutAxis = branchNode->m_cutAxis;
			// determine closer child
			Uint32 closerChild = m_pos.A[ cutAxis ] <= branchNode->m_cutPoint ? 0 : 1;
			Uint32 farChild = !closerChild;

			// test closer child
			{
				Float prevBounding = regionRect[ farChild ].A[ cutAxis ];
				regionRect[ farChild ].A[ cutAxis ] = branchNode->m_cutPoint;
				NodeInitialTestRec( branchNode->ChildNode( closerChild ), regionRect );
				regionRect[ farChild ].A[ cutAxis ] = prevBounding;
			}

			// test other child
			{
				// try to skip the test
				Float distSq = branchNode->m_cutPoint - m_pos.A[ cutAxis ];
				distSq *= distSq;
				
				if ( distSq >= m_maxDistSq )
				{
					return;
				}

				// do test
				Float prevBounding = regionRect[ closerChild ].A[ cutAxis ];
				regionRect[ closerChild ].A[ cutAxis ] = branchNode->m_cutPoint;
				NodeTestRec( branchNode->ChildNode( farChild ), regionRect );
				regionRect[ closerChild ].A[ cutAxis ] = prevBounding;
			}
		}
		const Vector2&				m_pos;
		Functor&					m_acceptor;
		Float						m_maxDistSq;
		typename CBinTreeNode::ElementType	m_ret;
	} local( pos, acceptor, maxDistSq );
	Vector2 branchBBox[2];
	branchBBox[ 0 ].Set( -FLT_MAX, -FLT_MAX );
	branchBBox[ 1 ].Set( FLT_MAX, FLT_MAX );

	local.NodeInitialTestRec( node, branchBBox );

	maxDistSq = local.m_maxDistSq;

	return local.m_ret;
}

template< class OutputType >
struct FindBinTreeNClosestElementsContext
{
	FindBinTreeNClosestElementsContext( Float maxDistSq, Uint32 elementsToFind, OutputType* outElements )
		: m_maxDistSq( maxDistSq )
		, m_elementsToFind( elementsToFind )
		, m_elementsFound( outElements )
		, m_elementsFoundCount( 0 ) {}

	void StoreOutput( const OutputType& outputType, Uint32 index ) { m_elementsFound[ index ] = outputType; }

	Float						m_maxDistSq;
	Uint32						m_elementsToFind;
	OutputType*					m_elementsFound;
	Uint32						m_elementsFoundCount;
	Float						m_elementsFoundDist[64];
};

template < class CBinTreeNode, class Functor, class Context >
RED_INLINE static Uint32 FindBinTreeNClosestElements( const CBinTreeNode* node, const Vector2& pos, Functor& acceptor, Context& context )
{
	struct UberAcceptor : public Red::System::NonCopyable
	{
		UberAcceptor( Functor& acceptor, Context& context ) 
			: m_acceptor( acceptor )
			, m_context( context ) {}

		Float operator()( const typename CBinTreeNode::ElementType& elem )
		{
			Float distSq = this->m_acceptor( elem );
			auto& context = this->m_context;
			if ( distSq < context.m_maxDistSq )
			{
				Uint32 elementIndex = 0;
				for ( ; elementIndex < context.m_elementsFoundCount; ++elementIndex )
				{
					if ( context.m_elementsFoundDist[ elementIndex ] > distSq )
					{
						break;
					}
				}
				if ( context.m_elementsFoundCount < context.m_elementsToFind )
				{
					++context.m_elementsFoundCount;
				}
				ASSERT( elementIndex < context.m_elementsFoundCount );
				for ( Uint32 i = context.m_elementsFoundCount; i > elementIndex; --i )
				{
					context.m_elementsFound[ i ] = context.m_elementsFound[ i-1 ];
					context.m_elementsFoundDist[ i ] = context.m_elementsFoundDist[ i-1 ];
				}
				context.StoreOutput( elem, elementIndex );
				context.m_elementsFoundDist[ elementIndex ] = distSq;
				
				if ( context.m_elementsToFind == context.m_elementsFoundCount )
				{
					context.m_maxDistSq = context.m_elementsFoundDist[ context.m_elementsFoundCount - 1 ];
				}

				return context.m_maxDistSq;
			}

			return FLT_MAX;
		}
		static typename CBinTreeNode::ElementType InvalidElement() { return Functor::InvalidElement(); }

		Functor&						m_acceptor;
		Context&						m_context;
	} uberAcceptor( acceptor, context );

	FindBinTreeClosestElement( node, pos, uberAcceptor, context.m_maxDistSq );

	return context.m_elementsFoundCount;
}

template < class CBinTreeNode, class Acceptor >
RED_INLINE static typename CBinTreeNode::ElementType FindBinTreeElement( const CBinTreeNode* node, const Vector2& pos, Acceptor& acceptor )
{
	// search through tree
	while ( !node->m_isLeaf )
	{
		const typename CBinTreeNode::CBinTreeBranchNode* branchNode = static_cast< const typename CBinTreeNode::CBinTreeBranchNode* >( node );
		node = branchNode->ChildNode( (pos.A[ static_cast< Uint32 >( branchNode->m_cutAxis ) ] <= branchNode->m_cutPoint ? 0 : 1 ) );
	}
	// search the leaf node
	const typename CBinTreeNode::CBinTreeLeafNode* leafNode = static_cast< const typename CBinTreeNode::CBinTreeLeafNode* >( node );
	// iterate over all triangles
	for (Uint32 i = 0; i < leafNode->m_elementsCount; ++i)
	{
		if ( acceptor( leafNode->m_elements[ i ] ) )
		{
			return leafNode->m_elements[ i ];
		}
	}
	return Acceptor::InvalidElement();
}

template < class CBinTreeNode >
RED_INLINE static const typename CBinTreeNode::CBinTreeLeafNode* FindBinTreeLeaf( const CBinTreeNode* node, const Vector2& pos, Box2& bbox )
{
	// search through tree
	while ( !node->m_isLeaf )
	{
		const typename CBinTreeNode::CBinTreeBranchNode* branchNode = static_cast< const typename CBinTreeNode::CBinTreeBranchNode* >( node );
		if ( pos.A[ static_cast< Uint32 >( branchNode->m_cutAxis ) ] <= branchNode->m_cutPoint )
		{
			bbox.Max.A[ static_cast< Uint32 >( branchNode->m_cutAxis ) ] = branchNode->m_cutPoint;
			node = branchNode->ChildNode( 0 );
		}
		else
		{
			bbox.Min.A[ static_cast< Uint32 >( branchNode->m_cutAxis ) ] = branchNode->m_cutPoint;
			node = branchNode->ChildNode( 1 );
		}
	}
	const typename CBinTreeNode::CBinTreeLeafNode* leafNode = static_cast< const typename CBinTreeNode::CBinTreeLeafNode* >( node );
	return leafNode;
}

//template < class _Iter >
//static Bool Sorted_AllElementsUnique( _Iter begin, _Iter end )
//{
//	for ( _Iter it = begin+1, prevIt = begin; it != end; prevIt = it++ )
//	{
//		if ( it->m_ending == prevIt->m_ending && it->m_element == prevIt->m_element )
//		{
//			return false;
//		}
//	}
//	return true;
//}

template < class BinTreeInput >
static TBinTreeNode< typename BinTreeInput::ElementType >* ComputeBinTreeOnPoints( BinTreeInput& input )
{
	struct Local
	{
		typedef TBinTreeNode< typename BinTreeInput::ElementType > CBinTreeNode;
		typedef TBinTreeLeafNode< typename BinTreeInput::ElementType > CBinTreeLeafNode;
		typedef TBinTreeBranchNode< typename BinTreeInput::ElementType > CBinTreeBranchNode;

		static void* GetAllocated( BinTreeInput& input, Uint32 index )
		{
			auto& dataBuffer = input.m_dataBuffer;
			return &input.m_dataBuffer[ index ];
		}

		static CBinTreeNode* GetAllocatedNode( BinTreeInput& input, Uint32 index )
		{
			return static_cast< CBinTreeNode* >( GetAllocated( input, index ) );
		}

		static Uint32 AllocateIndex( BinTreeInput& input, Uint32 size )
		{
			auto& dataBuffer = input.m_dataBuffer;
			Uint32 currentIndex = dataBuffer.Size();
			dataBuffer.Grow( size );
			return currentIndex;
		}

		static Uint32 CreateLeafNode( BinTreeInput& input )
		{
			Uint32 elementsCount = static_cast< Uint32 >( input.m_itEnd - input.m_itBegin );
			Uint32 leafNodeMemIndex = AllocateIndex( input, sizeof( CBinTreeLeafNode ) + (elementsCount * sizeof(typename BinTreeInput::ElementType) ) );
			CBinTreeLeafNode* leafNode = new (GetAllocated( input, leafNodeMemIndex )) CBinTreeLeafNode(elementsCount);
			Uint32 elementIndex = 0;
			for ( auto it = input.m_itBegin; elementIndex < elementsCount; ++it )
			{
				leafNode->m_elements[ elementIndex++ ] = BinTreeInput::GetElement( it );
			}
			return leafNodeMemIndex;
		}
		static Uint32 CreateTreeRec( BinTreeInput& input, Uint32 axis )
		{
			Uint32 elementsCount = static_cast< Uint32 >( input.m_itEnd - input.m_itBegin );

			if ( elementsCount <= BinTreeInput::MIN_NODE_ELEMENTS )
			{
				return CreateLeafNode( input );
			}

			input.SortInput( axis );

			// store input pointers
			auto itBegin = input.m_itBegin;
			auto itEnd = input.m_itEnd;

			// compute split point
			auto itMedian = input.m_itBegin + (elementsCount / 2);

			Float splitPoint = BinTreeInput::GetElementPosition( itMedian ).A[ axis ];

			// move split points right, as far as nodes has same coordinate value. Move at least one step, as we are 'end' (exclusive) iterator
			do
			{
				++itMedian;
			}
			while( itMedian != itEnd && BinTreeInput::GetElementPosition( itMedian ).A[ axis ] == splitPoint );

			if ( itMedian == itEnd )
			{
				return CreateLeafNode( input );
			}

			// We know that we decided to create branch node. So do the branch node and setup it later ( so he is allocated b4 other nodes ).
			Uint32 branchNodeMemIndex = AllocateIndex( input, sizeof( CBinTreeBranchNode ) );

			// split input and create child nodes
			Uint32 minChildNodeMemIndex;
			Uint32 maxChildNodeMemIndex;

			// create child nodes
			input.m_itEnd = itMedian;
			minChildNodeMemIndex = CreateTreeRec( input, axis ^ 1 );
			input.m_itBegin = itMedian;
			input.m_itEnd = itEnd;
			maxChildNodeMemIndex = CreateTreeRec( input, axis ^1 );

			CBinTreeBranchNode* branchNode = new ( GetAllocated( input, branchNodeMemIndex ) ) CBinTreeBranchNode( GetAllocatedNode( input, minChildNodeMemIndex ), GetAllocatedNode( input, maxChildNodeMemIndex ), splitPoint, axis );

			return branchNodeMemIndex;
		}
	};

	Local::CreateTreeRec( input, 0 );
	return reinterpret_cast< TBinTreeNode< typename BinTreeInput::ElementType >* >( &input.m_dataBuffer[ 0 ] );
}


template < class BinTreeInput >
static TBinTreeNode< typename BinTreeInput::ElementType >* ComputeBinTree( BinTreeInput& input )
{
	struct Local
	{
		typedef TBinTreeNode< typename BinTreeInput::ElementType > CBinTreeNode;
		typedef TBinTreeLeafNode< typename BinTreeInput::ElementType > CBinTreeLeafNode;
		typedef TBinTreeBranchNode< typename BinTreeInput::ElementType > CBinTreeBranchNode;

		static void* GetAllocated( BinTreeInput& input, Uint32 index )
		{
			return &input.m_dataBuffer[ index ];
		}

		static CBinTreeNode* GetAllocatedNode( BinTreeInput& input, Uint32 index )
		{
			return static_cast< CBinTreeNode* >( GetAllocated( input, index ) );
		}

		static Uint32 AllocateIndex( BinTreeInput& input, Uint32 size )
		{
			auto& dataBuffer = input.m_dataBuffer;
			Uint32 currentIndex = dataBuffer.Size();
			dataBuffer.Grow( size );
			return currentIndex;
		}
		static void* Allocate( BinTreeInput& input, Uint32 size )
		{
			return GetAllocated( input, AllocateIndex( input, size ) );
		}

		static Uint32 CreateLeafNode( BinTreeInput& input )
		{
			Uint32 elementsCount = static_cast< Uint32 >( (input.m_itEnd - input.m_itBegin) / 2 );
			Uint32 leafNodeMemIndex = AllocateIndex( input, sizeof( CBinTreeLeafNode ) + (elementsCount * sizeof(typename BinTreeInput::ElementType) ) );
			CBinTreeLeafNode* leafNode = new (GetAllocated( input, leafNodeMemIndex )) CBinTreeLeafNode(elementsCount);
			Uint32 elementIndex = 0;
			for ( auto it = input.m_itBegin; elementIndex < elementsCount; ++it )
			{
				ASSERT( it != input.m_itEnd );
				if  ( !it->m_ending )
				{
					leafNode->m_elements[ elementIndex++ ] = it->m_element;
				}

			}
			return leafNodeMemIndex;
		}
		static Uint32 CreateTreeRec( BinTreeInput& input )
		{
			ASSERT( ((input.m_itEnd - input.m_itBegin) & 1) == 0, TXT("Odd elements count ruin all algorithm!") );
			Uint32 elementsCount = static_cast< Uint32 >( (input.m_itEnd - input.m_itBegin) / 2 );
			// I don't care about performance much so this function is
			// both time and memory consuming.
			if ( elementsCount <= BinTreeInput::MIN_NODE_ELEMENTS )
			{
				return CreateLeafNode( input );
			}

			// Determine split axis
			Int32 bestScore[2];
			Uint32 bestSplitIndex[ 2 ];
			Int32 bestAxisNodes[ 2 ];
			Int32 bestMinChildNodes[ 2 ];
			Int32 bestMaxChildNodes[ 2 ];

			for ( Int32 axis = 0; axis < 2; ++axis )
			{
				input.SortInput( axis );

				Int32 axisNodes = 0;
				Int32 minChildNodes = 0;
				Int32 maxChildNodes = elementsCount;
				bestScore[ axis ] = 0;
				bestSplitIndex[ axis ] = 0;

				for ( auto it = input.m_itBegin; it != input.m_itEnd; ++it )
				{
					if ( !it->m_ending )
					{
						// new triangle begins - its never good split moment
						++minChildNodes;
						++axisNodes;
					}
					else
					{
						// triangle end
						--axisNodes;
						--maxChildNodes;

						// compute score
						Int32 score = Min( minChildNodes, maxChildNodes ) - axisNodes;

						// check if we have best split moment so far
						if ( score > bestScore[ axis ] )
						{
							bestSplitIndex[ axis ] = static_cast< Uint32 >( it - input.m_itBegin );
							bestScore[ axis ] = score;
							bestAxisNodes[ axis ] = axisNodes;
							bestMinChildNodes[ axis ] = minChildNodes;
							bestMaxChildNodes[ axis ] = maxChildNodes;
						}
					}
				}
			}

			Uint32 bestAxis =
				bestScore[ 0 ] == bestScore[ 1 ] ? GEngine->GetRandomNumberGenerator().Get< Uint32 >() & 1 :	// TODO: no random
				bestScore[ 0 ] > bestScore[ 1 ] ? 0 : 1;

			if ( bestScore[ bestAxis ] <= 2 )
			{
				return CreateLeafNode( input );
			}

			// We know that we decided to create branch node. So do the branch node and setup it later ( so he is allocated b4 other nodes ).
			Uint32 branchNodeMemIndex = AllocateIndex( input, sizeof( CBinTreeBranchNode ) );

			Uint32 splitIndex = bestSplitIndex[ bestAxis ];
			Uint32 minChildElements = bestMinChildNodes[ bestAxis ];
			Uint32 maxChildElements = bestMaxChildNodes[ bestAxis ];
			Uint32 axisElements = bestAxisNodes[ bestAxis ];

			// split input and create child nodes
			Uint32 minChildNodeMemIndex;
			Uint32 maxChildNodeMemIndex;

			// MIN node
			
			// first sort input to have it like in splitting algorithm
			if ( bestAxis != 1 )
			{
				input.SortInput( bestAxis );
			}
			//Sorted_AllElementsUnique( input.m_itBegin, input.m_itEnd );

			// store split point for further use
			Float splitPoint = input.GetElementPosition( *(input.m_itBegin + splitIndex) ).A[ bestAxis ];
			
			// now tricky part
			// a very simple, but not straight forward algorithm to gurantee we can reuse same table for recursive CreateTreeRec calls
			// we only care for 
			struct CustomComperator
			{
				RED_INLINE Bool operator()(const typename BinTreeInput::InputData& n1, const typename BinTreeInput::InputData& n2) const
				{
					if ( n1.m_ending == n2.m_ending )
					{
						return n1.m_element < n2.m_element;
					}
					else
					{
						return n1.m_ending < n2.m_ending;
					}
				}
			};
			struct CustomReverseComperator
			{
				RED_INLINE Bool operator()(const typename BinTreeInput::InputData& n1, const typename BinTreeInput::InputData& n2) const
				{
					if ( n1.m_ending == n2.m_ending )
					{
						return n1.m_element < n2.m_element;
					}
					else
					{
						return n1.m_ending > n2.m_ending;
					}
				}
			};

			::Sort( input.m_itBegin, input.m_itBegin + splitIndex + 1, CustomComperator() );			// take all element begins before splitindex
			::Sort( input.m_itBegin + splitIndex + 1, input.m_itEnd, CustomReverseComperator() );		// store all other begins (that came after splitindex) so they won't be overwritten in the process

			// rebuild ending of element that will go as min child node input
			for ( Uint32 i = 0; i < minChildElements; ++i )
			{
				auto itElem = input.m_itBegin + i;
				auto itElemShifted = itElem + minChildElements;
				ASSERT( !itElem->m_ending && itElemShifted->m_ending );
				*itElemShifted = *itElem;
				itElemShifted->m_ending = true;
			}

			// rebuild ending of elements that don't go to min child node
			{
				Uint32 leftofts = elementsCount - minChildElements;
				ASSERT( leftofts == maxChildElements - axisElements );
				for ( Uint32 i = 0; i < leftofts; ++i )
				{
					auto itElem = input.m_itEnd - 1 - i ;
					auto itElemShifted = itElem - leftofts;
					ASSERT( !itElem->m_ending && itElemShifted->m_ending );
					*itElemShifted = *itElem;
					itElemShifted->m_ending = true;
				}
			}

			//Sorted_AllElementsUnique( input.m_itBegin, input.m_itEnd );


			auto itBegin = input.m_itBegin;
			auto itEnd = input.m_itEnd;

			input.m_itEnd = input.m_itBegin + (minChildElements * 2);

			minChildNodeMemIndex = CreateTreeRec( input );

			input.m_itEnd = itEnd;

			// split input for MAX node
			input.SortInput( bestAxis );

			//Sorted_AllElementsUnique( input.m_itBegin, input.m_itEnd );

			::Sort( input.m_itBegin + splitIndex + 1, input.m_itEnd, CustomComperator() );					// take all element endings after splitindex
			::Sort( input.m_itBegin, input.m_itBegin + splitIndex + 1, CustomReverseComperator() );			// store all other endings
			
			// rebuild ending of element that will go as max child node input
			for ( Uint32 i = 0; i < maxChildElements; ++i )
			{
				auto itElem = itEnd - 1 - i;
				auto itElemShifted = itElem - maxChildElements;
				ASSERT( itElem->m_ending && !itElemShifted->m_ending );
				*itElemShifted = *itElem;
				itElemShifted->m_ending = false;
			}

			// rebuild ending of elements that don't go to min child node
			{
				Uint32 leftofts = elementsCount - maxChildElements;
				ASSERT( leftofts == minChildElements - axisElements );
				for ( Uint32 i = 0; i < leftofts; ++i )
				{
					auto itElem = input.m_itBegin + i;
					auto itElemShifted = input.m_itBegin + leftofts + i;
					ASSERT( itElem->m_ending && !itElemShifted->m_ending );
					*itElemShifted = *itElem;
					itElemShifted->m_ending = false;
				}
			}

			//Sorted_AllElementsUnique( input.m_itBegin, input.m_itEnd );
			
			input.m_itBegin = input.m_itEnd - (maxChildElements*2);

			maxChildNodeMemIndex = CreateTreeRec( input );

			input.m_itBegin = itBegin;

			CBinTreeBranchNode* branchNode = new ( GetAllocated( input, branchNodeMemIndex ) ) CBinTreeBranchNode( GetAllocatedNode( input, minChildNodeMemIndex ), GetAllocatedNode( input, maxChildNodeMemIndex ), splitPoint, bestAxis );
			RED_UNUSED( branchNode );
			return branchNodeMemIndex;
		}
	};
	Local::CreateTreeRec( input );
	return reinterpret_cast< TBinTreeNode< typename BinTreeInput::ElementType >* >( &input.m_dataBuffer[ 0 ] );
}

};			// namespace BinTree

};			// namespace PathLib

RED_WARNING_POP()
