#include "build.h"
#include "pathlibKDTree.h"

#include "pathlibNodeSet.h"

#include "pathlibNavgraphHelper.inl"

namespace PathLib
{

namespace KD
{

///////////////////////////////////////////////////////////////////////////////
// CCel
///////////////////////////////////////////////////////////////////////////////
void CCel::Initialize( const KDBox& bbox )
{
	m_celBox = bbox;
}
Bool CCel::RequestConstruct()
{
	if ( m_constructed )
	{
		return !m_binTree.Empty();
	}

	m_constructed = true;

	m_binTree.ClearFast();

	if ( m_navNodes.Empty() )
	{
		return false;
	}

	struct BinTreeInput : public Red::System::NonCopyable
	{
		typedef const CNavNode* ElementType;

		typedef NodeList				InputBuffer;
		typedef InputBuffer::iterator	InputIterator;

		InputBuffer&							m_data;
		InputBuffer::iterator					m_itBegin;
		InputBuffer::iterator					m_itEnd;
		TDynArray< Uint8 >&						m_dataBuffer;

		enum { MIN_NODE_ELEMENTS = 3 };

		BinTreeInput( NodeList& inputData, TDynArray< Uint8 >& data )
			: m_data( inputData )
			, m_itBegin( inputData.Begin() )
			, m_itEnd( inputData.End() )
			, m_dataBuffer( data )	{}

		RED_INLINE static ElementType GetElement( InputIterator it )
		{
			return (*it);
		}
		RED_INLINE static const Vector3& GetElementPosition( InputIterator it )
		{
			return (*it)->GetPosition();
		}

		void SortInput( Uint32 dimm )
		{
			struct Comperator
			{
				Comperator( Uint32 dimm )
					: m_dimm( dimm ) {}
				RED_INLINE Bool operator()( const CNavNode* p1, const CNavNode* p2 ) const
				{
					return p1->GetPosition().A[ m_dimm ] < p2->GetPosition().A[ m_dimm ];
				}
				Uint32				m_dimm;
			};

			::Sort( m_itBegin, m_itEnd, Comperator( dimm ) );
		}
	} input( m_navNodes, m_binTree );

	PathLib::BinTree::ComputeBinTreeOnPoints( input );

	return !m_binTree.Empty();
}

void CCel::Invalidate()
{
	if ( m_constructed )
	{
		m_constructed = false;
		m_binTree.ClearFast();
	}
}

void CCel::NodeCreated( CNavNode* node )
{
	Invalidate();
	m_navNodes.PushBack( node );
	//PATHLIB_ASSERT( m_navNodes.Find( nodeId ) == m_navNodes.End() );
	//m_navNodes.Insert( nodeId );
}
void CCel::NodeRemoved( CNavNode* node )
{
	Invalidate();
	m_navNodes.Remove( node );
	//auto itFind = m_navNodes.Find( node );
	//PATHLIB_ASSERT( itFind != m_navNodes.End() );
	//m_navNodes.Erase( itFind );
}
void CCel::CollectionDone()
{
	//m_navNodes.HeapSort();
}



///////////////////////////////////////////////////////////////////////////////
// CNodeMap
///////////////////////////////////////////////////////////////////////////////
void CNodeMap::InitializeCelMap()
{
	// base constants setup
	m_defaultTest = m_navgraph.GetMaxNodesDistance();
	Float defaultCelSize = 2.f * m_defaultTest;

	Vector2 areaSize = m_gridBounds.Max - m_gridBounds.Min;
	Float celsX = ceilf( areaSize.X / defaultCelSize );
	Float celsY = ceilf( areaSize.Y / defaultCelSize );
	m_celSize.Set( areaSize.X / celsX, areaSize.Y / celsY );
	m_celsX = Max( Int16( celsX ), Int16( 1 ) );
	m_celsY = Max( Int16( celsY ), Int16( 1 ) );

	m_grid.ResizeFast( GetCelIndex( m_celsX, m_celsY )-1 );

	for ( Int16 y = 0; y < m_celsY; ++y )
	{
		for ( Int16 x = 0; x < m_celsX; ++x )
		{
			KDBox celBox;
			ComputeCelBox( x, y, celBox );
			GetCel( x, y ).Initialize( celBox );
		}
	}
}
void CNodeMap::Initialize()
{
	if ( m_isInitialized )
	{
		return;
	}
	m_navgraph.GetNodeFinder().Compute();
	//m_isInitialized = true;
	//AreaRegionId regionId = m_regionId;


	//// calculate boundings
	//{
	//	KDBox gridBounds( KDBox::RESET );

	//	auto funBounds =
	//		[ &gridBounds, regionId ] ( const CNavNode& node )
	//		{
	//			if ( node.GetAreaRegionId() == regionId )
	//			{
	//				const Vector3& pos = node.GetPosition();
	//				gridBounds.Min.X = Min( gridBounds.Min.X, pos.X );
	//				gridBounds.Max.X = Max( gridBounds.Max.X, pos.X );
	//				gridBounds.Min.Y = Min( gridBounds.Min.Y, pos.Y );
	//				gridBounds.Max.Y = Max( gridBounds.Max.Y, pos.Y );
	//			}
	//		};

	//	NavgraphHelper::ForAllNodes( m_navgraph, funBounds, true );

	//	m_gridBounds = gridBounds;
	//}

	//InitializeCelMap();
}
void CNodeMap::Clear()
{
	m_grid.ClearFast();
	m_isInitialized = false;
	m_isValid = false;
}

void CNodeMap::Populate()
{
	if ( !m_isValid )
	{
		// populate every node map
		m_navgraph.GetNodeFinder().Compute();
	}
}
void CNodeMap::Invalidate()
{
	Clear();
	// NOTICE: I could just clear grid cels and redo them later, but I want to 
}

void CNodeMap::CompactData()
{
	m_grid.Clear();
	m_isInitialized = false;
	m_isValid = false;
}

void CNodeMap::NodeCreated( CNavNode* node )
{
	if ( m_isValid )
	{
		const Vector3& pos = node->GetPosition();

		if ( m_gridBounds.PointTest( pos ) )
		{
			Int16 x, y;
			PosToCoord( pos.AsVector2(), x, y );
			auto& cel = GetCel( x, y );
			cel.NodeCreated( node );
		}
		else
		{
			Invalidate();
		}
		
	}
}

void CNodeMap::NodeRemoved( CNavNode* node )
{
	if ( m_isValid )
	{
		const Vector3& pos = node->GetPosition();
		Int16 x, y;
		PosToCoord( pos.AsVector2(), x, y );
		auto& cel = GetCel( x, y );
		cel.NodeRemoved( node );

		// invalidate node map if we removed last element from edge cel (so boundings are possibly changed)
		if ( cel.IsEmpty() && ( x == 0 || y == 0 || x == m_celsX-1 || y == m_celsY-1 )  )
		{
			Invalidate();
		}
	}
}

void CNodeMap::WriteToBuffer( CSimpleBufferWriter& writer )
{

}
Bool CNodeMap::ReadFromBuffer( CSimpleBufferReader& reader )
{
	return true;
}


void CNodeMap::PreCenteralInitialization( CNavGraph& navgraph, AreaRegionId regionId )
{
	if ( !m_isValid )
	{
		Clear();
		m_gridBounds = KDBox( KDBox::RESET );
	}
}
void CNodeMap::CentralInitializationBBoxUpdate( CNavNode& node )
{
	if ( !m_isValid )
	{
		const Vector3& pos = node.GetPosition();
		m_gridBounds.Min.X = Min( m_gridBounds.Min.X, pos.X );
		m_gridBounds.Max.X = Max( m_gridBounds.Max.X, pos.X );
		m_gridBounds.Min.Y = Min( m_gridBounds.Min.Y, pos.Y );
		m_gridBounds.Max.Y = Max( m_gridBounds.Max.Y, pos.Y );
	}
}
void CNodeMap::CentralInitilzationComputeCelMap()
{
	if ( !m_isValid )
	{
		InitializeCelMap();
	}
}
void CNodeMap::CentralInitializationCollectNode( CNavNode& node )
{
	if ( !m_isValid )
	{
		const Vector2& pos = node.GetPosition().AsVector2();
		Int16 x,y;
		PosToCoord( pos, x, y );
		GetCel( x, y ).CollectNode( &const_cast< CNavNode& >( node ) );
	}
}
void CNodeMap::PostCentralInitialization()
{
	if ( !m_isValid )
	{
		m_isInitialized = true;
		m_isValid = true;
	}
}

};			// namespace KD

};			// namespace PathLib
