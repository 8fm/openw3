
//////////////////////////////////////////////////////////////////////////
template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
void TkdTree<TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner>::CookedInit( Uint8* mem, size_t memSize, const kdTreeMediator< TThisTree >* mediator, Int32 maxNodes, Int32 bucketSize /*= 1*/ )
{
	mediator->GetPointSet( m_pointSet );

	const Int32 numPoints				= m_pointSet->GetPointNum();
	const Int32 pointIdxSize			= sizeof( typename TThisTree::TreeIdx ) * numPoints;
	const Int32 nodesSize				= sizeof( typename TThisTree::kdTreeNode ) * maxNodes;
	const Int32 bboxSize				= sizeof( typename TThisTree::Coord ) * m_pointSet->GetPointDim();
	const size_t pointIdxSizeAligned	= AlignOffset( pointIdxSize, TThisTree::Allocator::Alignment( pointIdxSize ) );
	const size_t nodesSizeAligned		= AlignOffset( nodesSize, TThisTree::Allocator::Alignment( nodesSize ) );
	const size_t bboxSizeAligned		= AlignOffset( bboxSize, TThisTree::Allocator::Alignment( bboxSize ) );
	m_memSize							= pointIdxSizeAligned + nodesSizeAligned + ( bboxSizeAligned * 2 );

	ASSERT( m_memSize == memSize, TXT("CookedInit() failed. Wrong data size. Please DEBUG.") )
	if ( m_memSize != memSize )
	{
		return;
	}

	m_pointsIdx = reinterpret_cast< typename TThisTree::TreeIdx* > ( mem );
	m_numNodes = maxNodes;
	m_nodes = reinterpret_cast< typename TThisTree::kdTreeNode* > ( mem + pointIdxSizeAligned );
	m_bndBoxHi = reinterpret_cast< typename TThisTree::Point > ( mem + pointIdxSizeAligned + nodesSizeAligned );
	m_bndBoxLo = reinterpret_cast< typename TThisTree::Point > ( mem + pointIdxSizeAligned + nodesSizeAligned + bboxSizeAligned );
	m_isCooked = true;
}

///////////////////////////////////////////////////////////////////////////////

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::Search( const Point q, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter /*= nullptr*/, TFilterOwner* filterOwner /*= nullptr*/, Float eps /*= 0.f*/ ) const
{
	if ( !m_nodes || collector.m_nnNum >= m_pointSet->GetPointNum() )
	{
		return;
	}

	const Float maxErr = KD_TREE_POW( 1.f + eps );

	kdTreeSearchContext< TThisTree, TFilterOwner > context( q, maxErr, collector, filter, filterOwner );

	const Dist distToTreeBox = kdTreeUtils::BoxDistance< TThisTree >( q, m_bndBoxLo, m_bndBoxHi, m_pointSet->GetPointDim() );

	ProcessNodeSearch( m_nodes[0], distToTreeBox, context );

	for ( Int32 i = 0; i<collector.m_nnNum; i++ ) 
	{
		collector.m_nnDist[i] = context.m_nn->IthSmallestKey(i);
		collector.m_nnIdx[i] = context.m_nn->IthSmallestInfo(i);
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const
{
	if ( node.IsLeaf() )
	{
		ProcessLeafSearch( node, boxDist, context );
	}
	else
	{
		ProcessSplitSearch( node, boxDist, context );
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const
{
	const Int32 numPoints = m_pointSet->GetPointNum();

	if ( numPoints != 0 && context.m_numVisited > numPoints )
	{
		return;
	}

	const Int32 cd = node.GetSplitAxis();
	const Coord cdVal = node.GetSplitVal();

	Coord cutDiff = context.m_queryPoint[ cd ] - cdVal;

	if ( cutDiff < 0 ) 
	{
		const Coord bndLo = node.GetSplitMin();

		const kdTreeNode* left = node.GetLeft();
		if ( left && !left->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *left, boxDist, context );
		}

		Coord boxDiff = bndLo - context.m_queryPoint[ cd ];

		if ( boxDiff < 0.f )
		{
			boxDiff = 0.f;
		}

		boxDiff = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff ), KD_TREE_POW( cutDiff ) ) );

		if ( boxDist * context.m_maxErr < context.m_nn->MaxKey() )
		{
			const kdTreeNode* right = node.GetRight();
			if ( right && !right->IsEmptyLeaf() )
			{
				ProcessNodeSearch( *right, boxDist, context );
			}
		}

	}
	else 
	{
		const Coord bndHi = node.GetSplitMax();

		const kdTreeNode* right = node.GetRight();
		if ( right && !right->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *right, boxDist, context );
		}

		Coord boxDiff = context.m_queryPoint[ cd ] - bndHi;

		if ( boxDiff < 0.f )
		{
			boxDiff = 0.f;
		}

		boxDiff = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff ), KD_TREE_POW( cutDiff ) ) );

		if ( boxDist * context.m_maxErr < context.m_nn->MaxKey() )
		{
			const kdTreeNode* left = node.GetLeft();
			if ( left && !left->IsEmptyLeaf() )
			{
				ProcessNodeSearch( *left, boxDist, context );
			}
		}
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner >
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeSearchContext< TThisTree, TFilterOwner >& context ) const
{
	Dist minDist = context.m_nn->MaxKey();

	const Int32 dim = m_pointSet->GetPointDim();
	const Int32 leafSize = node.GetLeafSize();
	const TreeIdx* leafData = node.GetLeafData( m_pointsIdx );

	for ( int i = 0; i < leafSize; i++ ) 
	{
		if ( false == context.Filter( leafData[ i ] ) )
		{
			continue;
		}

		const Coord* pp = (*m_pointSet)[ leafData[i] ];
		const Coord* qq = context.m_queryPoint;
		Dist dist = 0;

		Int32 d;
		Coord t;

		for( d = 0; d < dim; d++ ) 
		{
			t = *(qq++) - *(pp++);

			if ( ( dist = KD_TREE_SUM( dist, KD_TREE_POW(t) ) ) > minDist ) 
			{
				break;
			}
		}

		if ( d >= dim && ( context.m_allowSelfMatch || dist != 0.f ) ) 
		{ 
			context.m_nn->Insert( dist, leafData[i] );
			minDist = context.m_nn->MaxKey();
		}
	}

	context.m_numVisited += leafSize;
}

///////////////////////////////////////////////////////////////////////////////

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
Int32 TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::SearchR( const Point q, Dist radius, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter /*= nullptr*/, TFilterOwner* filterOwner /*= nullptr*/, Float eps ) const
{
	if ( !m_nodes )
	{
		return 0;
	}

	if ( collector.m_nnNum >= m_pointSet->GetPointNum() )
	{
		return -1;
	}

	const Float maxErr = KD_TREE_POW( 1.f + eps );

	kdTreeRadiusSearchContext< TThisTree, TFilterOwner > context( q, maxErr, collector, KD_TREE_POW( radius ), filter, filterOwner );

	const Dist distToTreeBox = kdTreeUtils::BoxDistance< TThisTree >( q, m_bndBoxLo, m_bndBoxHi, m_pointSet->GetPointDim() );

	ProcessNodeSearch( m_nodes[0], distToTreeBox, context );

	const Int32 nnNum = Min< Int32 >( collector.m_nnNum, context.m_numInRange );
	for ( Int32 i = 0; i<nnNum; i++ ) 
	{
		collector.m_nnDist[i] = context.m_nn->IthSmallestKey(i);
		collector.m_nnIdx[i] = context.m_nn->IthSmallestInfo(i);
	}

	return nnNum;
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const
{
	if ( node.IsLeaf() )
	{
		ProcessLeafSearch( node, boxDist, context );
	}
	else
	{
		ProcessSplitSearch( node, boxDist, context );
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const
{
	const Int32 numPoints = m_pointSet->GetPointNum();

	if ( numPoints != 0 && context.m_numVisited > numPoints ) 
	{ 
		return;
	}

	const Int32 cd = node.GetSplitAxis();
	const Coord cdVal = node.GetSplitVal();

	Coord cutDiff = context.m_queryPoint[ cd ] - cdVal;

	if ( cutDiff < 0.f ) 
	{
		const Coord bndLo = node.GetSplitMin();

		const kdTreeNode* left = node.GetLeft();
		if ( left && !left->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *left, boxDist, context );
		}

		Coord boxDiff = bndLo - context.m_queryPoint[ cd ];

		if ( boxDiff < 0.f )
		{
			boxDiff = 0.f;
		}

		boxDist = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff ), KD_TREE_POW( cutDiff ) ) );

		if ( boxDist * context.m_maxErr <= context.m_sqRadius )
		{
			const kdTreeNode* right = node.GetRight();
			if ( right && !right->IsEmptyLeaf() )
			{
				ProcessNodeSearch( *right, boxDist, context );
			}
		}
	}
	else
	{
		const Coord bndHi = node.GetSplitMax();

		const kdTreeNode* right = node.GetRight();
		if ( right && !right->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *right, boxDist, context );
		}

		Coord boxDiff = context.m_queryPoint[ cd ] - bndHi;

		if ( boxDiff < 0.f )
		{
			boxDiff = 0.f;
		}

		boxDist = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff ), KD_TREE_POW( cutDiff ) ) );

		if ( boxDist * context.m_maxErr <= context.m_sqRadius )
		{
			const kdTreeNode* left = node.GetLeft();
			if ( left && !left->IsEmptyLeaf() )
			{
				ProcessNodeSearch( *left, boxDist, context );
			}
		}
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeRadiusSearchContext< TThisTree, TFilterOwner >& context ) const
{
	const Int32 dim = m_pointSet->GetPointDim();
	const Int32 leafSize = node.GetLeafSize();
	const TreeIdx* leafData = node.GetLeafData( m_pointsIdx );

	for ( Int32 i = 0; i < leafSize; i++ ) 
	{
		if ( false == context.Filter( leafData[ i ] ) )
		{
			continue;
		}

		const Coord* pp = (*m_pointSet)[ leafData[i] ];
		const Coord* qq = context.m_queryPoint;
		Dist dist = 0;

		Int32 d;
		Coord t;

		for( d = 0; d < dim; d++ ) 
		{
			t = *(qq++) - *(pp++);

			if ( (dist = KD_TREE_SUM( dist, KD_TREE_POW(t) ) ) > context.m_sqRadius ) 
			{
				break;
			}
		}

		if ( d >= dim && ( context.m_allowSelfMatch || dist != 0.f ) )
		{
			context.m_nn->Insert( dist, leafData[i] );
			context.m_numInRange++;			
		}
	}

	context.m_numVisited += leafSize;
}

///////////////////////////////////////////////////////////////////////////////

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::FastSearch( const Point q, kdTreeNNCollector< TThisTree >& collector, typename kdTreeFilter< TFilterOwner >::FuncPtr filter /*= nullptr*/, TFilterOwner* filterOwner /*= nullptr*/, Float eps ) const
{
	const Int32 numPoints = m_pointSet->GetPointNum();

	if ( !m_nodes || collector.m_nnNum >= numPoints )
	{
		return;
	}

	const Float maxErr = KD_TREE_POW( 1.f + eps );

	kdTreeFastSearchContext< TThisTree, TFilterOwner > context( q, maxErr, collector, numPoints, filter, filterOwner );

	Dist boxDist = kdTreeUtils::BoxDistance< TThisTree >( q, m_bndBoxLo, m_bndBoxHi, m_pointSet->GetPointDim() );

	context.m_prioQueue->Insert( boxDist, &(m_nodes[0]) );

	while ( !context.m_prioQueue->Empty() && !( numPoints != 0 && context.m_numVisited > numPoints ) ) 
	{
		const kdTreeNode* np;

		context.m_prioQueue->ExtractMin( boxDist, np );

		if ( boxDist * context.m_maxErr >= context.m_nn->MaxKey() )
		{
			break;
		}

		ProcessNodeSearch( *np, boxDist, context );
	}

	for ( Int32 i = 0; i<collector.m_nnNum; i++ ) 
	{
		collector.m_nnDist[i] = context.m_nn->IthSmallestKey(i);
		collector.m_nnIdx[i] = context.m_nn->IthSmallestInfo(i);
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessNodeSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const
{
	if ( node.IsLeaf() )
	{
		ProcessLeafSearch( node, boxDist, context );
	}
	else
	{
		ProcessSplitSearch( node, boxDist, context );
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessSplitSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const
{
	Dist newDist;

	const Int32 cd = node.GetSplitAxis();
	const Coord cdVal = node.GetSplitVal();

	Coord cutDiff = context.m_queryPoint[ cd ] - cdVal;

	if ( cutDiff < 0.f ) 
	{
		const Coord bndLo = node.GetSplitMin();

		Coord boxDiff = bndLo - context.m_queryPoint[ cd ];

		if ( boxDiff < 0.f )
		{
			boxDiff = 0;
		}

		newDist = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff ), KD_TREE_POW( cutDiff ) ) );

		const kdTreeNode* right = node.GetRight();
		if ( right && !right->IsEmptyLeaf() )
		{
			context.m_prioQueue->Insert( newDist, right );
		}

		const kdTreeNode* left = node.GetLeft();
		if ( left && !left->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *left, boxDist, context );
		}
	}
	else 
	{
		const Coord bndHi = node.GetSplitMax();

		Coord boxDiff = context.m_queryPoint[ cd ] - bndHi;

		if ( boxDiff < 0.f )
		{
			boxDiff = 0.f;
		}

		newDist = (Dist) KD_TREE_SUM( boxDist, KD_TREE_DIFF( KD_TREE_POW( boxDiff), KD_TREE_POW( cutDiff ) ) );

		const kdTreeNode* left = node.GetLeft();
		if ( left && !left->IsEmptyLeaf() )
		{
			context.m_prioQueue->Insert( newDist, left );
		}

		const kdTreeNode* right = node.GetRight();
		if ( right && !right->IsEmptyLeaf() )
		{
			ProcessNodeSearch( *right, boxDist, context );
		}
	}
}

template< typename TCoord, typename TDist, typename TTreeIndex, class TPointSet, class TAlloc, class TFilterOwner > 
void TkdTree< TCoord, TDist, TTreeIndex, TPointSet, TAlloc, TFilterOwner >::ProcessLeafSearch( const kdTreeNode& node, Dist boxDist, kdTreeFastSearchContext< TThisTree, TFilterOwner >& context ) const
{
	const Int32 dim = m_pointSet->GetPointDim();
	const Int32 leafSize = node.GetLeafSize();
	const TreeIdx* leafData = node.GetLeafData( m_pointsIdx );

	Dist minDist = context.m_nn->MaxKey();

	for ( int i=0; i<leafSize; i++ ) 
	{
		if ( false == context.Filter( leafData[ i ] ) )
		{
			continue;
		}

		const Coord* pp = (*m_pointSet)[ leafData[i] ];
		const Coord* qq = context.m_queryPoint;
		Dist dist = 0;

		Int32 d;
		Coord t;

		for( d=0; d<dim; d++) 
		{
			t = *(qq++) - *(pp++);

			if( ( dist = KD_TREE_SUM( dist, KD_TREE_POW(t) ) ) > minDist ) 
			{
				break;
			}
		}

		if ( d >= dim && ( context.m_allowSelfMatch || dist != 0.f ) ) 
		{ 
			context.m_nn->Insert( dist, leafData[i] );
			minDist = context.m_nn->MaxKey();
		}
	}

	context.m_numVisited += leafSize;
}
