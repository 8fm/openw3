#include "build.h"
#include "pathlibTerrain.h"

#include "../core/directory.h"

#include "clipMap.h"
#include "globalWater.h"
#include "pathlibWorld.h"
#include "pathlibNavgraph.h"
#include "pathlibNavmeshArea.h"
#include "pathlibObstaclesMap.h"
#include "pathlibSpatialQuery.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibTerrainSurfaceProcessing.h"
#include "terrainTile.h"
#include "world.h"


#define F_SQRT2                  (1.41421356237309504880f)


////////////////////////////////////////////////////////////////////////////
// CTerrainMap spatial queries local stuff
////////////////////////////////////////////////////////////////////////////

namespace
{
	static const Uint32 MAX_MIPMAP_LEVELS = 14;
	static const Uint32 MIPMAP_LEVEL_START_INDEX[MAX_MIPMAP_LEVELS+1] =
	{
		0,
		1,								// 1
		5,
		21,
		85,
		341,							// 5
		1365,
		5461,
		21845,
		87381,
		349525,							// 10
		1398101,
		5592405,
		22369621,
		89478485						// 14
	};
};


namespace PathLib
{

const Float CTerrainAreaDescription::HEIGHT_UNSET = -100000.f;

////////////////////////////////////////////////////////////////////////////
// CTerrainHeightComputationContext
////////////////////////////////////////////////////////////////////////////
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT

CTerrainHeightComputationContext::CTerrainHeightComputationContext()
	: m_tileTexels( NULL )
	, m_isValid( false )
{
}
CTerrainHeightComputationContext::~CTerrainHeightComputationContext()
{
	Clear();
}

Bool CTerrainHeightComputationContext::InitializeSync( CWorld* world, Int32 tileX, Int32 tileY )
{
	// don't do anything unless we are invalidated or uninitialized
	if ( m_isValid )
	{
		return true;
	}
	// free all shit if we are invalidated
	Clear();

	// allocate and copy all shit
	CClipMap* clipMap = world->GetTerrain();
	CTerrainTile* tile = clipMap ? clipMap->GetTile( tileX, tileY ) : NULL;
	if ( tile )
	{
		const Uint16* hm = tile->GetLevelSyncHM( 0 );

		if ( hm )
		{
			m_tileSize = clipMap->GetTileSize();
			m_tileRes = clipMap->GetTilesMaxResolution();
			m_lowestElevation = clipMap->GetLowestElevation();
			m_heightRange = clipMap->GetHeighestElevation() - m_lowestElevation;
			Uint32 texelsCount = m_tileRes*m_tileRes;
			m_tileTexels = static_cast< Uint16* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_PathLib, sizeof( Uint16 ) * texelsCount ) );
			Red::System::MemoryCopy( m_tileTexels, hm, sizeof( Uint16 ) * texelsCount );
			m_isValid = true;
			return true;
		}
	}
	
	return false;
}
void CTerrainHeightComputationContext::Clear()
{
	if ( m_tileTexels )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_PathLib, m_tileTexels );
		m_tileTexels = NULL;
		m_isValid = false;
	}
}

Float CTerrainHeightComputationContext::ComputeHeight( const Vector2& localPos ) const
{
	Float height;
	CTerrainTile::GetHeightFromHeightmap( m_tileTexels, localPos, m_tileSize, m_tileRes, m_lowestElevation, m_heightRange, height );
	return height;
}

Float CTerrainHeightComputationContext::GetVertHeight( Uint32 x, Uint32 y ) const
{
	Uint32 h = m_tileTexels[ y * m_tileRes + x ];

	return m_lowestElevation + m_heightRange * ( Float(h) / 65536.f );
}

Float CTerrainHeightComputationContext::ComputeTexelNormalizedHeight( Int32 texelX1, Int32 texelY1 ) const
{
	Uint32 texelX2 = texelX1 == Int32(m_tileRes) - 1 ? m_tileRes - 1 : texelX1+1;
	Uint32 texelY2 = texelY1 == Int32(m_tileRes) - 1 ? m_tileRes - 1 : texelY1+1;

	// Calculate heights
	Uint32 heightLL = m_tileTexels[ texelY1 * m_tileRes + texelX1 ];
	Uint32 heightHL = m_tileTexels[ texelY1 * m_tileRes + texelX2 ];
	Uint32 heightLH = m_tileTexels[ texelY2 * m_tileRes + texelX1 ];
	Uint32 heightHH = m_tileTexels[ texelY2 * m_tileRes + texelX2 ];

	Uint32 heightAvarage = (heightLL + heightLH + heightHL + heightHH + 2) / 4;

	// renormalize height and get the real value
	return m_lowestElevation + m_heightRange * ( Float(heightAvarage) / 65536.f );
}

void CTerrainHeightComputationContext::GetTexelVertsHeight( Int32 texelX1, Int32 texelY1, Float* texelHeight ) const
{
	Uint32 texelX2 = texelX1 == Int32(m_tileRes) - 1 ? m_tileRes - 1 : texelX1+1;
	Uint32 texelY2 = texelY1 == Int32(m_tileRes) - 1 ? m_tileRes - 1 : texelY1+1;

	// Calculate heights
	Uint32 heightLL = m_tileTexels[ texelY1 * m_tileRes + texelX1 ];
	Uint32 heightHL = m_tileTexels[ texelY1 * m_tileRes + texelX2 ];
	Uint32 heightLH = m_tileTexels[ texelY2 * m_tileRes + texelX1 ];
	Uint32 heightHH = m_tileTexels[ texelY2 * m_tileRes + texelX2 ];

	texelHeight[ 0 ] = m_lowestElevation + m_heightRange * ( Float(heightLL) / 65536.f );
	texelHeight[ 1 ] = m_lowestElevation + m_heightRange * ( Float(heightHL) / 65536.f );
	texelHeight[ 2 ] = m_lowestElevation + m_heightRange * ( Float(heightLH) / 65536.f );
	texelHeight[ 3 ] = m_lowestElevation + m_heightRange * ( Float(heightHH) / 65536.f );
}

void CTerrainHeightComputationContext::GetTexelHeightLimits( Int32 texelX, Int32 texelY, Float& zMin, Float& zMax ) const
{
	Float z[ 4 ];
	GetTexelVertsHeight( texelX, texelY, z );
	zMin = Min( z[ 0 ], z[ 1 ], Min( z[ 2 ], z[ 3 ] ) );
	zMax = Max( z[ 0 ], z[ 1 ], Max( z[ 2 ], z[ 3 ] ) );
}

void CTerrainHeightComputationContext::GetTexelHeightLimits( Int32 minX, Int32 maxX, Int32 minY, Int32 maxY, Float& zMin, Float& zMax ) const
{
	for ( Int32 y = minY; y < maxY; ++y )
	{
		for ( Int32 x = minX; x < maxX; ++x )
		{
			Uint32 height = m_tileTexels[ y * m_tileRes + x ];
			Float z = m_lowestElevation + m_heightRange * ( Float( height ) / 65536.f );
			zMin = Min( z, zMin );
			zMax = Max( z, zMax );
		}
	}
}


#endif	// PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
////////////////////////////////////////////////////////////////////////////
// MipMapTester
////////////////////////////////////////////////////////////////////////////
template < class Predicate >
struct MipMapTester : public Red::System::NonCopyable
{
protected:
	const CTerrainMap*		m_this;
	Predicate&				m_predicate;
	union
	{
		struct
		{
			Int32				m_testMinX;
			Int32				m_testMinY;
			Int32				m_testMaxX;
			Int32				m_testMaxY;
		};
		Int32					m_testBBox[4];
	};
	Bool					m_navmeshesTestValid;
	Uint32					m_navmeshAreasCount;
	const AreaId*			m_navmeshAreas;
	Box2					m_navmeshCelBounds;

	RED_INLINE Bool OnSpatialQueryHitInstance( const Vector2& v )
	{
		auto& query = m_predicate.m_query;
		if ( m_predicate.m_query.m_flags & CT_MULTIAREA )
		{
			typedef typename Predicate::QueryType::MultiArea MultiArea;
			MultiArea* multiAreaQuery = MultiArea::GetMultiAreaData( query );

			if ( !multiAreaQuery->HasAreaCapacity() )
			{
				return true;
			}

			CAreaDescription& terrainArea = *m_this->m_area;
			CPathLibWorld& pathlib = terrainArea.GetPathLib();

			Vector2 w = v;
			terrainArea.LocalToWorld( w );

			if ( !m_navmeshesTestValid || !m_navmeshCelBounds.Contains( w ) )
			{
				if ( !pathlib.GetInstanceMap()->GetCelAt( w, m_navmeshAreasCount, m_navmeshAreas, m_navmeshCelBounds ) )
				{
					return true;
				}

				m_navmeshesTestValid = true;
			}

			
			for ( Uint32 i = 0, n = m_navmeshAreasCount; i < n; ++i )
			{
				AreaId areaId = m_navmeshAreas[ i ];
				if ( multiAreaQuery->VisitedArea( areaId ) )
				{
					continue;
				}

				CNavmeshAreaDescription* naviArea = pathlib.GetInstanceAreaDescription( areaId );
				if( !naviArea || !naviArea->IsReady() || !naviArea->GetBBox().ContainsExcludeEdges( w ) )
				{
					continue;
				}

				Float naviZ;
				if ( !naviArea->ComputeHeight( w, -1024.f, 1024.f, naviZ ) )
				{
					continue;
				}
				Float terrainZ = m_this->ComputeHeight( v );
				if ( Abs( terrainZ - naviZ ) <= GEOMETRY_AND_NAVMESH_MAX_DISTANCE ||
					naviArea->ComputeHeight( Vector3( w.X, w.Y, terrainZ ), naviZ ) )
				{
					multiAreaQuery->PushAreaSafe( areaId, Vector3( w.X, w.Y, terrainZ ) );
					multiAreaQuery->NoticeAreaEdgeHit();
					return true;
				}
			}

			return true;
		}
		return ( query.m_flags & CT_IGNORE_OTHER_AREAS ) != 0;
	}
	Bool TriTest( const Vector2& v1, const Vector2& v2, const Vector2& v3 )
	{
		if ( m_predicate.Intersect( v1, v2 ) )
		{
			if ( Predicate::AUTOFAIL )
			{
				return false;
			}
			else
			{
				m_predicate.OnFail( v1, v2 );
			}
		}

		if ( m_predicate.Intersect( v2, v3 ) )
		{
			if ( Predicate::AUTOFAIL )
			{
				return false;
			}
			else
			{
				m_predicate.OnFail( v2, v3 );
			}
		}
		if ( m_predicate.Intersect( v3, v1 ) )
		{
			if ( Predicate::AUTOFAIL )
			{
				return false;
			}
			else
			{
				m_predicate.OnFail( v3, v1 );
			}
		}
		return true;
	}

	RED_INLINE Bool QuadTest( Uint32 x, Uint32 y )
	{
		CTerrainMap::eQuadState state = m_this->GetQuadState( m_this->GetQuadIndex( x, y ) );

		if ( state == CTerrainMap::QUAD_FREE )
		{
			if ( Predicate::REPORT_SUCCESS )
			{
				Float quadSize = m_this->GetTerrainInfo()->GetQuadSize();

				Vector2 quadMin( Float(x) * quadSize, Float(y) * quadSize );
				Vector2 quadMax( Float(x+1) * quadSize, Float(y+1) * quadSize );

				return m_predicate.OnSuccess( (quadMin + quadMax) * 0.5f );
			}
			return true;
		}

		Float quadSize = m_this->GetTerrainInfo()->GetQuadSize();

		Vector2 quadMin( Float(x) * quadSize, Float(y) * quadSize );
		Vector2 quadMax( Float(x+1) * quadSize, Float(y+1) * quadSize );

		switch ( state )
		{
		case CTerrainMap::QUAD_FREE:
		case CTerrainMap::QUAD_OBSTACLE:
			if ( Predicate::REPORT_SUCCESS )
			{
				return m_predicate.OnSuccess( (quadMin + quadMax) * 0.5f );
			}
			break;
		case CTerrainMap::QUAD_BLOCK_ALL:
			if ( m_predicate.IntersectRect( quadMin, quadMax ) )
			{
				if ( Predicate::AUTOFAIL )
				{
					return false;
				}
				else
				{
					m_predicate.OnFailRect( quadMin, quadMax );
				}
			}
			break;
		case CTerrainMap::QUAD_INSTANCE:
			if ( m_predicate.IntersectRect( quadMin, quadMax ) )
			{
				if ( !OnSpatialQueryHitInstance( (quadMin + quadMax) * 0.5f ) )
				{
					if ( Predicate::AUTOFAIL )
					{
						return false;
					}
					else
					{
						m_predicate.OnFailRect( quadMin, quadMax );
					}
				}
			}
			if ( Predicate::REPORT_SUCCESS )
			{
				return m_predicate.OnSuccess( (quadMin + quadMax) * 0.5f );
			}
			break;
		default:
			ASSUME( false );
		}
		return true;
	
	}
	RED_INLINE Bool CelTest( Uint32 level, Uint32 x, Uint32 y )
	{
		Uint32 index = CTerrainMap::MipMapIndex( level, x, y );
		CTerrainMap::eMipMapQuadState state = m_this->m_mipMap.Get( index );
		if ( state == CTerrainMap::MIPMAP_FREE )
		{
			if ( Predicate::REPORT_SUCCESS )
			{
				Uint32 levelShift = m_this->MipMapShift( level );
				Float quadSize = m_this->GetTerrainInfo()->GetQuadSize();

				Int32 celQuadMinX = x << levelShift;
				Int32 celQuadMinY = y << levelShift;
				Int32 celQuadMaxX = (x+1) << levelShift;
				Int32 celQuadMaxY = (y+1) << levelShift;

				Vector2 celMin( Float(celQuadMinX) * quadSize, Float(celQuadMinY) * quadSize );
				Vector2 celMax( Float(celQuadMaxX) * quadSize, Float(celQuadMaxY) * quadSize );

				if ( m_predicate.IntersectRect( celMin, celMax ) )
				{
					return m_predicate.OnSuccess( (celMin + celMax) * 0.5f );
				}
			}
			return true;
		}
		// detailed rectangle intersection test (we can assume that test bbox overlap - that was tested on previous recursion level)
		Uint32 levelShift = m_this->MipMapShift( level );
		Float quadSize = m_this->GetTerrainInfo()->GetQuadSize();

		Int32 celQuadMinX = x << levelShift;
		Int32 celQuadMinY = y << levelShift;
		Int32 celQuadMaxX = (x+1) << levelShift;
		Int32 celQuadMaxY = (y+1) << levelShift;

		Vector2 celMin( Float(celQuadMinX) * quadSize, Float(celQuadMinY) * quadSize );
		Vector2 celMax( Float(celQuadMaxX) * quadSize, Float(celQuadMaxY) * quadSize );
		if ( !m_predicate.IntersectRect( celMin, celMax ) )
		{
			if ( Predicate::REPORT_SUCCESS )
			{
				return m_predicate.OnSuccess( (celMin + celMax) * 0.5f );
			}
			return true;
		}

		if ( state == CTerrainMap::MIPMAP_BLOCKED )
		{
			if ( Predicate::AUTOFAIL )
			{
				return false;
			}
			else
			{
				m_predicate.OnFailRect( celMin, celMax );
				return true;
			}
		}
		// state == CTerrainMap::MIPMAP_MIXED
		// run lower level test
		return Cels4Test( level+1, x << 1, y << 1 );
	}
	RED_INLINE Bool Cels4Test( Uint32 level, Int32 x, Int32 y )
	{
		if ( level == m_this->m_mipMapLevels )
		{
			// lowest level, detailed map test
			Bool testXLow	= x >= m_testMinX;
			Bool testXHigh	= x < m_testMaxX;
			Bool testYLow	= y >= m_testMinY;
			Bool testYHigh	= y < m_testMaxY;

			if ( testXLow && testYLow )
			{
				if ( !QuadTest( x, y ) )
				{
					return false;
				}
			}
			if ( testXHigh && testYLow )
			{
				if ( !QuadTest( x+1, y ) )
				{
					return false;
				}
			}
			if ( testXLow && testYHigh )
			{
				if ( !QuadTest( x, y+1 ) )
				{
					return false;
				}
			}
			if ( testXHigh && testYHigh )
			{
				if ( !QuadTest( x+1, y+1 ) )
				{
					return false;
				}
			}
		}
		else
		{
			Uint32 levelShift = m_this->MipMapShift( level );

			Int32 xMin = m_testMinX >> levelShift;
			Int32 xMax = m_testMaxX >> levelShift;
			Int32 yMin = m_testMinY >> levelShift;
			Int32 yMax = m_testMaxY >> levelShift;

			Bool testXLow = x >= xMin;
			Bool testXHigh = x < xMax;
			Bool testYLow = y >= yMin;
			Bool testYHigh = y < yMax;

			// if cases filter algorithm with simple bbox test
			if ( testXLow && testYLow )
			{
				if ( !CelTest( level, x, y ) )
				{
					return false;
				}
			}
			if ( testXHigh && testYLow )
			{
				if ( !CelTest( level, x+1, y ) )
				{
					return false;
				}
			}
			if ( testXLow && testYHigh )
			{
				if ( !CelTest( level, x, y+1 ) )
				{
					return false;
				}
			}
			if ( testXHigh && testYHigh )
			{
				if ( !CelTest( level, x+1, y+1 ) )
				{
					return false;
				}
			}
		}

		return true;
	}

	RED_INLINE Bool HitNeighbour( Int32 x, Int32 y )
	{
		auto& query = m_predicate.m_query;
		if ( query.m_flags & CT_MULTIAREA )
		{
			const auto& area = m_this->GetArea();
			AreaId neighbourId = area.GetNeighbourId( x, y );
			if ( neighbourId == INVALID_AREA_ID )
			{
				return false;
			}

			// Entry point is currently unused for terrain based tests
			Vector3 entryPoint = query.m_basePos;
			area.LocalToWorld( entryPoint );

			typedef typename Predicate::QueryType QueryType;
			typedef typename QueryType::MultiArea MultiArea;

			MultiArea* multiAreaQuery = MultiArea::GetMultiAreaData( query );
			multiAreaQuery->PushAreaUnique( neighbourId, entryPoint );
			multiAreaQuery->NoticeAreaEdgeHit();

			return true;
		}
		return ( query.m_flags & CT_IGNORE_OTHER_AREAS ) != 0;
	}
public:
	MipMapTester( const CTerrainMap* me, Predicate& p )
		: m_this( me )
		, m_predicate( p )
		, m_navmeshesTestValid( false )
	{
	}

	RED_INLINE Bool DoTest()
	{
		// THIS SHOULDN'T BE THE CASE AND IT SHOULD BE HANDLED UP, BUT THERE IS SUCH BUG SOMEWHERE, AND I CAN'T JUST UNCOMMENT THIS TO MAKE IT CRASH.
		ASSERT( m_this, TXT("Plz plz plz. Send assert callstack to Michal Slapa.") );
		if( !m_this )
		{
			return false;
		}
		
		if ( m_predicate.m_query.m_flags & CT_FORCE_BASEPOS_ZTEST )
		{
			const Vector3& basePos = m_predicate.m_query.m_basePos;
			Float baseZ = m_this->ComputeHeight( basePos.AsVector2() );
			if ( baseZ < basePos.Z - 2.f || baseZ > basePos.Z + 2.f )
			{
				return false;
			}
		}

		Vector2 testBBox[ 2 ];
		m_predicate.m_query.ComputeBBox( testBBox );


		const CTerrainInfo* terrainInfo = m_this->GetTerrainInfo();
		Float tileSize = terrainInfo->GetTileSize();

		// Handle neighbour terrain areas
		if ( testBBox[ 0 ].X < 0.f )
		{
			if ( m_predicate.Intersect( Vector2( 0.f, 0.f ), Vector2( 0.f, tileSize ) ) )
			{
				if ( !HitNeighbour( -1, 0 ) )
				{
					return false;
				}
			}
		}
		if ( testBBox[ 1 ].X > tileSize )
		{
			if ( m_predicate.Intersect( Vector2( tileSize, 0.f ), Vector2( tileSize, tileSize ) ) )
			{
				if ( !HitNeighbour( 1, 0 ) )
				{
					return false;
				}
			}
		}
		if ( testBBox[ 0 ].Y < 0.f )
		{
			if ( m_predicate.Intersect( Vector2( 0.f, 0.f ), Vector2( tileSize, 0.f ) ) )
			{
				if ( !HitNeighbour( 0, -1 ) )
				{
					return false;
				}
			}
		}
		if ( testBBox[ 1 ].Y > tileSize )
		{
			if ( m_predicate.Intersect( Vector2( 0.f, tileSize ), Vector2( tileSize, tileSize ) ) )
			{
				if ( !HitNeighbour( 0, 1 ) )
				{
					return false;
				}
			}
		}

		m_this->GetQuadCoordsAt( testBBox[ 0 ], m_testMinX, m_testMinY );
		m_this->GetQuadCoordsAt( testBBox[ 1 ], m_testMaxX, m_testMaxY );

		Uint32 testSize = Max( m_testMaxX - m_testMinX, m_testMaxY - m_testMinY );

		Int32 testLevel = m_this->m_mipMapLevels - 1;
		testSize = testSize >> 1;
		while ( testSize > 2 && testLevel > 0 )
		{
			testSize = testSize >> 1;
			--testLevel;
		}
		ASSERT( testLevel >= 0 );

		Uint32 levelShift = m_this->MipMapShift( testLevel );

		Uint32 xMin = m_testMinX >> levelShift;
		Uint32 xMax = m_testMaxX >> levelShift;
		Uint32 yMin = m_testMinY >> levelShift;
		Uint32 yMax = m_testMaxY >> levelShift;

		Uint32 baseIndex = ::MIPMAP_LEVEL_START_INDEX[ testLevel ];

		for ( Uint32 y = yMin; y <= yMax; ++y )
		{
			Uint32 baseY = baseIndex + (y << testLevel);
			for ( Uint32 x = xMin; x <= xMax; ++x )
			{
				Uint32 index = baseY + x;
				CTerrainMap::eMipMapQuadState state = m_this->m_mipMap.Get( index );
				switch ( state )
				{
				case CTerrainMap::MIPMAP_FREE:
					break;

				case CTerrainMap::MIPMAP_HOMOGENOUS:
					{
						CTerrainMap::eQuadState quadState = m_this->MipMapGetHomogenousType( testLevel, x, y );
						if ( quadState == CTerrainMap::QUAD_INSTANCE )
						{
							Vector2 rectMin;
							Vector2 rectMax;
							m_this->GetMipMapLocalPosition( testLevel, x, y, rectMin, rectMax );
							if ( OnSpatialQueryHitInstance( (rectMin + rectMax) * 0.5f ) )
							{
								break;
							}
						}
						else if ( quadState == CTerrainMap::QUAD_OBSTACLE )
						{
							// TODO:
							break;
						}
					}
					// no break (for instance)
				case CTerrainMap::MIPMAP_BLOCKED:
					{
						Float quadSize = terrainInfo->GetQuadSize();

						Int32 celQuadMinX = x << levelShift;
						Int32 celQuadMinY = y << levelShift;
						Int32 celQuadMaxX = (x+1) << levelShift;
						Int32 celQuadMaxY = (y+1) << levelShift;

						Vector2 celMin( Float(celQuadMinX) * quadSize, Float(celQuadMinY) * quadSize );
						Vector2 celMax( Float(celQuadMaxX) * quadSize, Float(celQuadMaxY) * quadSize );

						if ( m_predicate.IntersectRect( celMin, celMax ) )
						{
							if ( Predicate::AUTOFAIL )
							{
								return false;
							}
							else
							{
								m_predicate.OnFailRect( celMin, celMax );
							}
						}
					}
					
					break;

				default:
				case CTerrainMap::MIPMAP_MIXED:
					// run lower level test
					if ( !Cels4Test( testLevel+1, x << 1, y << 1 ) )
					{
						return false;
					}
					break;
				}
			}
		}

		return true;
	}
};

template < class Predicate >
RED_INLINE Bool CTerrainMap::MipMapTest( Predicate& c ) const
{
	MipMapTester< Predicate > functor( this, c );
	return functor.DoTest();
}

Uint32 CTerrainMap::CalculateMipMapLevels( QuadIndex tileResoultion ) const
{
	// calculate mipmap levels count
	Uint32 resolution = tileResoultion >> 1;
	Uint32 levels = 0;
	while ( resolution > 0 )
	{
		++levels;
		resolution = resolution >> 1;
	}
	ASSERT ( levels <= MAX_MIPMAP_LEVELS );
	return levels;
}
void CTerrainMap::InitializeMipMap()
{
	// initialize mipmap data
	m_mipMapLevels = CalculateMipMapLevels( m_terrainInfo->GetTilesResolution() );
	m_mipMap.Resize( ::MIPMAP_LEVEL_START_INDEX[ m_mipMapLevels ] );
	m_mipMap.SetZero();
}
void CTerrainMap::CalculateMipMap()
{
	// initialize mipmap data
	m_mipMapLevels = CalculateMipMapLevels( m_terrainInfo->GetTilesResolution() );
	m_mipMap.Resize( ::MIPMAP_LEVEL_START_INDEX[ m_mipMapLevels ] );

	// initial level
	Uint32 startLevel = m_mipMapLevels-1;
	Uint32 baseResolution = 1 << startLevel;
	Uint32 baseIndex = ::MIPMAP_LEVEL_START_INDEX[ startLevel ];

	Uint32 currIndex = baseIndex;
	for ( Uint32 y = 0; y < baseResolution; ++y )
	{
		for ( Uint32 x = 0; x < baseResolution; ++x )
		{
			eMipMapQuadState state;

			eQuadState state1 = GetQuadState( GetQuadIndex( x*2+0, y*2+0 ) );
			eQuadState state2 = GetQuadState( GetQuadIndex( x*2+1, y*2+0 ) );
			eQuadState state3 = GetQuadState( GetQuadIndex( x*2+0, y*2+1 ) );
			eQuadState state4 = GetQuadState( GetQuadIndex( x*2+1, y*2+1 ) );

			if ( state1 == QUAD_FREE && state2 == QUAD_FREE && state3 == QUAD_FREE && state4 == QUAD_FREE )
			{
				state = MIPMAP_FREE;
			}
			else if ( state1 == QUAD_BLOCK_ALL && state2 == QUAD_BLOCK_ALL && state3 == QUAD_BLOCK_ALL && state4 == QUAD_BLOCK_ALL )
			{
				state = MIPMAP_BLOCKED;
			}
			else if ( state1 == QUAD_INSTANCE && state2 == QUAD_INSTANCE && state3 == QUAD_INSTANCE && state4 == QUAD_INSTANCE )
			{
				state = MIPMAP_HOMOGENOUS;
			}
			else if ( state1 == QUAD_OBSTACLE && state2 == QUAD_OBSTACLE && state3 == QUAD_OBSTACLE && state4 == QUAD_OBSTACLE )
			{
				state = MIPMAP_HOMOGENOUS;
			}
			else
			{
				state = MIPMAP_MIXED;
			}
			m_mipMap.Set( currIndex++, state );
		}
	}

	Int32 currLevel = startLevel-1;
	while ( currLevel >= 0 )
	{
		Uint32 levelResolution = 1 << currLevel;
		Uint32 prevLevel = currLevel+1;
		Uint32 baseIndex = ::MIPMAP_LEVEL_START_INDEX[ currLevel ];
		Uint32 prevBaseIndex = ::MIPMAP_LEVEL_START_INDEX[ prevLevel ];

		Uint32 currIndex = baseIndex;
		for ( Uint32 y = 0; y < levelResolution; ++y )
		{
			for ( Uint32 x = 0; x < levelResolution; ++x )			
			{
				eMipMapQuadState state;

				Uint32 x2 = x << 1;
				Uint32 y2 = y << 1;

				eMipMapQuadState state1 = m_mipMap.Get( prevBaseIndex + (y2 << prevLevel) + x2 );
				eMipMapQuadState state2 = m_mipMap.Get( prevBaseIndex + (y2 << prevLevel) + x2 + 1 );
				eMipMapQuadState state3 = m_mipMap.Get( prevBaseIndex + ((y2+1) << prevLevel) + x2 );
				eMipMapQuadState state4 = m_mipMap.Get( prevBaseIndex + ((y2+1) << prevLevel) + x2 + 1 );

				if ( state1 == MIPMAP_FREE && state2 == MIPMAP_FREE && state3 == MIPMAP_FREE && state4 == MIPMAP_FREE )
				{
					state = MIPMAP_FREE;
				}
				else if ( state1 == MIPMAP_BLOCKED && state2 == MIPMAP_BLOCKED && state3 == MIPMAP_BLOCKED && state4 == MIPMAP_BLOCKED )
				{
					state = MIPMAP_BLOCKED;
				}
				else if ( state1 == MIPMAP_HOMOGENOUS && state2 == MIPMAP_HOMOGENOUS && state3 == MIPMAP_HOMOGENOUS && state4 == MIPMAP_HOMOGENOUS )
				{
					eQuadState s1 = MipMapGetHomogenousType( prevLevel, x2, y2 );
					eQuadState s2 = MipMapGetHomogenousType( prevLevel, x2+1, y2 );
					eQuadState s3 = MipMapGetHomogenousType( prevLevel, x2, y2+1 );
					eQuadState s4 = MipMapGetHomogenousType( prevLevel, x2+1, y2+1 );
					if ( s1 == s2 && s1 == s3 && s1 == s4 )
					{
						state = MIPMAP_HOMOGENOUS;
					}
					else
					{
						state = MIPMAP_MIXED;
					}
				}
				else
				{
					state = MIPMAP_MIXED;
				}
				m_mipMap.Set( currIndex++, state );
			}
		}
		--currLevel;
	}

}
void CTerrainMap::UpdateMipMap( eQuadState quadState, Int32 quadX, Int32 quadY )
{
	Int32 currLevel = m_mipMapLevels-1;
	Uint32 x = quadX >> 1;
	Uint32 y = quadY >> 1;
	// first step must be done by hand (because there is different interface to access detailed map)
	Uint32 currMipMapIndex = MipMapIndex( currLevel, x, y );
	eMipMapQuadState currState = m_mipMap.Get( currMipMapIndex );
	eMipMapQuadState baseState =
		quadState == QUAD_FREE ? MIPMAP_FREE :
		quadState == QUAD_BLOCK_ALL ? MIPMAP_BLOCKED :
		quadState == QUAD_INSTANCE ? MIPMAP_HOMOGENOUS :
		quadState == QUAD_OBSTACLE ? MIPMAP_HOMOGENOUS :
		MIPMAP_MIXED;
	
	if ( baseState != MIPMAP_MIXED )
	{
		eQuadState state1 = GetQuadState( GetQuadIndex( x*2+0, y*2+0 ) );
		eQuadState state2 = GetQuadState( GetQuadIndex( x*2+1, y*2+0 ) );
		eQuadState state3 = GetQuadState( GetQuadIndex( x*2+0, y*2+1 ) );
		eQuadState state4 = GetQuadState( GetQuadIndex( x*2+1, y*2+1 ) );

		if ( baseState == MIPMAP_FREE )
		{
			if ( state1 != QUAD_FREE || state2 != QUAD_FREE || state3 != QUAD_FREE || state4 != QUAD_FREE )
			{
				baseState = MIPMAP_MIXED;
			}
		}
		else if ( baseState == MIPMAP_BLOCKED )
		{
			if ( state1 != QUAD_BLOCK_ALL || state2 != QUAD_BLOCK_ALL || state3 != QUAD_BLOCK_ALL || state4 != QUAD_BLOCK_ALL )
			{
				baseState = MIPMAP_MIXED;
			}
		}
		else  // baseState == MIPMAP_HOMOGENOUS
		{
			if ( state1 != state2 || state1 != state3 || state1 != state4 )
			{
				baseState = MIPMAP_MIXED;
			}
		}
	}
	if ( currState == baseState )
	{
		return;
	}
	m_mipMap.Set( currMipMapIndex, baseState );

	while ( --currLevel >= 0 )
	{
		x = x >> 1;
		y = y >> 1;
		Uint32 prevLevel = currLevel+1;
		Uint32 prevBaseIndex = ::MIPMAP_LEVEL_START_INDEX[ prevLevel ];
		Uint32 currMipMapIndex = MipMapIndex( currLevel, x, y );
		eMipMapQuadState currState = m_mipMap.Get( currMipMapIndex );
		if ( baseState != MIPMAP_MIXED )
		{
			Uint32 x2 = x << 1;
			Uint32 y2 = y << 1;

			eMipMapQuadState state1 = m_mipMap.Get( prevBaseIndex + ((y2+0) << prevLevel) + (x2+0) );
			eMipMapQuadState state2 = m_mipMap.Get( prevBaseIndex + ((y2+0) << prevLevel) + (x2+1) );
			eMipMapQuadState state3 = m_mipMap.Get( prevBaseIndex + ((y2+1) << prevLevel) + (x2+0) );
			eMipMapQuadState state4 = m_mipMap.Get( prevBaseIndex + ((y2+1) << prevLevel) + (x2+1) );

			if ( state1 != baseState || state2 != baseState || state3 != baseState || state4 != baseState )
			{
				baseState = MIPMAP_MIXED;
			}
			else if ( baseState == MIPMAP_HOMOGENOUS )
			{
				// mipmap cel is possibly homogenous - but we need to check if each cel has same homogenous type
				eQuadState s1 = MipMapGetHomogenousType( prevLevel, x2+0, y2+0 );
				eQuadState s2 = MipMapGetHomogenousType( prevLevel, x2+1, y2+0 );
				eQuadState s3 = MipMapGetHomogenousType( prevLevel, x2+0, y2+1 );
				eQuadState s4 = MipMapGetHomogenousType( prevLevel, x2+1, y2+1 );
				if ( s1 != s2 || s1 != s3 || s1 != s4 )
				{
					baseState = MIPMAP_MIXED;
				}
			}
		}
		if ( currState == baseState )
		{
			return;
		}
		m_mipMap.Set( currMipMapIndex, baseState );
	}
}
Uint32 CTerrainMap::MipMapIndex( Uint32 level, Uint32 x, Uint32 y )
{
	return ::MIPMAP_LEVEL_START_INDEX[ level ] + (y << level) + x;
}
CTerrainMap::eQuadState CTerrainMap::MipMapGetHomogenousType( Uint32 level, Uint32 x, Uint32 y ) const
{
	Uint32 shift = MipMapShift( level ); 
	return GetQuadState( GetQuadIndex( x << shift, y << shift ) );
}

////////////////////////////////////////////////////////////////////////////
// CTerrainMap
////////////////////////////////////////////////////////////////////////////
CTerrainMap::QuadIndex CTerrainMap::GetQuadIndex( Int32 x, Int32 y ) const
{
	return QuadIndex(y * m_terrainInfo->GetTilesResolution() + x);
}

//CTerrainMap::QuadIndex CTerrainMap::GetQuadIndex( const Vector2& v ) const
//{
//	 Int32 x = Int32( v.X / m_terrainManager->GetTileSize() );
//	 if ( x < 0 )
//	 {
//		 x = 0;
//	 }
//	 else if ( x >= Int32(m_terrainManager->GetTilesResolution()) )
//	 {
//		 x = m_terrainManager->GetTilesResolution()-1;
//	 }
//	 Int32 y = Int32( v.Y / m_terrainManager->GetTileSize() );
//	 if ( y < 0 )
//	 {
//		 y = 0;
//	 }
//	 else if ( y >= Int32(m_terrainManager->GetTilesResolution()) )
//	 {
//		 y = m_terrainManager->GetTilesResolution()-1;
//	 }
//	 return GetQuadIndex( x, y );
//}
void CTerrainMap::GetQuadCoords( QuadIndex q, Int32& x, Int32& y ) const
{
	Uint32 resolution = m_terrainInfo->GetTilesResolution();
	y = q / resolution;
	x = q % resolution;
}

CTerrainMap::eQuadState CTerrainMap::GetQuadState( QuadIndex quad ) const
{
	return m_detailedMap.Get( quad );
}
Bool CTerrainMap::ForceQuadState( QuadIndex index, eForcedState state, Bool autoUpdate )
{
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	eForcedState currentState = eForcedState( m_forcedData.IsBitSet( index ) );
	if ( currentState != state )
	{
		m_forcedData.SetBit( index, state != 0 );
		if ( !autoUpdate )
		{
			return true;
		}
		return UpdateQuad( index );
	}
	return false;
#else
	return false;
#endif
}
Bool CTerrainMap::SetTerrainQuadState( QuadIndex index, eQuadState state, Bool updateMipMap )
{
#ifndef PATHLIB_COMPECT_TERRAIN_DATA		
	eQuadState currentState = m_terrainData.Get( index );
	Int32 currentCollisionState = currentState & QUAD_MASK_COLLISION;
	Int32 currentInstanceFlag = currentState & QUAD_MASK_FLAGS;
	if ( currentCollisionState != state )
	{
		m_terrainData.Set( index, eQuadState( state | currentInstanceFlag ) );
		if ( !autoUpdate )
		{
			return true;
		}
		return UpdateQuad( index, updateMipMap );
	}
	return false;
#else
	Int32 currentState = m_detailedMap.Get( index );
	if ( currentState == state )
	{
		return false;
	}
	m_detailedMap.Set( index, state );
	return true;
#endif
}
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
CTerrainMap::eQuadState CTerrainMap::GetTerrainQuadState( QuadIndex index )
{
	return CTerrainMap::eQuadState( m_terrainData.Get( index ) & QUAD_MASK_COLLISION );
}
CTerrainMap::eQuadState CTerrainMap::ComputeQuadState( QuadIndex index ) const
{
	eForcedState forcedState = eForcedState( m_forcedData.IsBitSet( index ) );
	eQuadState terrainState = m_terrainData.Get( index );
	return
		terrainState & QUAD_INSTANCE ? QUAD_INSTANCE :
		(forcedState == FORCED_BLOCK) ? QUAD_BLOCK_ALL
		: terrainState ;
}
Bool CTerrainMap::UpdateQuad( QuadIndex index, Bool updateMipMap )
{
	eQuadState currentState = GetQuadState( index );
	eQuadState computedState = ComputeQuadState( index );

	if ( computedState != currentState )
	{
		m_detailedMap.Set( index, computedState );
		Int32 x, y;
		GetQuadCoords( index, x, y );

		if ( updateMipMap )
		{
			UpdateMipMap( computedState, x, y );
		}
		
		++m_version;
		return true;
	}
	return false;
}
#endif
void CTerrainMap::GetQuadCoordsAt( const Vector2& localPos, Int32& x, Int32& y ) const
{
	Uint32 resolution = m_terrainInfo->GetTilesResolution();
	Float quadSize = m_terrainInfo->GetQuadSize();
	Vector2 pos = localPos / quadSize;
	x = Clamp( Int32(pos.X), 0, Int32(resolution-1) );
	y = Clamp( Int32(pos.Y), 0, Int32(resolution-1) );
}
Int32 CTerrainMap::GetQuadCoordFromScalar( Float scalar ) const
{
	Uint32 resolution = m_terrainInfo->GetTilesResolution();
	Float quadSize = m_terrainInfo->GetQuadSize();
	Float pos = scalar / quadSize;
	return Clamp( Int32( pos ), 0, Int32( resolution-1 ) );
}
void CTerrainMap::GetMipMapLocalPosition( Int32 mipmapLevel, Int32 x, Int32 y, Vector2& outLocalPosMin, Vector2& outLocalPosMax ) const
{
	Float quadSize = m_terrainInfo->GetQuadSize();

	Uint32 levelShift = MipMapShift( mipmapLevel );

	Int32 celQuadMinX = x << levelShift;
	Int32 celQuadMinY = y << levelShift;
	Int32 celQuadMaxX = (x+1) << levelShift;
	Int32 celQuadMaxY = (y+1) << levelShift;

	outLocalPosMin.Set(
		Float(celQuadMinX) * quadSize,
		Float(celQuadMinY) * quadSize
		);

	outLocalPosMax.Set(
		Float(celQuadMaxX) * quadSize,
		Float(celQuadMaxY) * quadSize
		);
}
void CTerrainMap::GetQuadLocalPosition( Int32 x, Int32 y, Vector2& outLocalPosMin, Vector2& outLocalPosMax ) const
{
	Float quadSize =  m_terrainInfo->GetQuadSize();

	outLocalPosMin.Set(
		Float(x) * quadSize,
		Float(y) * quadSize
		);

	outLocalPosMax.Set(
		Float(x+1) * quadSize,
		Float(y+1) * quadSize
		);
}
Vector2 CTerrainMap::GetQuadCenterLocal( Int32 x, Int32 y ) const
{
	Float quadSize = m_terrainInfo->GetQuadSize();

	return Vector2(
		(Float(x)+0.5f) * quadSize,
		(Float(y)+0.5f) * quadSize
		);
}

Bool CTerrainMap::GenerateHeightData()
{
	return m_height.Construct( m_area );
}

CTerrainMap::CTerrainMap()
	: CVersionTracking()
{
}

CTerrainMap::~CTerrainMap()
{

}

Bool CTerrainMap::IsInitialized() const
{
	return m_detailedMap.Size() == m_terrainInfo->GetQuadsCount();
}

void CTerrainMap::Initialize()
{
	QuadIndex quads = m_terrainInfo->GetQuadsCount();
	m_detailedMap.Resize( quads );
	m_detailedMap.SetZero();
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	m_forcedData.Resize( quads );
	Red::System::MemorySet( m_forcedData.m_mem.Data(), 0, m_forcedData.m_mem.DataSize() );
	m_terrainData.Resize( quads );
	m_terrainData.SetZero();
#endif
	InitializeMipMap();
	++m_version;
}
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
Bool CTerrainMap::UpdateMap()
{
	Bool dirty = false;
	Uint32 maxIndex = m_terrainInfo->GetQuadsCount();
	for ( QuadIndex idx = 0; idx < maxIndex; ++idx )
	{
		dirty = UpdateQuad( idx ) || dirty;
	}
	if ( dirty )
	{
		++m_version;
		return true;
	}
	return false;
}
#endif

Bool CTerrainMap::Save( const String& depotPath ) const
{
	return CAreaRes::Save( this, depotPath );
}

Bool CTerrainMap::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Uint16 tileResolution = Uint16( m_terrainInfo->GetTilesResolution() );
	QuadIndex quads = tileResolution*tileResolution;

	if ( m_detailedMap.Size() != quads )
	{
		return false;
	}

	writer.Put( tileResolution );
	writer.Write( m_detailedMap.DataPtr(), m_detailedMap.DataSize() );
	//writer.Write( m_mipMap.DataPtr(), m_mipMap.DataSize() );
	m_height.WriteToBuffer( writer );
	
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	Uint32 forcedDataSize = quads / 8 + ((quads % 8) > 0 ? 1 : 0);
	Uint32 editorData = m_terrainData.DataSize() + forcedDataSize;
	ASSERT( forcedDataSize <=  m_forcedData.m_mem.DataSize() );
	writer.Put( editorData );
	writer.Write( m_forcedData.m_mem.Data(), forcedDataSize );
	writer.Write( m_terrainData.DataPtr(), m_terrainData.DataSize() );
#else
	//Uint32 editorData  = 0;
	//writer.Write( editorData );
#endif
	return true;
}
Bool CTerrainMap::IsValid() const
{
	if ( m_detailedMap.Size() != m_terrainInfo->GetQuadsCount() )
	{
		return false;
	}

	return true;
}

Bool CTerrainMap::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( reader.GetVersion() != RES_VERSION )
	{
		return false;
	}

	Uint16 tileResolution;
	
	if ( !reader.Get( tileResolution ) )
	{
		return false;
	}

	QuadIndex quads = QuadIndex( tileResolution ) * QuadIndex( tileResolution );

	m_detailedMap.Resize( quads );
	if ( !reader.Read( m_detailedMap.DataPtr(), m_detailedMap.DataSize() ) )
	{
		return false;
	}
	m_mipMapLevels = CalculateMipMapLevels( tileResolution );
	m_mipMap.Resize( ::MIPMAP_LEVEL_START_INDEX[ m_mipMapLevels ] );
	//if ( !reader.Read( m_mipMap.DataPtr(), m_mipMap.DataSize() ) )
	//{
	//	return false;
	//}
	if ( !m_height.ReadFromBuffer( reader ) )
	{
		return false;
	}
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	Uint32 editorData;
	if ( !reader.Get( editorData ) )
		return false;
	m_forcedData.Resize( quads );
	Uint32 forcedDataSize = quads / 8 + ((quads % 8) > 0 ? 1 : 0);
	m_terrainData.Resize( quads );
	ASSERT( forcedDataSize <=  m_forcedData.m_mem.DataSize() );
	if ( editorData == m_terrainData.DataSize()+ forcedDataSize )
	{
		reader.Read( m_forcedData.m_mem.Data(), forcedDataSize );
		reader.Read( m_terrainData.DataPtr(), m_terrainData.DataSize() );
	}
	else if ( editorData != 0 )
	{
		return false;
	}
#else
	//reader.Skip( editorData );
#endif

	return true;
}

void CTerrainMap::SetTerrainInfo( const CTerrainInfo* manager )
{
	m_terrainInfo = manager;
	
}

Bool CTerrainMap::TestLocation(const Vector2& pos) const
{
	Int32 startingX, startingY;
	GetQuadCoordsAt( pos, startingX, startingY );
	eQuadState state = GetQuadState( GetQuadIndex( startingX, startingY ) );
	switch ( state )
	{
	case QUAD_FREE:
	case QUAD_OBSTACLE:
		return true;
	case QUAD_INSTANCE:
		break;
	case QUAD_BLOCK_ALL:
		break;
	default:
		ASSUME( false );
	}
	return false;
}

Bool CTerrainMap::SpatialQuery( CCircleQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CCircleQueryData >
	{
		Predicate( CCircleQueryData& query ) : DefaultPredicate( query ) {}
		RED_INLINE Bool Intersect(const Vector2& p0, const Vector2& p1) const
		{
			return MathUtils::GeometryUtils::TestIntersectionCircleLine2D( m_query.m_circleCenter.AsVector2(), m_query.m_radius, p0, p1 );
		}
	} predicate( query );
	return MipMapTest( predicate );
}
Bool CTerrainMap::SpatialQuery( CClosestObstacleCircleQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CClosestObstacleCircleQueryData >
	{
		enum { AUTOFAIL = false };

		Predicate( CClosestObstacleCircleQueryData& query ) : DefaultPredicate( query ) {}

		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax ) const
		{
			Vector2 v;
			MathUtils::GeometryUtils::ClosestPointToRectangle2D( rectMin, rectMax, m_query.m_circleCenter.AsVector2(), v );
			Float distSq = ( m_query.m_circleCenter.AsVector2() - v ).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_query.m_closestDistSq = distSq;
				m_query.m_pointOut = v;
				// TODO: calculate Z
				m_query.m_pointOut.Z = m_query.m_basePos.Z;
				m_query.m_obstacleHit = true;
			}
		}
		RED_INLINE void OnFail( const Vector2& p0, const Vector2& p1 ) const
		{
			Vector2 v;
			MathUtils::GeometryUtils::TestClosestPointOnLine2D( m_query.m_circleCenter.AsVector2(), p0.AsVector2(), p1.AsVector2(), v );
			Float distSq = ( m_query.m_circleCenter.AsVector2() - v ).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_query.m_closestDistSq = distSq;
				m_query.m_pointOut = v;
				// TODO: calculate Z
				m_query.m_pointOut.Z = m_query.m_basePos.Z;
				m_query.m_obstacleHit = true;
			}
		}

	} predicate( query );
	return MipMapTest( predicate );
}

Bool CTerrainMap::SpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CCollectCollisionPointsInCircleQueryData >
	{
		enum { AUTOFAIL = false };

		Predicate( CCollectCollisionPointsInCircleQueryData& query ) : DefaultPredicate( query ) {}

		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax ) const
		{
			Vector2 closestSpot;
			m_query.ClosestSpotRect( rectMin, rectMax, closestSpot );
			m_query.OnIntersection( closestSpot );
		}
		RED_INLINE void OnFail( const Vector2& p0, const Vector2& p1 ) const
		{
			Vector2 closestSpot;
			m_query.ClosestSpotSegment( p0, p1, closestSpot );
			m_query.OnIntersection( closestSpot );
		}

	} predicate( query );
	return MipMapTest( predicate );
}


Bool CTerrainMap::SpatialQuery( CLineQueryData& query ) const
{
	DefaultPredicate< CLineQueryData > predicate( query );
	return MipMapTest( predicate );
}
Bool CTerrainMap::SpatialQuery( CWideLineQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CWideLineQueryData >
	{
		Float m_rSq;
		Predicate( CWideLineQueryData& query )
			: DefaultPredicate( query ), m_rSq( query.m_radius * query.m_radius ) {}
		RED_INLINE Bool Intersect(const Vector2& vP0, const Vector2& vP1) const
		{
			return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( vP0, vP1, m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2() ) <= m_rSq;
		}
	} predicate( query );
	ASSERT( query.m_radius > 0.f );

	return MipMapTest( predicate );
}
Bool CTerrainMap::SpatialQuery( CClosestObstacleWideLineQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CClosestObstacleWideLineQueryData >
	{
		enum { AUTOFAIL = false };
		Predicate( CClosestObstacleWideLineQueryData& query )
			: DefaultPredicate( query ) {}
		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax )
		{
			Vector2 vT, vL;
			MathUtils::GeometryUtils::ClosestPointLineRectangle2D( m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2(), rectMin, rectMax, vL, vT );
			Float distSq = ( vT - vL ).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_query.m_closestDistSq = distSq;
				m_query.m_closestPointOnSegment = vL;
				m_query.m_closestGeometryPoint = vT;
				// TODO: calculate Z
				m_query.m_closestPointOnSegment.Z = m_query.m_v1.Z;
				m_query.m_closestGeometryPoint.Z = m_query.m_v1.Z;
				m_query.m_obstacleHit = true;
			}
		}
		RED_INLINE void OnFail( const Vector2& vP0, const Vector2& vP1 )
		{
			Vector2 vT, vL;
			MathUtils::GeometryUtils::ClosestPointsLineLine2D( vP0, vP1, m_query.m_v1.AsVector2(), m_query.m_v2.AsVector2(), vT, vL );
			Float distSq = ( vT - vL ).SquareMag();
			if ( distSq <= m_query.m_closestDistSq )
			{
				m_query.m_closestDistSq = distSq;
				m_query.m_closestPointOnSegment = vL;
				m_query.m_closestGeometryPoint = vT;
				// TODO: calculate Z
				m_query.m_closestPointOnSegment.Z = m_query.m_v1.Z;
				m_query.m_closestGeometryPoint.Z = m_query.m_v1.Z;
				m_query.m_obstacleHit = true;
			}
		}
	} predicate( query );
	return MipMapTest( predicate );
}

Bool CTerrainMap::SpatialQuery( CClearWideLineInDirectionQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CClearWideLineInDirectionQueryData >
	{
		enum { AUTOFAIL = false };
		Predicate( CClearWideLineInDirectionQueryData& query )
			: DefaultPredicate( query ) {}
		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax )
		{
			struct Functor
			{
				Vector2 m_rectMin;
				Vector2 m_rectMax;

				Functor( const Vector2& rectMin, const Vector2& rectMax )
					: m_rectMin( rectMin )
					, m_rectMax( rectMax ) {}

				Bool operator()( const Vector2& v0, const Vector2& v1, Float ps ) const
				{
					return MathUtils::GeometryUtils::TestDistanceSqrLineRectangle2D( v0, v1, m_rectMin, m_rectMax ) > ps;
				}
				Bool operator()( const Vector2& v, Float ps ) const
				{
					return !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( m_rectMin, m_rectMax, v, ps );
				}
			} fun( rectMin, rectMax );

			m_query.OnIntersection( fun );
		}
		RED_INLINE void OnFail( const Vector2& vP0, const Vector2& vP1 )
		{
			struct Functor
			{
				Vector2 m_p0;
				Vector2 m_p1;

				Functor( const Vector2& vP0, const Vector2& vP1 )
					: m_p0( vP0 )
					, m_p1( vP1 ) {}

				Bool operator()( const Vector2& v0, const Vector2& v1, Float ps ) const
				{
					return MathUtils::GeometryUtils::TestDistanceSqrLineLine2D( v0, v1, m_p0, m_p1 ) > (ps*ps);
				}
				Bool operator()( const Vector2& v, Float ps ) const
				{
					return !MathUtils::GeometryUtils::TestIntersectionCircleLine2D( v, ps, m_p0, m_p1 );
				}
			} fun( vP0, vP1 );

			m_query.OnIntersection( fun );
		}
		//
	} predicate( query );
	return MipMapTest( predicate );
}


Bool CTerrainMap::SpatialQuery( CRectangleQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CRectangleQueryData >
	{
		Predicate( CRectangleQueryData& query )
			: DefaultPredicate( query ) {}
	} predicate( query );
	return MipMapTest( predicate );
}

Bool CTerrainMap::SpatialQuery( CCollectGeometryInCirceQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CCollectGeometryInCirceQueryData >
	{
		Predicate( CCollectGeometryInCirceQueryData& query, const CTerrainMap* me )
			: DefaultPredicate( query ) , m_this( me ) {}

		enum { AUTOFAIL = false };

		RED_INLINE void OnFail( const Vector2& v1, const Vector2& v2 ) const
		{
			auto& o =  m_query.m_output;
			Uint32 i = o.Size();
			o.Grow( 2 );
			o[ i+0 ].Set3( v1.X, v1.Y, m_this->ComputeHeight( v1 ) );
			o[ i+1 ].Set3( v2.X, v2.Y, m_this->ComputeHeight( v2 ) );
		}
		RED_INLINE void OnFailRect( const Vector2& rectMin, const Vector2& rectMax ) const
		{
			Vector v1( rectMin.X, rectMin.Y, m_this->ComputeHeight( Vector2( rectMin.X, rectMin.Y ) ) );
			Vector v2( rectMin.X, rectMax.Y, m_this->ComputeHeight( Vector2( rectMin.X, rectMax.Y ) ) );
			Vector v3( rectMax.X, rectMin.Y, m_this->ComputeHeight( Vector2( rectMax.X, rectMin.Y ) ) );
			Vector v4( rectMax.X, rectMax.Y, m_this->ComputeHeight( Vector2( rectMax.X, rectMax.Y ) ) );

			Uint32 index = m_query.m_output.Size();
			m_query.m_output.Grow( 8 );
			m_query.m_output[ index+0 ] = v1;
			m_query.m_output[ index+1 ] = v2;
			m_query.m_output[ index+2 ] = v2;
			m_query.m_output[ index+3 ] = v4;
			m_query.m_output[ index+4 ] = v4;
			m_query.m_output[ index+5 ] = v3;
			m_query.m_output[ index+6 ] = v3;
			m_query.m_output[ index+7 ] = v1;
		}

		const CTerrainMap*			m_this;
	} predicate( query, this );
	return MipMapTest( predicate );
}

Bool CTerrainMap::SpatialQuery( CCustomTestQueryData& query ) const
{
	struct Predicate : public DefaultPredicate< CCustomTestQueryData >
	{
		SCustomCollisionTester& m_functor;
		Predicate( CCustomTestQueryData& query  )
			: DefaultPredicate( query ), m_functor( *query.m_customTester ) {}
		
		RED_INLINE Bool Intersect(const Vector2& v1, const Vector2& v2) const
		{
			return m_functor.IntersectLine( v1, v2 );
		}
	} predicate( query );
	return MipMapTest( predicate );
}

template < class TQuery >
Bool CTerrainMap::TFindWalkablePlace( TQuery* query, Vector2& outPosition ) const
{
	struct Predicate : public DefaultPredicate< TQuery >
	{
		Predicate( Vector2& outPosition )
			: m_outPoint( outPosition )
			, m_success( false ) {}
		enum
		{
			REPORT_SUCCESS = true,
			AUTOFAIL = false
		};

		RED_INLINE Bool OnSuccess( const Vector2& centralPoint ) const
		{
			m_outPoint = centralPoint;
			m_success = true;
			return false;
		}
		Vector2 m_outPoint;
		Bool m_success;
	} predicate( outPosition );
	MipMapTest( predicate );
	return predicate.m_success;
}

Float CTerrainMap::GetClosestFloorInRange(const Vector3& pos, Float radius, Vector3& pointOut)
{
	return true;
}

void CTerrainMap::OnPreLoad( CAreaDescription* area )
{
	ASSERT( area->IsTerrainArea() );

	m_area = static_cast< CTerrainAreaDescription* >( area );

	m_terrainInfo = &area->GetPathLib().GetTerrainInfo();
}

void CTerrainMap::OnPostLoad( CAreaDescription* area )
{
	CalculateMipMap();

	m_height.OnPostLoad( area->AsTerrainArea() );
}

Float CTerrainMap::ComputeHeight( const Vector2& localPos, Bool smooth ) const
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	const auto& heightData = GetArea().GetHeightData();
	if ( heightData.IsInitialized() )
	{
		return heightData.ComputeHeight( localPos );
	}
#endif
	ASSERT( !m_area->IsProcessing() );

	return
		smooth
		? m_height.ComputeHeightSmooth( localPos )
		: m_height.ComputeHeightFast( localPos );
}
Bool CTerrainMap::VHasChanged() const
{
	return !IsInitialVersion();
}
Bool CTerrainMap::VSave( const String& depotPath ) const
{
	return Save( depotPath );
}
void CTerrainMap::VOnPreLoad( CAreaDescription* area )
{
	OnPreLoad( area );
}
Bool CTerrainMap::VLoad( const String& depotPath, CAreaDescription* area )
{
	return Load( this, depotPath );
}
void CTerrainMap::VOnPostLoad( CAreaDescription* area )
{
	OnPostLoad( area );
}
const Char* CTerrainMap::VGetFileExtension() const
{
	return GetFileExtension();
}
ENavResType CTerrainMap::VGetResType() const
{
	return GetResType();
}


////////////////////////////////////////////////////////////////////////////
// CTerrainAreaDescription
////////////////////////////////////////////////////////////////////////////
CTerrainAreaDescription::CTerrainAreaDescription()
	: CAreaDescription()
	, m_tileCorner( 0,0 )
{
}
CTerrainAreaDescription::CTerrainAreaDescription( CPathLibWorld& pathlib, Id id )
	: CAreaDescription( pathlib, id )
{
	ComputeBBox();

	const CTerrainInfo& terrainInfo = pathlib.GetTerrainInfo();

	Int32 x, y;
	terrainInfo.GetTileCoordsFromId( m_id, x, y );
	m_tileCorner = terrainInfo.GetTileCorner( x, y );
}
CTerrainAreaDescription::~CTerrainAreaDescription()
{

}
#ifndef NO_EDITOR_PATHLIB_SUPPORT
void CTerrainAreaDescription::GenerateAsync( CAreaGenerationJob* job )
{
	CAreaNavgraphsRes* graphs = m_graphs.Get();
	if ( !graphs )
	{
		m_graphs.Construct();
		graphs = m_graphs.Get();
		graphs->OnPreLoad( this );
		graphs->OnPostLoad( this );
	}

	for ( Uint16 i = 0; i < MAX_ACTOR_CATEGORIES; ++i )
	{
		Uint32 version = 0;
		CNavGraph* g = graphs->GetGraph( i );
		if ( g )
		{
			version = g->GetVersion();
			graphs->ClearDetachedGraph( i );
		}

		if ( i < m_pathlib->GetGlobalSettings().GetCategoriesCount() && m_usedCategories & (1 << i))
		{
			g = graphs->NewGraph( i, this );
			g->SetVersion( version );
			if ( !g->Generate( m_pathlib, m_terrain.Get(), job ) )
			{
				graphs->ClearDetachedGraph( i );
				continue;
			}
			g->CompactData();
		}
	}

	CNavModyficationMap* metalinks = GetMetalinks();
	if ( metalinks )
	{
		metalinks->RemoveNotAppliedModyfications();
	}
}
void CTerrainAreaDescription::GenerateSync()
{
	GenerateAsync( NULL );
}
Bool CTerrainAreaDescription::PreGenerateSync()
{
	if ( !Super::PreGenerateSync() )
	{
		return false;
	}

#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	Int32 tileX, tileY;
	m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, tileX, tileY );
	if ( !m_heightData.InitializeSync( m_pathlib->GetWorld(), tileX, tileY ) )
	{
		PATHLIB_ERROR( TXT("Error while trying to get height data for tile %x, %x!\n"), tileX, tileY );
		return false;
	}
#endif

	return true;
}

void CTerrainAreaDescription::Describe( String& description )
{
	Int32 x, y;
	m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, x, y );
	description = String::Printf( TXT("TerrainTile %dx%d"), x, y );
}

void CTerrainAreaDescription::ExtractSurfaceData( CDetailedSurfaceData* surfaceData )
{
	CTerrainMap* terrain = m_terrain.Get();
	if ( !terrain )
	{
		return;
	}

	const CTerrainInfo* terrainInfo = terrain->GetTerrainInfo();

	Uint32 resolution = terrainInfo->GetTilesResolution();

	for ( Uint32 y = 0; y < resolution; ++y )
	{
		for ( Uint32 x = 0; x < resolution; ++x )
		{
			CDetailedSurfaceData::FieldData detailedField;

			detailedField.m_surface = CDetailedSurfaceData::FieldData::ESURFACE_Ok;
			detailedField.m_isMarkedByInstance = false;
			detailedField.m_isSmoothed = false;

			switch( terrain->GetTerrainQuadState( terrain->GetQuadIndex( resolution-1, y ) ) )
			{
			case CTerrainMap::QUAD_FREE:
				break;
			case CTerrainMap::QUAD_BLOCK_ALL:
				detailedField.m_surface =  CDetailedSurfaceData::FieldData::ESURFACE_Slope;
				break;
			case CTerrainMap::QUAD_INSTANCE:
				detailedField.m_isMarkedByInstance = true;
				break;
			default:
			case CTerrainMap::QUAD_OBSTACLE:
				ASSERT( TXT("Not supported!") );
				ASSUME( false );
			}

			surfaceData->SetField( x, y, detailedField );
		}
	}
}

Bool CTerrainAreaDescription::ApplySurfaceData( CDetailedSurfaceData* surfaceData )
{
	CTerrainMap* terrain = m_terrain.Get();
	if ( !terrain )
	{
		return false;
	}

	const CTerrainInfo* terrainInfo = terrain->GetTerrainInfo();

	Uint32 resolution = terrainInfo->GetTilesResolution();

	Bool markDirty = false;
	for ( Uint32 y = 0; y < resolution; ++y )
	{
		for ( Uint32 x = 0; x < resolution; ++x )
		{
			CDetailedSurfaceData::FieldData detailedField = surfaceData->GetField( x, y );

			CTerrainMap::eQuadState quadState = CTerrainMap::QUAD_FREE;
			if ( detailedField.m_isMarkedByInstance )
			{
				quadState = CTerrainMap::QUAD_INSTANCE;
			}
			else if ( detailedField.m_surface != CDetailedSurfaceData::FieldData::ESURFACE_Ok )
			{
				quadState = CTerrainMap::QUAD_BLOCK_ALL;
			}

			markDirty = terrain->SetTerrainQuadState( terrain->GetQuadIndex( x, y ), quadState, false ) || markDirty;
		}
	}

	if ( markDirty )
	{
		terrain->CalculateMipMap();
		terrain->MarkVersionDirty();
		return true;
	}
	return false;
}

Bool CTerrainAreaDescription::ComputeTerrainData( CTerrainSurfaceProcessingThread* task )
{
	CTerrainMap* terrain = m_terrain.Get();
	if ( !terrain )
	{
		return false;
	}

	const CTerrainInfo* terrainInfo = terrain->GetTerrainInfo();

	CDetailedSurfaceData data;
	data.Initialize( terrainInfo->GetTilesResolution() );
	ComputeTerrainData( &data, task );
	return ApplySurfaceData( &data );
}

Bool CTerrainAreaDescription::ComputeTerrainData( CDetailedSurfaceData* surface, CTerrainSurfaceProcessingThread* task )
{
	const CTerrainInfo* terrainInfo = &m_pathlib->GetTerrainInfo();

#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	if ( !m_heightData.IsInitialized() )
	{
		return false;
	}
#else
	CWorld* world = m_pathlib->GetWorld();
	const CClipMap* clipMap = world->GetTerrain();
	if ( !clipMap )
		return false;

	Int32 x, y;
	terrainInfo->GetTileCoordsFromId( m_id, x, y );
	CTerrainTile* tile = clipMap->GetTile( x, y );
	if ( !tile )
		return false;

	Uint16* hightMapData = tile->GetLevelSyncHM( 0 );
#endif

	CGlobalWater* globalWater = m_pathlib->GetWorld()->GetGlobalWater();
	const Float seaLevel = m_pathlib->GetGlobalSettings().GetSeaLevel();
	Uint32 resolution = terrainInfo->GetTilesResolution();
	Float quadSize = terrainInfo->GetQuadSize();

	const Float maxTerrainAngle = m_pathlib->GetGlobalSettings().GetMaxTerrainSlope();

	const Float maxZDiff = quadSize * tan( DEG2RAD( maxTerrainAngle ) );

	Bool markDirty = false;
	for ( Uint32 y = 0; y < resolution-1; ++y )
	{
		for ( Uint32 x = 0; x < resolution-1; ++x )
		{
			if ( task )
			{
				task->SetTaskProgress( Float(y * resolution + x) / Float(resolution*resolution) );
			}

			struct Local
			{
				static Uint32 TexelIndex( Int32 x, Int32 y, Uint32 tileResolution )
				{
					return y * tileResolution + x;
				}
			};

			// read hight data
			Float z[4];
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
			m_heightData.GetTexelVertsHeight( x, y, z );
#else
			z[0] = clipMap->TexelHeightToHeight( hightMapData[ Local::TexelIndex( x, y, resolution ) ] );
			z[1] = clipMap->TexelHeightToHeight( hightMapData[ Local::TexelIndex( x+1, y, resolution ) ] );
			z[2] = clipMap->TexelHeightToHeight( hightMapData[ Local::TexelIndex( x, y+1, resolution ) ] );
			z[3] = clipMap->TexelHeightToHeight( hightMapData[ Local::TexelIndex( x+1, y+1, resolution ) ] );
#endif

			Vector texelPosition(
				m_tileCorner.X + x * quadSize + 0.5f * quadSize,
				m_tileCorner.Y + y * quadSize + 0.5f * quadSize,
				0.f );


			// compute height difference
			Float minZ = z[0];
			Float maxZ = z[0];
			for ( Uint32 i = 1; i < 4; ++i )
			{
				minZ = Min( minZ, z[i] );
				maxZ = Max( maxZ, z[i] );
			}
			
			Float waterLevel = globalWater ? globalWater->GetWaterLevelBasic( texelPosition.X, texelPosition.Y ) + seaLevel : -1024.f;
			// decide over final terrain state
			CDetailedSurfaceData::FieldData::ESurfaceState terrainState = CDetailedSurfaceData::FieldData::ESURFACE_Ok;

			Bool underWater = minZ < waterLevel;
			if ( underWater )
			{
				terrainState = CDetailedSurfaceData::FieldData::ESURFACE_Underwater;
			}
			else if ( maxZ - minZ > maxZDiff )
			{
				terrainState = CDetailedSurfaceData::FieldData::ESURFACE_Slope;
			}

			CDetailedSurfaceData::FieldData field = surface->GetField( x, y );
			field.m_surface = terrainState;
			surface->SetField( x, y, field );
		}

		// TODO: TAKE CARE OF IT
		{
			CDetailedSurfaceData::FieldData field = surface->GetField( resolution-1, y );
			CDetailedSurfaceData::FieldData fieldLast = surface->GetField( resolution-2, y );
			field.m_surface = fieldLast.m_surface;
			surface->SetField( resolution-1, y, field );
		}
	}

	// TODO: TAKE CARE OF IT
	for ( Uint32 x = 0; x < resolution; ++x )
	{
		CDetailedSurfaceData::FieldData field = surface->GetField( x, resolution-1 );
		CDetailedSurfaceData::FieldData fieldLast = surface->GetField( x, resolution-2 );
		field.m_surface = fieldLast.m_surface;
		surface->SetField( x, resolution-1, field );
	}

	//SmoothOutTerrainData( task );
	return true;
}

#endif
void CTerrainAreaDescription::VPrecomputeObstacleSpawnData( CObstacleSpawnContext& context )
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	CTerrainMap* terrainMap = m_terrain.Get();

	ASSERT( m_heightData.IsInitialized() || m_pathlib->IsGameRunning() );

	const auto& baseData = context.m_baseData;
	auto& optData = context.m_optimizedData;
	const auto& baseShapes = baseData.m_shapes;
	auto& optShapes = optData.m_shapes;

	enum QuadFlags
	{
		QF_FREE				= 0,
		QF_OCCUPIED			= 1,
		QF_REACHABLE		= 2,
	};
	struct QuadData
	{
		Uint8					m_flags;
		Uint8					m_convexId;
	};


	// copy base data
	optData.m_collisionType = baseData.m_collisionType;
	optData.m_mapping = baseData.m_mapping;

	// compute boungings
	Int32 xmin, ymin, xmax, ymax, xreach, yreach;
	//Float quadSize = terrainMap->GetTerrainInfo()->GetQuadSize();

	Vector2 localBounds[2];
	localBounds[ 0 ] = baseData.m_bbox.Min.AsVector2();
	localBounds[ 1 ] = baseData.m_bbox.Max.AsVector2();
	WorldToLocal( localBounds[ 0 ] );
	WorldToLocal( localBounds[ 1 ] );

	terrainMap->GetQuadCoordsAt( localBounds[ 0 ]/* - Vector2( quadSize, quadSize )*/, xmin, ymin );
	terrainMap->GetQuadCoordsAt( localBounds[ 1 ]/* + Vector2( quadSize, quadSize )*/, xmax, ymax );

	xreach = xmax + 1 - xmin;
	yreach = ymax + 1 - ymin;

	Int32 fieldSize = xreach * yreach;

	auto funMapIndex = [ xreach, xmin, ymin, xmax, ymax ] ( Int32 x, Int32 y ) -> Int32
	{
		ASSERT( x >= xmin && x <= xmax && y >= ymin && y <= ymax );
		return (y-ymin) * xreach + (x - xmin);
	};

	//auto funMapCoord = [ xreach, xmin, ymin, xmax, ymax ] ( Int32 i, Int32& x, Int32& y )
	//{
	//	y = i / xreach + ymin;
	//	x = i % xreach + xmin;
	//};

	// setup data structures
	TDynArray< QuadData > map;
	TDynArray< TPair< Uint16, Uint16 > > activeMapIndexes;
	TDynArray< Vector3 > pointCloud;
	TDynArray< Vector2 > convexHullInput;


	map.ResizeFast( fieldSize );
	Red::System::MemoryZero( map.Data(), map.DataSize() );

	optShapes.Reserve( baseShapes.Size() );

	// process all shapes
	for ( auto it = baseShapes.Begin(), end = baseShapes.End(); it != end; ++it )
	{
		CObstacleShapeGeometryData* shapeData = *it;
		switch ( shapeData->m_type )
		{
		case CObstacleShapeGeometryData::T_CYLINDER:
		case CObstacleConvexOccluderData::T_PURE_CONVEX:
			{
				CObstacleConvexOccluderData* convexData = nullptr;
				CObstacleCylinderData* cylinderData = nullptr;
				if ( shapeData->m_type == CObstacleConvexOccluderData::T_PURE_CONVEX )
				{
					convexData = static_cast< CObstacleConvexOccluderData* >( shapeData );
				}
				else
				{
					cylinderData = static_cast< CObstacleCylinderData* >( shapeData );
				}

				// Check if convex is contacting the ground
				Bool touchGround = false;

				Box localBBox = shapeData->m_shapeBBox;
				WorldToLocal( localBBox );

				Float shapeMinZ;
				Float shapeMaxZ;
				
				if ( m_heightData.IsInitialized() )
				{
					shapeMinZ = FLT_MAX;
					shapeMaxZ = -FLT_MAX;

					Int32 xMin, yMin, xMax, yMax;
					terrainMap->GetQuadCoordsAt( localBBox.Min.AsVector2(), xMin, yMin );
					terrainMap->GetQuadCoordsAt( localBBox.Max.AsVector2(), xMax, yMax );

				

					for ( Int32 y = yMin; y <= yMax && !touchGround; ++y )
					{
						for ( Int32 x = xMin; x <= xMax; ++x )
						{
							// if 
							CTerrainMap::QuadIndex quadIndex = terrainMap->GetQuadIndex( x, y );
							CTerrainMap::eQuadState quadState = terrainMap->GetQuadState( quadIndex );
							// test if terrain at given point is accessible
							
							if ( quadState == CTerrainMap::QUAD_BLOCK_ALL || quadState == CTerrainMap::QUAD_INSTANCE )
							{
								// quad is blocked and we are not interested in it
								continue;
							}

							Float z[4];
							m_heightData.GetTexelVertsHeight( x, y, z );
							Float zmin = Min( z[ 0 ], z[ 1 ], Min( z[ 2 ], z[ 3 ] ) ) + 0.2f;
							Float zmax = Max( z[ 0 ], z[ 1 ], Max( z[ 2 ], z[ 3 ] ) ) + DEFAULT_AGENT_HEIGHT;
							// test if obstacle is in Z-vicinity of terrain
							if ( !MathUtils::GeometryUtils::RangeOverlap1D( localBBox.Min.Z, localBBox.Max.Z, zmin, zmax ) )
							{
								continue;
							}

							Vector2 quadMin, quadMax;
							terrainMap->GetQuadLocalPosition( x, y, quadMin, quadMax );
							LocalToWorld( quadMin );
							LocalToWorld( quadMax );
							// test if obstacle is in fact over this terrain tile
							if ( convexData )
							{
								if ( !MathUtils::GeometryUtils::TestIntersectionPolygonRectangle2D( convexData->m_occluder, quadMin, quadMax, 0.f ) )
								{
									continue;
								}
							}
							else // if ( cylinderData )
							{
								if ( !MathUtils::GeometryUtils::TestIntersectionCircleRectangle2D( quadMin, quadMax, cylinderData->m_pos, cylinderData->m_radius ) )
								{
									continue;
								}
							}
							touchGround = true;
							shapeMinZ = Min( zmin, shapeMinZ );
							shapeMaxZ = Max( zmax, shapeMaxZ );
						}
					}

					if ( !touchGround )
					{
						continue;
					}
				}
				else
				{
					shapeMinZ = shapeData->m_shapeBBox.Min.Z;
					shapeMaxZ = shapeData->m_shapeBBox.Max.Z;
				}

				CObstacleShapeGeometryData* newShape;
				if ( convexData )
				{
					newShape = new CObstacleConvexOccluderData( Move( *convexData ) );
				}
				else // if ( cylinderData )
				{
					newShape = new CObstacleCylinderData( Move( *cylinderData ) );
				}
				newShape->m_shapeBBox.Min.Z = Min( shapeMinZ, newShape->m_shapeBBox.Min.Z );
				newShape->m_shapeBBox.Max.Z = Max( shapeMaxZ + DEFAULT_AGENT_HEIGHT, newShape->m_shapeBBox.Max.Z );

				optShapes.PushBack( newShape );
			}
			
			break;
		case CObstacleShapeGeometryData::T_INDVERTS:
			{
				if ( !m_heightData.IsInitialized() )
				{
					continue;
				}

				const CObstacleIndicedVertsGeometryData& baseShapeDef = static_cast< const CObstacleIndicedVertsGeometryData& >( *shapeData );

				Bool acceptAnyTriangle = false;

				// reset map data
				Red::System::MemoryZero( map.Data(), map.DataSize() );

				// filter triangles by their accessibility
				for ( Uint32 index = 0, indexCount = baseShapeDef.m_indices.Size(); index < indexCount; index += 3 )
				{
					Int32 triMinX, triMinY, triMaxX, triMaxY;

					// get indexes
					Uint32 i0 = baseShapeDef.m_indices[ index+0 ];
					Uint32 i1 = baseShapeDef.m_indices[ index+1 ];
					Uint32 i2 = baseShapeDef.m_indices[ index+2 ];

					// get vertex
					Vector v0 = baseShapeDef.m_verts[ i0 ];
					Vector v1 = baseShapeDef.m_verts[ i1 ];
					Vector v2 = baseShapeDef.m_verts[ i2 ];

					WorldToLocal( v0.AsVector3() );
					WorldToLocal( v1.AsVector3() );
					WorldToLocal( v2.AsVector3() );

					// compute triangle boundings
					Vector2 posTriMin(
						Min( v0.X, v1.X, v2.X ),
						Min( v0.Y, v1.Y, v2.Y )
						);

					Vector2 posTriMax(
						Max( v0.X, v1.X, v2.X ),
						Max( v0.Y, v1.Y, v2.Y )
						);

					terrainMap->GetQuadCoordsAt( posTriMin, triMinX, triMinY );
					terrainMap->GetQuadCoordsAt( posTriMax, triMaxX, triMaxY );

					ASSERT( triMinX >= xmin && triMaxX <= xmax && triMinY >= ymin && triMaxY <= ymax );

					Box quadBox;
					Bool acceptTriangle = false;

					for ( Int32 y = triMinY; y <= triMaxY; ++y )
					{
						for ( Int32 x = triMinX; x <= triMaxX; ++x )
						{
							// test if quad is accessible
							CTerrainMap::QuadIndex quadIndex = terrainMap->GetQuadIndex( x, y );
							CTerrainMap::eQuadState quadState = terrainMap->GetQuadState( quadIndex );
							if ( quadState == CTerrainMap::QUAD_BLOCK_ALL || quadState == CTerrainMap::QUAD_INSTANCE )
							{
								// quad is blocked and we are not interested in it
								continue;
							}

							// test quad-triangle intersection
							terrainMap->GetQuadLocalPosition( x, y, quadBox.Min.AsVector2(), quadBox.Max.AsVector2() );
							Float celZMin, celZMax;
							m_heightData.GetTexelHeightLimits( x, y, celZMin, celZMax );
							quadBox.Min.Z = celZMin + 0.2f;
							quadBox.Max.Z = celZMax + DEFAULT_AGENT_HEIGHT;

							if ( !MathUtils::GeometryUtils::RangeOverlap1D( Min( v0.Z, v1.Z, v2.Z ), Max( v0.Z, v1.Z, v2.Z ), quadBox.Min.Z, quadBox.Max.Z ) )
							{
								continue;
							}

							if ( !MathUtils::GeometryUtils::TestIntersectionTriAndRectangle2D( v0.AsVector2(), v1.AsVector2(), v2.AsVector2(), quadBox.Min.AsVector2(), quadBox.Max.AsVector2() ) )
								//if ( !MathUtils::GeometryUtils::TestIntersectionTriAndBox( v0, v1, v2, quadBox ) )
							{
								continue;
							}
							acceptTriangle = true;
							acceptAnyTriangle = true;
							// mark triangle
							Int32 mapIndex = funMapIndex( x, y );
							map[ mapIndex ].m_flags = QF_OCCUPIED;
						}
					}
					if ( acceptTriangle )
					{
						// feed point cloud
						const Uint32 eOccupied = QF_OCCUPIED;
						auto funMarkCel =
							[ this, &funMapIndex, &map, &pointCloud, eOccupied ] ( Int32 x, Int32 y, const Vector3& v0, const Vector3& v1 )
						{
							Int32 mapIndex = funMapIndex( x, y );
							if ( (map[ mapIndex ].m_flags & eOccupied) == 0 )
							{
								return;
							}
							Float celZMin, celZMax;
							m_heightData.GetTexelHeightLimits( x, y, celZMin, celZMax );
							celZMin = celZMin + 0.1f;
							celZMax = celZMax + DEFAULT_AGENT_HEIGHT;
							Float edgeZMin = Min( v0.Z, v1.Z );
							Float edgeZMax = Max( v0.Z, v1.Z );
							if ( edgeZMin >= celZMin && edgeZMax <= celZMax )
							{
								pointCloud.PushBack( v0 );
								pointCloud.PushBack( v1 );
							}
							else if ( MathUtils::GeometryUtils::RangeOverlap1D( edgeZMin, edgeZMax, celZMin, celZMax ) )
							{
								Float zRatio0;
								Float zRatio1;

								if ( v0.Z >= celZMin && v0.Z <= celZMax )
								{
									zRatio0 = 0.f;
								}
								else if ( v0.Z < celZMin )
								{
									zRatio0 = (celZMin - v0.Z) / (v1.Z - v0.Z);
								}
								else
								{
									zRatio0 = (v0.Z - celZMax) / (v0.Z - v1.Z);
								}
								ASSERT( zRatio0 >= 0.f - NumericLimits< Float >::Epsilon() && zRatio0 <= 1.f + NumericLimits< Float >::Epsilon() );

								if ( v1.Z >= celZMin && v1.Z <= celZMax )
								{
									zRatio1 = 0.f;
								}
								else if ( v1.Z < celZMin )
								{
									zRatio1= (celZMin - v1.Z) / (v0.Z - v1.Z);
								}
								else
								{
									zRatio1 = (v1.Z - celZMax) / (v1.Z - v0.Z);
								}
								ASSERT( zRatio1 >= 0.f - NumericLimits< Float >::Epsilon() && zRatio1 <= 1.f + NumericLimits< Float >::Epsilon() );

								Vector3 diff = v1 - v0;
								Vector3 point0 = v0 + diff * zRatio0;
								Vector3 point1 = v0 + diff * (1.f-zRatio1);
								pointCloud.PushBack( point0 );
								pointCloud.PushBack( point1 );
							}
						};
						auto funMarkEdge =
							[ terrainMap, &funMarkCel ] ( const Vector3& v0, const Vector3& v1 )
						{
							Int32 xStart, yStart;
							Int32 xEnd, yEnd;
							terrainMap->GetQuadCoordsAt( v0.AsVector2(), xStart, yStart );
							terrainMap->GetQuadCoordsAt( v1.AsVector2(), xEnd, yEnd );
							Int32 x = xStart;
							Int32 y = yStart;
							Vector3 dir = v1 - v0;
							Int32 dirX = dir.X >= 0.f ? 1 : -1;
							Int32 dirY = dir.Y >= 0.f ? 1 : -1;
							Bool testDone = false;
							do 
							{
								// get cel sizes
								Vector2 quadMin, quadMax;
								terrainMap->GetQuadLocalPosition( x, y, quadMin, quadMax );
								Vector2 splitPoint(
									dirX > 0 ? quadMax.X : quadMin.X,
									dirY > 0 ? quadMax.Y : quadMin.Y
									);

								Vector3 celV0, celV1;
								if ( !(v0.AsVector2() - v1.AsVector2()).IsAlmostZero() )
								{
									Vector2 tmp;
									MathUtils::GeometryUtils::ClosestPointLineRectangle2D( v0.AsVector2(), v1.AsVector2(), quadMin, quadMax, celV0.AsVector2(), tmp );
									//ASSERT( (celV0.AsVector2() - tmp).SquareMag() < 0.1f );
									MathUtils::GeometryUtils::ClosestPointLineRectangle2D( v1.AsVector2(), v0.AsVector2(), quadMin, quadMax, celV1.AsVector2(), tmp );
									//ASSERT( (celV1.AsVector2() - tmp).SquareMag() < 0.1f );
									Float ratio0, ratio1;
									MathUtils::GeometryUtils::TestClosestPointOnLine2D( celV0.AsVector2(), v0.AsVector2(), v1.AsVector2(), ratio0, tmp );
									MathUtils::GeometryUtils::TestClosestPointOnLine2D( celV1.AsVector2(), v0.AsVector2(), v1.AsVector2(), ratio1, tmp );
									celV0.Z = v0.Z + ratio0 * dir.Z;
									celV1.Z = v0.Z + ratio1 * dir.Z;
								}
								else
								{
									celV0 = v0;
									celV1 = v1;
								}

								funMarkCel( x, y, celV0, celV1 );

								Bool goRight = (splitPoint - v0.AsVector2()).CrossZ( dir.AsVector2() ) <= 0.f;
								Bool moveX = goRight ^ (dirX > 0) ^ (dirY > 0);

								if ( moveX )
								{
									x += dirX;
								}
								else
								{
									y += dirY;
								}

								testDone = false;
								if ( dirX > 0 )
								{
									if ( x > xEnd )
									{
										testDone = true;
									}
								}
								else
								{
									if ( x < xEnd )
									{
										testDone = true;
									}
								}
								if ( dirY > 0 )
								{
									if ( y > yEnd )
									{
										testDone = true;
									}
								}
								else
								{
									if ( y < yEnd )
									{
										testDone = true;
									}
								}
							} while ( !testDone );
						};

						funMarkEdge( v0.AsVector3(), v1.AsVector3() );
						funMarkEdge( v1.AsVector3(), v2.AsVector3() );
						funMarkEdge( v2.AsVector3(), v0.AsVector3() );
					}
				}

				if ( !acceptAnyTriangle )
				{
					// No triangles accepted. Skip this shape in generation.
					continue;
				}

				// now lets possibly split input data into separate convexes
				Uint8 convexCount = 0;
				Bool usePrecalculatedZBoundings = true;
				Float zMinMax[ 512 ];
				for ( Int32 y = ymin; y <= ymax; ++y )
				{
					for ( Int32 x = xmin; x <= xmax; ++x )
					{
						Int32 mapIndex = funMapIndex( x, y );

						if ( map[ mapIndex ].m_flags == QF_OCCUPIED && map[ mapIndex ].m_convexId == 0 )
						{
							if ( convexCount == 0xff )
							{
								convexCount = 1;
								usePrecalculatedZBoundings = false;
								break;
							}
							Uint32 zMinMaxIndex = convexCount*2;
							++convexCount;
							
							zMinMax[ zMinMaxIndex+0 ] = FLT_MAX;
							zMinMax[ zMinMaxIndex+1 ] = -FLT_MAX;

							map[ mapIndex ].m_convexId = convexCount;

							auto funCheckField =
								[ &map, &funMapIndex, convexCount/*, xmin, ymin, this*/ ] ( Int32 x, Int32 y ) -> Bool
							{
								auto& f = map[ funMapIndex( x, y ) ];
								if ( f.m_flags == 1 && f.m_convexId != convexCount )
								{
									f.m_convexId = convexCount;
									return true;
								}
								return false;
							};


							activeMapIndexes.PushBack( TPair< Uint16, Uint16 >( Uint16(x), Uint16(y) ) );
							do 
							{
								auto& e = activeMapIndexes.Back();
								Int32 x = e.m_first;
								Int32 y = e.m_second;
								activeMapIndexes.PopBackFast();
								map[ funMapIndex( x, y ) ].m_convexId = convexCount;

								Float celZMin, celZMax;
								m_heightData.GetTexelHeightLimits( x, y, celZMin, celZMax );

								zMinMax[ zMinMaxIndex+0 ] = Min( zMinMax[ zMinMaxIndex+0 ], celZMin );
								zMinMax[ zMinMaxIndex+1 ] = Max( zMinMax[ zMinMaxIndex+1 ], celZMax );

								if ( x > xmin && funCheckField( x-1, y )  )
								{
									activeMapIndexes.PushBack( TPair< Uint16, Uint16 >( Uint16(x-1), Uint16(y) ) );
								}

								if ( x < xmax && funCheckField( x+1, y ) )
								{
									activeMapIndexes.PushBack( TPair< Uint16, Uint16 >( Uint16(x+1), Uint16(y) ) );
								}

								if ( y > ymin && funCheckField( x, y-1 ) )
								{
									activeMapIndexes.PushBack( TPair< Uint16, Uint16 >( Uint16(x), Uint16(y-1) ) );
								}

								if ( y < ymax && funCheckField( x, y+1 ) )
								{
									activeMapIndexes.PushBack( TPair< Uint16, Uint16 >( Uint16(x), Uint16(y+1) ) );
								}

							} while ( !activeMapIndexes.Empty() );
						}
					}
				}


				for ( Uint32 convexId = 1; convexId <= convexCount; ++convexId )
				{
					CObstacleConvexOccluderData optDef = CObstacleConvexOccluderData();

					// compute shape bbox
					optDef.m_shapeBBox.Clear();

					if ( usePrecalculatedZBoundings )
					{
						optDef.m_shapeBBox.Min.Z = zMinMax[ (convexId-1)*2+0 ];
						optDef.m_shapeBBox.Max.Z = zMinMax[ (convexId-1)*2+1 ] + DEFAULT_AGENT_HEIGHT;
					}

					for ( Uint32 index = 0, indexCount = pointCloud.Size(); index < indexCount; ++index )
					{
						Vector3 point = pointCloud[ index ];
						Int32 x,y;
						terrainMap->GetQuadCoordsAt( point.AsVector2(), x, y );

						if ( map[ funMapIndex( x, y ) ].m_convexId == convexId )
						{
							LocalToWorld( point );
							convexHullInput.PushBack( point.AsVector2() );

							if ( !usePrecalculatedZBoundings )
							{
								optDef.m_shapeBBox.Min.Z = Min( optDef.m_shapeBBox.Min.Z, point.Z );
								optDef.m_shapeBBox.Max.Z = Max( optDef.m_shapeBBox.Min.Z, point.Z );
							}
						}
					}
					//ASSERT( !convexHullInput.Empty() );
					if ( !convexHullInput.Empty() )
					{
						MathUtils::GeometryUtils::ComputeConvexHull2D( convexHullInput, optDef.m_occluder );
						convexHullInput.ClearFast();

						for ( auto it = optDef.m_occluder.Begin(), end = optDef.m_occluder.End(); it != end; ++it )
						{
							const Vector2& v = *it;

							optDef.m_shapeBBox.Min.X = Min( optDef.m_shapeBBox.Min.X, v.X );
							optDef.m_shapeBBox.Max.X = Max( optDef.m_shapeBBox.Max.X, v.X );
							optDef.m_shapeBBox.Min.Y = Min( optDef.m_shapeBBox.Min.Y, v.Y );
							optDef.m_shapeBBox.Max.Y = Max( optDef.m_shapeBBox.Max.Y, v.Y );
						}

						optShapes.PushBack( new CObstacleConvexOccluderData( Move( optDef ) ) );
					}
				}

				pointCloud.ClearFast();
			}
			break;
		}
	}

	
	
	// compute cumulative bbox
	optData.m_bbox.Clear();
	for ( auto it = optShapes.Begin(), end = optShapes.End(); it != end; ++it )
	{
		optData.m_bbox.AddBox( (*it)->m_shapeBBox );
	}
	
	context.m_dataOptimized = true;
#endif	// !PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
}


void CTerrainAreaDescription::Initialize()
{
	CAreaDescription::Initialize();

	CTerrainMap* terrainMap = m_terrain.Get();
	if ( terrainMap )
	{
		terrainMap->Initialize();
	}
}



void CTerrainAreaDescription::WriteToBuffer( CSimpleBufferWriter& writer )
{
	Super::WriteToBuffer( writer );

	writer.SmartPut( m_terrain );
}
Bool CTerrainAreaDescription::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	if ( !reader.SmartGet( m_terrain ) )
	{
		return false;
	}

	return true;
}

void CTerrainAreaDescription::SetPathLib( CPathLibWorld* pathlib )
{
	CAreaDescription::SetPathLib( pathlib );
	ComputeBBox();

	const CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();

	Int32 x, y;
	terrainInfo.GetTileCoordsFromId( m_id, x, y );
	m_tileCorner = terrainInfo.GetTileCorner( x, y );

	CTerrainMap* terrainMap = m_terrain.Get();
	if ( terrainMap )
	{
		terrainMap->SetTerrainInfo( &m_pathlib->GetTerrainInfo() );
	}
}

Bool CTerrainAreaDescription::IterateAreaResources( ResourceFunctor& functor )
{
	if ( !functor.Handle( m_terrain, m_terrain.GetResType() ) )
	{
		return false;
	}

	return Super::IterateAreaResources( functor );
}

void CTerrainAreaDescription::OnPreUnload()
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	m_heightData.Clear();
#endif

	Super::OnPreUnload();
}

void CTerrainAreaDescription::OnPostLoad()
{
	Super::OnPostLoad();
	// TODO: Make sure we do have valid terrain!
	CTerrainMap* terrainMap = m_terrain.Get();
	if ( !terrainMap )
	{
		m_terrain.Construct();
		terrainMap = m_terrain.Get();
		terrainMap->OnPreLoad( this );
		terrainMap->Initialize();
		terrainMap->OnPostLoad( this );
		// its already worth saving
		terrainMap->MarkVersionDirty();
	}
}

AreaId CTerrainAreaDescription::GetNeighbourId( Int32 xDiff, Int32 yDiff ) const
{
	const CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();
	Int32 x, y;
	terrainInfo.GetTileCoordsFromId( m_id, x, y );
	x += xDiff;
	y += yDiff;
	if ( x < 0 )
	{
		return INVALID_AREA_ID;
	}
	else if ( Uint32(x) > terrainInfo.GetTilesInRow() )
	{
		return INVALID_AREA_ID;
	}
	if ( y < 0 )
	{
		return INVALID_AREA_ID;
	}
	else if ( Uint32(y) > terrainInfo.GetTilesInRow() )
	{
		return INVALID_AREA_ID;
	}
	return terrainInfo.GetTileIdFromCoords( x, y );
}
void CTerrainAreaDescription::GatherPossibleConnectors( AreaId neighbourAreaId, TDynArray< Vector3 >& outLocations ) const
{
	struct Local : public Red::System::NonCopyable
	{
		CTerrainMap*				m_terrain;
		TDynArray< Vector3 >&		m_outLocations;

		Local( CTerrainMap* terrain, TDynArray< Vector3 >& outLocations )
			: m_terrain( terrain )
			, m_outLocations( outLocations ) {}
		void HandleRegion( Int32 startX, Int32 startY, Int32 endX, Int32 endY, AreaId neighbourAreaId )
		{
			CTerrainAreaDescription* area = &m_terrain->GetArea();

			// compute how many nodes we want to spawn
			Vector2 borderPos1 = m_terrain->GetQuadCenterLocal( startX, startY );
			Vector2 borderPos2 = m_terrain->GetQuadCenterLocal( endX, endY );

			Vector2 diff2D = borderPos2 - borderPos1;
			Uint32 nodesToSpawn = 1 + Uint32( diff2D.Mag() / area->GetMaxNodesDistance() );
			Vector2 nodesDiff = diff2D * (1.f / (1.f + Float( nodesToSpawn ) ) );
			for ( Uint32 i = 0; i < nodesToSpawn; ++i )
			{
				// compute node position
				Vector3 connectorPos;
				connectorPos.AsVector2() = borderPos1 + nodesDiff * Float( i+1 );
				connectorPos.Z = m_terrain->ComputeHeight( connectorPos.AsVector2() );
				area->LocalToWorld( connectorPos );

				m_outLocations.PushBack( connectorPos );
			}
		}
		void Iterate( Int32 otherCoordBaseVal, Int32 iterateCoord, AreaId neighbourAreaId )
		{
			const CTerrainInfo* terrainInfo = m_terrain->GetTerrainInfo();
			const Int32 tileResolution = terrainInfo->GetTilesResolution();

			Int32 regionStart[ 2 ];															// start of region we are iterating
			Int32 coord[ 2 ];																	// current iteration coordinates
			coord[ !iterateCoord ] = otherCoordBaseVal;
			regionStart[ !iterateCoord ] = otherCoordBaseVal;
			regionStart[ iterateCoord ] = -1;
			for ( coord[ iterateCoord ] = 0; coord[ iterateCoord ] < tileResolution - 1; ++coord[ iterateCoord ] )
			{
				CTerrainMap::eQuadState quadState = m_terrain->GetQuadState( m_terrain->GetQuadIndex( coord[ 0 ], coord[ 1 ] ) );
				if ( quadState == CTerrainMap::QUAD_FREE )
				{
					if ( regionStart[ iterateCoord ] < 0 )
					{
						// start new region
						regionStart[ iterateCoord ] = coord[ iterateCoord ];
					}
				}
				else
				{
					if ( regionStart[ iterateCoord ] >= 0 )
					{
						--coord[ iterateCoord ];
						HandleRegion( regionStart[ 0 ], regionStart[ 1 ], coord[ 0 ], coord[ 1 ], neighbourAreaId );
						++coord[ iterateCoord ];
					}
					regionStart[ iterateCoord ] = -1;
				}
			}
			if ( regionStart >= 0 )
			{
				coord[ iterateCoord ] = tileResolution-1;
				Local::HandleRegion( regionStart[ 0 ], regionStart[ 1 ], coord[ 0 ], coord[ 1 ], neighbourAreaId );
			}
		}
	};

	CTerrainMap* terrain = m_terrain.Get();
	
	Local local( terrain, outLocations );

	const CTerrainInfo* terrainInfo = terrain->GetTerrainInfo();
	const Uint32 tileResolution = terrainInfo->GetTilesResolution();
	const Int32 tilesInRow = terrainInfo->GetTilesInRow();
	AreaId tileId = GetId();
	Int32 tileX, tileY;
	terrainInfo->GetTileCoordsFromId( tileId, tileX, tileY );

	// iterate x
	if ( tileY > 0 )
	{
		AreaId neighbourTileId = terrainInfo->GetTileIdFromCoords( tileX, tileY-1 );
		if ( neighbourAreaId == neighbourTileId )
		{
			local.Iterate( 0, 0, neighbourTileId );							// [ 0..r, 0 ]
		}
	}

	if ( tileY < tilesInRow-1 )
	{
		AreaId neighbourTileId = terrainInfo->GetTileIdFromCoords( tileX, tileY+1 );
		if ( neighbourAreaId == neighbourTileId )
		{
			local.Iterate( tileResolution-1, 0, neighbourTileId );			// [ 0..r, r ]
		}
	}

	// iterate y
	if ( tileX > 0 )
	{
		AreaId neighbourTileId = terrainInfo->GetTileIdFromCoords( tileX-1, tileY );
		if ( neighbourAreaId == neighbourTileId )
		{
			local.Iterate( 0, 1, neighbourTileId );							// [ 0, 0..r ]
		}
	}

	if ( tileX < tilesInRow-1 )
	{
		AreaId neighbourTileId = terrainInfo->GetTileIdFromCoords( tileX+1, tileY );
		if ( neighbourAreaId == neighbourTileId )
		{
			local.Iterate( tileResolution-1, 1, neighbourTileId );			// [ r, 0..r ]
		}
	}
}


void CTerrainAreaDescription::GetAreaFileList( TDynArray< CDiskFile* >& fileList, Bool onlyChanged )
{
	Super::GetAreaFileList( fileList, onlyChanged );

	CTerrainMap* terrainMap = m_terrain.Get();

	if ( terrainMap && ( !onlyChanged || !terrainMap->IsInitialVersion() ) )
	{
		CDirectory* dir = m_pathlib->GetSourceDataDirectory();
		if ( !dir )
		{
			return;
		}

		String fileName;
		m_pathlib->GetTerrainFileName( m_id, fileName );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			fileList.PushBack( file );
		}
	}
}

Float CTerrainAreaDescription::GetMaxNodesDistance() const
{
	return 25.f;
}

void CTerrainAreaDescription::GetTileCoords( Int32& x, Int32& y ) const
{
	m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, x, y );
}

void CTerrainAreaDescription::RemarkInstancesAtLocation( CDetailedSurfaceData* surface, const Vector2& localMin, const Vector2& localMax, AreaId ignoreInstance )
{
	Int32 minX, minY, maxX, maxY;

	CTerrainMap* terrain = m_terrain.Get();
	terrain->GetQuadCoordsAt( localMin, minX, minY );
	terrain->GetQuadCoordsAt( localMax, maxX, maxY );

	Int32 tileX, tileY;
	m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, tileX, tileY );

	const CInstanceMapCel& mapCel = m_pathlib->GetInstanceMap()->GetMapCelAt( tileX, tileY );
	const auto& areaIdList = mapCel.GetInstancesList();

	TDynArray< CNavmeshAreaDescription* > areaPtrs;
	areaPtrs.Reserve( areaIdList.Size() );
	for ( Uint32 i = 0, n = areaIdList.Size(); i < n; ++i )
	{
		if ( areaIdList[ i ] == ignoreInstance )
			continue;

		CNavmeshAreaDescription* naviArea = m_pathlib->GetInstanceAreaDescription( areaIdList[ i ] );
		if ( naviArea && naviArea->IsReady() && naviArea->GetNavmesh() )
		{
			areaPtrs.PushBack( naviArea );
		}
	}

	for ( Int32 y = minY; y <= maxY; ++y )
	{
		for ( Int32 x = minX; x <= maxX; ++x )
		{
			Bool collidesInstance = false;
			Vector2 quadMin, quadMax;
			terrain->GetQuadLocalPosition( x, y, quadMin, quadMax );

			for ( auto it = areaPtrs.Begin(), end = areaPtrs.End(); it != end; ++it )
			{
				if ( IsQuadOccupiedByInstance( quadMin, quadMax, *it ) )
				{
					collidesInstance = true;
					break;

				}
			}

			CDetailedSurfaceData::FieldData field = surface->GetField( x, y );
			field.m_isMarkedByInstance = collidesInstance;
			surface->SetField( x, y, field );
		}
	}
	CObstaclesMap* obstaclesMap = GetObstaclesMap();
	if ( obstaclesMap )
	{
		obstaclesMap->UpdateObstaclesAreaMarking( localMin, localMax );
	}
}

Bool CTerrainAreaDescription::RemarkInstancesAtLocation( const Vector2& localMin, const Vector2& localMax, AreaId ignoreInstance )
{
	CDetailedSurfaceData data;
	data.Initialize( m_pathlib->GetTerrainInfo().GetTilesResolution() );
	ExtractSurfaceData( &data );
	RemarkInstancesAtLocation( &data, localMin, localMax, ignoreInstance );
	return ApplySurfaceData( &data );
}
Bool CTerrainAreaDescription::IsQuadOccupiedByInstance( const Vector2& localMin, const Vector2& localMax, CNavmeshAreaDescription* naviArea )
{
	CTerrainMap* terrain = m_terrain.Get();
	Vector3 centerLocal;
	centerLocal.AsVector2() = ( localMin + localMax ) * 0.5f;
	centerLocal.Z = terrain->ComputeHeight( centerLocal.AsVector2() );
	Vector3 centerWorld = centerLocal;
	LocalToWorld( centerWorld );

	if ( naviArea->VComputeHeight( centerWorld.AsVector2(), centerWorld.Z - 2.f, centerWorld.Z + 2.f, centerWorld.Z ) )
	{
		Vector2 worldMin = localMin;
		Vector2 worldMax = localMax;
		LocalToWorld( worldMin );
		LocalToWorld( worldMax );

		CRectangleQueryData query( CT_NO_OBSTACLES, worldMin, worldMax, centerWorld.Z );
		if ( naviArea->VSpatialQuery( query ) )
		{
			return true;
		}
	}
	return false;
}
Bool CTerrainAreaDescription::Mark( CDetailedSurfaceData* surface, CNavmeshAreaDescription* naviArea, Bool markNavmeshConnections )
{
	CTerrainMap* terrain = m_terrain.Get();
	Box localBBox = naviArea->GetBBox();
	WorldToLocal( localBBox );
	Int32 minX, minY, maxX, maxY;
	terrain->GetQuadCoordsAt( localBBox.Min.AsVector2(), minX, minY );
	terrain->GetQuadCoordsAt( localBBox.Max.AsVector2(), maxX, maxY );
	Bool markDirty = false;
	for ( Int32 y = minY; y <= maxY; ++y )
	{
		for ( Int32 x = minX; x <= maxX; ++x )
		{
			Vector2 quadMin, quadMax;
			terrain->GetQuadLocalPosition( x, y, quadMin, quadMax );
			if ( IsQuadOccupiedByInstance( quadMin, quadMax, naviArea ) )
			{
				// Mark quad
				CDetailedSurfaceData::FieldData field = surface->GetField( x, y );
				if ( !field.m_isMarkedByInstance )
				{
					field.m_isMarkedByInstance = true;
					surface->SetField( x, y, field );
					markDirty = true;
				}
			}
		}
	}
	if ( markNavmeshConnections )
	{
		TDynArray< Vector3 > locations;
		naviArea->GatherPossibleConnectors( m_id, locations );

		for ( auto it = locations.Begin(), end = locations.End(); it != end; ++it )
		{
			Vector2 location = it->AsVector2();
			WorldToLocal( location );
			Int32 x, y;
			terrain->GetQuadCoordsAt( location, x, y );
			surface->GetField( x, y );
			CDetailedSurfaceData::FieldData field = surface->GetField( x, y );
			if ( !field.m_isInstanceConnection )
			{
				field.m_isInstanceConnection = true;
				surface->SetField( x, y, field );
				markDirty = true;
			}
		}
	}

	return markDirty;
}

void CTerrainAreaDescription::VLocalToWorld( Box& v ) const			{ LocalToWorld( v ); }
void CTerrainAreaDescription::VWorldToLocal( Box& v ) const			{ WorldToLocal( v ); }
void CTerrainAreaDescription::VLocalToWorld( Vector3& v ) const		{ LocalToWorld( v ); }
void CTerrainAreaDescription::VWorldToLocal( Vector3& v ) const		{ WorldToLocal( v ); }
void CTerrainAreaDescription::VLocalToWorld( Vector2& v ) const		{ LocalToWorld( v ); }
void CTerrainAreaDescription::VWorldToLocal( Vector2& v ) const		{ WorldToLocal( v ); }

void CTerrainAreaDescription::LocalToWorld( Box& v ) const
{
	v.Min.AsVector2() += m_tileCorner;
	v.Max.AsVector2() += m_tileCorner;
}
void CTerrainAreaDescription::WorldToLocal( Box& v ) const
{
	v.Min.AsVector2() -= m_tileCorner;
	v.Max.AsVector2() -= m_tileCorner;
}

void CTerrainAreaDescription::LocalToWorld( Vector3& v ) const
{
	v.AsVector2() += m_tileCorner;
}
void CTerrainAreaDescription::WorldToLocal( Vector3& v ) const
{
	v.AsVector2() -= m_tileCorner;
}
void CTerrainAreaDescription::LocalToWorld( Vector2& v ) const
{
	v += m_tileCorner;
}
void CTerrainAreaDescription::WorldToLocal( Vector2& v ) const
{
	v -= m_tileCorner;
}
Bool CTerrainAreaDescription::VContainsPoint( const Vector3& v ) const
{
	return ContainsPoint( v );
}
Bool CTerrainAreaDescription::VComputeHeight( const Vector3& v, Float& z, Bool smooth )
{
	ComputeHeight( v.AsVector2(), z, smooth );
	return true;
}
Bool CTerrainAreaDescription::VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth )
{
	ComputeHeight( v, z, smooth );
	return z >= minZ && z <= maxZ;
}
Bool CTerrainAreaDescription::VComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth )
{
	if ( pos.X < m_bbox.Min.X || pos.Y < m_bbox.Min.Y || pos.X > m_bbox.Max.X || pos.Y > m_bbox.Max.Y )
	{
		return false;
	}
	ComputeHeight( pos, outHeight, smooth );
	return true;
}
Bool CTerrainAreaDescription::VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax )
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	if ( m_heightData.IsInitialized() )
	{
		CTerrainMap* terrainMap = m_terrain.Get();

		Float minZ = FLT_MAX;
		Float maxZ = -FLT_MAX;
		Float avgZ = 0.f;
		Float totalField = 0.f;

		Box localBBox = bbox;
		WorldToLocal( localBBox.Min.AsVector3() );
		WorldToLocal( localBBox.Max.AsVector3() );
		Int32 xMin, yMin, xMax, yMax;
		terrainMap->GetQuadCoordsAt( localBBox.Min.AsVector2(), xMin, yMin );
		terrainMap->GetQuadCoordsAt( localBBox.Max.AsVector2(), xMax, yMax );
		for ( Int32 y = yMin; y <= yMax; ++y )
		{
			for ( Int32 x = xMin; x <= xMax; ++x )
			{
				if ( terrainMap->GetQuadState( terrainMap->GetQuadIndex( x, y ) ) == CTerrainMap::QUAD_FREE )
				{
					Float h = m_heightData.ComputeTexelNormalizedHeight( x, y );
					if ( h < localBBox.Min.Z || h > localBBox.Max.Z )
					{
						continue;
					}
					minZ = Min( minZ, h );
					maxZ = Max( maxZ, h );
					Float newField = totalField + 1.f;
					avgZ = avgZ * (totalField / newField) + h * (1.f / newField);
					totalField = newField;
				}
			}
		}
		if ( totalField > 0.f )
		{
			zAverage = avgZ;
			zMin = minZ;
			zMax = maxZ;
			return true;
		}
		return false;
	}
#endif
	ASSERT( !IsProcessing() );
	Float z[5];
	ComputeHeight( bbox.Min.AsVector2(), z[0] );
	ComputeHeight( Vector2( bbox.Min.X, bbox.Max.Y ), z[1] );
	ComputeHeight( bbox.Max.AsVector2(), z[2] );
	ComputeHeight( Vector2( bbox.Max.X, bbox.Min.Y ), z[3] );
	ComputeHeight( ( bbox.Min.AsVector2() + bbox.Max.AsVector2() ) * 0.5f, z[4] );

	zAverage = (z[0] + z[1] + z[2] + z[3] + z[4]) / 5.f;
	zMin = z[0];
	zMax = z[0];
	for ( Uint32 i = 1; i < 5; ++i )
	{
		zMin = Min( z[i], zMin );
		zMax = Max( z[i], zMax );
	}
	return true;
}
Bool CTerrainAreaDescription::VTestLocation( const Vector3& v1, Uint32 collisionFlags )
{
	return TestLocation( v1, collisionFlags );
}
Bool CTerrainAreaDescription::ContainsPoint( const Vector3& v ) const
{
	Vector2 localpos = v.AsVector2() - m_tileCorner;
	const CTerrainInfo& terrainInfo = m_pathlib->GetTerrainInfo();
	if ( localpos.X < 0.f || localpos.Y < 0.f || localpos.X > terrainInfo.GetTileSize() || localpos.Y > terrainInfo.GetTileSize() )
	{
		return false;
	}
	CTerrainMap* terrainMap = m_terrain.Get();
	if ( !terrainMap->TestLocation( localpos ) )
	{
		return false;
	}
	Float z;
	ComputeHeight( v.AsVector2(), z );
	return z >= v.Z - 2.f && z <= v.Z + 2.f;
}
void CTerrainAreaDescription::ComputeHeight( const Vector2& v, Float& z, Bool smooth ) const
{
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	if ( m_heightData.IsInitialized() )
	{
		Vector2 localPos = v;
		WorldToLocal( localPos );
		z = m_heightData.ComputeHeight( localPos );
		return;
	}
#endif
	ASSERT( !IsProcessing() );
	Vector2 localPos = v;
	WorldToLocal( localPos );
	z =
		smooth
		? m_terrain->GetHeightContext().ComputeHeightSmooth( localPos )
		: m_terrain->GetHeightContext().ComputeHeightFast( localPos );
}
Bool CTerrainAreaDescription::TestLocation(const Vector3& v1, Uint32 collisionFlags ) const
{
	CTerrainMap* terrainMap = m_terrain.Get();
	Vector3 local1 = v1;
	WorldToLocal( local1 );
	if ( !terrainMap->TestLocation( local1.AsVector2() ) )
	{
		return false;
	}
	if ( collisionFlags & CT_FORCE_BASEPOS_ZTEST )
	{
		Float z;
		ComputeHeight( v1.AsVector2(), z );
		if ( z < v1.Z - 2.f || z > v1.Z + 2.f )
		{
			return false;
		}
	}
	if ( (collisionFlags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
	{
		CObstaclesMap* obstaclesMap = GetObstaclesMap();
		if ( obstaclesMap && !obstaclesMap->TestLocation( local1 ) )
		{
			return false;
		}
	}

	return true;
}

template < class TQuery >
RED_INLINE Bool CTerrainAreaDescription::LocalSpaceSpatialQuery( TQuery& query ) const
{
	CTerrainMap* terrain = m_terrain.Get();
	if ( !terrain->SpatialQuery( query ) )
	{
		return false;
	}
	if ( (query.m_flags & CT_NO_OBSTACLES) != CT_NO_OBSTACLES )
	{
		CObstaclesMap* obstaclesMap = GetObstaclesMap();
		if ( obstaclesMap && !obstaclesMap->TSpatialQuery( query ) )
		{
			return false;
		}
	}

	return true;
}

template < class TQuery >
Bool CTerrainAreaDescription::SpatialQuery( TQuery& query ) const
{
	query.Transform( *this );
	Bool ret = LocalSpaceSpatialQuery( query );
	query.CancelTransformOutput( *this );
	return ret;
}

#ifndef NO_EDITOR_PATHLIB_SUPPORT
Bool CTerrainAreaDescription::ProcessDirtySurface( CGenerationManager::CAsyncTask** outJob )
{
	if ( outJob )
	{
		CTerrainSurfaceProcessingThread* newJob = new CTerrainSurfaceProcessingThread( this );
		*outJob = newJob;
	}
	else
	{
		ComputeTerrainData();
	}
	
	return true;
}
Bool CTerrainAreaDescription::ProcessDirtyCollisionData()
{
#ifndef PATHLIB_COMPECT_TERRAIN_DATA
	CTerrainMap* terrain = m_terrain.Get();
	if ( terrain->UpdateMap() )
	{
		MarkDirty( DIRTY_GENERATE, 1.f );
#ifndef NO_EDITOR_PATHLIB_SUPPORT
		const Vector2& tileCorner = m_tileCorner;
		Float tileSize = m_pathlib->GetTerrainInfo().GetTileSize();
		Box bbox(
			Vector( tileCorner.X, tileCorner.Y, -FLT_MAX ),
			Vector( tileCorner.X + tileSize, tileCorner.Y + tileSize, FLT_MAX )
			);
		m_pathlib->GetGenerationManager()->RecalculateWaypoints( bbox );
#endif
	}
#endif
	return true;
}
Bool CTerrainAreaDescription::RemarkInstances()
{
	Float tileSize = m_pathlib->GetTerrainInfo().GetTileSize();
	return RemarkInstancesAtLocation( Vector2( 0,0 ), Vector2( tileSize, tileSize ) );
}

Bool CTerrainAreaDescription::CorrectNeighbourList()
{
	CAreaNavgraphs* graphs = m_graphs.Get();
	if ( !graphs )
	{
		return false;
	}
	DetachFromNeighbours();
	//Int32 x, y;
	//m_pathlib->GetTerrainInfo().GetTileCoordsFromId( m_id, x, y );
	//const CInstanceMapCel& mapCel = m_pathlib->GetInstanceMap()->GetMapCelAt( x, y );
	//const TDynArray< AreaId >& instancesList = mapCel.GetInstancesList();
	//TDynArray< Vector3 > connectorLocations;
	//TDynArray< TPair< Vector3, AreaId > > input;
	//auto fun =
	//	[ this, &connectorLocations, &input, instancesList ] ( CNavGraph* g )
	//{
	//	for ( auto it = instancesList.Begin(), end = instancesList.End(); it != end; ++it )
	//	{
	//		AreaId naviAreaId = *it;
	//		CNavmeshAreaDescription* naviArea = m_pathlib->GetInstanceAreaDescription( naviAreaId );
	//		if ( naviArea )
	//		{
	//			CNavGraph* navgraph = naviArea->GetNavigationGraph( g->GetCategory() );
	//			if ( navgraph )
	//			{
	//				navgraph->CollectConnectors( m_id, connectorLocations );
	//				ForEach( connectorLocations,
	//					[ &input, naviAreaId ] ( const Vector3& v )
	//				{
	//					input.PushBack( TPair< Vector3, AreaId >( v, naviAreaId ) );
	//				}
	//				);
	//				connectorLocations.ClearFast();
	//			}
	//		}
	//	}
	//	g->SetInstanceConnectors( input );
	//	input.ClearFast();
	//};
	//graphs->IterateGraphs( fun );
	//ConnectWithNeighbours();
	return true;
}

Bool CTerrainAreaDescription::GenerateHeightData()
{
	CTerrainMap* terrainMap = m_terrain.Get();
	if ( !terrainMap )
	{
		return false;
	}
	return terrainMap->GenerateHeightData();
}

void CTerrainAreaDescription::PostGenerationSyncProcess()
{
	Super::PostGenerationSyncProcess();
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
	// block further use of invalidated height data
	if ( !m_heightData.IsValid() )
	{
		m_heightData.Clear();
	}
#endif
}
#endif
void CTerrainAreaDescription::ComputeBBox()
{
	const CTerrainInfo& info = m_pathlib->GetTerrainInfo();
	Int32 x, y;
	info.GetTileCoordsFromId( m_id, x, y );
	Vector2 tileMin = info.GetTileCorner( x, y );
	Vector2 tileMax = info.GetTileCorner( x+1, y+1 );
	m_bbox.Min.Set3( Vector( tileMin.X, tileMin.Y, -FLT_MAX) );
	m_bbox.Max.Set3( Vector( tileMax.X, tileMax.Y, FLT_MAX) );
}

Bool CTerrainAreaDescription::VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const		{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CCircleQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CClosestObstacleCircleQueryData& query ) const			{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const	{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CLineQueryData& query ) const							{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CWideLineQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const		{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const		{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CRectangleQueryData& query ) const						{ return SpatialQuery( query ); }
Bool CTerrainAreaDescription::VSpatialQuery( CCustomTestQueryData& query ) const					{ return SpatialQuery( query ); }

template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CCircleQueryData >( CCircleQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CClosestObstacleCircleQueryData >( CClosestObstacleCircleQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CCollectCollisionPointsInCircleQueryData >( CCollectCollisionPointsInCircleQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CLineQueryData >( CLineQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CWideLineQueryData >( CWideLineQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CClosestObstacleWideLineQueryData >( CClosestObstacleWideLineQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CClearWideLineInDirectionQueryData >( CClearWideLineInDirectionQueryData& query ) const;
template Bool CTerrainAreaDescription::LocalSpaceSpatialQuery< CCustomTestQueryData >( CCustomTestQueryData& query ) const;




};		// namespace PathLib
