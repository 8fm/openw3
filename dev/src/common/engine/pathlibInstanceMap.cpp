#include "build.h"
#include "pathlibInstanceMap.h"

#include "pathlibNavmeshArea.h"
#include "pathlibBinTree.h"
#include "pathlibTerrainInfo.h"
#include "pathlibWorld.h"

namespace PathLib
{

typedef Uint16 InstanceMapElementType;


CInstanceMapCel::CInstanceMapCel()
{

}
CInstanceMapCel::~CInstanceMapCel()
{

}

void CInstanceMapCel::Build( CPathLibWorld& pathlib )
{
	struct RectInput : Red::System::NonCopyable
	{
		typedef InstanceMapElementType ElementType;
		static_assert( sizeof( ElementType ) == sizeof( CNavmesh::TriangleIndex ), "Incompatible navmesh bin tree types!" );

		RectInput( TDynArray< Uint8 >& dataBuffer )
			: m_dataBuffer( dataBuffer )											{}

		struct InputData
		{
			AreaId									m_element;
			Bool									m_ending;
			Vector2									m_bboxMin;
			Vector2									m_bboxMax;
		};
		TDynArray< InputData >					m_data;
		TDynArray< InputData >::iterator		m_itBegin;
		TDynArray< InputData >::iterator		m_itEnd;
		TDynArray< Uint8 >&						m_dataBuffer;

		enum { MIN_NODE_ELEMENTS = 1 };

		void SortInput( Uint32 dimm )
		{
			struct Comperator
			{
				Comperator( Uint32 dimm )
					: m_dimm( dimm ) {}
				RED_INLINE Bool operator()( const InputData& n1, const InputData& n2 ) const
				{
					Float f1 = n1.m_ending ? n1.m_bboxMax.A[ m_dimm ] : n1.m_bboxMin.A[ m_dimm ];
					Float f2 = n2.m_ending ? n2.m_bboxMax.A[ m_dimm ] : n2.m_bboxMin.A[ m_dimm ];
					if ( f1 == f2 )
						return n1.m_ending > n2.m_ending;
					return f1 < f2;
				}
				Uint32				m_dimm;
			};

			::Sort( m_itBegin, m_itEnd, Comperator( dimm ) );
		}

		RED_INLINE const Vector2& GetElementPosition( const InputData& d )
		{
			return d.m_ending ? d.m_bboxMax : d.m_bboxMin;
		}
	};

	m_data.ClearFast();

	if ( m_instances.Empty() )
	{
		return;
	}

	RectInput input( m_data );
	input.m_data.Resize( m_instances.Size() * 2 );
	for( Uint32 i = 0; i < m_instances.Size(); ++i )
	{
		CNavmeshAreaDescription* naviArea = pathlib.GetInstanceAreaDescription( m_instances[ i ] );
		const Box& bbox = naviArea->GetBBox();
		RectInput::InputData areaData;
		areaData.m_element = m_instances[ i ];
		areaData.m_ending = false;
		areaData.m_bboxMin = bbox.Min;
		areaData.m_bboxMax = bbox.Max;
		input.m_data[ (Uint32(i) << 1U) + 0 ] = areaData;
		areaData.m_ending = true;
		input.m_data[ (Uint32(i) << 1U) + 1 ] = areaData;

	}
	input.m_itBegin = input.m_data.Begin();
	input.m_itEnd = input.m_data.End();

	BinTree::ComputeBinTree( input );
}
void CInstanceMapCel::Clear()
{
	m_data.ClearFast();
}

CNavmeshAreaDescription* CInstanceMapCel::GetInstanceAt( CPathLibWorld& pathlib, const Vector3& v, AreaId ignoreId ) const
{
	if ( m_data.Empty() )
	{
		return NULL;
	}
	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );
	struct Acceptor : public Red::System::NonCopyable
	{
		Acceptor(  CPathLibWorld& pathlib, const Vector3& v, AreaId ignoreId )
			: m_pathlib( pathlib )
			, m_v( v )
			, m_ignoreId( ignoreId )
			, m_ret( NULL ) {}

		Bool operator()( AreaId areaId )
		{
			if ( m_ignoreId == areaId )
			{
				return false;
			}
			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( areaId );
			if ( !naviArea->IsLoaded() )
			{
				return false;
			}
			if ( naviArea->IsUsingTransformation() )
			{
				if ( naviArea->AsTransformedNavmeshArea()->TestLocation( m_v, CT_DEFAULT ) )
				{
					m_ret = naviArea;
					return true;
				}
			}
			else if ( naviArea->TestLocation( m_v, CT_DEFAULT ) )
			{
				m_ret = naviArea;
				return true;
			}
			return false;
		}

		static AreaId InvalidElement() { return INVALID_AREA_ID; }

		CPathLibWorld& m_pathlib;
		const Vector3& m_v;
		AreaId m_ignoreId;
		CNavmeshAreaDescription* m_ret;
	} acceptor( pathlib, v, ignoreId );
	FindBinTreeElement( rootNode, v.AsVector2(), acceptor );
	
	return acceptor.m_ret;
}

CNavmeshAreaDescription* CInstanceMapCel::GetInstanceAt( CPathLibWorld& pathlib, const Vector2& v, Float zMin, Float zMax, Float& zOut ) const
{
	if ( m_data.Empty() )
	{
		return NULL;
	}
	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );

	struct Acceptor : public Red::System::NonCopyable
	{
		Acceptor(  CPathLibWorld& pathlib, const Vector2& v, Float zMin, Float zMax, Float& zOut )
			: m_pathlib( pathlib )
			, m_v( v )
			, m_zMin( zMin )
			, m_zMax( zMax )
			, m_zOut( zOut )
			, m_ret( NULL ) {}

		Bool operator()( AreaId areaId )
		{
			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( areaId );
			if ( naviArea->IsLoaded() && naviArea->ComputeHeight( m_v, m_zMin, m_zMax, m_zOut ) )
			{
				m_zMin = m_zOut;
				m_ret = naviArea;
			}
			return false;
		}

		static AreaId InvalidElement()													{ return INVALID_AREA_ID; }

		CPathLibWorld& m_pathlib;
		const Vector2& m_v;
		Float m_zMin;
		Float m_zMax;
		Float& m_zOut;
		CNavmeshAreaDescription* m_ret;
	} acceptor( pathlib, v, zMin, zMax, zOut );
	FindBinTreeElement( rootNode, v.AsVector2(), acceptor );

	return acceptor.m_ret;
}

AreaId CInstanceMapCel::GetClosestInstance( CPathLibWorld& pathlib, const Vector3& v, Float maxDistSq ) const
{
	if ( m_data.Empty() )
	{
		return CAreaDescription::INVALID_ID;
	}

	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );
	struct Functor : public Red::System::NonCopyable
	{
		Functor( CPathLibWorld& pathlib, const Vector3& v )
			: m_pathlib( pathlib )
			, m_pos( v )
		{}

		Float operator()( AreaId areaId )
		{
			if ( m_processed.Find( areaId) != m_processed.End() )
				return FLT_MAX;

			// costly
			m_processed.Insert( areaId );

			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( areaId );
				
			const Box& bbox = naviArea->GetBBox();

			if ( bbox.Contains( m_pos ) )
			{
				CNavmesh* navmesh = naviArea->IsReady() ? naviArea->GetNavmesh() : nullptr;
				if ( navmesh )
				{
					Vector3 localPos = m_pos;
					naviArea->VWorldToLocal( localPos );
					if (  naviArea->GetNavmesh()->ComputeHeight( localPos.AsVector2(), localPos.Z - 15.f, localPos.Z + 10.f, localPos.Z ) )
					{
						return 0.f;
					}
				}
				
				return 0.01f;
			}
			
			return bbox.SquaredDistance( m_pos ) + 0.01f;
		}
		static AreaId InvalidElement() { return INVALID_AREA_ID; }

		CPathLibWorld&				m_pathlib;
		const Vector3&				m_pos;
		TSortedArray< AreaId >		m_processed;
	} functor( pathlib, v );
	return FindBinTreeClosestElement( rootNode, v.AsVector2(), functor, maxDistSq );
}

void CInstanceMapCel::GetCelAt( const Vector2& v, Uint32& outAreasCount, const AreaId*& outAreas, Box2& bbox ) const
{
	if ( m_data.Empty() )
	{
		outAreasCount = 0;
		return;
	}
	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );
	const BinTree::TBinTreeNode< InstanceMapElementType >::CBinTreeLeafNode* leafNode = FindBinTreeLeaf( rootNode, v, bbox );
	outAreas = leafNode->m_elements;
	outAreasCount = leafNode->m_elementsCount;
}

template < class Functor >
RED_INLINE AreaId CInstanceMapCel::IterateAreasAt( const Vector2& v, Functor& fun ) const
{
	if ( m_data.Empty() )
	{
		return INVALID_AREA_ID;
	}

	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );

	return FindBinTreeElement( rootNode, v, fun );
}

template < class Functor >
RED_INLINE void CInstanceMapCel::IterateAreasAt( const Vector2& bbMin, const Vector2& bbMax, Functor& fun ) const
{
	if ( m_data.Empty() )
	{
		return;
	}

	const BinTree::TBinTreeNode< InstanceMapElementType >* rootNode = reinterpret_cast< const BinTree::TBinTreeNode< InstanceMapElementType >* >( m_data.Data() );

	IterateElementsInBoundings( rootNode, bbMin, bbMax, fun );
}

CInstanceMap::CInstanceMap( CPathLibWorld& pathlib )
	: m_pathlib( pathlib )
	, m_isInitialized( false )
	, m_cellsInRow( 0 )
	, m_cells( 0 )
	, m_cellsCorner( 0.f, 0.f )
	, m_celSize( 1.f )
{

}
CInstanceMap::~CInstanceMap()
{
	Shutdown();
}


void CInstanceMap::GetCelCoordsAtPosition( const Vector2& pos, Int16& outX, Int16& outY ) const
{
	Vector2 diff = pos - m_cellsCorner;
	diff /= m_celSize;
	outX = Clamp( Int16(diff.X), Int16(0), Int16(m_cellsInRow-1) );
	outY = Clamp( Int16(diff.Y), Int16(0), Int16(m_cellsInRow-1) );
}

Bool CInstanceMap::GetAndTestCelCoordsAtPosition( const Vector2& pos, Int16& outX, Int16& outY ) const
{
	Vector2 diff = pos - m_cellsCorner;
	diff /= m_celSize;

	outX = Int16(diff.X);
	outY = Int16(diff.Y);

	if ( outX < 0 || outX >= Int16(m_cellsInRow) || outY < 0 || outY >= Int16(m_cellsInRow) )
	{
		return false;
	}
	return true;
}

Uint32 CInstanceMap::GetCelIndexFromCoords( Int16 x, Int16 y ) const
{
	return x * m_cellsInRow + y;
}
void CInstanceMap::GetCelCoordsFromIndex( Uint32 id, Int16& x, Int16& y ) const
{
	x = Int16(id / m_cellsInRow);
	y = Int16(id % m_cellsInRow);
}

void CInstanceMap::Shutdown()
{
	if ( m_isInitialized )
	{
		m_isInitialized = false;
		for ( auto it = m_map.Begin(), end = m_map.End(); it != end; ++it )
		{
			CInstanceMapCel* cel = *it;
			delete cel;
		}
		m_map.Clear();
	}
}

void CInstanceMap::Initialize()
{
	Shutdown();

	const CTerrainInfo& terrainInfo = m_pathlib.GetTerrainInfo();
	if ( terrainInfo.IsInitialized() )
	{
		m_cellsInRow = terrainInfo.GetTilesInRow();
		m_cells = terrainInfo.GetTilesCount();
		m_cellsCorner = terrainInfo.GetTerrainCorner();
		m_celSize = terrainInfo.GetTileSize();
	}
	else
	{
		m_cellsInRow = 1;
		m_cells = 1;
		m_cellsCorner.Set( -1024.f, -1024.f );
		m_celSize = 2048.f;
	}

	m_isInitialized = true;
	
	m_map.Resize( m_cells );

	for ( Uint32 celIndex = 0; celIndex < m_cells; ++ celIndex )
	{
		m_map[ celIndex ] = new CInstanceMapCel();
	}

	for ( auto it = m_pathlib.m_instanceAreas.Begin(), end = m_pathlib.m_instanceAreas.End(); it != end; ++it )
	{
		CNavmeshAreaDescription* area = it->m_area;

		const Box& bbox = area->GetBBox();

		Int16 minX, minY, maxX, maxY;
		GetCelCoordsAtPosition( bbox.Min.AsVector2(), minX, minY );
		GetCelCoordsAtPosition( bbox.Max.AsVector2(), maxX, maxY );

		for ( Int16 x = minX; x <= maxX; ++x )
		{
			for ( Int16 y = minY; y <= maxY; ++y )
			{
				Uint32 celIndex = GetCelIndexFromCoords( x, y );
				m_map[ celIndex ]->m_instances.PushBack( area->GetId() );
			}
		}
		area->SetInstanceMapBoundings( minX, minY, maxX, maxY );
	}

	for ( Uint32 celIndex = 0; celIndex < m_cells; ++ celIndex )
	{
		m_map[ celIndex ]->Build( m_pathlib );
	}
}

void CInstanceMap::AddInstance( CNavmeshAreaDescription* area )
{
	const Box& bbox = area->GetBBox();

	Int16 minX, minY, maxX, maxY;
	GetCelCoordsAtPosition( bbox.Min.AsVector2(), minX, minY );
	GetCelCoordsAtPosition( bbox.Max.AsVector2(), maxX, maxY );

	for ( Int16 x = minX; x <= maxX; ++x )
	{
		for ( Int16 y = minY; y <= maxY; ++y )
		{
			Uint32 celIndex = GetCelIndexFromCoords( x, y );
			m_map[ celIndex ]->m_instances.PushBack( area->GetId() );
			m_map[ celIndex ]->Build( m_pathlib );
		}
	}
	area->SetInstanceMapBoundings( minX, minY, maxX, maxY );
}
void CInstanceMap::RemoveInstance( CNavmeshAreaDescription* area )
{
	Int16 minX, minY, maxX, maxY;

	area->GetInstanceMapBoundings( minX, minY, maxX, maxY );

	for ( Int16 x = minX; x <= maxX; ++x )
	{
		for ( Int16 y = minY; y <= maxY; ++y )
		{
			Uint32 celIndex = GetCelIndexFromCoords( x, y );
			m_map[ celIndex ]->m_instances.Remove( area->GetId() );
			m_map[ celIndex ]->Build( m_pathlib );
		}
	}
}
void CInstanceMap::UpdateInstance( CNavmeshAreaDescription* area )
{
	Int16 prevMinX, prevMinY, prevMaxX, prevMaxY;
	Int16 minX, minY, maxX, maxY;

	area->GetInstanceMapBoundings( prevMinX, prevMinY, prevMaxX, prevMaxY );

	const Box& bbox = area->GetBBox();
	GetCelCoordsAtPosition( bbox.Min.AsVector2(), minX, minY );
	GetCelCoordsAtPosition( bbox.Max.AsVector2(), maxX, maxY );

	for ( Int16 x = prevMinX; x <= prevMaxX; ++x )
	{
		for ( Int16 y = prevMinY; y <= prevMaxY; ++y )
		{
			if ( x >= minX && x <= maxX && y >= minY && y <= maxY )
			{
				// cel will be still occuppied
				continue;
			}
			Uint32 celIndex = GetCelIndexFromCoords( x, y );
			m_map[ celIndex ]->m_instances.Remove( area->GetId() );
			m_map[ celIndex ]->Build( m_pathlib );
		}
	}

	for ( Int16 x = minX; x <= maxX; ++x )
	{
		for ( Int16 y = minY; y <= maxY; ++y )
		{
			Uint32 celIndex = GetCelIndexFromCoords( x, y );
			if ( x < prevMinX || x > prevMaxX || y < prevMinY || y > prevMaxY )
			{
				// new instance cel
				m_map[ celIndex ]->m_instances.PushBack( area->GetId() );
			}
			
			m_map[ celIndex ]->Build( m_pathlib );
		}
	}
}

CNavmeshAreaDescription* CInstanceMap::GetInstanceAt( const Vector3& v, AreaId ignoreId ) const
{
	Int16 x,y;
	GetCelCoordsAtPosition( v.AsVector2(), x, y );
	Uint32 celIndex = GetCelIndexFromCoords( x, y );
	return m_map[ celIndex ]->GetInstanceAt( m_pathlib, v, ignoreId );
}

CNavmeshAreaDescription* CInstanceMap::GetInstanceAt( const Vector2& v, Float zMin, Float zMax, Float& zOut ) const
{
	Int16 x,y;
	GetCelCoordsAtPosition( v.AsVector2(), x, y );
	Uint32 celIndex = GetCelIndexFromCoords( x, y );
	return m_map[ celIndex ]->GetInstanceAt( m_pathlib, v, zMin, zMax, zOut );
}

AreaId CInstanceMap::GetClosestIntance( const Vector3& v, Float maxDist ) const
{
	Int16 x,y;
	GetCelCoordsAtPosition( v.AsVector2(), x, y );
	Uint32 celIndex = GetCelIndexFromCoords( x, y );
	return m_map[ celIndex ]->GetClosestInstance( m_pathlib, v, maxDist*maxDist );
	// TODO: iterate areas around if maxDist is small enought
}

Bool CInstanceMap::IterateAreasAt( const Vector3& v, CInstanceFunctor* it ) const
{
	Int16 x,y;
	GetCelCoordsAtPosition( v.AsVector2(), x, y );
	Uint32 celIndex = GetCelIndexFromCoords( x, y );
	struct Functor : public CInstanceMapCel::DefaultIterator
	{
		Functor( const CPathLibWorld& pathlib, CInstanceFunctor* fun )
			: m_pathlib( pathlib ), m_fun( fun ) {}
		Bool operator()( AreaId naviAreaId )
		{
			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( naviAreaId );
			if ( !naviArea )
			{
				return false;
			}
			return m_fun->Handle( naviArea );
		}
		const CPathLibWorld& m_pathlib;
		CInstanceFunctor* m_fun;
	} funIterate( m_pathlib, it );
	return m_map[ celIndex ]->IterateAreasAt( v.AsVector2(), funIterate ) != INVALID_AREA_ID;
}
Bool CInstanceMap::GetCelAt( const Vector2& v, Uint32& outAreasCount, const AreaId*& outAreas, Box2& bbox ) const
{
	Int16 x,y;
	if ( !GetAndTestCelCoordsAtPosition( v.AsVector2(), x, y ) )
	{
		return false;
	}
	Float fX = x;
	Float fY = y;

	bbox.Min.X = m_cellsCorner.X + fX * m_celSize;
	bbox.Max.X = m_cellsCorner.X + (fX+1.f) * m_celSize;
	bbox.Min.Y = m_cellsCorner.Y + fY * m_celSize;
	bbox.Max.Y = m_cellsCorner.Y + (fY+1.f) * m_celSize;
	Uint32 celIndex = GetCelIndexFromCoords( x, y );
	m_map[ celIndex ]->GetCelAt( v, outAreasCount, outAreas, bbox );
	return true;
}

void CInstanceMap::IterateAreasAt( Int32 x, Int32 y, const Box& bb, CInstanceFunctor* it ) const
{
	struct Functor : public CInstanceMapCel::DefaultIterator
	{
		Functor( const Box& bb, CInstanceFunctor* it, const CPathLibWorld& pathlib ) 
			: m_bbox( bb ), m_functor( it ), m_pathlib( pathlib ) {}
		Bool operator()( AreaId naviAreaId )
		{
			// check if area was already visited
			auto itFind = ::LowerBound( m_visited.Begin(), m_visited.End(), naviAreaId );
			if ( itFind != m_visited.End() && *itFind == naviAreaId )
			{
				// already visited
				return false;
			}

			// add area to alread visited - until we reach cache limit
			if ( m_visited.Size() < m_visited.Capacity() )
			{
				Uint32 pos = PtrDiffToUint32( (void*)(itFind - m_visited.Begin()) );
				m_visited.Insert( pos, naviAreaId );
			}

			// get instance for processing
			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( naviAreaId );
			if ( !naviArea )
			{
				return false;
			}

			// z-boundings test. We didn't move it higher because it requires to find area by id
			const Box& areaBox = naviArea->GetBBox();
			if ( !MathUtils::GeometryUtils::RangeOverlap1D( m_bbox.Min.Z, m_bbox.Max.Z, areaBox.Min.Z, areaBox.Max.Z ) )
			{
				return false;
			}

			// do stuff
			return m_functor->Handle( naviArea );
		}
		const Box&						m_bbox;
		CInstanceFunctor*				m_functor;
		const CPathLibWorld&			m_pathlib;
		TStaticArray< AreaId, 128 >		m_visited;
	} functor( bb, it, m_pathlib );
	m_map[ GetCelIndexFromCoords( Int16( x ), Int16( y ) ) ]->IterateAreasAt( bb.Min.AsVector2(), bb.Max.AsVector2(), functor );
}
void CInstanceMap::IterateAreasAt( const Box& bb, CInstanceFunctor* it ) const
{
	Int16 minX, minY, maxX, maxY;
	GetCelCoordsAtPosition( bb.Min.AsVector2(), minX, minY );
	GetCelCoordsAtPosition( bb.Max.AsVector2(), maxX, maxY );

	if ( minX < 0 || minY < 0 || maxX < 0 || maxY < 0 )
	{
		return;
	}

	struct Functor : public CInstanceMapCel::DefaultIterator
	{
		Functor( const Box& bb, CInstanceFunctor* it, const CPathLibWorld& pathlib ) 
			: m_bbox( bb ), m_functor( it ), m_pathlib( pathlib ) {}
		Bool operator()( AreaId naviAreaId )
		{
			// check if area was already visited
			auto itFind = ::LowerBound( m_visited.Begin(), m_visited.End(), naviAreaId );
			if ( itFind != m_visited.End() && *itFind == naviAreaId )
			{
				// already visited
				return false;
			}

			// add area to alread visited - until we reach cache limit
			if ( m_visited.Size() < m_visited.Capacity() )
			{
				Uint32 pos = PtrDiffToUint32( (void*)(itFind - m_visited.Begin()) );
				m_visited.Insert( pos, naviAreaId );
			}

			// get instance for processing
			CNavmeshAreaDescription* naviArea = m_pathlib.GetInstanceAreaDescription( naviAreaId );
			if ( !naviArea )
			{
				return false;
			}

			// boundings test. We didn't move it higher because it requires to find area by id
			const Box& areaBox = naviArea->GetBBox();
			if ( !m_bbox.Touches( areaBox ) )
			{
				return false;
			}

			// do stuff
			return m_functor->Handle( naviArea );
		}
		const Box&						m_bbox;
		CInstanceFunctor*				m_functor;
		const CPathLibWorld&			m_pathlib;
		TStaticArray< AreaId, 128 >		m_visited;
	} functor( bb, it, m_pathlib );
	
	for ( Int16 y = minY; y <= maxY; ++y )
	{
		for ( Int16 x = minX; x <= maxX; ++x )
		{
			m_map[ GetCelIndexFromCoords( Int16( x ), Int16( y ) ) ]->IterateAreasAt( bb.Min.AsVector2(), bb.Max.AsVector2(), functor );
		}
	}
}

};			// namespace PathLib

