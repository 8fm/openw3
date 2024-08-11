#pragma once

namespace PathLib
{

namespace KD 
{
///////////////////////////////////////////////////////////////////////////////////
// KDBox2
KDBox2::KDBox2()
{}
KDBox2::KDBox2( eResetState )
	: Min( FLT_MAX, FLT_MAX )
	, Max( -FLT_MAX, -FLT_MAX )
{}
KDBox2::KDBox2( const Vector2& min, const Vector2& max )
	: Min( min )
	, Max( max )
{}
KDBox2::KDBox2( const Box& bbox )
	: Min( bbox.Min.AsVector2() )
	, Max( bbox.Max.AsVector2() )
{}

Bool KDBox2::Intersect( const KDBox2& box ) const
{
	if( Min.X > box.Max.X || box.Min.X > Max.X ) return false;
	if( Min.Y > box.Max.Y || box.Min.Y > Max.Y ) return false;

	return true;
}
Bool KDBox2::Intersect( const KDBox3& box ) const
{
	if( Min.X > box.Max.X || box.Min.X > Max.X ) return false;
	if( Min.Y > box.Max.Y || box.Min.Y > Max.Y ) return false;

	return true;
}

Bool KDBox2::Encompass( const KDBox2& box ) const
{
	if (( Max.X < box.Max.X ) || ( Min.X > box.Min.X ) ||
		( Max.Y < box.Max.Y ) || ( Min.Y > box.Min.Y ))
	{
		return false;
	}

	return true;
}
Bool KDBox2::Encompass( const KDBox3& box ) const
{
	if (( Max.X < box.Max.X ) || ( Min.X > box.Min.X ) ||
		( Max.Y < box.Max.Y ) || ( Min.Y > box.Min.Y ))
	{
		return false;
	}

	return true;
}

Bool KDBox2::PointTest( const Vector2& v ) const
{
	if ( Min.X > v.X || v.X > Max.X ||
		 Min.Y > v.Y || v.Y > Max.Y )
		return false;

	return true;
}
Bool KDBox2::PointTest( const Vector3& v ) const
{
	return PointTest( v.AsVector2() );
}

///////////////////////////////////////////////////////////////////////////////////
// KDBox3
KDBox3::KDBox3()
{}
KDBox3::KDBox3( eResetState )
	: Min( FLT_MAX, FLT_MAX, FLT_MAX )
	, Max( -FLT_MAX, -FLT_MAX, -FLT_MAX )
{}
KDBox3::KDBox3( const Vector3& min, const Vector3& max )
	: Min( min )
	, Max( max )
{}
KDBox3::KDBox3( const Box& bbox )
	: Min( bbox.Min.AsVector3() )
	, Max( bbox.Max.AsVector3() )
{}
Bool KDBox3::Intersect( const KDBox2& box ) const
{
	if( Min.X > box.Max.X || box.Min.X > Max.X ) return false;
	if( Min.Y > box.Max.Y || box.Min.Y > Max.Y ) return false;

	return true;
}
Bool KDBox3::Intersect( const KDBox3& box ) const
{
	if( Min.X > box.Max.X || box.Min.X > Max.X ) return false;
	if( Min.Y > box.Max.Y || box.Min.Y > Max.Y ) return false;
	if( Min.Z > box.Max.Z || box.Min.Z > Max.Z ) return false;

	return true;
}

Bool KDBox3::Encompass( const KDBox2& box ) const
{
	if (( Max.X < box.Max.X ) || ( Min.X > box.Min.X ) ||
		( Max.Y < box.Max.Y ) || ( Min.Y > box.Min.Y ))
	{
		return false;
	}

	return true;
}
Bool KDBox3::Encompass( const KDBox3& box ) const
{
	if (( Max.X < box.Max.X ) || ( Min.X > box.Min.X ) ||
		( Max.Y < box.Max.Y ) || ( Min.Y > box.Min.Y ) ||
		( Max.Z < box.Max.Z ) || ( Min.Z > box.Min.Z ))
	{
		return false;
	}

	return true;
}

Bool KDBox3::PointTest( const Vector2& v ) const
{
	if ( Min.X > v.X || v.X > Max.X ||
		 Min.Y > v.Y || v.Y > Max.Y )
		return false;

	return true;
}
Bool KDBox3::PointTest( const Vector3& v ) const
{
	if ( Min.X > v.X || v.X > Max.X ||
		 Min.Y > v.Y || v.Y > Max.Y ||
		 Min.Z > v.Z || v.Z > Max.Z )
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// CCel
template < class Acceptor >
RED_INLINE CNavNode* CCel::FindClosestNode( const Vector2& pos, Float& closestDistSq, Acceptor& acceptor )
{
	if ( !RequestConstruct() )
	{
		return nullptr;
	}

	const BinTree::TBinTreeNode< ElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< ElementType >* >( m_binTree.Data() );

	return BinTree::FindBinTreeClosestElement( rootNode, pos, acceptor, closestDistSq );
}

template < class Acceptor >
RED_INLINE Uint32 CCel::FindNClosestNodes( const Vector2& pos, Float& closestDistSq, Acceptor& acceptor, MultiNodesTestContext& context )
{
	if ( !RequestConstruct() )
	{
		return context.m_elementsFoundCount;
	}

	const BinTree::TBinTreeNode< ElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< ElementType >* >( m_binTree.Data() );

	return BinTree::FindBinTreeNClosestElements( rootNode, pos, acceptor, context );
}

template < class Functor >
RED_INLINE Bool CCel::IterateNodes( const KDBox& kdBox, Functor& functor )
{
	if ( kdBox.Encompass( m_celBox ) )
	{
		for ( auto it = m_navNodes.Begin(), end = m_navNodes.End(); it != end; ++it )
		{
			functor( (*it) );
		}
		return true;
	}

	if ( !RequestConstruct() )
	{
		return true;
	}

	const BinTree::TBinTreeNode< ElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< ElementType >* >( m_binTree.Data() );

	IterateElementsInBoundings( rootNode, kdBox.Min, kdBox.Max, functor );

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
// CNodeMap
RED_INLINE Uint32 CNodeMap::GetCelIndex( Int16 x, Int16 y )
{
	return x + y * m_celsX;
}

RED_INLINE CCel& CNodeMap::GetCel( Int16 x, Int16 y )
{
	return m_grid[ x + y * m_celsX ];
}
RED_INLINE void CNodeMap::ComputeCelBox( Int16 x, Int16 y, KDBox& outCelBox )
{
	Float fx = x; Float fy = y;
	outCelBox.Min = m_gridBounds.Min + Vector2( fx * m_celSize.X, fy * m_celSize.Y );
	outCelBox.Max = m_gridBounds.Min + Vector2( (fx+1.f) * m_celSize.X, (fy+1.f) * m_celSize.Y );
}
RED_INLINE void CNodeMap::GetCelCenter( Int16 x, Int16 y, Vector2& outCelCenter )
{
	outCelCenter = m_gridBounds.Min + Vector2( (Float( x ) + 0.5f) * m_celSize.X, (Float( y ) + 0.5f) * m_celSize.Y );
}
RED_INLINE void CNodeMap::PosToCoord( const Vector2& pos, Int16& x, Int16& y )
{
	Vector2 l = (pos - m_gridBounds.Min);
	l.X /= m_celSize.X;
	l.Y /= m_celSize.Y;
	x = Int16( l.X );
	y = Int16( l.Y );
	x = Clamp< Int16 >( x, 0, m_celsX-1 );
	y = Clamp< Int16 >( y, 0, m_celsY-1 );
}

template < class Acceptor >
RED_INLINE CNavNode* CNodeMap::FindClosestNode( const Vector3& pos, Float& maxDist, Acceptor& acceptor )
{
	Populate();

	if ( !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_gridBounds.Min, m_gridBounds.Max, pos.AsVector2(), maxDist ) )
	{
		return nullptr;
	}

	Int16 baseX, baseY;
	PosToCoord( pos.AsVector2(), baseX, baseY );

	Float closestNodeSq = maxDist*maxDist;

	CNavNode* closestNode = GetCel( baseX, baseY ).FindClosestNode( pos.AsVector2(), closestNodeSq, acceptor );

	// TODO: Optimize all this shit
	Int16 x0, y0, x1, y1;
	{
		Float closestNodeDist = MSqrt( closestNodeSq );
		
		KDBox testBox( Vector2( pos.X - closestNodeDist, pos.Y - closestNodeDist ), Vector2( pos.X + closestNodeDist, pos.Y + closestNodeDist ) );

		PosToCoord( testBox.Min, x0, y0 );
		PosToCoord( testBox.Max, x1, y1 );
	}
	

	for( Int16 y = y0; y <= y1; ++y )
	{
		for ( Int16 x = x0; x <= x1; ++x )
		{
			if ( x != baseX && y != baseY )
			{
				CCel& cel = GetCel( x, y );
				const KDBox& celBox = cel.GetBoundings();
				Vector2 pointOut;
				MathUtils::GeometryUtils::ClosestPointToRectangle2D( celBox.Min, celBox.Max, pos.AsVector2(), pointOut );
				if ( ( pointOut - pos.AsVector2() ).SquareMag() >= closestNodeSq )
				{
					continue;
				}

				CNavNode* newClosestNode = cel.FindClosestNode( pos.AsVector2(), closestNodeSq, acceptor );
				if ( newClosestNode )
				{
					closestNode = newClosestNode;	
				}
			}
		}
	}

	maxDist = sqrt( closestNodeSq );

	return closestNode;
}
template < class Acceptor >
RED_INLINE CNavNode* CNodeMap::FindClosestNodeInBoundings( const Vector3& pos, const Box& boundings, Float& maxDist,Acceptor& acceptor )
{
	struct CelAcceptor : public DefaultAcceptor
	{
		CelAcceptor( Acceptor& acceptor, const Box& boundings )
			: m_acceptor( acceptor )
			, m_bbox( boundings ) {}

		Float operator()( CNavNode* node )
		{
			if ( !this->m_bbox.PointTest( node->GetPosition() ) )
			{
				return FLT_MAX;
			}
			return this->m_acceptor( node );
		}

		Acceptor&		m_acceptor;
		KDBox			m_bbox;
	} celAcceptor( acceptor, boundings );

	Populate();

	if ( !m_gridBounds.Intersect( celAcceptor.m_bbox ) )
	{
		return nullptr;
	}

	Int16 baseX,baseY;
	PosToCoord( pos.AsVector2(), baseX, baseY );

	Float closestNodeSq = maxDist*maxDist;

	CNavNode* closestNode = GetCel( baseX, baseY ).FindClosestNode( pos.AsVector2(), closestNodeSq, celAcceptor );

	// Go neighbour cels
	// TODO: Optimize all this shit
	Int16 x0, y0, x1, y1;
	PosToCoord( boundings.Min.AsVector2(), x0, y0 );
	PosToCoord( boundings.Max.AsVector2(), x1, y1 );

	for( Int16 y = y0; y <= y1; ++y )
	{
		for ( Int16 x = x0; x <= x1; ++x )
		{
			if ( x != baseX && y != baseY )
			{
				CCel& cel = GetCel( x, y );
				const KDBox& celBox = cel.GetBoundings();
				Vector2 pointOut;
				MathUtils::GeometryUtils::ClosestPointToRectangle2D( celBox.Min, celBox.Max, pos.AsVector2(), pointOut );
				if ( ( pointOut - pos.AsVector2() ).SquareMag() >= closestNodeSq )
				{
					continue;
				}

				CNavNode* newClosestNode = cel.FindClosestNode( pos.AsVector2(), closestNodeSq, acceptor );
				if ( newClosestNode )
				{
					closestNode = newClosestNode;	
				}
			}
		}
	}

	maxDist = sqrt( closestNodeSq );

	return closestNode;
}
template < class Acceptor >
RED_INLINE Uint32 CNodeMap::FindNClosestNodes( const Vector3& pos, Float& maxDist, Acceptor& acceptor, Uint32 outNodesMaxCount, CNavNode::Id* outNodes )
{
	Populate();

	if ( !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_gridBounds.Min, m_gridBounds.Max, pos.AsVector2(), maxDist ) )
	{
		return 0;
	}

	Float closestDistSq = maxDist*maxDist;

	Int16 baseX,baseY;
	PosToCoord( pos.AsVector2(), baseX, baseY );

	CCel::MultiNodesTestContext context( closestDistSq, outNodesMaxCount, outNodes );

	GetCel( baseX, baseY ).FindNClosestNodes( pos.AsVector2(), closestDistSq, acceptor, context );

	// TODO: Optimize all this shit
	Int16 x0, y0, x1, y1;
	{
		Float closestNodeDist = MSqrt( context.m_maxDistSq );

		KDBox testBox( Vector2( pos.X - closestNodeDist, pos.Y - closestNodeDist ), Vector2( pos.X + closestNodeDist, pos.Y + closestNodeDist ) );

		PosToCoord( testBox.Min, x0, y0 );
		PosToCoord( testBox.Max, x1, y1 );
	}


	for( Int16 y = y0; y <= y1; ++y )
	{
		for ( Int16 x = x0; x <= x1; ++x )
		{
			if ( x != baseX && y != baseY )
			{
				CCel& cel = GetCel( x, y );
				const KDBox& celBox = cel.GetBoundings();
				Vector2 pointOut;
				MathUtils::GeometryUtils::ClosestPointToRectangle2D( celBox.Min, celBox.Max, pos.AsVector2(), pointOut );
				if ( ( pointOut - pos.AsVector2() ).SquareMag() >= context.m_maxDistSq )
				{
					continue;
				}

				cel.FindNClosestNodes( pos.AsVector2(), closestDistSq, acceptor, context );
			}
		}
	}

	maxDist = sqrt( closestDistSq );

	return context.m_elementsFoundCount;
}
template < class Handler >
RED_INLINE void CNodeMap::IterateNodes( const Box& boundings, Handler& handler )
{
	Populate();

	KDBox kdbox( boundings );

	if ( !m_gridBounds.Intersect( kdbox ) )
	{
		return;
	}

	Int16 x0, y0, x1, y1;
	PosToCoord( kdbox.Min, x0, y0 );
	PosToCoord( kdbox.Max, x1, y1 );

	for( Int16 y = y0; y <= y1; ++y )
	{
		for ( Int16 x = x0; x <= x1; ++x )
		{
			CCel& cel = GetCel( x, y );
			cel.IterateNodes( kdbox, handler );
		}
	}
}


};				// namespace KD

};				// namespace PathLib

